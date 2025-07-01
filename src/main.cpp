#include "ThreadWrapper/ThreadWrapperApp.hpp"
#include "TestThread/thread0.hpp"
#include "TestThread/thread1.hpp"
#include "TestThread/thread2.hpp"
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>

// 假设 Message 结构体定义在某个头文件中
// 为了编译通过，我们在这里提供一个最小定义
struct Message;

int main(int argc, char* agrv[])
{
    std::cout << "应用启动..." << std::endl;
    auto& app = get_thread_wrapper_app_instance();

    auto result_queue = std::make_shared<ThreadSafeQueue<std::shared_ptr<Message>>>(100);
    
    std::vector<ThreadWrapperParam> params;

    params.push_back({
        std::make_unique<TestThread0>(std::vector<std::string>{"TestThread2", "TestThread1"}),
        "TestThread0"
    });

    params.push_back({
        std::make_unique<TestThread1>(),
        "TestThread1"
    });

    // 优化：将共享的队列传递给 TestThread2
    params.push_back({
        std::make_unique<TestThread2>(result_queue),
        "TestThread2"
    });

    if (app.start(params) != ThreadWrapperError::OK) {
        std::cerr << "启动线程失败！" << std::endl;
        return -1;
    }
    std::cout << "所有线程已成功启动并初始化。" << std::endl;

    // 4. 向所有已启动的线程发送开始工作的消息
    for (const auto& p : params) {
        if (p.thread_instance_id != INVALID_INSTANCE_ID) 
        {
            std::cout << "向 " << p.thread_instance_name << " (ID: " << p.thread_instance_id << ") 发送启动消息..." << std::endl;
            send_message(p.thread_instance_id, MSG_APP_START, nullptr);
        }
    }

    // 5. 让应用运行一段时间，模拟工作负载
    std::cout << "\n--- 应用运行中，等待 2 秒... ---\n" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 6. 停止应用，这将安全地关闭所有线程
    std::cout << "\n--- 准备关闭所有线程... ---\n" << std::endl;
    app.stop();
    std::cout << "所有线程已停止。" << std::endl;

    // 7. 处理剩余在结果队列中的消息
    // 优化：在所有工作线程都停止后，再处理结果队列，避免了无限循环和忙等待
    std::cout << "处理剩余结果:" << std::endl;
    std::shared_ptr<Message> msg;
    while (result_queue->try_pop(msg)) 
    {
        if (msg) 
        {
            printf("收到结果值: %d\n", msg->value);
        }
    }

    std::cout << "应用已安全退出。" << std::endl;
    return 0;
}