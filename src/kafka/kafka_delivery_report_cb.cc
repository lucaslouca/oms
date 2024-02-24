#include "kafka_delivery_report_cb.h"

#include <iostream>

static std::string name = "KafkaDeliveryReportCb";

void KafkaDeliveryReportCb::dr_cb(RdKafka::Message &message) {
    if (message.err()) {
        std::cout << name << ": Message publication failed: " << message.errstr();
    } else {
        std::cout << name << ": Message published to topic " << message.topic_name() << " ["
                  << std::to_string(message.partition()) << "] at offset " << std::to_string(message.offset());
    }
}