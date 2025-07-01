#ifndef THREAD0_HPP
#define THREAD0_HPP

#include "ThreadWrapper/ThreadSafeQueue.hpp"
#include "ThreadWrapper/ThreadWrapper.hpp"
#include "param.hpp"

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
        ret = send_message(self_thread_id_, MSG_CREATE, nullptr);
        if (ret != tw::OK)
        {
            printf("Process app start message failed, error %d", ret);
        }
        return ret;
    }

    ThreadWrapperError app_start()
    {
        ThreadWrapperError ret = send_message(self_thread_id_, MSG_CREATE, nullptr);
        if (ret != tw::OK)
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