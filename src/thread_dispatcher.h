#ifndef THREAD_DISPATCHER_H
#define THREAD_DISPATCHER_H

#include <iostream>
#include <memory>
#include <thread>

#include "data_source.h"
#include "logging/log_signal.h"

class SignalChannel;

class ThreadDispatcher {
   private:
    std::shared_ptr<DataSource> m_producer;
    std::thread m_thread;
    std::shared_ptr<SignalChannel> m_sig_channel;
    std::shared_ptr<LogSignal> m_log_signal;

    static void Loop(ThreadDispatcher *self) {
        std::shared_ptr<DataSource> producer = self->m_producer;
        while (!self->m_sig_channel->m_shutdown_requested.load()) {
            {
                /*
                Without blocks log_lock doesn't release until the iteration ends. Right after that iteration ends,
                it starts the next one. This means, we only have time between the destruction of lock and then
                the initialization of lock in the next iteration. Since that happens as basically the next
                instruction there basically isn't any time for anyone else to acquire a lock on the mutex.
                */

                // Lock first, then check predicate, if false unlock and then wait
                std::unique_lock lock(self->m_sig_channel->m_cv_mutex);

                /*
                When it wakes up it tries to re-lock the mutex and check the
                predicate.
                */
                self->m_sig_channel->m_cv.wait_for(lock,
                                                   // wait for up to 5 milliseconds
                                                   std::chrono::milliseconds(5),

                                                   // when the condition variable is woken up and this predicate
                                                   // returns true, the wait is stopped:
                                                   [&self]() {
                                                       bool stop_waiting =
                                                           self->m_sig_channel->m_shutdown_requested.load();
                                                       return stop_waiting;
                                                   });

                if (self->m_sig_channel->m_shutdown_requested.load()) {
                    break;
                }
            }  // wait() reacquired the lock on exit. So we release it here since there is no reason to hold it while
               // polling.

            /*
            Atomic since we are modifying it from multiple processors and we want it to be be threadsafe.

            Although atomic and threadsafe when multiple CsvProcessors are modifying it is better to lock in case:
            1. A LogProcessor was waken up (maybe by a different CsvProcessor) and is ready to check the condition to
            see if it is allowed to log. It expects the condition to be true. At this point the LogProcessor is not in a
            waiting state.
            2. Just before it manages to reacquire the mutex and check, another CsvProcessor is scheduled and wants to
            parse a CSV file. Because we want the CsvProcessors to have higher priority than logging, we quickly lock
            the mutex to prevent the LogProcessor from doing its work. Even if it is just checking the condition.

            But then again... if we are locking anyways we don't really need atomic.
            */
            {
                std::unique_lock lock(self->m_log_signal->m_log_mutex);
                self->m_log_signal->active_processors.fetch_add(1);
            }
            producer->Poll();
            /*
            Why do we protect writes to shared var even if it is atomic?
            There could be problems if write to shared variable happens between checking it in predicate and waiting on
            condition. Consider the following:
            1. Waiting thread (e.g. LogProcessor) wakes up spuriously, aquires mutex, checks predicate and evaluates it
            to false, so it must wait in cv again.
            2. Controlling thread (e.g. CsvProcessor) set the shared variable to true (or 0 in case of our count).
            3. Controlling thread sends notification, which is not received by anybody, because there is no thread
            waiting on condition variable
            4. Waiting thread waits on condition variable. Since notification was already sent, it would wait until the
            next spurious wakeup, or next time when controlling thread sends notification. Potentially waiting
            indefinetly.
            */
            {
                std::unique_lock lock(self->m_log_signal->m_log_mutex);
                self->m_log_signal->active_processors.fetch_add(-1);
            }
            auto delay = std::chrono::milliseconds(producer->NextPollInterval());
            std::this_thread::sleep_for(delay);
        }
        std::cout << "ThreadDispatcher shutting down:" << self->m_sig_channel->m_shutdown_requested.load() << std::endl;
        producer->Stop();
    }

   public:
    explicit ThreadDispatcher(std::shared_ptr<DataSource> p, std::shared_ptr<SignalChannel> s,
                              std::shared_ptr<LogSignal> log_signal)
        : m_producer(p), m_sig_channel(s), m_log_signal(log_signal), m_thread(Loop, this) {}

    ~ThreadDispatcher() { m_thread.join(); }
};

#endif