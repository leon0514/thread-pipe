#ifndef PRODUCER_THREAD_HPP
#define PRODUCER_THREAD_HPP

#include <random>
#include <stdexcept>
#include <iostream>
#include <utility>
#include "ThreadWrapper/ThreadWrapper.hpp"
#include "ThreadWrapper/ThreadWrapperApp.hpp" // For global functions
#include "param.hpp"

inline int generate_value(int min_val, int max_val) {
    if (min_val > max_val) {
        throw std::invalid_argument("Error: min_val cannot be greater than max_val.");
    }
    // 使用 thread_local 保证每个线程拥有自己的随机数生成器实例，完全避免竞争
    thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<> distrib(min_val, max_val);
    return distrib(generator);
}


inline ThreadWrapperError send_with_retry(int dest_id, MessageId msg_id, std::shared_ptr<void> data) {
    ThreadWrapperError ret;
    while (true) {
        ret = send_message(dest_id, static_cast<int>(msg_id), data);
        if (ret == ThreadWrapperError::ENQUEUE_FAILED) {
            // 队列已满，稍作等待后重试，避免CPU空转
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        // 发送成功或遇到不可恢复的错误时退出循环
        break;
    }
    return ret;
}


// 管道的发起者，负责创建消息并启动管道流程
class ProducerThread : public ThreadWrapper {
public:
    // 构造时传入完整的路由路径（反向）
    explicit ProducerThread(std::vector<std::string> pipeline_route) 
        : pipeline_route_(std::move(pipeline_route)) {}

    ThreadWrapperError initialize() override {
        // LOG_INFO("Producer '{}' initialized.", self_instance_name());
        return ThreadWrapperError::OK;
    }

    ThreadWrapperError process(int msg_id, std::shared_ptr<void> data) override {
        auto message_id_enum = static_cast<MessageId>(msg_id);

        switch (message_id_enum) {
            case MessageId::APP_START:
            case MessageId::CREATE_PIPELINE_MSG:
                create_and_send_message();
                break;
            default:
                // LOG_WARN("Producer '{}' ignored unknown message id: {}", self_instance_name(), msg_id);
                break;
        }
        return ThreadWrapperError::OK;
    }

private:
    void create_and_send_message() {
        auto msg = std::make_shared<PipelineMessage>();
        msg->routing_slip = pipeline_route_; // 设置路由单
        msg->value = generate_value(1, 100);
        
        forward_message(msg);

        // 为了持续演示，再给自己发送一个消息以创建下一个管道消息
        ThreadWrapperError ret = send_message(self_instance_id(), static_cast<int>(MessageId::CREATE_PIPELINE_MSG), nullptr);
        if (ret != ThreadWrapperError::OK) {
            // LOG_ERROR("Producer '{}' failed to schedule next message creation. Error: {}", self_instance_name(), static_cast<int>(ret));
        }
    }

    void forward_message(const std::shared_ptr<PipelineMessage>& msg) {
        if (msg->routing_slip.empty()) {
            // LOG_WARN("Producer '{}' received a message with an empty route.", self_instance_name());
            return;
        }

        std::string next_thread_name = msg->routing_slip.back();
        msg->routing_slip.pop_back();
        int next_thread_id = get_thread_wrapper_id_by_name(next_thread_name);

        if (next_thread_id == INVALID_INSTANCE_ID) {
            return;
        }

        send_with_retry(next_thread_id, MessageId::PROCESS_PIPELINE_MSG, msg);
    }

    std::vector<std::string> pipeline_route_;
};

#endif // PRODUCER_THREAD_HPP