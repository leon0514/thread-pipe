#ifndef THREADWRAPPERMGR_HPP
#define THREADWRAPPERMGR_HPP

#include <memory>
#include <thread>
#include <string>
#include <future>
#include <atomic>

#include "ThreadWrapper/ThreadSafeQueue.hpp"
#include "ThreadWrapper/ThreadWrapper.hpp"
#include "ThreadWrapper/ThreadWrapperMessage.hpp"

enum class ThreadWrapperStatus {
    READY,
    RUNNING,
    EXITING,
    EXITED,
    ERROR
};

class ThreadWrapperMgr
{
public:
    ThreadWrapperMgr(std::unique_ptr<ThreadWrapper> thread_instance, const std::string& thread_name, uint32_t msg_queue_size);
    ~ThreadWrapperMgr();

    ThreadWrapperMgr(const ThreadWrapperMgr&) = delete;
    ThreadWrapperMgr& operator=(const ThreadWrapperMgr&) = delete;

    void start_thread();
    void join_thread();

    const std::string& get_thread_name() const noexcept { return name_; }
    ThreadWrapperStatus get_status() const noexcept { return status_; }
    void set_status(ThreadWrapperStatus status) noexcept { status_ = status; }
    
    ThreadWrapperError push_message_to_queue(std::shared_ptr<ThreadWrapperMessage> message);
    ThreadWrapperError wait_for_init();

    uint32_t get_queue_size() const;

private:
    void thread_entry();

    std::unique_ptr<ThreadWrapper> thread_instance_;
    std::string name_;
    ThreadSafeQueue<std::shared_ptr<ThreadWrapperMessage>> msg_queue_;

    std::thread thread_;
    std::promise<bool> init_promise_;

    std::atomic<ThreadWrapperStatus> status_;
};

#endif // THREADWRAPPERMGR_HPP