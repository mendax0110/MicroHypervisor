#ifndef RPCBASE_H
#define RPCBASE_H

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "Logger.h"
#include <winsock.h>


namespace rpc
{
    struct rpcServerConfig
    {
        std::string ip;
        int port;
        uint32_t maxConnections = 5;
        uint32_t maxThreads = std::thread::hardware_concurrency();
        uint32_t maxQueue = 100;
    };

    class RpcBase : public std::enable_shared_from_this<RpcBase>
    {
    public:
        RpcBase();
        virtual ~RpcBase();

        bool SetupRpc(const std::string& ip, const int port);
        void AddMethod(const std::string& name, std::function<void(const std::vector<std::string>&)> method);
        void CallMethod(const std::string& name, const std::vector<std::string>& args);
        bool SetupRpcServer(const rpcServerConfig& config);
        std::map<std::string, std::list<std::string>> MonitorRpcIO();
        bool StartRpc(const std::string& ip, int port);
        void StopRpc();
        std::map<std::string, std::list<std::string>> GetIoMap();
        void MonitorIoThread();

    private:
        void WorkerThread();
        void EnqueueRequest(const std::string& request);

        Logger logger_;
        SOCKET server_socket_ = INVALID_SOCKET;

        std::map<std::string, std::function<void(const std::vector<std::string>&)>> m_methods;

        std::vector<std::thread> thread_pool_;
        std::queue<std::string> request_queue_;
        std::mutex queue_mutex_;
        std::condition_variable queue_condition_;
        bool stop_threads_ = false;
        bool stop_monitoring_ = false;
        std::mutex io_map_mutex_;
        std::map<std::string, std::list<std::string>> io_map_;
        std::thread monitoring_thread_;
    };
}

#endif // RPCBASE_H
