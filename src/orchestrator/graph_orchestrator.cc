#include "graph_orchestrator.h"

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <functional>  //for std::hash
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <vector>

#include "../graph/helper.h"
#include "../graph/in_memory_graph.h"
#include "../grpc/graph.grpc.pb.h"
#include "../logging/logging.h"
#include "graph_client.h"
using graph::Edge;
using graph::Graph;
using graph::Host;
using graph::SearchResults;
using graph::Vertex;
using grpc::Channel;

GraphOrchestrator::GraphOrchestrator(std::string name_) : m_name(name_) {}

void GraphOrchestrator::AddVertex(std::string key, std::string data) {
    std::hash<std::string> hasher;
    auto hashed = hasher(key);
    int worker_index = hashed % m_worker_clients.size();
    Logging::DEBUG("Pushing vertex '" + key + "' to worker '" + m_worker_address[worker_index] + "'", m_name);
    m_worker_clients[worker_index].AddVertices(InMemoryGraph<std::string, std::string>::InMemoryVertex(key, data));
}

// void GraphClient::AddEdges(const InMemoryGraph<std::string, std::string>::InMemoryVertex& v, const
// InMemoryGraph<std::string, std::string>::InMemoryEdge& e, const std::string& lookup_from)
void GraphOrchestrator::AddEdge(std::string from, std::string to, std::string label) {
    std::hash<std::string> hasher;
    auto from_worker_hashed = hasher(from);
    int from_worker_index = from_worker_hashed % m_worker_clients.size();

    auto lookup_to_worker_hashed = hasher(to);
    int lookup_to_worker_index = lookup_to_worker_hashed % m_worker_clients.size();

    Logging::INFO("Push edge [" + from + "][" + label + "][" + to + "] to worker '" +
                      std::to_string(from_worker_index) + "' with lookup_to: '" +
                      std::to_string(lookup_to_worker_index) + "' (" + m_worker_address[lookup_to_worker_index] + ")",
                  m_name);

    InMemoryGraph<std::string, std::string>::InMemoryVertex from_vertex(from, from);
    InMemoryGraph<std::string, std::string>::InMemoryEdge edge(to, label, m_worker_address[lookup_to_worker_index]);

    m_worker_clients[from_worker_index].AddEdges(from_vertex, edge, m_worker_address[from_worker_index]);
}

bool GraphOrchestrator::Healthy() { return m_healthy.load(); }

void GraphOrchestrator::Init() {
    /*************************************************************************
     *
     * ADVERTISE HOSTS
     *
     *************************************************************************/
    for (const auto& address : m_worker_address) {
        for (const auto& worker : m_worker_clients) {
            worker.AddHost(address, address);
        }
    }
}

void GraphOrchestrator::Query() {
    std::shared_ptr<std::string> payload = m_input_queue->Pop();
    if (payload) {
        Logging::INFO("Got " + *payload, m_name);
        while (!Healthy()) {
            Logging::ERROR("Graph doesn't seem to be healthy. Not attemping to add node", m_name);
        }

        /*
        {"from": "a","to": "b","label": "friend"}
        */
        using json = nlohmann::json;
        try {
            json data = json::parse(*payload);
            std::string from = data.at("from");
            std::string to = data.at("to");
            std::string label = data.at("label");

            AddVertex(from, from);
            AddVertex(to, to);
            AddEdge(from, to, label);
        } catch (...) {
            Logging::ERROR("Malformed payload: '" + *payload + "'", m_name);
        }
    }
}

void GraphOrchestrator::Stop() { Logging::INFO("Stopping", m_name); }

void GraphOrchestrator::Ping() {
    // Not thread safe!

    bool ok = true;
    for (const auto& worker : m_worker_clients) {
        ok = worker.Ping();
        if (!ok) {
            break;
        }
    }

    if (ok) {
        m_healthy.store(true);
    } else {
        m_healthy.store(false);
    }
}

/*
*************************************************************************
* USER FACING API
*************************************************************************
*/

Status GraphOrchestrator::Search(std::string query_key, int level, std::vector<std::string>& vertices,
                                 std::vector<std::string>& edges) {
    SearchResults result;

    std::hash<std::string> hasher;
    auto hashed = hasher(query_key);
    int worker_index = hashed % m_worker_clients.size();
    Logging::INFO("Start search vertex '" + query_key + "' with at: '" + std::to_string(worker_index) + "' (" +
                      m_worker_address[worker_index] + ")",
                  m_name);
    Status status = m_worker_clients[worker_index].Search(query_key, level, result);

    if (!status.ok()) {
        Logging::ERROR("Search rpc failed", m_name);
    } else {
        for (auto& v : result.vertices()) {
            vertices.emplace_back(v.key());
        }

        for (auto& e : result.edges()) {
            edges.emplace_back(e.label());
        }
    }

    return status;
}