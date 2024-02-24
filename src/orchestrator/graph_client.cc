#include "graph_client.h"

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../grpc/graph.grpc.pb.h"
#include "../logging/logging.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientWriter;
using grpc::Status;

using graph::Graph;
using graph::GraphSummary;
using graph::Host;
using graph::PingRequest;
using graph::PingResponse;
using graph::SearchResults;

GraphClient::GraphClient(std::shared_ptr<Channel> channel) : stub_(Graph::NewStub(channel)) {}

void GraphClient::AddVertices(const InMemoryGraph<std::string, std::string>::InMemoryVertex& v) const {
    ClientContext context;
    GraphSummary stats;

    std::unique_ptr<ClientWriter<Vertex>> writer(stub_->AddVertex(&context, &stats));

    if (!writer->Write(MakeVertex(v.key_, v.data_))) {
        Logging::ERROR("AddVertex error on write", m_name);
    }

    writer->WritesDone();
    Status status = writer->Finish();
    if (status.ok()) {
        Logging::INFO("AddVertices finished with " + std::to_string(stats.vertex_count()) + " vertices", m_name);
    } else {
        Logging::ERROR("AddVertex rpc failed", m_name);
    }
}

void GraphClient::DeleteVertex(const std::string& key) const {
    ClientContext context;
    GraphSummary stats;

    std::unique_ptr<ClientWriter<Vertex>> writer(stub_->DeleteVertex(&context, &stats));

    if (!writer->Write(MakeVertex(key, ""))) {
        Logging::ERROR("DeleteVertex error on write", m_name);
    }

    writer->WritesDone();
    Status status = writer->Finish();
    if (status.ok()) {
        Logging::INFO("DeleteVertex finished with " + std::to_string(stats.vertex_count()) + " vertices", m_name);
    } else {
        Logging::ERROR("DeleteVertex rpc failed", m_name);
    }
}

void GraphClient::AddEdges(const InMemoryGraph<std::string, std::string>::InMemoryVertex& v,
                           const InMemoryGraph<std::string, std::string>::InMemoryEdge& e,
                           const std::string& lookup_from) const {
    ClientContext context;
    GraphSummary stats;

    std::unique_ptr<ClientWriter<Edge>> writer(stub_->AddEdge(&context, &stats));

    if (!writer->Write(MakeEdge(v.key_, e.to_, e.data_, lookup_from, e.lookup_to_))) {
        Logging::ERROR("AddEdges error on write", m_name);
    }

    writer->WritesDone();
    Status status = writer->Finish();
    if (status.ok()) {
        Logging::INFO("AddEdge finished with " + std::to_string(stats.edge_count()) + " edges", m_name);
    } else {
        Logging::ERROR("AddEdge rpc failed", m_name);
    }
}

void GraphClient::DeleteEdge(const std::string& from, const std::string& to) const {
    ClientContext context;
    GraphSummary stats;

    std::unique_ptr<ClientWriter<Edge>> writer(stub_->DeleteEdge(&context, &stats));

    if (!writer->Write(MakeEdge(from, to, "", "", ""))) {
        Logging::ERROR("DeleteEdge error on write", m_name);
    }

    writer->WritesDone();
    Status status = writer->Finish();
    if (status.ok()) {
        Logging::INFO("DeleteEdge finished with " + std::to_string(stats.edge_count()) + " edges", m_name);
    } else {
        Logging::ERROR("DeleteEdge rpc failed", m_name);
    }
}

Status GraphClient::Search(const std::string& key, const int max_level, SearchResults& result) const {
    ClientContext context;
    SearchArgs args = MakeSearchArgs(key, max_level);
    Status status = stub_->Search(&context, args, &result);
    if (!status.ok()) {
        Logging::ERROR("Search rpc failed", m_name);
    } else {
        std::stringstream s;
        s << "Finished with " << result.vertices().size() << " vertices:[";
        std::string sep;
        for (auto& v : result.vertices()) {
            s << sep << v.key();
            sep.assign(", ");
        }
        s << "] and " << result.edges().size() << " edges: [";
        sep.assign("");
        for (auto& e : result.edges()) {
            s << sep << e.label();
            sep.assign(", ");
        }
        s << "]";
        Logging::INFO(s.str(), m_name);
    }
    return status;
}

void GraphClient::AddHost(const std::string& key, const std::string& address) const {
    ClientContext context;
    Host host;
    host.set_key(key);
    host.set_address(address);
    ::google::protobuf::Empty result;
    Status status = stub_->AddHost(&context, host, &result);
    if (status.ok()) {
        Logging::INFO("AddHost finished", m_name);
    } else {
        Logging::ERROR("AddHost rpc failed", m_name);
    }
}

bool GraphClient::Ping() const {
    ClientContext context;
    PingRequest ping;
    ping.set_data("hello");

    PingResponse response;
    Status status = stub_->Ping(&context, ping, &response);
    if (status.ok()) {
        // Logging::INFO("Ping ok response: '" + response.data() + "'", m_name);
        return true;
    } else {
        // Logging::ERROR("Ping rpc failed", m_name);
        return false;
    }
}

Vertex GraphClient::MakeVertex(std::string key, std::string value) const {
    graph::Vertex v;
    v.set_key(key);
    v.set_value(value);
    return v;
}

SearchArgs GraphClient::MakeSearchArgs(std::string key, const int max_level) const {
    graph::SearchArgs a;
    a.set_start_key(key);
    a.set_level(max_level);
    return a;
}

Edge GraphClient::MakeEdge(const std::string& from, const std::string& to, const std::string& label,
                           const std::string& lookup_from, const std::string& lookup_to) const {
    graph::Edge e;
    e.set_from(from);
    e.set_to(to);
    e.set_label(label);
    e.set_lookup_from(lookup_from);
    e.set_lookup_to(lookup_to);
    return e;
}