# THREAD PIPELINE

## 项目说明
仿照 Ascend AclThread 编写的多线程pipeline模块

## 项目特性
1. 不同线程间通过队列通信
2. 在数据源头定义数据走向，可以实现任意不同线程间的通信
3. 更好的节点复用，用在深度学习模型检测中，可以更好的复用模型


## 使用
1. 每个线程继承 `ThreadWrapper` 类，实现 `initialize` `process` 方法
2. 源头数据定义数据走向，通过 `call_thread_name_list_` 逆序存储。

### 示例
``` C++
#include <iostream>
#include "ThreadWrapper/ThreadWrapperApp.hpp"
#include <vector>
#include "TestThread/thread0.hpp"
#include "TestThread/thread1.hpp"
#include "TestThread/thread2.hpp"

int main(int argc, char*agrv[])
{
    auto& app = get_thread_wrapper_app_instance();

    ThreadWrapperParam params0;
    params0.thread_instance = new TestThread0({"TestThread2", "TestThread1"});
    params0.thread_instance_name = "TestThread0";

    ThreadWrapperParam params1;
    params1.thread_instance = new TestThread1();
    params1.thread_instance_name = "TestThread1";

    ThreadWrapperParam params2;
    params2.thread_instance = new TestThread2();
    params2.thread_instance_name = "TestThread2";

    std::vector<ThreadWrapperParam> params = {params0, params1, params2};

    
    app.start(params);

    for (int i = 0; i < params.size(); i++) {
        send_message(params[i].thread_instance_id, MSG_APP_START, nullptr);
    }

    app.wait_end();
    getchar();
    return 0;
}
```

```
TestThread0 create message
TestThread1 add message value + 1
TestThread1 add message value + 2
TestThread0 create message
TestThread1 add message value + 1
TestThread1 add message value + 2
TestThread0 create message
TestThread1 add message value + 1
TestThread1 add message value + 2
TestThread0 create message
TestThread1 add message value + 1
TestThread1 add message value + 2
```

