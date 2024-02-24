#include "api_runner.h"

#include "logging/logging.h"
#include "orchestrator_api.h"
ApiRunner::ApiRunner(std::shared_ptr<GraphOrchestrator> orchestrator, std::shared_ptr<SignalChannel> s,
                     std::shared_ptr<LogSignal> log_signal)
    : m_orchestrator(orchestrator), m_sig_channel(s), m_log_signal(log_signal), m_thread(&ApiRunner::RunServer, this) {}

void ApiRunner::RunServer() {
    std::string server_address("0.0.0.0:" + std::to_string(50050));
    OrchestratorApi service("localhost:" + std::to_string(50050), m_orchestrator);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    m_grpc_server = std::move(builder.BuildAndStart());
    Logging::INFO("API running on '" + server_address + "'", m_name);
    m_grpc_server->Wait();
}

ApiRunner::~ApiRunner() {
    m_grpc_server->Shutdown();
    m_thread.join();
}