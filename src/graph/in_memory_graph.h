#ifndef IN_MEMORY_GRAPH_H_
#define IN_MEMORY_GRAPH_H_

#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <vector>

#include "../grpc/graph.grpc.pb.h"
#include "../worker/worker_graph_client.h"

template <typename VERTEX_DATA, typename EDGE_DATA>
class InMemoryGraph {
   public:
    int test;
    using VERTEX_KEY = std::string;
    using EDGE_KEY = std::string;

    struct InMemoryVertex {
        VERTEX_KEY key_;
        VERTEX_DATA data_;

        InMemoryVertex(VERTEX_KEY key, const VERTEX_DATA& data) : key_(key), data_(data) {}

        bool const operator==(const InMemoryVertex& o) { return key_ == o.key_; }

        bool operator<(const InMemoryVertex& o) const { return key_ < o.key_; }
    };

    struct InMemoryEdge {
        VERTEX_KEY to_;
        EDGE_DATA data_;
        std::string lookup_to_;

        InMemoryEdge(VERTEX_KEY to, const EDGE_DATA& edge_data, std::string lookup_to)
            : to_(to), data_(edge_data), lookup_to_(lookup_to) {}

        bool operator<(const InMemoryEdge& o) const { return to_ < o.to_ || (to_ == o.to_ && data_ < o.data_); }
    };

   private:
    std::map<VERTEX_KEY, InMemoryVertex> vertices_;
    std::map<VERTEX_KEY, std::set<InMemoryEdge>> edges_;
    std::string worker_id_;
    std::map<std::string, std::unique_ptr<graph::Graph::Stub>> worker_clients_;

   public:
    InMemoryGraph(std::string id) : worker_id_(id) {}
    int NumberOfVertices() const { return vertices_.size(); }
    int NumberOfEdges() const {
        int count = 0;
        for (const auto& [key, value] : edges_) {
            count += value.size();
        }
        return count;
    }
    int NumberOfEdges(VERTEX_KEY key) const {
        auto pos = edges_.find(key);
        assert(pos != edges_.end());
        return pos->second.size();
    }

    bool HasVertex(VERTEX_KEY key) { return (vertices_.find(key) != vertices_.end()); }

    void AddVertex(VERTEX_KEY key, const VERTEX_DATA& data) {
        std::cout << "[AddVertex] Adding vertex: '" << key << "' with data: '" << data << "'" << std::endl;
        std::cout << "[AddVertex] Available edges before add:" << std::endl;
        print_edges();

        if (this->HasVertex(key)) {
            std::cerr << "[AddVertex] Vertex with key: '" << key << "' already exists" << std::endl;
            return;
        }

        assert(vertices_.find(key) == vertices_.end());
        vertices_.insert({key, InMemoryVertex(key, data)});
        edges_.insert({key, std::set<InMemoryEdge>()});

        std::cout << "[AddVertex] Available edges after add:" << std::endl;
        print_edges();
    }

    void DeleteVertex(VERTEX_KEY key) {
        std::cout << "[DeleteVertex] Deleting vertex: '" << key << "'" << std::endl;
        std::cout << "[DeleteVertex] Available edges before delete:" << std::endl;
        print_edges();

        if (!this->HasVertex(key)) {
            std::cerr << "[DeleteVertex] Vertex with key: '" << key << "' does not exist" << std::endl;
            return;
        }

        assert(vertices_.find(key) != vertices_.end());
        const auto it_v = vertices_.find(key);
        vertices_.erase(it_v);

        const auto it_e = edges_.find(key);
        edges_.erase(it_e);

        std::cout << "[DeleteVertex] Available edges after delete:" << std::endl;
        print_edges();
    }

    void print_edges() {
        for (const auto& [key, to_edges] : edges_) {
            for (const auto& to_edge : to_edges) {
                std::cout << "'" << key << "'--[" << to_edge.data_ << "]-->'" << to_edge.to_ << "' with lookup_to: '"
                          << to_edge.lookup_to_ << "'" << std::endl;
            }
        }
    }

    void AddEdge(VERTEX_KEY from, VERTEX_KEY to, const EDGE_DATA& data, const std::string lookup_to) {
        std::cout << "[AddEdge] Adding edge: '" << from << "'-'" << to << "' with data: '" << data
                  << "' and lookup_to: '" << lookup_to << "'" << std::endl;
        std::cout << "[AddEdge] Available edges before add:" << std::endl;
        print_edges();

        if (!this->HasVertex(from)) {
            std::cerr << "[AddEdge] Vertex with key: '" << from << "' does not exist" << std::endl;
            return;
        }

        assert(edges_.find(from) != edges_.end());
        edges_[from].emplace(InMemoryEdge(to, data, lookup_to));

        std::cout << "[AddEdge] Available edges after add:" << std::endl;
        print_edges();
    }

    void DeleteEdge(VERTEX_KEY from, VERTEX_KEY to) {
        std::cout << "[DeleteEdge] Attempting to delete edge: '" << from << "'-'" << to << "'" << std::endl;
        std::cout << "[DeleteEdge] Available edges before delete:" << std::endl;
        print_edges();

        if (edges_.find(from) == edges_.end()) {
            std::cerr << "[DeleteEdge] Edge: '" << from << "'-'" << to << "' not available" << std::endl;
            return;
        }

        assert(edges_.find(from) != edges_.end());
        std::set<InMemoryEdge> edges = edges_.find(from)->second;
        typename std::set<InMemoryEdge>::iterator it_e;
        for (it_e = edges.begin(); it_e != edges.end();) {
            if (!to.compare(it_e->to_)) {
                it_e = edges.erase(it_e);
            } else {
                ++it_e;
            }
        }

        std::cout << "[DeleteEdge] Available edges after delete:" << std::endl;
        print_edges();
    }

    void AddUndirectedEdge(VERTEX_KEY from, VERTEX_KEY to, const EDGE_DATA& data, const std::string lookup_to,
                           const std::string lookup_from) {
        AddEdge(from, to, data, lookup_to);
        AddEdge(to, from, data, lookup_from);
    }

    bool IsLocal(const std::string& data_source) { return !worker_id_.compare(data_source); }

    void Search(std::string key, int max_level, std::set<graph::Vertex>& result_nodes,
                std::set<graph::Edge>& result_edges, std::set<std::string>& ids_so_far,
                const std::map<std::string, WorkerGraphClient>& rpc_clients) {
        if (!this->HasVertex(key)) {
            std::cout << "Vertex with key '" << key << "' is not in this graph" << std::endl;
            return;
        }

        struct BFSEntry {
            VERTEX_KEY key_;
            int level_;
            std::string data_source_;
        };

        ids_so_far.insert(key);

        std::queue<BFSEntry> q;
        q.push({key, 0, worker_id_});

        while (!q.empty()) {
            const auto queue_entry = q.front();
            q.pop();
            const auto current_key = queue_entry.key_;
            const auto current_level = queue_entry.level_;
            const auto data_source = queue_entry.data_source_;
            if (IsLocal(data_source)) {
                graph::Vertex rpc_vertex;
                rpc_vertex.set_key(current_key);
                result_nodes.insert(rpc_vertex);

                if (current_level < max_level) {
                    // Iterate over its adjacent vertices

                    for (typename InMemoryGraph<std::string, std::string>::AdjacencyIterator e =
                             this->begin(current_key);
                         e != this->end(current_key); ++e) {
                        const auto edge = e.Edge();
                        const auto& to_key = e.To();
                        const auto& label = e.Data();
                        const auto& lookup_to = e.LookupTo();
                        std::string edge_key;
                        if (to_key.compare(current_key) < 0) {
                            edge_key.assign(to_key + label + current_key);
                        } else {
                            edge_key.assign(current_key + label + to_key);
                        }
                        graph::Edge rpc_edge;
                        rpc_edge.set_key(edge_key);
                        rpc_edge.set_from(current_key);
                        rpc_edge.set_to(to_key);
                        rpc_edge.set_label(label);
                        rpc_edge.set_lookup_from(worker_id_);
                        rpc_edge.set_lookup_to(lookup_to);

                        result_edges.insert(rpc_edge);

                        if (ids_so_far.find(to_key) == ids_so_far.end()) {
                            ids_so_far.insert(to_key);
                            q.push({to_key, current_level + 1, lookup_to});
                        }
                    }
                }

            } else {
                int new_max_level = max_level - current_level;
                if (new_max_level >= 0) {
                    rpc_clients.at(data_source)
                        .Search(current_key, new_max_level, result_nodes, result_edges, ids_so_far);
                }
            }
        }
    }

    class VertexIterator {
        std::vector<InMemoryVertex> vertices_;
        int j_;  // current vertex

       public:
        VertexIterator(const InMemoryGraph& g, int j) : j_(j) {
            std::transform(
                g.vertices_.begin(), g.vertices_.end(), std::back_inserter(vertices_),
                [](const typename std::map<VERTEX_KEY, InMemoryVertex>::value_type& pair) { return pair.second; });
        }
        VertexIterator& operator++() {
            assert(j_ < vertices_.size());
            ++j_;
            return *this;
        }

        InMemoryVertex& Vertex() { return vertices_[j_]; }
        VERTEX_KEY Key() { return vertices_[j_].key_; }
        const VERTEX_DATA& Data() { return vertices_[j_].data_; }
        bool operator!=(const VertexIterator& rhs) { return j_ != rhs.j_; }
    };

    VertexIterator begin() const { return VertexIterator(*this, 0); }
    VertexIterator end() const { return VertexIterator(*this, NumberOfVertices()); }

    class AdjacencyIterator {
        std::vector<InMemoryEdge> edges_;
        int j_;  // current edge

       public:
        AdjacencyIterator(const InMemoryGraph& g, VERTEX_KEY v, int j)
            : edges_(std::vector<InMemoryEdge>(g.edges_.at(v).begin(), g.edges_.at(v).end())), j_(j) {}
        AdjacencyIterator& operator++() {
            assert(j_ < edges_.size());
            ++j_;
            return *this;
        }

        InMemoryEdge& Edge() { return edges_[j_]; }
        VERTEX_KEY To() { return edges_[j_].to_; }
        const EDGE_DATA& Data() { return edges_[j_].data_; }
        const std::string LookupTo() { return edges_[j_].lookup_to_; }
        bool operator!=(const AdjacencyIterator& rhs) { return j_ != rhs.j_; }
    };

    AdjacencyIterator begin(VERTEX_KEY v) const { return AdjacencyIterator(*this, v, 0); }
    AdjacencyIterator end(VERTEX_KEY v) const { return AdjacencyIterator(*this, v, NumberOfEdges(v)); }
};
#endif