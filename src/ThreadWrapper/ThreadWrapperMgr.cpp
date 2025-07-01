#include "ThreadWrapperMgr.hpp"
#include <unistd.h> 

namespace
{
    const uint32_t wait_10_millseconds = 10000;
    const uint32_t wait_thread_start = 1000;
}

ThreadWrapperMgr::ThreadWrapperMgr(
    ThreadWrapper* thread_instance, 
    const std::string& thread_name, 
    const uint32_t mag_queue_size)
    : running_(false), 
      status_(THREAD_READY), 
      thread_instance_(thread_instance), 
      name_(thread_name), 
      msg_queue_(mag_queue_size)
{
}

ThreadWrapperMgr::~ThreadWrapperMgr()
{
    thread_instance_ = nullptr;
    while(!msg_queue_.empty())
    {
        msg_queue_.pop();
    }
}

void ThreadWrapperMgr::create_thread()
{
    std::thread engine(&ThreadWrapperMgr::thread_entry, (void *)this);
    engine.detach();
}


void ThreadWrapperMgr::thread_entry(void* arg)
{
    ThreadWrapperMgr* th_mgr = (ThreadWrapperMgr*)arg;
    ThreadWrapper* instance = th_mgr->get_instance();
    if (instance == nullptr) 
    {
        return;
    }

    std::string instance_name = instance->self_instance_name();

    auto ret = instance->initialize();
    if (ret) {
        th_mgr->set_status(ThreadWrapperStatus::THREAD_ERROR);
        return;
    }

    th_mgr->set_status(ThreadWrapperStatus::THREAD_RUNNING);
    while (ThreadWrapperStatus::THREAD_RUNNING == th_mgr->get_status()) 
    {
        // printf("%d\n",th_mgr->get_status());
        // get data from queue
        std::shared_ptr<ThreadWrapperMessage> msg = th_mgr->pop_message_from_queue();
        if (msg == nullptr) 
        {
            usleep(wait_10_millseconds);
            continue;
        }
        // call function to process thread msg
        ret = instance->process(msg->msg_id, msg->data);
        msg->data = nullptr;
        if (ret) 
        {
            th_mgr->set_status(ThreadWrapperStatus::THREAD_ERROR);
            return;
        }
        usleep(1);
    }
    th_mgr->set_status(ThreadWrapperStatus::THREAD_EXITED);

    return;
}


ThreadWrapperError ThreadWrapperMgr::wait_thread_init_end()
{
    while (true) 
    {
        if (status_ == ThreadWrapperStatus::THREAD_RUNNING) 
        {
            break;
        } 
        else if (status_ > ThreadWrapperStatus::THREAD_RUNNING) 
        {
            std::string instance_name = thread_instance_->self_instance_name();
            printf("%s thread not running\n", instance_name.c_str());
            return TW::START_THREAD_FAILED;
        } 
        else 
        {
            usleep(wait_thread_start);
        }
    }

    return TW::OK;
}

ThreadWrapperError ThreadWrapperMgr::push_message_to_queue(std::shared_ptr<ThreadWrapperMessage>& messgae)
{
    if (status_ != ThreadWrapperStatus::THREAD_RUNNING) {
        return TW::THREAD_ABNORMAL;
    }
    return msg_queue_.push(messgae) ? TW::OK : TW::ENQUEUE_FAILED;
}
