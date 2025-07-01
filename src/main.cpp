#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>

#include "Task/TaskManager.hpp"

#include "ThreadWrapper/ThreadWrapperApp.hpp"
#include "TestThread/ProducerThread.hpp"
#include "TestThread/ProcessorThread.hpp"
#include "TestThread/ConsumerThread.hpp"
#include "TestThread/param.hpp" 


int main() {
    auto& task_manager = get_task_manager_instance();

    // --- Task A needs a Producer and the shared Logger ---
    std::cout << "--- Creating Task A (uses Producer and Logger) ---" << std::endl;
    std::vector<ThreadWrapperParam> task_a_params;

    // *** FIX: Explicitly create an empty vector for the constructor. ***
    // The ProducerThread needs a (possibly empty) routing slip.
    task_a_params.push_back({
        std::make_unique<ProducerThread>(std::vector<std::string>{}), 
        "Producer-A"
    });
    // The "Logger" thread will be reused.
    task_a_params.push_back({
        std::make_unique<ProcessorThread>(), // This one was correct as it has a default constructor.
        "Logger"
    });
    
    if (!task_manager.create_task("TaskA", task_a_params)) {
        std::cerr << "Failed to create Task A" << std::endl;
        return -1;
    }


    // --- Task B needs a Consumer and the shared Logger ---
    std::cout << "\n--- Creating Task B (uses Consumer and Logger) ---" << std::endl;
    std::vector<ThreadWrapperParam> task_b_params;
    auto result_queue = std::make_shared<ThreadSafeQueue<std::shared_ptr<PipelineMessage>>>(100);
    
    task_b_params.push_back({
        std::make_unique<ConsumerThread>(result_queue), 
        "Consumer-B"
    });
    // Reuse the "Logger" thread.
    task_b_params.push_back({
        std::make_unique<ProcessorThread>(),
        "Logger"
    });

    if (!task_manager.create_task("TaskB", task_b_params)) {
        std::cerr << "Failed to create Task B" << std::endl;
        return -1;
    }

    
    std::cout << "\n--- Both tasks running for 5 seconds ---" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // --- Stop Task A ---
    std::cout << "\n--- Stopping Task A ---" << std::endl;
    task_manager.stop_task("TaskA");
    
    std::cout << "\n--- Task A stopped. Task B (with Logger) is still running for 3 seconds ---" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // --- Stop Task B ---
    std::cout << "\n--- Stopping Task B ---" << std::endl;
    task_manager.stop_task("TaskB");

    std::cout << "\n--- Application exiting ---" << std::endl;
    // TaskManager destructor will be called here, ensuring all resources are freed.
    return 0;
}