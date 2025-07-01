#ifndef THREAD0_HPP
#define THREAD0_HPP
#include <random>
#include "ThreadWrapper/ThreadSafeQueue.hpp"
#include "ThreadWrapper/ThreadWrapper.hpp"
#include "param.hpp"

static std::random_device s_rd_range;
static std::mt19937 s_gen_range(s_rd_range());

int generate_value(int min_val, int max_val) {
    if (min_val <= 0 || max_val <= 0) {
        throw std::invalid_argument("Error: Both min_val and max_val must be positive.");
    }
    if (min_val > max_val) {
        throw std::invalid_argument("Error: min_val cannot be greater than max_val.");
    }

    // 在函数内部创建分布对象，这样每次调用可以指定不同的范围
    std::uniform_int_distribution<> distrib(min_val, max_val);
    return distrib(s_gen_range);
}

class TestThread0 : public ThreadWrapper
{
public:
    TestThread0(std::vector<std::string> call_thread_name_list) : call_thread_name_list_(call_thread_name_list)
    {

    }
    int initialize()
    {
        self_thread_id_ = self_instance_id();
        return 0;
    }
    int process(int msg_id, std::shared_ptr<void> data)
    {
        std::shared_ptr<Message> message = std::make_shared<Message>();
        switch (msg_id)
        {
            case MSG_APP_START:
                app_start();
                break;
            case MSG_CREATE:
                printf("TestThread0 create message\n");
                MsgSend(message);
                break;
            default:
                printf("TestThread0 thread ignore msg %d\n", msg_id);
                break;
        }
        return 0;
    }

    ThreadWrapperError MsgSend(std::shared_ptr<Message> &message)
    {
        message->call_thread_name_list_ = call_thread_name_list_;
        message->value = generate_value(1, 1000);
        printf("original_value : %d\n", message->value);
        std::string next_thread_name = message->call_thread_name_list_.back();
        message->call_thread_name_list_.pop_back();
        int next_thread_id = get_thread_wrapper_id_by_name(next_thread_name);
        ThreadWrapperError ret;
        while (1)
        {
            ret = send_message(next_thread_id, MSG_ADD, message);
            if (ret == TW::ENQUEUE_FAILED) 
            {
                continue;
            } 
            else if (ret == TW::OK) 
            {
                break;
            } 
            else 
            {
                return ret;
            }
        }
        ret = send_message(self_thread_id_, MSG_CREATE, nullptr);
        if (ret != TW::OK)
        {
            printf("Process app start message failed, error %d", ret);
        }
        return ret;
    }

    ThreadWrapperError app_start()
    {
        ThreadWrapperError ret = send_message(self_thread_id_, MSG_CREATE, nullptr);
        if (ret != TW::OK)
        {
            printf("Process app start message failed, error %d", ret);
        }
        return ret;
    }

private:
    int self_thread_id_;
    std::vector<std::string> call_thread_name_list_;
};

#endif