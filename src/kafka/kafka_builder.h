#ifndef KAFKA_BUILDER_H
#define KAFKA_BUILDER_H

#include <memory>
#include <string>
#include <vector>

#include "kafka_data_source.h"
#include "kafka_delivery_report_cb.h"

class KafkaMessageStrategy;

class KafkaBuilder {
   private:
    std::string m_name;
    std::string m_bootstrap_servers;
    std::string m_client_id;
    std::string m_group_id;
    std::unique_ptr<KafkaDeliveryReportCb> m_delivery_report_callback;
    std::vector<std::string> m_topics;
    std::unique_ptr<KafkaMessageStrategy> m_kafka_strategy;

   public:
    KafkaBuilder();
    KafkaBuilder& WithName(std::string v);
    KafkaBuilder& WithBootstrapServers(std::string v);
    KafkaBuilder& WithClientId(std::string v);
    KafkaBuilder& WithGroupId(std::string v);
    KafkaBuilder& WithDeliveryReportCallback(std::unique_ptr<KafkaDeliveryReportCb> v);
    KafkaBuilder& WithTopics(std::vector<std::string> v);
    KafkaBuilder& WithKafkaMessageStrategy(std::unique_ptr<KafkaMessageStrategy> v);
    std::unique_ptr<KafkaDataSource> Build();
};

#endif