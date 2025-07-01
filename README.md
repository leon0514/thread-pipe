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

本框架采用分层设计，将复杂性封装在不同的抽象层级中：

1.  **Level 1: 核心线程封装 (Core Wrappers)**
    *   `ThreadWrapper`: 业务逻辑的抽象基类。
    *   `ThreadSafeQueue`: 线程安全的消息队列。
    *   `ThreadWrapperMgr`: 单个线程及其资源的管理器。

2.  **Level 2: 应用级线程池 (Application Pool)**
    *   `ThreadWrapperApp`: 一个单例，管理应用生命周期内的所有线程，提供一个全局的线程池视图。

3.  **Level 3: 业务任务管理 (Task Management)**
    *   `TaskManager`: (可选扩展层) 一个单例，允许用户将一组相关的线程组织成一个“任务”，并按名称对整个任务进行创建和销毁。

这种分层使得开发者可以根据需求选择合适的抽象级别进行交互。

## 设计亮点

*   **优雅停机 (Graceful Shutdown)**: 使用“毒丸”模式 (`nullptr` 消息)唤醒阻塞的线程并使其安全退出，从根本上避免了停机死锁。
*   **精确的初始化同步**: 使用 `std::promise` 和 `std::future`，确保主线程可以在工作线程初始化完成后才继续执行，实现了精确、低开销的同步。
*   **无锁的状态管理**: 使用 `std::atomic` 包装线程状态，避免了使用重量级互斥锁的开销。
*   **引用计数的线程池**: `TaskManager` 内部实现了对线程的引用计数，这是实现安全线程复用的基石。



## 用法示例

### 基础用法：实现单个工作线程

开发者只需继承 `ThreadWrapper` 并实现 `initialize` 和 `process` 即可。

```cpp
class MyWorker : public ThreadWrapper {
public:
    ThreadWrapperError initialize() override {
        // 初始化资源...
        return ThreadWrapperError::OK;
    }
    ThreadWrapperError process(int msgId, std::shared_ptr<void> data) override {
        // 处理消息...
        return ThreadWrapperError::OK;
    }
};
```

### 高级用法1：实现消息管道 (Message Pipeline)

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


### 终极用法：带线程复用的多任务管理

这是本框架最强大的功能。`TaskManager` 允许我们定义多个独立的业务任务，并在这些任务之间安全地共享通用线程（如深度学习模型复用等）。

**场景**: 我们将同时运行两个任务，它们共享一个名为 `"Logger"` 的线程。
*   **任务A**: 需要一个专属的 `"Producer-A"` 线程和一个共享的 `"Logger"` 线程。
*   **任务B**: 需要一个专属的 `"Consumer-B"` 线程和一个共享的 `"Logger"` 线程。


```cpp
#include "Task/TaskManager.hpp"
#include "TestThread/ProducerThread.hpp"
#include "TestThread/ProcessorThread.hpp"
#include "TestThread/ConsumerThread.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    auto& task_manager = get_task_manager_instance();

    // 1. 定义任务 A，它需要 "Producer-A" 和 "Logger"
    std::cout << "--- 1. 创建任务 A (需要 Producer-A, Logger) ---" << std::endl;
    std::vector<ThreadWrapperParam> task_a_params;
    task_a_params.push_back({std::make_unique<ProducerThread>(...), "Producer-A"});
    task_a_params.push_back({std::make_unique<ProcessorThread>(), "Logger"});
    task_manager.create_task("TaskA", task_a_params);
    // 预期输出:
    //  - Created new thread 'Producer-A' ... ref count: 1
    //  - Created new thread 'Logger' ... ref count: 1

    // 2. 定义任务 B，它需要 "Consumer-B" 和 "Logger"
    std::cout << "\n--- 2. 创建任务 B (需要 Consumer-B, Logger) ---" << std::endl;
    std::vector<ThreadWrapperParam> task_b_params;
    task_b_params.push_back({std::make_unique<ConsumerThread>(...), "Consumer-B"});
    task_b_params.push_back({std::make_unique<ProcessorThread>(), "Logger"}); // 复用 Logger
    task_manager.create_task("TaskB", task_b_params);
    // 预期输出:
    //  - Created new thread 'Consumer-B' ... ref count: 1
    //  - Reusing thread 'Logger', new reference count: 2  <-- 核心！线程被复用

    std::cout << "\n--- 3. 两个任务并行运行... ---" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 4. 停止任务 A
    std::cout << "\n--- 4. 停止任务 A ---" << std::endl;
    task_manager.stop_task("TaskA");
    // 预期输出:
    //  - 'Producer-A' 引用计数减为 0，将被停止。
    //  - 'Logger' 引用计数减为 1，将继续运行。 <-- 核心！共享线程不受影响

    std::cout << "\n--- 5. 任务 B 和共享的 Logger 仍在运行... ---" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // 6. 停止任务 B
    std::cout << "\n--- 6. 停止任务 B ---" << std::endl;
    task_manager.stop_task("TaskB");
    // 预期输出:
    //  - 'Consumer-B' 引用计数减为 0，将被停止。
    //  - 'Logger' 引用计数减为 0，现在也将被停止。 <-- 核心！共享线程最后被安全销毁

    std::cout << "\n--- 7. 应用退出 ---" << std::endl;
    return 0; // TaskManager 析构函数会确保所有资源都被释放
}
```
这个例子清晰地展示了 `TaskManager` 如何智能地管理线程的生命周期，实现了高效、安全的资源复用

## 构建与运行

**使用提供的 Makefile:**
*   `make`: 构建共享库 (`.so`) 和可执行测试程序 (`pro`)。
*   `make so`: 只构建共享库。
*   `make run`: 运行测试程序。
*   `make clean`: 清理所有生成的文件。