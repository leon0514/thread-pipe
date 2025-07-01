#ifndef THREADWRAPPERAPP_HPP
#define THREADWRAPPERAPP_HPP

#include <vector>
#include <memory>
#include "ThreadWrapper/ThreadDetails.hpp"
#include "ThreadWrapper/ThreadWrapperMgr.hpp"

class ThreadWrapperApp
{
public:
    static ThreadWrapperApp& get_instance();

    ~ThreadWrapperApp();

    ThreadWrapperApp(const ThreadWrapperApp&) = delete;
    ThreadWrapperApp& operator=(const ThreadWrapperApp&) = delete;

    int create_thread_wrapper(std::unique_ptr<ThreadWrapper> thread_instance, const std::string& instance_name, int device_id, uint32_t msg_queue_size);

    ThreadWrapperError start(std::vector<ThreadWrapperParam>& thread_param_list);
    
    int get_thread_wrapper_id_by_name(const std::string& thread_name) const;
    ThreadWrapperError send_message(int dest_id, int msg_id, std::shared_ptr<void> data);

    void stop();
    void stop_threads(const std::vector<int>& thread_ids);

    std::optional<ThreadDetails> get_thread_details_by_name(const std::string& name) const;

private:
    ThreadWrapperApp();
    
    int create_thread_wrapper_mgr(std::unique_ptr<ThreadWrapper> thread_instance, const std::string& instance_name, int device_id, uint32_t msg_queue_size);
    bool is_name_unique(const std::string& thread_name) const;
    void release_threads();

    std::vector<std::unique_ptr<ThreadWrapperMgr>> thread_mgr_list_;

    mutable std::mutex app_mutex_;

    static constexpr int MAIN_THREAD_ID = 0;
};

ThreadWrapperApp& get_thread_wrapper_app_instance();
ThreadWrapperError send_message(int dest, int msg_id, std::shared_ptr<void> data);
int get_thread_wrapper_id_by_name(const std::string& thread_name);

#endif // THREADWRAPPERAPP_HPP