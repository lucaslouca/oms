/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include "../config/config_parser.h"
#include "../graph/helper.h"
#include "../graph/in_memory_graph.h"
#include "../grpc/graph.grpc.pb.h"
#include "worker_graph_client.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using std::chrono::system_clock;

using graph::Edge;
using graph::Graph;
using graph::GraphSummary;
using graph::Host;
using graph::PingRequest;
using graph::PingResponse;
using graph::SearchArgs;
using graph::SearchResults;
using graph::Vertex;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

class GraphImpl final : public Graph::Service {
   public:
    using InMemoryGraphType = InMemoryGraph<std::string, std::string>;

    explicit GraphImpl(const std::string& id) : graph_(InMemoryGraph<std::string, std::string>(id)) {}

    Status AddHost(ServerContext* context, const Host* request, ::google::protobuf::Empty* response) override {
        rpc_clients_.insert({request->key(), WorkerGraphClient(grpc::CreateChannel(
                                                 request->address(), grpc::InsecureChannelCredentials()))});
        return Status::OK;
    }

    Status AddVertex(ServerContext* context, ServerReader<Vertex>* reader, GraphSummary* response) override {
        Vertex vertex;
        while (reader->Read(&vertex)) {
            graph_.AddVertex(vertex.key(), vertex.value());
        }
        response->set_vertex_count(graph_.NumberOfVertices());
        return Status::OK;
    }

    Status DeleteVertex(ServerContext* context, ServerReader<Vertex>* reader, GraphSummary* response) override {
        Vertex vertex;
        while (reader->Read(&vertex)) {
            graph_.DeleteVertex(vertex.key());
        }
        response->set_vertex_count(graph_.NumberOfVertices());
        return Status::OK;
    }

    Status DeleteEdge(ServerContext* context, ServerReader<Edge>* reader, GraphSummary* response) override {
        Edge edge;
        while (reader->Read(&edge)) {
            graph_.DeleteEdge(edge.from(), edge.to());
        }
        response->set_edge_count(graph_.NumberOfEdges());
        return Status::OK;
    }

    Status AddEdge(ServerContext* context, ServerReader<Edge>* reader, GraphSummary* response) override {
        Edge edge;
        while (reader->Read(&edge)) {
            graph_.AddEdge(edge.from(), edge.to(), edge.label(), edge.lookup_to());
        }
        response->set_edge_count(graph_.NumberOfEdges());
        return Status::OK;
    }

    Status Search(ServerContext* context, const SearchArgs* request, SearchResults* response) override {
        std::set<graph::Vertex> result_nodes;
        std::set<graph::Edge> result_edges;
        std::set<std::string> ids_so_far;

        for (const auto& v : request->vertices()) {
            result_nodes.insert(v);
        }

        for (const auto& e : request->edges()) {
            result_edges.insert(e);
        }

        for (const auto& v : request->ids_so_far()) {
            ids_so_far.insert(v);
        }

        graph_.Search(request->start_key(), request->level(), result_nodes, result_edges, ids_so_far, rpc_clients_);

        // Collect the final results from the BFS Search
        for (const auto& v : result_nodes) {
            graph::Vertex* vertex = response->add_vertices();
            *vertex = v;
        }

        for (const auto& e : result_edges) {
            Edge* edge = response->add_edges();
            *edge = e;
        }

        return Status::OK;
    }

    Status Ping(ServerContext* context, const PingRequest* request, PingResponse* response) override {
        response->set_data("I'm alive!");
        return Status::OK;
    }

   private:
    InMemoryGraphType graph_;
    std::map<std::string, WorkerGraphClient> rpc_clients_;
};

void RunServer(const int port) {
    std::string server_address("0.0.0.0:" + std::to_string(port));
    GraphImpl service("localhost:" + std::to_string(port));

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "[Worker] Listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char* argv[]) {
    // ./src/build/graph_worker -c configs/worker_50051.yaml
    std::string db_path;
    std::string config_file;

    /*************************************************************************
     *
     * COMMANDLINE ARGUMENTS
     *
     *************************************************************************/
    graph::ParseArgs(argc, argv, config_file);

    /*************************************************************************
     *
     * CONFIGURATION
     *
     *************************************************************************/
    ConfigParser& config = ConfigParser::instance(config_file);
    std::map<std::string, std::string> server_config = config.Server();
    int listen_port = atoi(server_config.at("port").c_str());
    RunServer(listen_port);
    return 0;
}
