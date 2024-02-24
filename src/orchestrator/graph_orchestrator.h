#ifndef GRAPH_ORCHESTRATOR_H
#define GRAPH_ORCHESTRATOR_H

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "../data_source.h"
#include "../lock_free_queue.h"
#include "graph_client.h"

class OrchestratorBuilder;

class GraphOrchestrator : public DataSource {
   private:
    std::string m_name;
    std::vector<GraphClient> m_worker_clients;
    std::vector<std::string> m_worker_address;
    std::shared_ptr<std::atomic<size_t>> m_active_processors;
    std::shared_ptr<LockFreeQueue<std::string>> m_input_queue;
    std::atomic<bool> m_healthy;

   protected:
    void Query() override;

   public:
    GraphOrchestrator(std::string name_);
    void AddVertex(std::string key, std::string data);
    void AddEdge(std::string from, std::string to, std::string data);
    Status Search(std::string query_key, int level, std::vector<std::string>& vertices,
                  std::vector<std::string>& edges);
    void Init();
    void Ping();
    bool Healthy();
    void Stop() override;

    friend class OrchestratorBuilder;
};

#endif