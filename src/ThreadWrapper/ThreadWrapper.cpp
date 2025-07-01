#include "ThreadWrapper.hpp"


ThreadWrapper::ThreadWrapper() : instance_id_(INVALID_INSTANCE_ID), instance_name_(""), base_configed_(false)
{

}

ThreadWrapperError ThreadWrapper::base_config(int instance_id, const std::string& thread_name, int device_id)
{
    if (base_configed_)
    {
        return TW::ALREADY_INITED;
    }
    instance_id_ = instance_id;
    instance_name_.assign(thread_name.c_str());
    base_configed_ = true;

    return TW::OK;
}


ThreadWrapper::~ThreadWrapper()
{
    
}