#include "orchestrator_builder.h"

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "../graph/helper.h"

OrchestratorBuilder& OrchestratorBuilder::WithName(std::string v) {
    m_name = v;
    return *this;
}

OrchestratorBuilder& OrchestratorBuilder::WithWorkers(std::map<std::string, std::string> v) {
    m_workers_config = v;
    return *this;
}

OrchestratorBuilder& OrchestratorBuilder::WithInputQueue(std::shared_ptr<LockFreeQueue<std::string>> v) {
    m_input_queue = v;
    return *this;
}

std::shared_ptr<GraphOrchestrator> OrchestratorBuilder::Build() {
    if (m_name.empty()) {
        m_name = "Graph Orchestrator";
    }

    if (m_workers_config.empty()) {
        throw std::runtime_error("No workers configuration provided");
    }

    if (!m_input_queue) {
        throw std::runtime_error("No input queue provided");
    }

    std::shared_ptr<GraphOrchestrator> orchestrator = std::make_shared<GraphOrchestrator>(m_name);

    std::vector<GraphClient> worker_clients;
    std::vector<std::string> worker_address;
    for (const auto& [id, port] : m_workers_config) {
        std::string address = "localhost:" + port;
        worker_address.emplace_back(address);
        worker_clients.emplace_back(GraphClient(grpc::CreateChannel(address, grpc::InsecureChannelCredentials())));
    }

    orchestrator->m_worker_address = std::move(worker_address);
    orchestrator->m_worker_clients = std::move(worker_clients);
    orchestrator->m_input_queue = m_input_queue;

    return orchestrator;
}