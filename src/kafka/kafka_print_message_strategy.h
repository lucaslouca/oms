#ifndef KAFKA_PRINT_MESSAGE_STRATEGY_H
#define KAFKA_PRINT_MESSAGE_STRATEGY_H
#include <rdkafkacpp.h>

#include <string>

#include "../lock_free_queue.h"
#include "kafka_message_strategy.h"
#include "kafka_strategy_factory.h"

class KafkaPrintMessageStrategy : public KafkaMessageStrategy {
   private:
    REGISTER_DEC_TYPE(KafkaPrintMessageStrategy);  // Declare a static member variable, reg,  of type
                                                   // StrategyRegister<KafkaPrintMessageStrategy>

   public:
    std::shared_ptr<LockFreeQueue<std::string>> m_output_queue;

   public:
    using KafkaMessageStrategy::KafkaMessageStrategy;
    void Run(RdKafka::Message *message, void *opaque) const override;
};

#endif