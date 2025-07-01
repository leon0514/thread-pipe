#ifndef TASK_MANAGER_HPP
#define TASK_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "ThreadWrapper/ThreadWrapperApp.hpp" // Needs ThreadWrapperParam

// A structure to hold information about a running task.
struct TaskInfo {
    std::string name;
    std::vector<int> thread_ids;
};

class TaskManager {
public:
    // Get the singleton instance
    static TaskManager& get_instance();

    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    /**
     * @brief Creates and starts a new task composed of multiple threads.
     * @param task_name A unique name for the task.
     * @param thread_params A list of parameters for each thread in the task.
     * @return true on success, false if the task name already exists or creation fails.
     */
    bool create_task(const std::string& task_name, std::vector<ThreadWrapperParam>& thread_params);

    /**
     * @brief Stops a running task by its name, shutting down all its associated threads.
     * @param task_name The name of the task to stop.
     * @return true on success, false if the task is not found.
     */
    bool stop_task(const std::string& task_name);

    /**
     * @brief Checks if a task with the given name exists.
     */
    bool task_exists(const std::string& task_name) const;

private:
    TaskManager(); // Private constructor for singleton
    ~TaskManager(); // Destructor to clean up any remaining tasks

    std::map<std::string, TaskInfo> running_tasks_;
    mutable std::mutex task_mutex_; // To protect access to the running_tasks_ map
};

// Global accessor function for convenience
inline TaskManager& get_task_manager_instance() {
    return TaskManager::get_instance();
}

#endif // TASK_MANAGER_HPP