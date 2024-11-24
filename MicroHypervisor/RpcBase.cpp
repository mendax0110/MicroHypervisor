#include "RpcBase.h"
#include <iostream>

using namespace rpc;

RpcBase::RpcBase() : logger_("RpcBase"), stop_threads_(false), stop_monitoring_(true)
{
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        logger_.Log(Logger::LogLevel::Error, "WSAStartup failed");
        throw std::runtime_error("WSAStartup failed");
    }
}

RpcBase::~RpcBase()
{
    stop_threads_ = true;
    queue_condition_.notify_all();

    for (auto& thread : thread_pool_)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    if (server_socket_ != INVALID_SOCKET)
    {
        closesocket(server_socket_);
    }

    StopRpc();
    WSACleanup();
}

bool RpcBase::StartRpc(const std::string& ip, int port)
{
    if (!SetupRpc(ip, port))
    {
        logger_.Log(Logger::LogLevel::Error, "Failed to start RPC server");
        return false;
    }

    stop_monitoring_ = false;
    monitoring_thread_ = std::thread(&RpcBase::MonitorIoThread, this);
    return true;
}

void RpcBase::StopRpc()
{
    stop_monitoring_ = true;
    stop_threads_ = true;
    queue_condition_.notify_all();

    if (monitoring_thread_.joinable())
    {
        monitoring_thread_.join();
    }

    // Stop worker threads
    for (auto& thread : thread_pool_)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    if (server_socket_ != INVALID_SOCKET)
    {
        closesocket(server_socket_);
        server_socket_ = INVALID_SOCKET;
    }
}

std::map<std::string, std::list<std::string>> RpcBase::GetIoMap()
{
    std::lock_guard<std::mutex> lock(io_map_mutex_);
    return io_map_;
}

void RpcBase::MonitorIoThread()
{
    while (!stop_monitoring_)
    {
        {
            std::unique_lock<std::mutex> lock(io_map_mutex_);
            io_map_ = MonitorRpcIO();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool RpcBase::SetupRpc(const std::string& ip, const int port)
{
    rpcServerConfig config{ ip, port };
    return SetupRpcServer(config);
}

bool RpcBase::SetupRpcServer(const rpcServerConfig& config)
{
    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config.port);
    server_addr.sin_addr.s_addr = inet_addr(config.ip.c_str());

    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == INVALID_SOCKET)
    {
        logger_.Log(Logger::LogLevel::Error, "Socket creation failed: " + std::to_string(WSAGetLastError()));
        return false;
    }

    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        logger_.Log(Logger::LogLevel::Error, "Socket bind failed: " + std::to_string(WSAGetLastError()));
        closesocket(server_socket_);
        return false;
    }

    if (listen(server_socket_, config.maxConnections) == SOCKET_ERROR)
    {
        logger_.Log(Logger::LogLevel::Error, "Socket listen failed: " + std::to_string(WSAGetLastError()));
        closesocket(server_socket_);
        return false;
    }

    logger_.Log(Logger::LogLevel::Info, "Server listening on " + config.ip + ":" + std::to_string(config.port));

    for (uint32_t i = 0; i < config.maxThreads; ++i)
    {
        thread_pool_.emplace_back(&RpcBase::WorkerThread, this);
    }

    sockaddr_in client_addr = {};
    int addr_len = sizeof(client_addr);
    while (!stop_threads_)
    {
        SOCKET client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET)
        {
            if (stop_threads_) break;
            logger_.Log(Logger::LogLevel::Error, "Socket accept failed: " + std::to_string(WSAGetLastError()));
            continue;
        }

        char recv_buf[1024] = { 0 };
        int recv_len = recv(client_socket, recv_buf, sizeof(recv_buf), 0);
        if (recv_len > 0)
        {
            EnqueueRequest(std::string(recv_buf, recv_len));
        }

        closesocket(client_socket);
    }

    return true;
}

void RpcBase::AddMethod(const std::string& name, std::function<void(const std::vector<std::string>&)> method)
{
    std::unique_lock<std::mutex> lock(queue_mutex_);
    m_methods[name] = std::move(method);
    logger_.Log(Logger::LogLevel::Info, "Method added: " + name);
}

void RpcBase::CallMethod(const std::string& name, const std::vector<std::string>& args)
{
    auto it = m_methods.find(name);
    if (it != m_methods.end())
    {
        it->second(args);
    }
    else
    {
        logger_.Log(Logger::LogLevel::Error, "Method not found: " + name);
    }
}

void RpcBase::WorkerThread()
{
    while (!stop_threads_)
    {
        std::string request;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_condition_.wait(lock, [this] { return !request_queue_.empty() || stop_threads_; });

            if (stop_threads_ && request_queue_.empty())
                return;

            request = std::move(request_queue_.front());
            request_queue_.pop();
        }

        std::vector<std::string> tokens;
        size_t pos = 0, prev_pos = 0;
        while ((pos = request.find(' ', prev_pos)) != std::string::npos)
        {
            tokens.emplace_back(request.substr(prev_pos, pos - prev_pos));
            prev_pos = pos + 1;
        }
        tokens.emplace_back(request.substr(prev_pos));

        if (!tokens.empty())
        {
            const std::string& method_name = tokens[0];
            CallMethod(method_name, std::vector<std::string>(tokens.begin() + 1, tokens.end()));
        }
    }
}

void RpcBase::EnqueueRequest(const std::string& request)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (request_queue_.size() < 100)
        {
            request_queue_.emplace(request);
        }
    }
    queue_condition_.notify_one();
}

std::map<std::string, std::list<std::string>> RpcBase::MonitorRpcIO()
{
    std::map<std::string, std::list<std::string>> io_map;
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        for (const auto& [name, method] : m_methods)
        {
            io_map[name] = std::list<std::string>();
        }

        std::queue<std::string> temp_queue = request_queue_;
        while (!temp_queue.empty())
        {
            const std::string& request = temp_queue.front();

            std::vector<std::string> tokens;
            size_t pos = 0, prev_pos = 0;
            while ((pos = request.find(' ', prev_pos)) != std::string::npos)
            {
                tokens.emplace_back(request.substr(prev_pos, pos - prev_pos));
                prev_pos = pos + 1;
            }
            tokens.emplace_back(request.substr(prev_pos));

            if (!tokens.empty())
            {
                const std::string& method_name = tokens[0];
                io_map[method_name].emplace_back(request);
            }

            temp_queue.pop();
        }
    }

    return io_map;
}
