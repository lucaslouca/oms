#include <signal.h>     // kill()
#include <sys/event.h>  // kqueue

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "config/config_parser.h"
#include "kafka/kafka_builder.h"
#include "kafka/kafka_data_source.h"
#include "kafka/kafka_message_strategy.h"
#include "kafka/kafka_print_message_strategy.h"
#include "kafka/kafka_strategy_factory.h"
#include "lock_free_queue.h"
#include "logging/log_signal.h"
#include "logging/logging.h"
#include "orchestrator/api_runner.h"
#include "orchestrator/health_checker.h"
#include "orchestrator/orchestrator_builder.h"
#include "safe_queue.h"
#include "signal_channel.h"
#include "thread_dispatcher.h"
static std::string name = "Main";

/**
 * Create a return a shared channel for SIGINT signals.
 *
 */
std::shared_ptr<SignalChannel> ListenForSigint(sigset_t& sigset) {
    std::shared_ptr<SignalChannel> sig_channel = std::make_shared<SignalChannel>();

#ifdef __linux__
    // Listen for sigint event line Ctrl^c
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

    std::thread signal_handler{[&sig_channel, &sigset]() {
        int signum = 0;

        // wait untl a signal is delivered
        sigwait(&sigset, &signum);
        sig_channel->m_shutdown_requested.store(true);

        // notify all waiting workers to check their predicate
        sig_channel->m_cv.notify_all();
        std::cout << "Received signal " << signum << "\n";
        return signum;
    }};
    signal_handler.detach();
#elif __APPLE__
    std::thread signal_handler{[&sig_channel]() {
        int kq = kqueue();

        /* Two kevent structs */
        struct kevent* ke = (struct kevent*)malloc(sizeof(struct kevent));

        /* Initialise struct for SIGINT */
        signal(SIGINT, SIG_IGN);
        EV_SET(ke, SIGINT, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);

        /* Register for the events */
        if (kevent(kq, ke, 1, NULL, 0, NULL) < 0) {
            perror("kevent");
            return false;
        }

        memset(ke, 0x00, sizeof(struct kevent));

        // Camp here for event
        if (kevent(kq, NULL, 0, ke, 1, NULL) < 0) {
            perror("kevent");
        }

        switch (ke->filter) {
            case EVFILT_SIGNAL:
                std::cout << "Received signal " << strsignal(ke->ident) << "\n";
                sig_channel->m_shutdown_requested.store(true);
                sig_channel->m_cv.notify_all();
                break;
            default:
                break;
        }

        return true;
    }};
    signal_handler.detach();
#endif

    return sig_channel;
}

/*************************************************************************
 *
 * MAIN
 *
 *************************************************************************/
int main(int argc, char** argv) {
    /*
    $ ps aux | grep orchestrator
    user  1268   0.0  0.1 408617344  87840   ??  TX   11:04AM   4:04.82 ...

    # Grab the definitive parent ID of whatever process ID you feed it and act on it
    $ ps -p 1268 -o ppid=
    1269

    $ kill -9 1269
    */
    // ./src/build/graph_orchestrator -c configs/orchestrator.yaml
    std::string config_file;

    /*************************************************************************
     *
     * COMMANDLINE ARGUMENTS
     *
     *************************************************************************/
    graph::ParseArgs(argc, argv, config_file);

    /*************************************************************************
     *
     * CONFIGURATION
     *
     *************************************************************************/
    ConfigParser& config = ConfigParser::instance(config_file);

    std::map<std::string, std::string> workers_config = config.Workers();

    /*************************************************************************
     *
     * SIGINT CHANNEL
     *
     *************************************************************************/
    sigset_t sigset;
    std::shared_ptr<SignalChannel> sig_channel = ListenForSigint(sigset);

    /*************************************************************************
     *
     * LOGGER
     *
     *************************************************************************/
    std::shared_ptr<LogSignal> log_signal = std::make_shared<LogSignal>();
    Logging::LogProcessor log_processor(log_signal);
    log_processor.start();

    /*************************************************************************
     *
     * ORCHESTRATOR
     *
     *************************************************************************/
    Logging::INFO("Init orchestrator", name);
    std::shared_ptr<LockFreeQueue<std::string>> graph_queue = std::make_shared<LockFreeQueue<std::string>>();
    OrchestratorBuilder orchestrator_builder;
    std::shared_ptr<GraphOrchestrator> orchestrator =
        orchestrator_builder.WithName("Orchestrator").WithWorkers(workers_config).WithInputQueue(graph_queue).Build();

    /*************************************************************************
     *
     * HEALTH CHECKER
     *
     *************************************************************************/
    std::unique_ptr<HealthChecker> checker = std::make_unique<HealthChecker>("HealthChecker", orchestrator);
    ThreadDispatcher graph_health_checker(std::move(checker), sig_channel, log_signal);

    while (!orchestrator->Healthy()) {
        Logging::INFO("Waiting for orchestrator to become healthy", name);
        auto delay = std::chrono::milliseconds(2000);
        std::this_thread::sleep_for(delay);
    }

    orchestrator->Init();
    ThreadDispatcher graph_poller(orchestrator, sig_channel, log_signal);

    /*************************************************************************
     *
     * API (USER FACING)
     *
     *************************************************************************/
    Logging::INFO("Init API", name);
    ApiRunner api_runner(orchestrator, sig_channel, log_signal);

    /*************************************************************************
     *
     * KAFKA
     *
     *************************************************************************/
    std::map<std::string, std::string> kafka_config = config.kafka();

    std::unique_ptr<KafkaMessageStrategy> ptr = KafkaFactory::GetInstance("PrintType", "Printer");

    dynamic_cast<KafkaPrintMessageStrategy*>(ptr.get())->m_output_queue = graph_queue;

    std::shared_ptr<KafkaDataSource> kafka =
        KafkaBuilder()
            .WithName("Kafka")
            .WithBootstrapServers(kafka_config["bootstrap.servers"])
            .WithClientId(kafka_config["client.id"])
            .WithGroupId("foo")
            .WithDeliveryReportCallback(std::move(std::make_unique<KafkaDeliveryReportCb>()))
            .WithTopics({"graph_data"})
            .WithKafkaMessageStrategy(std::move(ptr))
            .Build();

    ThreadDispatcher kafka_poller(kafka, sig_channel, log_signal);

    while (!sig_channel->m_shutdown_requested.load()) {
    }

    log_processor.stop();
    log_processor.join();
}
