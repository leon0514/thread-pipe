#ifndef CONSUMER_THREAD_HPP
#define CONSUMER_THREAD_HPP

#include <utility>
#include "ThreadWrapper/ThreadWrapper.hpp"
#include "ThreadWrapper/ThreadSafeQueue.hpp"
#include "param.hpp"

// 管道的终点，负责收集处理结果
class ConsumerThread : public ThreadWrapper {
public:
    explicit ConsumerThread(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<PipelineMessage>>> result_queue)
        : result_queue_(std::move(result_queue)) {}

    ThreadWrapperError initialize() override {
        if (!result_queue_) {
            return ThreadWrapperError::ERROR;
        }
        return ThreadWrapperError::OK;
    }

    ThreadWrapperError process(int msg_id, std::shared_ptr<void> data) override {
        if (static_cast<MessageId>(msg_id) == MessageId::PROCESS_PIPELINE_MSG) {
            auto msg = std::static_pointer_cast<PipelineMessage>(data);
            msg->value += 2;
            
            if (!msg->routing_slip.empty()) {
                
            }
            if (!result_queue_->push(msg)) {
                // LOG_WARN("Consumer '{}': Result queue is full. Message was dropped.", self_instance_name());
            }
        }
        return ThreadWrapperError::OK;
    }

private:
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<PipelineMessage>>> result_queue_;
};

#endif // CONSUMER_THREAD_HPP