#include "Task/TaskManager.hpp"
#include "TestThread/ProducerThread.hpp"
#include "TestThread/ProcessorThread.hpp"
#include "TestThread/ConsumerThread.hpp"
#include "TestThread/param.hpp"
#include "ThreadWrapper/ThreadDetails.hpp" // Make sure to include this for the detail structs
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip> // For std::setw
#include <optional>

// ====================================================================
// *** FIX: Define helper function BEFORE main so it's declared. ***
// ====================================================================
void print_task_details(const std::vector<TaskDetails>& all_tasks) {
    std::cout << "\n====================== TASK STATUS REPORT ======================\n";
    if (all_tasks.empty()) {
        std::cout << "No running tasks.\n";
    }
    for (const auto& task : all_tasks) {
        std::cout << "  [*] Task: " << std::left << std::setw(15) << task.name << "\n";
        std::cout << "    " << std::left << std::setw(20) << "Thread Name"
                  << std::setw(15) << "Status"
                  << std::setw(15) << "Queue Size"
                  << std::setw(15) << "Ref Count" << "\n";
        std::cout << "    " << std::string(60, '-') << "\n";
        for (const auto& thread : task.threads) {
            std::cout << "    " << std::left << std::setw(20) << thread.name
                      << std::setw(15) << status_to_string(thread.status)
                      << std::setw(15) << thread.queue_size
                      << std::setw(15) << thread.reference_count << "\n";
        }
    }
    std::cout << "================================================================\n";
}

int main() {
    auto& task_manager = get_task_manager_instance();

    // --- Task A needs a Producer and the shared Logger ---
    std::cout << "--- Creating Task A (uses Producer and Logger) ---" << std::endl;
    std::vector<ThreadWrapperParam> task_a_params;
    task_a_params.push_back({std::make_unique<ProducerThread>(std::vector<std::string>{}), "Producer-A"});
    task_a_params.push_back({std::make_unique<ProcessorThread>(), "Logger"});
    if (!task_manager.create_task("TaskA", task_a_params)) {
        std::cerr << "Failed to create Task A" << std::endl;
        return -1;
    }

    // --- Task B needs a Consumer and the shared Logger ---
    std::cout << "\n--- Creating Task B (uses Consumer and Logger) ---" << std::endl;
    std::vector<ThreadWrapperParam> task_b_params;
    auto result_queue = std::make_shared<ThreadSafeQueue<std::shared_ptr<PipelineMessage>>>(100);
    task_b_params.push_back({std::make_unique<ConsumerThread>(result_queue), "Consumer-B"});
    task_b_params.push_back({std::make_unique<ProcessorThread>(), "Logger"});
    if (!task_manager.create_task("TaskB", task_b_params)) {
        std::cerr << "Failed to create Task B" << std::endl;
        return -1;
    }
    
    std::cout << "\n--- Both tasks running for 2 seconds ---" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // DEMO: Query and print details for a SINGLE task before stopping it.
    std::cout << "\n--- Querying details for 'TaskA' before stopping ---" << std::endl;
    auto task_a_details_opt = task_manager.get_task_details_by_name("TaskA");
    
    // *** FIX: Correctly check and use the std::optional object. ***
    if (task_a_details_opt) { // This boolean check is correct for std::optional
        // To use the print function, we put the single result into a vector.
        print_task_details({*task_a_details_opt}); // Use '*' to get the value inside the optional.
    } else {
        std::cout << "Task 'TaskA' not found for querying.\n";
    }

    // Now, stop Task A
    std::cout << "\n--- Stopping Task A ---" << std::endl;
    task_manager.stop_task("TaskA");

    // Query all tasks again to see the result
    std::cout << "\n--- Querying all tasks after stopping TaskA ---" << std::endl;
    print_task_details(task_manager.get_all_task_details());

    // Stop Task B and exit
    std::cout << "\n--- Stopping Task B ---" << std::endl;
    task_manager.stop_task("TaskB");

    std::cout << "\n--- Application exiting ---" << std::endl;
    return 0;
}