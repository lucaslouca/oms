#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "../config/config_parser.h"
#include "../grpc/graph.grpc.pb.h"
#include "in_memory_graph.h"
namespace graph {

void Usage(const std::string me) {
    std::cerr << "Usage: " << me
              << " [options]\n"
                 "Inmemory Graph Server\n"
                 "\n"
                 "Options:\n"
                 " -c <path>       config path\n"
                 "\n"
                 "\n"
                 "Example:\n"
                 "  "
              << me << " -c ../../configs/slave.yaml\n\n";
    exit(1);
}

void ParseArgs(int argc, char* argv[], std::string& config_file) {
    int opt;
    while ((opt = getopt(argc, argv, "c:")) != -1) {
        switch (opt) {
            case 'c':
                config_file = optarg;
                break;
            default:
                std::cerr << "Unknown option -" << (char)opt << std::endl;
                Usage(argv[0]);
        }
    }
}

std::string GetDbFileContent(const std::string& db_path) {
    std::ifstream db_file(db_path);
    if (!db_file.is_open()) {
        std::cout << "Failed to open " << db_path << std::endl;
        abort();
    }
    std::stringstream db;
    db << db_file.rdbuf();
    return db.str();
}

class JSONParser {
   public:
    explicit JSONParser(const std::string& db) : db_(db) {}

    void Parse(InMemoryGraph<std::string, std::string>* graph) {
        using json = nlohmann::json;

        json data = json::parse(db_);
        for (const auto& l : data["nodes"].items()) {
            std::string key = l.key();
            std::string data = (l.value()["data"]).at("value");
            std::cout << "Got vertice '" << key << "' with data: '" << data << "'" << std::endl;
            graph->AddVertex(key, data);
        }

        for (const auto& l : data["edges"].items()) {
            std::string key = l.key();
            std::string from = (l.value()["from"]);
            std::string to = (l.value()["to"]);
            std::string label = (l.value()["data"]).at("label");
            std::cout << "Got edge '" << key << "' from: '" << from << "' to '" << to << "' with label: '" << label
                      << "'" << std::endl;

            graph->AddUndirectedEdge(from, to, label, "local", "local");
        }
    }

   private:
    std::string db_;
};

void ParseDb(const std::string& db, InMemoryGraph<std::string, std::string>* graph) {
    std::string db_content(db);
    db_content.erase(std::remove_if(db_content.begin(), db_content.end(), isspace), db_content.end());

    JSONParser parser(db_content);
    parser.Parse(graph);
    std::cout << "DB parsed" << std::endl;
}

bool operator<(const Edge& lhs, const Edge& rhs) { return lhs.key() < rhs.key(); }

bool operator<(const Vertex& lhs, const Vertex& rhs) { return lhs.key() < rhs.key(); }
}  // namespace graph
