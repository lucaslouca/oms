#ifndef ORCHESTRATOR_BUILDER_H
#define ORCHESTRATOR_BUILDER_H

#include <map>
#include <memory>
#include <string>

#include "../lock_free_queue.h"
#include "graph_orchestrator.h"

class OrchestratorBuilder {
   private:
    std::string m_name;
    std::map<std::string, std::string> m_workers_config;
    std::string m_db_content;
    std::shared_ptr<LockFreeQueue<std::string>> m_input_queue;

   public:
    OrchestratorBuilder& WithName(std::string v);
    OrchestratorBuilder& WithWorkers(std::map<std::string, std::string> v);
    OrchestratorBuilder& WithInputQueue(std::shared_ptr<LockFreeQueue<std::string>> v);
    std::shared_ptr<GraphOrchestrator> Build();
};

#endif