#ifndef TASK_MANAGER_HPP
#define TASK_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <mutex>
#include "ThreadWrapper/ThreadWrapperApp.hpp"

// Represents a thread in the global pool, with its reference count.
struct PooledThreadInfo {
    int id = INVALID_INSTANCE_ID;
    std::string name;
    int reference_count = 0;
};

class TaskManager {
public:
    static TaskManager& get_instance();

    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    /**
     * @brief Creates or updates a task, associating it with a set of threads.
     * This is the core function for thread reuse.
     * @param task_name A unique name for the task.
     * @param thread_params A list of parameters for threads this task needs.
     *        If a thread with the same name already exists, it will be reused.
     *        If not, it will be created.
     * @return true on success.
     */
    bool create_task(const std::string& task_name, std::vector<ThreadWrapperParam>& thread_params);

    /**
     * @brief Stops a task. This decrements the reference count of associated threads.
     * A thread is only truly stopped if its reference count drops to zero.
     * @param task_name The name of the task to stop.
     * @return true on success, false if the task is not found.
     */
    bool stop_task(const std::string& task_name);

    std::vector<TaskDetails> get_all_task_details() const;

    std::optional<TaskDetails> get_task_details_by_name(const std::string& task_name) const;


private:
    TaskManager();
    ~TaskManager();

    // The global pool of all active threads, mapped by their unique name.
    std::map<std::string, PooledThreadInfo> thread_pool_;

    // Maps a task name to the set of thread names it uses.
    std::map<std::string, std::set<std::string>> running_tasks_;

    mutable std::mutex mtx_; // A single mutex to protect both maps for simplicity.
};

inline TaskManager& get_task_manager_instance() {
    return TaskManager::get_instance();
}

#endif // TASK_MANAGER_HPP