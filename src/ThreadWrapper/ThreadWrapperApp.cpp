#include "ThreadWrapperApp.hpp"
#include <unistd.h>

namespace 
{
    const uint32_t wait_interval = 10000;
    const uint32_t thread_exit_retry = 3;
}


ThreadWrapperApp::ThreadWrapperApp():is_released_(false), wait_end_(false)
{
    initialize();
}

ThreadWrapperApp::~ThreadWrapperApp()
{
    release_threads();
}


ThreadWrapperError ThreadWrapperApp::initialize()
{
    const uint32_t msg_queue_size = 256;
    ThreadWrapperMgr* th_mgr = new ThreadWrapperMgr(nullptr, "main", msg_queue_size);
    thread_wrapper_mgr_list_.push_back(th_mgr);
    th_mgr->set_status(THREAD_RUNNING);
    return TW::OK;
}

ThreadWrapperError ThreadWrapperApp::start(std::vector<ThreadWrapperParam>& thread_param_list)
{
    for (size_t i = 0; i < thread_param_list.size(); i++) {
        int instance_id = create_thread_wrapper_mgr(
            thread_param_list[i].thread_instance,
            thread_param_list[i].thread_instance_name,
            thread_param_list[i].device_id,
            thread_param_list[i].queue_size);
        if (instance_id == INVALID_INSTANCE_ID) 
        {
            printf("Create thread_wrapper_mgr failed");
            return TW::ERROR;
        }
        thread_param_list[i].thread_instance_id = instance_id;
    }
    // Note:The instance id must generate first, then create thread,
    // for the user thread get other thread instance id in Init function
    for (size_t i = 0; i < thread_param_list.size(); i++) 
    {
        thread_wrapper_mgr_list_[thread_param_list[i].thread_instance_id]->create_thread();
    }

    for (size_t i = 0; i < thread_param_list.size(); i++) 
    {
        int instance_id = thread_param_list[i].thread_instance_id;
        ThreadWrapperError ret = thread_wrapper_mgr_list_[instance_id]->wait_thread_init_end();
        if (ret != TW::OK) 
        {
            return ret;
        }
    }

    return TW::OK;
}

int ThreadWrapperApp::create_thread_wrapper(ThreadWrapper* thread_instance, const std::string instance_name, int device_id, const uint32_t msg_queue_size)
{
    int instance_id = create_thread_wrapper_mgr(thread_instance, instance_name, device_id, msg_queue_size);
    if (instance_id == INVALID_INSTANCE_ID) 
    {
        return INVALID_INSTANCE_ID;
    }

    thread_wrapper_mgr_list_[instance_id]->create_thread();
    ThreadWrapperError ret = thread_wrapper_mgr_list_[instance_id]->wait_thread_init_end();
    if (ret != TW::OK) 
    {
        return INVALID_INSTANCE_ID;
    }

    return instance_id;
}

int ThreadWrapperApp::create_thread_wrapper_mgr(
    ThreadWrapper* thread_instance, const std::string& instance_name, int device_id, const uint32_t msg_queue_size)
{
    if (!check_thread_name_unique(instance_name)) 
    {
        return INVALID_INSTANCE_ID;
    }

    int instance_id = thread_wrapper_mgr_list_.size();
    ThreadWrapperError ret = thread_instance->base_config(instance_id, instance_name, device_id);
    if (ret != TW::OK) 
    {
        return INVALID_INSTANCE_ID;
    }

    ThreadWrapperMgr* th_mgr = new ThreadWrapperMgr(thread_instance, instance_name, msg_queue_size);
    thread_wrapper_mgr_list_.push_back(th_mgr);

    return instance_id;
}

bool ThreadWrapperApp::check_thread_name_unique(const std::string& thread_name)
{
    if (thread_name.size() == 0) {
        return false;
    }

    for (size_t i = 0; i < thread_wrapper_mgr_list_.size(); i++) 
    {
        if (thread_name == thread_wrapper_mgr_list_[i]->get_thread_name()) 
        {
            return false;
        }
    }

    return true;
}


void ThreadWrapperApp::release_threads()
{
    if (is_released_) return;
    thread_wrapper_mgr_list_[g_main_thread_id]->set_status(THREAD_EXITED);

    for (uint32_t i = 1; i < thread_wrapper_mgr_list_.size(); i++) 
    {
        if ((thread_wrapper_mgr_list_[i] != nullptr) &&
            (thread_wrapper_mgr_list_[i]->get_status() == THREAD_RUNNING))
            thread_wrapper_mgr_list_[i]->set_status(THREAD_EXITING);
    }

    int retry = thread_exit_retry;
    while (retry >= 0) 
    {
        bool exit_finish = true;
        for (uint32_t i = 0; i < thread_wrapper_mgr_list_.size(); i++) 
        {
            if (thread_wrapper_mgr_list_[i] == nullptr)
            {
                continue;
            }
            if (thread_wrapper_mgr_list_[i]->get_status() > THREAD_EXITING) 
            {
                delete thread_wrapper_mgr_list_[i];
                thread_wrapper_mgr_list_[i] = nullptr;
            } 
            else 
            {
                exit_finish = false;
            }
        }

        if (exit_finish)
            break;

        sleep(1);
        retry--;
    }
    is_released_ = true;
}


int ThreadWrapperApp::get_thread_wrapper_id_by_name(const std::string& thread_name)
{
    if (thread_name.empty()) 
    {
        return INVALID_INSTANCE_ID;
    }

    for (uint32_t i = 0; i < thread_wrapper_mgr_list_.size(); i++) 
    {
        if (thread_wrapper_mgr_list_[i]->get_thread_name() == thread_name) 
        {
            return i;
        }
    }
    
    return INVALID_INSTANCE_ID;
}

ThreadWrapperError ThreadWrapperApp::send_message(int dest, int msg_id, std::shared_ptr<void> data)
{
    if ((uint32_t)dest > thread_wrapper_mgr_list_.size()) 
    {
        return TW::ERROR_DEST_INVALID;
    }

    std::shared_ptr<ThreadWrapperMessage> p_message = std::make_shared<ThreadWrapperMessage>();
    p_message->dest = dest;
    p_message->msg_id = msg_id;
    p_message->data = data;

    return thread_wrapper_mgr_list_[dest]->push_message_to_queue(p_message);
}

void ThreadWrapperApp::wait()
{
    while (true) 
    {
        usleep(wait_interval);
        if (wait_end_) break;
    }
    thread_wrapper_mgr_list_[g_main_thread_id]->set_status(THREAD_EXITED);
}


ThreadWrapperApp& create_thread_wrapper_app_instance()
{
    return ThreadWrapperApp::get_app_instance();
}


ThreadWrapperApp& get_thread_wrapper_app_instance()
{
    return ThreadWrapperApp::get_app_instance();
}


ThreadWrapperError send_message(int dest, int msg_id, std::shared_ptr<void> data)
{
    ThreadWrapperApp& app = ThreadWrapperApp::get_app_instance();
    return app.send_message(dest, msg_id, data);

}


int get_thread_wrapper_id_by_name(const std::string& thread_name)
{
    ThreadWrapperApp& app = ThreadWrapperApp::get_app_instance();
    return app.get_thread_wrapper_id_by_name(thread_name);
}
