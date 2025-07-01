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

    // --- Define Task 1: "DataPipeline" ---
    std::cout << "--- Creating Task 1: DataPipeline ---" << std::endl;
    std::vector<ThreadWrapperParam> pipeline_params;
    auto result_queue1 = std::make_shared<ThreadSafeQueue<std::shared_ptr<PipelineMessage>>>(100);
    std::vector<std::string> pipeline_route = {"Consumer-1", "Processor-1"};
    pipeline_params.push_back({std::make_unique<ProducerThread>(pipeline_route), "Producer-1"});
    pipeline_params.push_back({std::make_unique<ProcessorThread>(), "Processor-1"});
    pipeline_params.push_back({std::make_unique<ConsumerThread>(result_queue1), "Consumer-1"});
    
    task_manager.create_task("DataPipeline", pipeline_params);
    
    // Trigger the pipeline
    send_message(get_thread_wrapper_id_by_name("Producer-1"), static_cast<int>(MessageId::APP_START), nullptr);

    // --- Define Task 2: "MonitoringTask" (a simple single-thread task) ---
    std::cout << "\n--- Creating Task 2: MonitoringTask ---" << std::endl;
    std::vector<ThreadWrapperParam> monitor_params;
    monitor_params.push_back({std::make_unique<ProcessorThread>(), "Monitor"}); // Reusing ProcessorThread for demo
    
    task_manager.create_task("MonitoringTask", monitor_params);


    // --- Let tasks run for a while ---
    std::cout << "\n--- All tasks running for 5 seconds ---" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // --- Stop one task specifically ---
    std::cout << "\n--- Stopping Task 1: DataPipeline ---" << std::endl;
    task_manager.stop_task("DataPipeline");

    std::cout << "\n--- DataPipeline stopped, MonitoringTask is still running for 3 more seconds ---" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // The TaskManager destructor will automatically stop any remaining tasks ("MonitoringTask")
    // when main exits. Or you can stop it manually:
    // task_manager.stop_task("MonitoringTask");

    std::cout << "\n--- Application exiting ---" << std::endl;
    return 0; // TaskManager destructor is called here
}