# ThreadWrapper C++ 线程框架

这是一个基于现代 C++ (C++11/17) 构建的、轻量级、消息驱动的线程框架。它旨在简化多线程应用程序的开发，通过提供清晰的抽象和健壮的线程管理机制，让开发者可以专注于业务逻辑的实现。

## 核心特性

*   **消息驱动 (Message-Driven)**: 线程之间通过异步消息传递进行通信，有效解耦。
*   **类型安全 (Type-Safe)**: 使用 `enum class` 和模板，在编译期保证类型安全。
*   **线程安全 (Thread-Safe)**: 内置线程安全的消息队列，无需开发者手动处理锁。
*   **清晰的生命周期管理 (Clear Lifecycle Management)**: 自动化线程的创建、初始化、运行和销毁，并能实现优雅停机。
*   **易于扩展 (Easy to Extend)**: 开发者只需继承 `ThreadWrapper` 基类并实现自己的业务逻辑即可。
*   **资源安全 (Resource-Safe)**: 全程使用智能指针管理资源，杜绝内存泄漏。

## 架构设计

本框架主要由以下几个核心组件构成：

*   `ThreadWrapper`: **业务逻辑基类**。用户需要继承这个类，并实现 `initialize()` (初始化) 和 `process()` (消息处理) 两个核心方法。
*   `ThreadSafeQueue`: **线程安全消息队列**。每个线程拥有一个自己的消息队列，用于接收来自其他线程的消息。这是一个有界队列，可以防止内存无限增长。
*   `ThreadWrapperMgr`: **线程管理器**。它负责管理一个具体的工作线程 (`std::thread`)、其消息队列以及对应的 `ThreadWrapper` 实例。它处理了线程的启动、销毁、状态同步等底层细节。
*   `ThreadWrapperApp`: **应用主控中心**。它是一个单例，负责管理整个应用中的所有 `ThreadWrapperMgr` 实例。提供了创建线程、启动、停止、发送消息等全局API。

其关系可以概括为：
`ThreadWrapperApp` -> 管理多个 `[ThreadWrapperMgr]` -> 每个 `[Mgr]` 拥有一个 `[std::thread] + [ThreadSafeQueue] + [ThreadWrapper实例]`

## 设计与优化亮点

本框架采用了多项现代C++的最佳实践来保证其健壮性和高性能。

1.  **优雅停机 (Graceful Shutdown) 与 “毒丸”模式**
    *   **问题**: 在多线程编程中，一个常见的难题是如何安全地停止线程，特别是当线程可能因等待任务而阻塞时。
    *   **解决方案**: 本框架采用“**毒丸 (Poison Pill)**”模式。当请求停机时，框架会向每个线程的消息队列中推送一个特殊的空消息 (`nullptr`)。工作线程从阻塞的 `wait_and_pop` 中被唤醒后，一旦检测到这个“毒丸”，就会立即安全退出循环，从而避免了死锁。

2.  **精确的初始化同步与 `std::promise`**
    *   **问题**: 主线程需要确保工作线程的初始化逻辑执行完毕后，才能向其发送消息。
    *   **解决方案**: 使用 `std::promise` 和 `std::future`。工作线程初始化完成后，会通过 `std::promise` 发出信号。主线程则通过 `std::future` 等待这个信号，实现了精确、低开销的同步。

3.  **无锁的状态管理与 `std::atomic`**
    *   **问题**: 线程状态（如 `RUNNING`, `EXITING`）需要在多个线程间共享，直接访问会产生数据竞争。
    *   **解决方案**: 使用 `std::atomic` 来包装线程状态。这使得状态的读写操作变为原子操作，避免了使用重量级互斥锁的开销。

4.  **RAII 与智能指针**
    *   **问题**: 手动管理动态分配的资源（如线程对象、业务逻辑对象）容易出错，导致内存泄漏。
    *   **解决方案**: 框架全面使用 `std::unique_ptr` 来管理所有权。当 `ThreadWrapperApp` 销毁时，它所管理的所有 `ThreadWrapperMgr` 对象及其包含的资源都会被自动、安全地释放，代码简洁且无泄漏风险。

## 高级用法：实现消息管道 (Message Pipeline)

本框架的灵活性允许您轻松实现复杂的设计模式。下面我们将演示如何构建一个“消息管道”，其中一个消息会按照预定的路径依次流经多个线程。

**场景**:
我们创建一个三阶段的处理管道：
1.  **`ProducerThread`**: 管道的起点。它负责创建消息，并将其发送到管道的第一个处理节点。
2.  **`ProcessorThread`**: 管道的中间节点。它接收消息，对其内容进行一些处理（例如，数值加1），然后转发给下一个节点。
3.  **`ConsumerThread`**: 管道的终点。它接收最终处理过的消息，并将其放入一个结果队列，供主线程或其他部分消费。

#### 第1步: 定义消息结构和ID (`param.hpp`)

消息体本身将携带“路由单”，告诉每个节点下一步该去哪里。

```cpp
// param.hpp
#ifndef PARAM_HPP
#define PARAM_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <memory>

enum class MessageId {
    APP_START = 1,
    CREATE_PIPELINE_MSG = 2,
    PROCESS_PIPELINE_MSG = 3,
};

struct PipelineMessage
{
    // 消息的路由单，定义了需要经过的线程名列表
    std::vector<std::string> routing_slip;
    uint32_t value;
};

#endif
```

#### 第2步: 实现管道节点

每个节点都是一个 `ThreadWrapper` 的子类。我们分别创建 `ProducerThread`, `ProcessorThread` 和 `ConsumerThread`。
*(为简洁起见，此处省略每个节点的完整代码，请参考 `ProducerThread.hpp`, `ProcessorThread.hpp`, `ConsumerThread.hpp` 文件)*

核心逻辑如下：
*   **`ProducerThread`**: 创建 `PipelineMessage`，填入初始值和完整的 `routing_slip`，然后根据路由单弹出下一站，发送消息。
*   **`ProcessorThread`**: 接收消息，执行业务逻辑（如 `msg->value += 1`），然后同样根据路由单弹出下一站，转发消息。
*   **`ConsumerThread`**: 作为管道终点，接收消息，执行最终的业务逻辑，然后将结果 `push` 到一个外部共享的 `ThreadSafeQueue` 中。

#### 第3步: 组装并运行管道 (`main.cpp`)

在 `main` 函数中，我们定义管道的路线，创建所有线程，并启动它们。

```cpp
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include <string>

#include "ThreadWrapper/ThreadWrapperApp.hpp"
#include "ProducerThread.hpp"
#include "ProcessorThread.hpp"
#include "ConsumerThread.hpp"
#include "param.hpp"

int main() {
    auto& app = get_thread_wrapper_app_instance();

    // 1. 准备共享资源：一个用于收集最终结果的队列
    auto result_queue = std::make_shared<ThreadSafeQueue<std::shared_ptr<PipelineMessage>>>(2048);
    
    // 2. 定义管道路由 (注意：顺序是反向的，因为代码使用 pop_back() 获取下一站)
    std::vector<std::string> pipeline_route = {"Consumer", "Processor"};
    
    // 3. 动态打印流程图，确认配置无误
    std::string flow_str = "Producer";
    for (auto it = pipeline_route.rbegin(); it != pipeline_route.rend(); ++it) {
        flow_str += " -> " + *it;
    }
    std::cout << "\n[配置] 消息管道流程: " << flow_str << std::endl;

    // 4. 配置所有线程参数
    std::vector<ThreadWrapperParam> params;
    params.push_back({std::make_unique<ProducerThread>(pipeline_route), "Producer"});
    params.push_back({std::make_unique<ProcessorThread>(), "Processor"});
    params.push_back({std::make_unique<ConsumerThread>(result_queue), "Consumer"});

    // 5. 启动所有线程
    if (app.start(params) != ThreadWrapperError::OK) {
        std::cerr << "错误: 启动线程框架失败！" << std::endl;
        return -1;
    }
    std::cout << "[成功] 所有线程已启动并初始化。" << std::endl;

    // 6. 向管道起点发送启动信号
    int producer_id = app.get_thread_wrapper_id_by_name("Producer");
    send_message(producer_id, static_cast<int>(MessageId::APP_START), nullptr);

    // 7. 让应用运行一段时间
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // 8. 优雅地停止整个应用
    app.stop();
    std::cout << "\n[成功] 所有工作线程已安全停止。" << std::endl;

    // 9. 处理并验证结果
    std::cout << "\n[验证] 开始处理并统计结果队列中的数据..." << std::endl;
    std::shared_ptr<PipelineMessage> msg;
    int message_count = 0;
    while (result_queue->try_pop(msg)) {
        if (msg) message_count++;
    }
    std::cout << "处理的消息总数: " << message_count << " 条" << std::endl;

    std::cout << "\n应用已安全退出。" << std::endl;
    return 0;
}
```
这个例子展示了如何利用本框架构建灵活、解耦、可扩展的多线程处理流程。消息本身驱动着整个业务逻辑的流转，而框架则在底层保证了线程的安全和稳定运行。

## 构建与运行
```shell
make pro
cd workspace && ./pro
```