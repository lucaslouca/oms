#ifndef ORCHESTRATOR_API_H
#define ORCHESTRATOR_API_H

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <memory>
#include <string>

#include "../grpc/orchestrator.grpc.pb.h"
#include "graph_orchestrator.h"
#include "logging/logging.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using std::chrono::system_clock;

using orchestrator::ApiEdge;
using orchestrator::ApiGraphSummary;
using orchestrator::ApiSearchArgs;
using orchestrator::ApiSearchResults;
using orchestrator::ApiVertex;
using orchestrator::Orchestrator;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

class OrchestratorApi final : public Orchestrator::Service {
   public:
    explicit OrchestratorApi(const std::string& name, std::shared_ptr<GraphOrchestrator> orchestrator)
        : m_name(name), m_orchestrator(orchestrator) {}

    Status AddVertex(ServerContext* context, ServerReader<ApiVertex>* reader, ApiGraphSummary* response) override {
        ApiVertex vertex;
        while (reader->Read(&vertex)) {
            Logging::INFO("Add vertex with key: '" + vertex.key() + "' and value: '" + vertex.value() + "'", m_name);
        }
        response->set_vertex_count(0);
        return Status::OK;
    }

    Status DeleteVertex(ServerContext* context, ServerReader<ApiVertex>* reader, ApiGraphSummary* response) override {
        ApiVertex vertex;
        while (reader->Read(&vertex)) {
            Logging::INFO("Delete vertex with key: '" + vertex.key() + "'", m_name);
        }
        response->set_vertex_count(0);
        return Status::OK;
    }

    Status DeleteEdge(ServerContext* context, ServerReader<ApiEdge>* reader, ApiGraphSummary* response) override {
        ApiEdge edge;
        while (reader->Read(&edge)) {
            Logging::INFO("Delete edge: '" + edge.from() + "'---->'" + edge.to() + "'", m_name);
        }
        response->set_edge_count(0);
        return Status::OK;
    }

    Status AddEdge(ServerContext* context, ServerReader<ApiEdge>* reader, ApiGraphSummary* response) override {
        ApiEdge edge;
        while (reader->Read(&edge)) {
            Logging::INFO("Add edge: '" + edge.from() + "'--[" + edge.label() + "]-->'" + edge.to() + "'", m_name);
        }
        response->set_edge_count(0);
        return Status::OK;
    }

    Status Search(ServerContext* context, const ApiSearchArgs* request, ApiSearchResults* response) override {
        std::string query_key = request->query_key();
        int level = request->level();
        Logging::INFO("Search: '" + query_key + "'", m_name);

        std::vector<std::string> vertices;
        std::vector<std::string> edges;
        Status status = m_orchestrator->Search(query_key, level, vertices, edges);

        for (std::vector<std::string>::iterator it = vertices.begin(); it != vertices.end(); ++it) {
            ApiVertex* vertex = response->add_vertices();
            ApiVertex v;
            v.set_key(*it);
            v.set_value(*it);
            *vertex = v;
        }

        for (std::vector<std::string>::iterator it = edges.begin(); it != edges.end(); ++it) {
            ApiEdge* edge = response->add_edges();
            ApiEdge e;
            e.set_label(*it);
            *edge = e;
        }

        return status;
    }

   private:
    std::string m_name;
    std::shared_ptr<GraphOrchestrator> m_orchestrator;
};

#endif