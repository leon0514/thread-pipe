#ifndef THREADWARPPER_HPP
#define THREADWARPPER_HPP

#include <string>
#include <memory>

#include "ThreadWrapperError.hpp"

#define INVALID_INSTANCE_ID (-1)


class ThreadWrapper
{

public:
    ThreadWrapper();
    virtual ~ThreadWrapper();
    virtual int initialize()
    {
        return 0;
    }

    virtual int process(int msgId, std::shared_ptr<void> msg_data) = 0;

    int self_instance_id()
    {
        return instance_id_;
    }

    std::string self_instance_name()
    {
        return instance_name_;
    }

    ThreadWrapperError base_config(int instance_id, const std::string& thread_name, int device_id);

private:
    int instance_id_;
    std::string instance_name_;
    bool base_configed_;
    bool running_;
    int device_id_;

};


struct ThreadWrapperParam
{
    ThreadWrapper* thread_instance = nullptr;
    std::string thread_instance_name = "";
    int device_id = 0;
    int thread_instance_id = INVALID_INSTANCE_ID;
    uint32_t queue_size = 256;
};

#endif // endif THREADWARP_HPP__