#ifndef LOG_SIGNAL_H
#define LOG_SIGNAL_H

#include <atomic>
#include <condition_variable>

class LogSignal {
   public:
    std::atomic<size_t> active_processors = 0;
    std::mutex m_log_mutex;
    std::condition_variable m_cv;
};

#endif