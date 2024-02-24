#ifndef WORKER_GRAPH_CLIENT_H_
#define WORKER_GRAPH_CLIENT_H_

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <memory>

#include "../graph/helper.h"
#include "../grpc/graph.grpc.pb.h"

using graph::Graph;
using graph::SearchArgs;
using graph::SearchResults;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class WorkerGraphClient {
   private:
    SearchArgs MakeSearchArgs(std::string key, int level, const std::set<std::string>& ids_so_far) const;
    void UpdateNodes(const SearchResults& result, std::set<graph::Vertex>& nodes) const;
    void UpdateEdges(const SearchResults& result, std::set<graph::Edge>& edges) const;
    void UpdateIdsSoFar(const SearchResults& result, std::set<std::string>& ids_so_far) const;

   public:
    WorkerGraphClient(std::shared_ptr<Channel> channel) : stub_(Graph::NewStub(channel)) {}
    void Search(const std::string& key, const int level, std::set<graph::Vertex>& result_nodes,
                std::set<graph::Edge>& result_edges, std::set<std::string>& ids_so_far) const;

   private:
    std::unique_ptr<graph::Graph::Stub> stub_;
};

#endif