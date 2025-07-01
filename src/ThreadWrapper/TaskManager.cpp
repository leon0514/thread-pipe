#include "ThreadWrapper/TaskManager.hpp"
#include <iostream>

TaskManager& TaskManager::get_instance() {
    static TaskManager instance;
    return instance;
}

TaskManager::TaskManager() = default;

TaskManager::~TaskManager() {
    std::vector<std::string> task_names;
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        for (const auto& pair : running_tasks_) {
            task_names.push_back(pair.first);
        }
    }
    for (const auto& name : task_names) {
        std::cout << "TaskManager destructor: stopping task '" << name << "'" << std::endl;
        stop_task(name);
    }
}


bool TaskManager::task_exists(const std::string& task_name) const {
    std::lock_guard<std::mutex> lock(task_mutex_);
    return running_tasks_.count(task_name) > 0;
}

bool TaskManager::create_task(const std::string& task_name, std::vector<ThreadWrapperParam>& thread_params) {
    if (task_name.empty() || thread_params.empty()) {
        return false;
    }
    
    // Ensure task name is unique
    if (task_exists(task_name)) {
        std::cerr << "Error: Task with name '" << task_name << "' already exists." << std::endl;
        return false;
    }

    // Use the underlying ThreadWrapperApp to start the threads
    auto& app = ThreadWrapperApp::get_instance();
    if (app.start(thread_params) != ThreadWrapperError::OK) {
        // NOTE: This is a simplification. If start fails midway, some threads might be running.
        // A robust implementation would need to roll back and stop the partially created threads.
        std::cerr << "Error: Failed to start threads for task '" << task_name << "'." << std::endl;
        return false;
    }

    // If successful, gather the new thread IDs and store the task info
    TaskInfo new_task;
    new_task.name = task_name;
    for (const auto& param : thread_params) {
        if (param.thread_instance_id != INVALID_INSTANCE_ID) {
            new_task.thread_ids.push_back(param.thread_instance_id);
        }
    }

    if (new_task.thread_ids.empty()) {
        // This case should ideally not happen if app.start() was successful
        return false;
    }

    std::lock_guard<std::mutex> lock(task_mutex_);
    running_tasks_[task_name] = new_task;

    std::cout << "Task '" << task_name << "' created successfully with " << new_task.thread_ids.size() << " threads." << std::endl;
    return true;
}

bool TaskManager::stop_task(const std::string& task_name) {
    TaskInfo task_to_stop;
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        auto it = running_tasks_.find(task_name);
        if (it == running_tasks_.end()) {
            std::cerr << "Error: Task '" << task_name << "' not found." << std::endl;
            return false;
        }
        task_to_stop = it->second;
        running_tasks_.erase(it); // Remove from map
    }
    
    // Use the new method in ThreadWrapperApp to stop only the threads of this task
    auto& app = ThreadWrapperApp::get_instance();
    app.stop_threads(task_to_stop.thread_ids);
    
    std::cout << "Task '" << task_name << "' stopped successfully." << std::endl;
    return true;
}