#include "kafka_data_source.h"

#include <rdkafkacpp.h>
#include <signal.h>  // kill()
#include <unistd.h>  // getpid()

#include <iostream>

#include "kafka_delivery_report_cb.h"
#include "kafka_message_strategy.h"
#include "kafka_print_message_strategy.h"

void KafkaDataSource::Query() {
    RdKafka::Message *message = m_kafka_consumer->consume(0);
    m_kafka_strategy->Run(message, NULL);
    delete message;
}

void KafkaDataSource::Stop() {
    std::cout << m_name << " stopping" << std::endl;

    m_kafka_consumer->close();
    delete m_kafka_consumer;

    /*
     * Wait for RdKafka to decommission.
     * This is not strictly needed (with check outq_len() above), but
     * allows RdKafka to clean up all its resources before the application
     * exits so that memory profilers such as valgrind wont complain about
     * memory leaks.
     */
    RdKafka::wait_destroyed(5000);
    std::cout << m_name << " stopped" << std::endl;
}

KafkaDataSource::~KafkaDataSource() {}