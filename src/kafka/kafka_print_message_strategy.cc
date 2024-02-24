
#include "kafka_print_message_strategy.h"

#include <iostream>

#include "../logging/logging.h"

/*
Initializing the static member, KafkaPrintMessageStrategy::reg, of type StrategyRegister<KafkaPrintMessageStrategy>
Expands to: StrategyRegister<KafkaPrintMessageStrategy> KafkaPrintMessageStrategy::reg("Print")

Note: If KafkaPrintMessageStrategy is not EXPLICITLY (aka mentioned as in
dynamic_cast<KafkaPrintMessageStrategy*>(ptr.get())->m_name = "Printer2";) used in the application. Then the linker will
not pull that object into the application and REGISTER_DEF_TYPE() will never be called.
*/
REGISTER_DEF_TYPE(KafkaPrintMessageStrategy, PrintType);

void KafkaPrintMessageStrategy::Run(RdKafka::Message *message, void *opaque) const {
    switch (message->err()) {
        case RdKafka::ERR__TIMED_OUT:
            break;

        case RdKafka::ERR_NO_ERROR:
            /* Real message */
            Logging::DEBUG("Read msg at offset " + std::to_string(message->offset()), m_name);
            RdKafka::MessageTimestamp ts;
            ts = message->timestamp();
            if (ts.type != RdKafka::MessageTimestamp::MSG_TIMESTAMP_NOT_AVAILABLE) {
                std::string tsname = "?";
                if (ts.type == RdKafka::MessageTimestamp::MSG_TIMESTAMP_CREATE_TIME)
                    tsname = "create time";
                else if (ts.type == RdKafka::MessageTimestamp::MSG_TIMESTAMP_LOG_APPEND_TIME)
                    tsname = "log append time";
                Logging::DEBUG("Timestamp: " + tsname + " " + std::to_string(ts.timestamp), m_name);
            }
            if (message->key()) {
                Logging::DEBUG("Key: " + *message->key(), m_name);
            }

            Logging::DEBUG("Payload: " + std::string(static_cast<const char *>(message->payload())), m_name);
            m_output_queue->Push(std::string(static_cast<const char *>(message->payload())));

            break;

        case RdKafka::ERR__PARTITION_EOF:
            Logging::ERROR("EOF reached for all partition(s)", m_name);
            break;

        case RdKafka::ERR__UNKNOWN_TOPIC:
        case RdKafka::ERR__UNKNOWN_PARTITION:
            Logging::ERROR("Consume failed: " + message->errstr(), m_name);
            break;

        default:
            /* Errors */
            Logging::ERROR("Consume failed: " + message->errstr(), m_name);
    }
}