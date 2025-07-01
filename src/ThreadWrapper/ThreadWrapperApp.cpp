#include "ThreadWrapper/ThreadWrapperApp.hpp"
#include <cstdio>
#include <utility>

ThreadWrapperApp& ThreadWrapperApp::get_instance()
{
    static ThreadWrapperApp app;
    return app;
}

ThreadWrapperApp& get_thread_wrapper_app_instance()
{
    return ThreadWrapperApp::get_instance();
}

ThreadWrapperError send_message(int dest, int msg_id, std::shared_ptr<void> data)
{
    return ThreadWrapperApp::get_instance().send_message(dest, msg_id, std::move(data));
}

int get_thread_wrapper_id_by_name(const std::string& thread_name)
{
    return ThreadWrapperApp::get_instance().get_thread_wrapper_id_by_name(thread_name);
}


ThreadWrapperApp::ThreadWrapperApp() 
{
    auto main_thread_mgr = std::make_unique<ThreadWrapperMgr>(nullptr, "main", 1);
    main_thread_mgr->set_status(ThreadWrapperStatus::RUNNING);
    thread_mgr_list_.push_back(std::move(main_thread_mgr));
}

ThreadWrapperApp::~ThreadWrapperApp()
{
    release_threads();
}

ThreadWrapperError ThreadWrapperApp::start(std::vector<ThreadWrapperParam>& thread_param_list)
{
    // *** FIX: Record the starting index of the new threads we are about to create. ***
    const size_t start_index = thread_mgr_list_.size();

    for (auto& params : thread_param_list) 
    {
        int instance_id = create_thread_wrapper_mgr(
            std::move(params.thread_instance),
            params.thread_instance_name,
            params.device_id,
            params.queue_size);

        if (instance_id == INVALID_INSTANCE_ID) 
        {
            // If creation fails, we should ideally roll back, but for now, we stop everything.
            printf("错误: 创建线程管理器 '%s' 失败。请检查线程名是否重复。\n", params.thread_instance_name.c_str());
            release_threads();
            return ThreadWrapperError::ERROR;
        }
        params.thread_instance_id = instance_id;
    }

    // *** FIX: Iterate only over the NEW threads that were added in THIS call. ***
    for (size_t i = start_index; i < thread_mgr_list_.size(); ++i) 
    {
        thread_mgr_list_[i]->start_thread();
    }

    // *** FIX: Wait only for the NEW threads to initialize. ***
    for (size_t i = start_index; i < thread_mgr_list_.size(); ++i) 
    {
        if (thread_mgr_list_[i]->wait_for_init() != ThreadWrapperError::OK) 
        {
            printf("错误: 线程 '%s' 初始化失败。\n", thread_mgr_list_[i]->get_thread_name().c_str());
            release_threads(); // Stop all threads on failure
            return ThreadWrapperError::START_THREAD_FAILED;
        }
    }

    return ThreadWrapperError::OK;
}

void ThreadWrapperApp::stop()
{
    release_threads();
}

void ThreadWrapperApp::stop_threads(const std::vector<int>& thread_ids)
{
    // Step 1: Send stop signal (poison pill) to specified threads
    for (int id : thread_ids) {
        if (id > 0 && static_cast<size_t>(id) < thread_mgr_list_.size()) {
            auto& mgr = thread_mgr_list_[id];
            if (mgr && mgr->get_status() == ThreadWrapperStatus::RUNNING) {
                mgr->set_status(ThreadWrapperStatus::EXITING);
                mgr->push_message_to_queue(nullptr); // Send poison pill
            }
        }
    }

    // Step 2: Wait for specified threads to join
    for (int id : thread_ids) {
        if (id > 0 && static_cast<size_t>(id) < thread_mgr_list_.size()) {
            if (thread_mgr_list_[id]) {
                thread_mgr_list_[id]->join_thread();
            }
        }
    }

    // Note: This implementation does not remove the thread managers from thread_mgr_list_
    // to keep thread IDs stable. A more complex implementation might mark them as 'reusable'.
    // For now, we assume tasks are created and destroyed, but not recreated with the same IDs.
}

void ThreadWrapperApp::release_threads()
{
    for (size_t i = MAIN_THREAD_ID + 1; i < thread_mgr_list_.size(); ++i) 
    {
        if (thread_mgr_list_[i] && thread_mgr_list_[i]->get_status() == ThreadWrapperStatus::RUNNING) 
        {
            thread_mgr_list_[i]->set_status(ThreadWrapperStatus::EXITING);
            thread_mgr_list_[i]->push_message_to_queue(nullptr);
        }
    }

    for (size_t i = MAIN_THREAD_ID + 1; i < thread_mgr_list_.size(); ++i) 
    {
        if (thread_mgr_list_[i]) 
        {
            thread_mgr_list_[i]->join_thread();
        }
    }

    thread_mgr_list_.clear();
}

int ThreadWrapperApp::create_thread_wrapper_mgr(
    std::unique_ptr<ThreadWrapper> thread_instance, const std::string& instance_name, int device_id, uint32_t msg_queue_size)
{
    if (!thread_instance || !is_name_unique(instance_name)) 
    {
        return INVALID_INSTANCE_ID;
    }

    int instance_id = thread_mgr_list_.size();
    if (thread_instance->configure(instance_id, instance_name, device_id) != ThreadWrapperError::OK) 
    {
        return INVALID_INSTANCE_ID;
    }

    auto th_mgr = std::make_unique<ThreadWrapperMgr>(std::move(thread_instance), instance_name, msg_queue_size);
    thread_mgr_list_.push_back(std::move(th_mgr));

    return instance_id;
}

bool ThreadWrapperApp::is_name_unique(const std::string& thread_name) const
{
    if (thread_name.empty()) 
    {
        return false;
    }
    for (const auto& mgr : thread_mgr_list_) 
    {
        if (mgr && mgr->get_thread_name() == thread_name) 
        {
            return false;
        }
    }
    return true;
}


int ThreadWrapperApp::get_thread_wrapper_id_by_name(const std::string& thread_name) const
{
    if (thread_name.empty()) 
    {
        return INVALID_INSTANCE_ID;
    }
    for (size_t i = 0; i < thread_mgr_list_.size(); ++i) 
    {
        if (thread_mgr_list_[i] && thread_mgr_list_[i]->get_thread_name() == thread_name) 
        {
            return static_cast<int>(i);
        }
    }
    return INVALID_INSTANCE_ID;
}

ThreadWrapperError ThreadWrapperApp::send_message(int dest_id, int msg_id, std::shared_ptr<void> data)
{
    if (dest_id <= 0 || static_cast<size_t>(dest_id) >= thread_mgr_list_.size()) 
    {
        return ThreadWrapperError::ERROR_DEST_INVALID;
    }

    auto p_message = std::make_shared<ThreadWrapperMessage>();
    p_message->dest = dest_id;
    p_message->msg_id = msg_id;
    p_message->data = std::move(data);

    return thread_mgr_list_[dest_id]->push_message_to_queue(p_message);
}