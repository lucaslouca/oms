#include "kafka_builder.h"

#include <rdkafkacpp.h>
#include <signal.h>  // kill()
#include <unistd.h>  // getpid()

#include <iostream>
KafkaBuilder::KafkaBuilder() {}

KafkaBuilder& KafkaBuilder::WithName(std::string v) {
    m_name = v;
    return *this;
}

KafkaBuilder& KafkaBuilder::WithBootstrapServers(std::string v) {
    m_bootstrap_servers = v;
    return *this;
}

KafkaBuilder& KafkaBuilder::WithClientId(std::string v) {
    m_client_id = v;
    return *this;
}

KafkaBuilder& KafkaBuilder::WithGroupId(std::string v) {
    m_group_id = v;
    return *this;
}

KafkaBuilder& KafkaBuilder::WithDeliveryReportCallback(std::unique_ptr<KafkaDeliveryReportCb> v) {
    m_delivery_report_callback = std::move(v);
    return *this;
}

KafkaBuilder& KafkaBuilder::WithTopics(std::vector<std::string> v) {
    m_topics = v;
    return *this;
}

KafkaBuilder& KafkaBuilder::WithKafkaMessageStrategy(std::unique_ptr<KafkaMessageStrategy> v) {
    m_kafka_strategy = std::move(v);
    return *this;
}

std::unique_ptr<KafkaDataSource> KafkaBuilder::Build() {
    if (m_name.empty()) {
        m_name = "Kafka";
    }

    if (m_bootstrap_servers.empty()) {
        throw std::runtime_error("No bootstrap servers provided");
    }

    if (m_client_id.empty()) {
        throw std::runtime_error("No client id provided");
    }

    if (m_group_id.empty()) {
        throw std::runtime_error("No group id provided");
    }

    if (!m_delivery_report_callback) {
        throw std::runtime_error("No delivery report callback provided");
    }

    if (m_topics.empty()) {
        throw std::runtime_error("No topics provided");
    }

    if (!m_kafka_strategy) {
        throw std::runtime_error("No Kafka strategy provided");
    }

    std::unique_ptr<KafkaDataSource> kafka = std::make_unique<KafkaDataSource>();
    kafka->m_name = std::move(m_name);
    kafka->m_bootstrap_servers = std::move(m_bootstrap_servers);
    kafka->m_client_id = std::move(m_client_id);
    kafka->m_group_id = std::move(m_group_id);
    kafka->m_delivery_report_callback = std::move(m_delivery_report_callback);
    kafka->m_topics = std::move(m_topics);
    kafka->m_kafka_strategy = std::move(m_kafka_strategy);

    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    std::string errstr;
    if (conf->set("bootstrap.servers", kafka->m_bootstrap_servers, errstr) != RdKafka::Conf::CONF_OK) {
        std::cout << errstr;
        kill(getpid(), SIGINT);
    }
    if (conf->set("client.id", kafka->m_client_id, errstr) != RdKafka::Conf::CONF_OK) {
        std::cout << errstr;
        kill(getpid(), SIGINT);
    }
    if (conf->set("group.id", kafka->m_group_id, errstr) != RdKafka::Conf::CONF_OK) {
        std::cout << errstr;
        kill(getpid(), SIGINT);
    }

    /* Set the delivery report callback.
     * This callback will be called once per message to inform
     * the application if delivery succeeded or failed.
     * See dr_msg_cb() above.
     * The callback is only triggered from ::poll() and ::flush().
     *
     * IMPORTANT:
     * Make sure the DeliveryReport instance outlives the Producer object,
     * either by putting it on the heap or as in this case as a stack variable
     * that will NOT go out of scope for the duration of the Producer object.
     */
    if (conf->set("dr_cb", kafka->m_delivery_report_callback.get(), errstr) != RdKafka::Conf::CONF_OK) {
        std::cout << errstr;
        kill(getpid(), SIGINT);
    }

    /* Create Kafka producer handle */
    // RdKafka::Producer* kafka_producer = RdKafka::Producer::create(conf, errstr);
    // if (!kafka_producer) {
    //     std::cout << "Failed to create Kafka producer: " << errstr;
    //     kill(getpid(), SIGINT);
    // }

    /* Create Kafka consumer handle */
    RdKafka::KafkaConsumer* kafka_consumer = RdKafka::KafkaConsumer::create(conf, errstr);
    if (!kafka_consumer) {
        std::cout << "Failed to create Kafka consumer: " << errstr;
        kill(getpid(), SIGINT);
    }

    RdKafka::ErrorCode err = kafka_consumer->subscribe(kafka->m_topics);
    if (err) {
        std::cerr << "Failed to subscribe to " << kafka->m_topics.size() << " topics" << RdKafka::err2str(err)
                  << std::endl;
        kill(getpid(), SIGINT);
    }

    kafka->m_kafka_consumer = kafka_consumer;

    delete conf;
    return kafka;
}