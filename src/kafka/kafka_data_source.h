#ifndef KAFKA_DATA_SOURCE_H
#define KAFKA_DATA_SOURCE_H

#include <rdkafkacpp.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../data_source.h"
#include "kafka_delivery_report_cb.h"
#include "kafka_message_strategy.h"

class KafkaBuilder;

class KafkaDataSource : public DataSource {
   private:
    std::string m_name;
    std::string m_bootstrap_servers;
    std::string m_client_id;
    std::string m_group_id;
    std::unique_ptr<KafkaDeliveryReportCb> m_delivery_report_callback;
    std::vector<std::string> m_topics;
    RdKafka::KafkaConsumer* m_kafka_consumer;
    std::unique_ptr<KafkaMessageStrategy> m_kafka_strategy;

   protected:
    void Query() override;

   public:
    ~KafkaDataSource();
    void Stop() override;

    friend class KafkaBuilder;
};

#endif