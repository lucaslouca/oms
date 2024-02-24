#ifndef API_RUNNER_H
#define API_RUNNER_H

#include <grpc/grpc.h>

#include <memory>
#include <string>
#include <thread>

#include "graph_orchestrator.h"
#include "logging/log_signal.h"
#include "orchestrator_api.h"

class SignalChannel;

class ApiRunner {
   private:
    std::string m_name = "Api";
    std::thread m_thread;
    std::shared_ptr<SignalChannel> m_sig_channel;
    std::shared_ptr<LogSignal> m_log_signal;
    std::unique_ptr<grpc::Server> m_grpc_server;
    std::shared_ptr<GraphOrchestrator> m_orchestrator;

   public:
    ApiRunner(std::shared_ptr<GraphOrchestrator> orchestrator, std::shared_ptr<SignalChannel> s,
              std::shared_ptr<LogSignal> log_signal);
    void RunServer();
    ~ApiRunner();
};

#endif