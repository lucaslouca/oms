#ifndef GRAPH_CLIENT_H
#define GRAPH_CLIENT_H

#include <memory>

#include "../graph/in_memory_graph.h"
#include "../grpc/graph.grpc.pb.h"

using graph::Edge;
using graph::SearchArgs;
using graph::SearchResults;
using graph::Vertex;
using grpc::Status;

class GraphClient {
   public:
    GraphClient(std::shared_ptr<Channel> channel);

    void AddVertices(const InMemoryGraph<std::string, std::string>::InMemoryVertex& v) const;

    void DeleteVertex(const std::string& key) const;

    void AddEdges(const InMemoryGraph<std::string, std::string>::InMemoryVertex& v,
                  const InMemoryGraph<std::string, std::string>::InMemoryEdge& e, const std::string& lookup_from) const;

    void DeleteEdge(const std::string& from, const std::string& to) const;

    Status Search(const std::string& key, const int max_level, SearchResults& result) const;

    void AddHost(const std::string& key, const std::string& address) const;

    bool Ping() const;

   private:
    Vertex MakeVertex(std::string key, std::string value) const;

    SearchArgs MakeSearchArgs(std::string key, const int max_level) const;

    Edge MakeEdge(const std::string& from, const std::string& to, const std::string& label,
                  const std::string& lookup_from, const std::string& lookup_to) const;

   private:
    std::unique_ptr<graph::Graph::Stub> stub_;
    std::string m_name = "GraphClient";
};

#endif