#include "TaskManager.hpp"
#include <iostream>

TaskManager& TaskManager::get_instance() {
    static TaskManager instance;
    return instance;
}

TaskManager::TaskManager() = default;

TaskManager::~TaskManager() {
    std::vector<std::string> task_names;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        for (const auto& pair : running_tasks_) {
            task_names.push_back(pair.first);
        }
    }
    for (const auto& name : task_names) {
        stop_task(name);
    }
}

bool TaskManager::create_task(const std::string& task_name, std::vector<ThreadWrapperParam>& thread_params) {
    if (task_name.empty()) return false;

    std::lock_guard<std::mutex> lock(mtx_);

    if (running_tasks_.count(task_name)) {
        std::cerr << "Error: Task '" << task_name << "' already exists." << std::endl;
        return false;
    }

    auto& app = ThreadWrapperApp::get_instance();
    std::set<std::string> threads_for_this_task;
    std::vector<ThreadWrapperParam> threads_to_create;

    // Step 1: Identify which threads are new and which are reused.
    for (auto& param : thread_params) {
        threads_for_this_task.insert(param.thread_instance_name);
        
        auto it = thread_pool_.find(param.thread_instance_name);
        if (it != thread_pool_.end()) {
            // Thread already exists, just increment its reference count.
            it->second.reference_count++;
            std::cout << "Reusing thread '" << param.thread_instance_name 
                      << "', new reference count: " << it->second.reference_count << std::endl;
        } else {
            // Thread is new, add it to the list of threads to create.
            threads_to_create.push_back(std::move(param));
        }
    }

    // Step 2: Create the new threads.
    if (!threads_to_create.empty()) {
        if (app.start(threads_to_create) != ThreadWrapperError::OK) {
            // This is complex to roll back. For now, we fail.
            // A production system would need to decrement ref counts for reused threads.
            std::cerr << "Error: Failed to start new threads for task '" << task_name << "'." << std::endl;
            return false;
        }

        // Add newly created threads to the pool with a reference count of 1.
        for (const auto& param : threads_to_create) {
            PooledThreadInfo info;
            info.id = param.thread_instance_id;
            info.name = param.thread_instance_name;
            info.reference_count = 1;
            thread_pool_[info.name] = info;
            std::cout << "Created new thread '" << info.name 
                      << "' with ID " << info.id << std::endl;
        }
    }

    // Step 3: Register the task.
    running_tasks_[task_name] = threads_for_this_task;
    std::cout << "Task '" << task_name << "' created successfully." << std::endl;
    return true;
}

bool TaskManager::stop_task(const std::string& task_name) {
    std::lock_guard<std::mutex> lock(mtx_);
    
    auto task_it = running_tasks_.find(task_name);
    if (task_it == running_tasks_.end()) {
        std::cerr << "Error: Task '" << task_name << "' not found." << std::endl;
        return false;
    }

    const auto& threads_used_by_task = task_it->second;
    std::vector<int> threads_to_stop;

    // Step 1: Decrement reference counts for all threads used by this task.
    for (const auto& thread_name : threads_used_by_task) {
        auto pool_it = thread_pool_.find(thread_name);
        if (pool_it != thread_pool_.end()) {
            pool_it->second.reference_count--;
            std::cout << "Decremented reference count for thread '" << thread_name 
                      << "', new count: " << pool_it->second.reference_count << std::endl;
            
            // If reference count drops to zero, mark this thread for stopping.
            if (pool_it->second.reference_count == 0) {
                threads_to_stop.push_back(pool_it->second.id);
                std::cout << "Thread '" << thread_name << "' is now unused and will be stopped." << std::endl;
            }
        }
    }

    // Step 2: Actually stop the threads whose ref count is zero.
    if (!threads_to_stop.empty()) {
        auto& app = ThreadWrapperApp::get_instance();
        app.stop_threads(threads_to_stop);
        
        // Remove the stopped threads from the pool.
        for (const auto& thread_name : threads_used_by_task) {
            if (thread_pool_.count(thread_name) && thread_pool_[thread_name].reference_count == 0) {
                thread_pool_.erase(thread_name);
            }
        }
    }

    // Step 3: Remove the task itself.
    running_tasks_.erase(task_it);
    std::cout << "Task '" << task_name << "' stopped successfully." << std::endl;
    return true;
}