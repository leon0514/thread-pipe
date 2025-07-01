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
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Message>>> result_queue = std::make_shared<ThreadSafeQueue<std::shared_ptr<Message>>>();
    params2.thread_instance = new TestThread2(result_queue);
    params2.thread_instance_name = "TestThread2";

    std::vector<ThreadWrapperParam> params = {params0, params1, params2};

    
    app.start(params);

    for (int i = 0; i < params.size(); i++) {
        send_message(params[i].thread_instance_id, MSG_APP_START, nullptr);
    }

    app.wait_end();
    while (true)
    {
        auto msg =  result_queue->pop();
        if (msg != nullptr)
        {
            printf("result value : %d\n", msg->value);
        }
    }
    return 0;
}