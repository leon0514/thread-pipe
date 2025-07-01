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

// 优化：采用“毒丸”模式，彻底修复死锁问题
void ThreadWrapperMgr::thread_entry()
{
    if (!thread_instance_) {
        init_promise_.set_value(false);
        return;
    }

    if (thread_instance_->initialize() != 0) {
        set_status(ThreadWrapperStatus::ERROR);
        init_promise_.set_value(false);
        return;
    }

    set_status(ThreadWrapperStatus::RUNNING);
    init_promise_.set_value(true);

    // 优化：循环条件改为 true，退出逻辑由“毒丸”消息控制
    while (true) {
        std::shared_ptr<ThreadWrapperMessage> msg;
        msg_queue_.wait_and_pop(msg); // 阻塞等待消息

        // 优化：检查是否收到了“毒丸”(nullptr)，如果是，则退出循环
        if (!msg) {
            break; 
        }

        // 正常处理消息
        if (thread_instance_->process(msg->msg_id, msg->data) != 0) {
            set_status(ThreadWrapperStatus::ERROR);
            break; // 业务处理出错也退出
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