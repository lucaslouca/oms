#include "../thread_guard.h"
#include "logging.h"

static std::string name = "LogProcessor";
SafeQueue<std::string> log_queue;

Logging::LogProcessor::LogProcessor(std::shared_ptr<LogSignal> log_signal) : m_log_signal(log_signal) {
    // Logging::configure({{"type", "file"}, {"file_name", "flycatcher.log"}, {"reopen_interval", "1"}});
    Logging::configure({{"type", "std_out"}});
    // Logging::configure({{"type", "daily"}, {"file_name", "logs/oms_master.log"}, {"hour", "2"}, {"minute", "30"}});
}

bool Logging::LogProcessor::start() {
    m_t = std::make_unique<std::thread>(&Logging::LogProcessor::run, this);
    return true;
}

void Logging::LogProcessor::join() const { ThreadGuard g(*m_t); }

void Logging::LogProcessor::run() {
    m_should_run = true;
    while (m_should_run) {
        // Do not log when we have active processors. Processors have priority over logging.

        {
            /*
            Without blocks log_lock doesn't release until the iteration ends. Right after that iteration ends,
            it starts the next one. This means, we only have time between the destruction of log_lock and then
            the initialization of log_lock in the next iteration. Since that happens as basically the next instruction
            there basically isn't any time for CsvProcessor to acquire a lock on the mutex.
            */

            // Lock first, then check predicate, if false unlock and then wait
            std::unique_lock log_lock(m_log_signal->m_log_mutex);

            /*
            When it wakes up it tries to re-lock the mutex and check the
            predicate.
            */
            m_log_signal->m_cv.wait_for(log_lock, std::chrono::seconds(10),
                                        /*
                                        When the condition variable is woken up (spurious or through
                                        notify_all()) and this predicate returns true, the wait is stopped.
                                        */
                                        [this]() {
                                            bool stop_waiting = !m_log_signal->active_processors.load();
                                            return stop_waiting;
                                        });

        }  // wait() reacquired the lock on exit. So we release it here since there is no reason to hold it while
           // printing.

        /*
        Read and log
        Important: after unlocking as we don't want to block strategies while waiting for dequeue if queue is empty
        */
        std::string message;
        log_queue.DequeueWithTimeout(10, message);
        if (!message.empty()) {
            Logging::log(message);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }  // end while

    Logging::log("Shutdown requested. Processing remaining " + std::to_string(log_queue.Size()) + " messages...",
                 Logging::Level::INFO, name);
    std::string msg;
    log_queue.DequeueWithTimeout(1000, msg);
    while (!msg.empty()) {
        Logging::log(msg, Logging::Level::INFO);
        msg.clear();
        log_queue.DequeueWithTimeout(1000, msg);
    }

    Logging::log("Shutting down", Logging::Level::INFO, name);
}

void Logging::LogProcessor::stop() { m_should_run = false; }

Logging::LogProcessor::~LogProcessor() {}