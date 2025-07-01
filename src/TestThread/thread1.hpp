#ifndef THREAD1_HPP
#define THREAD1_HPP

#include "ThreadWrapper/ThreadSafeQueue.hpp"
#include "ThreadWrapper/ThreadWrapper.hpp"
#include "param.hpp"

class TestThread1 : public ThreadWrapper
{
public:
    TestThread1() = default;
    ~TestThread1() = default;

    int initialize()
    {
        return 0;
    }

    int process(int msg_id, std::shared_ptr<void> data)
    {
        switch (msg_id)
        {
            case MSG_ADD:
            {
                printf("TestThread1 add message value + 1\n");
                auto msg = std::static_pointer_cast<Message>(data);
                msg->value += 1;
                MsgSend(msg);
                break;
            }
            default:
                printf("TestThread1 thread ignore msg %d\n", msg_id);
                break;
        }
        return 0;
    }

    ThreadWrapperError MsgSend(std::shared_ptr<Message> &message)
    {
        std::string next_thread_name = message->call_thread_name_list_.back();
        message->call_thread_name_list_.pop_back();
        int next_thread_id = get_thread_wrapper_id_by_name(next_thread_name);
        ThreadWrapperError ret;
        while (1)
        {
            ret = send_message(next_thread_id, MSG_ADD, message);
            if (ret == tw::ENQUEUE_FAILED) 
            {
                continue;
            } 
            else if (ret == tw::OK) 
            {
                break;
            } 
            else 
            {
                return ret;
            }
        }
        return ret;
    }
};

#endif