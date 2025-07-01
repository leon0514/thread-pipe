#include "ThreadWrapper/ThreadWrapperMgr.hpp"

ThreadWrapperMgr::ThreadWrapperMgr(
    std::unique_ptr<ThreadWrapper> thread_instance,
    const std::string& thread_name,
    uint32_t msg_queue_size)
    : thread_instance_(std::move(thread_instance)),
      name_(thread_name),
      msg_queue_(msg_queue_size),
      status_(ThreadWrapperStatus::READY)
{
}

ThreadWrapperMgr::~ThreadWrapperMgr()
{
    if (thread_.joinable()) {
        join_thread();
    }
}

void ThreadWrapperMgr::start_thread()
{
    thread_ = std::thread([this]() { this->thread_entry(); });
}

void ThreadWrapperMgr::join_thread()
{
    if (thread_.joinable()) {
        thread_.join();
    }
}

// 采用“毒丸”模式，修复死锁问题
void ThreadWrapperMgr::thread_entry()
{
    if (!thread_instance_) {
        init_promise_.set_value(false);
        return;
    }

    if (thread_instance_->initialize() != ThreadWrapperError::OK) {
        set_status(ThreadWrapperStatus::ERROR);
        init_promise_.set_value(false);
        return;
    }

    set_status(ThreadWrapperStatus::RUNNING);
    init_promise_.set_value(true);

    while (true) {
        std::shared_ptr<ThreadWrapperMessage> msg;
        msg_queue_.wait_and_pop(msg);

        if (!msg) {
            break; 
        }

        if (thread_instance_->process(msg->msg_id, msg->data) != ThreadWrapperError::OK) {
            set_status(ThreadWrapperStatus::ERROR);
            break;
        }
    }

    set_status(ThreadWrapperStatus::EXITED);
}


ThreadWrapperError ThreadWrapperMgr::wait_for_init()
{
    auto init_future = init_promise_.get_future();
    if (init_future.get()) {
        return ThreadWrapperError::OK;
    }
    return ThreadWrapperError::START_THREAD_FAILED;
}

ThreadWrapperError ThreadWrapperMgr::push_message_to_queue(std::shared_ptr<ThreadWrapperMessage> message)
{
    if (status_ == ThreadWrapperStatus::EXITED || status_ == ThreadWrapperStatus::ERROR) 
    {
       if (message != nullptr) 
       {
            return ThreadWrapperError::THREAD_ABNORMAL;
       }
    }
    if (!msg_queue_.push(std::move(message))) 
    {
        return ThreadWrapperError::ENQUEUE_FAILED;
    }
    return ThreadWrapperError::OK;
}

uint32_t ThreadWrapperMgr::get_queue_size() const {
    return msg_queue_.size();
}