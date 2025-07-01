#ifndef PROCESSOR_THREAD_HPP
#define PROCESSOR_THREAD_HPP

#include "ThreadWrapper/ThreadWrapper.hpp"
#include "ThreadWrapper/ThreadWrapperApp.hpp"
#include "param.hpp"

inline ThreadWrapperError send_with_retry(int dest_id, MessageId msg_id, std::shared_ptr<void> data);

class ProcessorThread : public ThreadWrapper {
public:
    ProcessorThread() = default;

    ThreadWrapperError initialize() override {
        return ThreadWrapperError::OK;
    }

    ThreadWrapperError process(int msg_id, std::shared_ptr<void> data) override
    {
        if (static_cast<MessageId>(msg_id) == MessageId::PROCESS_PIPELINE_MSG) 
        {
            auto msg = std::static_pointer_cast<PipelineMessage>(data);

            msg->value += 1;
            
            forward_message(msg);
        }
        return ThreadWrapperError::OK;;
    }

private:
    void forward_message(const std::shared_ptr<PipelineMessage>& msg) 
    {
        if (msg->routing_slip.empty()) {
            // LOG_ERROR("Processor '{}' received a message with an empty route. This should be handled by the consumer.", self_instance_name());
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
};

#endif // PROCESSOR_THREAD_HPP