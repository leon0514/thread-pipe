#ifndef THREADWRAPPERMGR_HPP__
#define THREADWRAPPERMGR_HPP__

#include <memory>
#include <thread>
#include "ThreadSafeQueue.hpp"
#include "ThreadWrapper.hpp"
#include "ThreadWrapperType.hpp"

enum ThreadWrapperStatus {
    THREAD_READY = 0,
    THREAD_RUNNING = 1,
    THREAD_EXITING = 2,
    THREAD_EXITED = 3,
    THREAD_ERROR = 4,
};


class ThreadWrapperMgr
{
public:
    ThreadWrapperMgr(ThreadWrapper* thread_instance, const std::string& thread_name, const uint32_t mag_queue_size);
    ~ThreadWrapperMgr();

    static void thread_entry(void* data);

    ThreadWrapper* get_instance()
    {
        return this->thread_instance_;
    }

    const std::string& get_thread_name()
    {
        return name_;
    }

    ThreadWrapperError push_message_to_queue(std::shared_ptr<ThreadWrapperMessage>& messgae);

    std::shared_ptr<ThreadWrapperMessage> pop_message_from_queue()
    {
        return this->msg_queue_.pop();
    }

    void create_thread();

    void set_status(ThreadWrapperStatus status)
    {
        status_ = status;
    }

    ThreadWrapperStatus get_status()
    {
        return status_;
    }

    ThreadWrapperError wait_thread_init_end();


private:
    bool running_;
    ThreadWrapperStatus status_;
    ThreadWrapper* thread_instance_;
    std::string name_;
    ThreadSafeQueue<std::shared_ptr<ThreadWrapperMessage>> msg_queue_;

};

#endif // THREADWRAPPERMGR_HPP