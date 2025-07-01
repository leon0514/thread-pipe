#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>

#include "ThreadWrapper/ThreadWrapperApp.hpp"
#include "TestThread/ProducerThread.hpp"
#include "TestThread/ProcessorThread.hpp"
#include "TestThread/ConsumerThread.hpp"
#include "TestThread/param.hpp" 

int main(int argc, char* argv[])
{
    std::cout << "========================================" << std::endl;
    std::cout << "             线程管道应用启动...         " << std::endl;
    std::cout << "========================================" << std::endl;

    auto& app = get_thread_wrapper_app_instance();

    auto result_queue = std::make_shared<ThreadSafeQueue<std::shared_ptr<PipelineMessage>>>(2048);
    
    std::vector<std::string> pipeline_route = {"Consumer", "Processor"};
    
    std::string flow_str = "Producer";
    for (auto it = pipeline_route.rbegin(); it != pipeline_route.rend(); ++it) {
        flow_str += " -> " + *it;
    }
    std::cout << "\n[配置] 消息管道流程: " << flow_str << std::endl;

    std::vector<ThreadWrapperParam> params;

    params.push_back({
        std::make_unique<ProducerThread>(pipeline_route),
        "Producer",
        0,
        INVALID_INSTANCE_ID,
        128
    });

    params.push_back({
        std::make_unique<ProcessorThread>(),
        "Processor",
        0,
        INVALID_INSTANCE_ID,
        128
    });

    params.push_back({
        std::make_unique<ConsumerThread>(result_queue),
        "Consumer",
        0,
        INVALID_INSTANCE_ID,
        128
    });

    std::cout << "\n[启动] 正在启动所有线程..." << std::endl;
    if (app.start(params) != ThreadWrapperError::OK) {
        std::cerr << "错误: 启动线程框架失败！程序将退出。" << std::endl;
        return -1;
    }
    std::cout << "[成功] 所有线程已成功启动并初始化。" << std::endl;

    int producer_id = app.get_thread_wrapper_id_by_name("Producer");
    if (producer_id != INVALID_INSTANCE_ID) {
        std::cout << "\n[运行] 向 Producer (ID: " << producer_id << ") 发送启动信号，开始生成消息..." << std::endl;
        send_message(producer_id, static_cast<int>(MessageId::APP_START), nullptr);
    } else {
        std::cerr << "错误: 找不到名为 'Producer' 的线程！" << std::endl;
        app.stop();
        return -1;
    }

    const int run_duration_seconds = 3;
    std::cout << "[运行] 应用将运行 " << run_duration_seconds << " 秒以处理数据..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(run_duration_seconds));

    std::cout << "\n[关闭] 正在准备关闭所有线程..." << std::endl;
    app.stop();
    std::cout << "[成功] 所有工作线程已安全停止。" << std::endl;

    std::cout << "\n[验证] 开始处理并统计结果队列中的数据..." << std::endl;
    std::shared_ptr<PipelineMessage> msg;
    int message_count = 0;
    while (result_queue->try_pop(msg)) {
        if (msg) {
            // 在我们的管道中，每个消息的值都会被 Processor (+1) 和 Consumer (+2) 修改
            // 所以最终值应该是 原始值 + 3
            // std::cout << "  收到结果: " << msg->value << std::endl;
            message_count++;
        }
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "             应用运行报告             " << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "  运行时间: " << run_duration_seconds << " 秒" << std::endl;
    std::cout << "  处理的消息总数: " << message_count << " 条" << std::endl;
    if (run_duration_seconds > 0 && message_count > 0) {
        std::cout << "  平均处理速率: " << message_count / run_duration_seconds << " 条/秒" << std::endl;
    }
    std::cout << "========================================" << std::endl;
    std::cout << "\n应用已安全退出。" << std::endl;

    return 0;
}