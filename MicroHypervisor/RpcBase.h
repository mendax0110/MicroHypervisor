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

/// @brief RPC namespace \namespace rpc
namespace rpc
{
    /**
     * @brief Struct with the configuration for the RPC Server
     */
    struct rpcServerConfig
    {
        std::string ip;
        int port;
        uint32_t maxConnections = 5;
        uint32_t maxThreads = std::thread::hardware_concurrency();
        uint32_t maxQueue = 100;
    };

    /// @brief Base class for the RPC \class RpcBase
    class RpcBase : public std::enable_shared_from_this<RpcBase>
    {
    public:
        RpcBase();
        virtual ~RpcBase();

        /**
         * @brief Function to setup the RPC
         * 
         * @param ip -> The IP Address to bind to
         * @param port -> The Port to bind to
         * @return -> true if the setup is successful, false otherwise
         */
        bool SetupRpc(const std::string& ip, const int port);
        
        /**
         * @brief Function to add a method to the RPC
         * 
         * @param name -> The name of the method
         * @param method -> The method to add
         */
        void AddMethod(const std::string& name, std::function<void(const std::vector<std::string>&)> method);
        
        /**
         * @brief Function to call a method
         * 
         * @param name -> The name of the method
         * @param args -> The arguments for the method
         */
        void CallMethod(const std::string& name, const std::vector<std::string>& args);
        
        /**
         * @brief Function to setup the RPC Server
         * 
         * @param config -> The configuration for the RPC Server
         * @return -> true if the setup is successful, false otherwise
         */
        bool SetupRpcServer(const rpcServerConfig& config);
        
        /**
         * @brief Function to stop the RPC Server
         * 
         * @return -> A map with the IO of the RPC
         */
        std::map<std::string, std::list<std::string>> MonitorRpcIO();
        
        /**
         * @brief Function to start the RPC
         * 
         * @param ip -> The IP Address to bind to
         * @param port -> The Port to bind to
         * @return -> true if the RPC is started successfully, false otherwise
         */
        bool StartRpc(const std::string& ip, int port);
        
        /**
         * @brief Function to stop the RPC
         * 
         */
        void StopRpc();
        
        /**
         * @brief Getter method to get the IO Map
         */
        std::map<std::string, std::list<std::string>> GetIoMap();
        
        /**
         * @brief Function to monitor the IO Thread
         */
        void MonitorIoThread();

    private:

        /**
         * @brief Function to handle the worker thread
         */
        void WorkerThread();

        /**
         * @brief Function to handle the request
         */
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
