#ifndef HEALTH_CHECKER_H
#define HEALTH_CHECKER_H

#include <memory>
#include <string>

#include "../data_source.h"
#include "graph_orchestrator.h"

class HealthChecker : public DataSource {
   private:
    std::string m_name;
    std::shared_ptr<GraphOrchestrator> m_orchestrator;

   protected:
    void Query() override;

   public:
    HealthChecker(std::string name, std::shared_ptr<GraphOrchestrator> orchestrator);
    void Stop() override;
};

#endif