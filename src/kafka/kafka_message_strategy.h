#ifndef KAFKA_MESSAGE_STRATEGY_H
#define KAFKA_MESSAGE_STRATEGY_H
#include <rdkafkacpp.h>

#include <memory>

class KafkaMessageStrategy {
   public:
    KafkaMessageStrategy(std::string name);
    virtual void Run(RdKafka::Message *message, void *opaque) const = 0;
    virtual ~KafkaMessageStrategy() = default;

   protected:
    std::string m_name;
};

#endif