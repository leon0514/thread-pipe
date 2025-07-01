#include "ThreadWrapper/ThreadWrapper.hpp"

// OPTIMIZED: Renamed from base_config to configure.
ThreadWrapperError ThreadWrapper::configure(int instance_id, const std::string& thread_name, int device_id)
{
    if (configured_)
    {
        return ThreadWrapperError::ALREADY_INITED;
    }
    instance_id_ = instance_id;
    instance_name_ = thread_name;
    device_id_ = device_id;
    configured_ = true;

    return ThreadWrapperError::OK;
}