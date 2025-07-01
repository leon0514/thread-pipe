#ifndef THREADWRAPPERAPP_HPP
#define THREADWRAPPERAPP_HPP

#include "ThreadWrapperMgr.hpp"
#include <vector>

namespace {
    int g_main_thread_id = 0;
}

class ThreadWrapperApp
{
public:
    ThreadWrapperApp();
    ThreadWrapperApp(const ThreadWrapperApp&) = delete;
    ThreadWrapperApp& operator=(const ThreadWrapperApp&) = delete;

    ~ThreadWrapperApp();

    static ThreadWrapperApp& get_app_instance()
    {
        static ThreadWrapperApp app;
        return app;
    }

    int create_thread_wrapper(ThreadWrapper* thread_instance, const std::string instance_name, int device_id, const uint32_t msg_queue_size);

    ThreadWrapperError start(std::vector<ThreadWrapperParam>& thread_param_list);
    void wait();
    
    int get_thread_wrapper_id_by_name(const std::string& thread_name);

    ThreadWrapperError send_message(int dest, int msg_id, std::shared_ptr<void> data);

    void wait_end()
    {
        wait_end_ = true;
    }

    void exit();

private:
    ThreadWrapperError initialize();
    int create_thread_wrapper_mgr(ThreadWrapper* thread_instance, const std::string& instance_name, int device_id, const uint32_t msg_queue_size);
    bool check_thread_abnormal();
    bool check_thread_name_unique(const std::string& thread_name);
    void release_threads();


private:
    bool is_released_;
    bool wait_end_;
    std::vector<ThreadWrapperMgr*> thread_wrapper_mgr_list_;
};

ThreadWrapperApp& create_thread_wrapper_app_instance();
ThreadWrapperApp& get_thread_wrapper_app_instance();
ThreadWrapperError send_message(int dest, int msg_id, std::shared_ptr<void> data);
int get_thread_wrapper_id_by_name(const std::string& thread_name);

void start_thread();

#endif