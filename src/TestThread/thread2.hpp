#ifndef THREAD2_HPP
#define THREAD2_HPP

#include "ThreadWrapper/ThreadSafeQueue.hpp"
#include "ThreadWrapper/ThreadWrapper.hpp"
#include "param.hpp"

class TestThread2 : public ThreadWrapper
{
public:
    TestThread2( std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Message>>> result_queue_ptr) : result_queue_ptr_(result_queue_ptr)
    {

    }
    int initialize()
    {
        if (result_queue_ptr_ == nullptr)
        {
            return 1;
        }
        return 0;
    }
    int process(int msg_id, std::shared_ptr<void> data)
    {
        switch (msg_id)
        {
            case MSG_ADD:
            {
                // printf("TestThread2 add message value + 2\n");
                auto msg = std::static_pointer_cast<Message>(data);
                msg->value += 2;
                MsgSend(msg);
                break;
            }
            default:
                // printf("TestThread2 thread ignore msg %d\n", msg_id);
                break;
        }
        return 0;
    }

private:
    ThreadWrapperError MsgSend(std::shared_ptr<Message> &message)
    {
        result_queue_ptr_->push(message);
        return TW::OK;
    }

private:
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Message>>> result_queue_ptr_;

};

#endif