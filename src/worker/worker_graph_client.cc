#include "worker_graph_client.h"

SearchArgs WorkerGraphClient::MakeSearchArgs(std::string key, int level,
                                             const std::set<std::string>& ids_so_far) const {
    graph::SearchArgs a;
    a.set_start_key(key);
    a.set_level(level);
    for (const auto& i : ids_so_far) {
        std::string* id = a.add_ids_so_far();
        *id = i;
    }

    return a;
}

void WorkerGraphClient::UpdateNodes(const SearchResults& result, std::set<graph::Vertex>& nodes) const {
    for (const auto& v : result.vertices()) {
        nodes.insert(v);
    }
}

void WorkerGraphClient::UpdateEdges(const SearchResults& result, std::set<graph::Edge>& edges) const {
    for (const auto& e : result.edges()) {
        edges.insert(e);
    }
}

void WorkerGraphClient::UpdateIdsSoFar(const SearchResults& result, std::set<std::string>& ids_so_far) const {
    for (const auto& v : result.ids_so_far()) {
        ids_so_far.insert(v);
    }
}

void WorkerGraphClient::Search(const std::string& key, const int level, std::set<graph::Vertex>& result_nodes,
                               std::set<graph::Edge>& result_edges, std::set<std::string>& ids_so_far) const {
    ClientContext context;
    SearchResults result;
    SearchArgs args = MakeSearchArgs(key, level, ids_so_far);
    Status status = stub_->Search(&context, args, &result);
    if (!status.ok()) {
        std::cerr << "Search rpc failed." << std::endl;
    } else {
        UpdateNodes(result, result_nodes);
        UpdateEdges(result, result_edges);
        UpdateIdsSoFar(result, ids_so_far);
    }
}