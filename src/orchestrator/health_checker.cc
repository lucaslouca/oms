#include "health_checker.h"

#include "../logging/logging.h"

HealthChecker::HealthChecker(std::string name, std::shared_ptr<GraphOrchestrator> orchestrator)
    : m_name(name), m_orchestrator(orchestrator) {}

void HealthChecker::Query() { m_orchestrator->Ping(); }

void HealthChecker::Stop() {}