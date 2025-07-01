#ifndef THREADWRAPPER_HPP
#define THREADWRAPPER_HPP

#include <string>
#include <memory>
#include "ThreadWrapper/ThreadWrapperError.hpp"

// OPTIMIZED: Replaced #define with a type-safe constant.
static constexpr int INVALID_INSTANCE_ID = -1;

/**
 * @class ThreadWrapper
 * @brief An abstract base class for a manageable thread.
 *
 * Users must inherit from this class and implement the process() method
 * to define the thread's message handling logic.
 */
class ThreadWrapper
{
public:
    ThreadWrapper() = default;
    // OPTIMIZED: A virtual destructor is crucial for base classes with virtual functions.
    virtual ~ThreadWrapper() = default;

    ThreadWrapper(const ThreadWrapper&) = delete;
    ThreadWrapper& operator=(const ThreadWrapper&) = delete;

    /**
     * @brief Initializes the thread-specific resources. Called once when the thread starts.
     * @return 0 on success, non-zero on failure.
     */
    virtual int initialize()
    {
        return 0;
    }

    /**
     * @brief Processes a message. This is the core logic of the thread.
     * @param msgId The ID of the message.
     * @param msg_data A shared pointer to the message data.
     * @return 0 on success, non-zero on failure (which will terminate the thread).
     */
    virtual int process(int msgId, std::shared_ptr<void> msg_data) = 0;

    /// @brief Gets the unique ID assigned to this thread instance.
    int self_instance_id() const noexcept
    {
        return instance_id_;
    }

    /// @brief Gets the name assigned to this thread instance.
    const std::string& self_instance_name() const noexcept
    {
        return instance_name_;
    }

    // OPTIMIZED: Renamed to configure() for clarity. This is an internal method called by the framework.
    ThreadWrapperError configure(int instance_id, const std::string& thread_name, int device_id);

private:
    int instance_id_ = INVALID_INSTANCE_ID;
    std::string instance_name_;
    bool configured_ = false;
    int device_id_ = 0; // Set but not used in this example; available for user extension.
};


/**
 * @struct ThreadWrapperParam
 * @brief Parameters for creating a new thread within the application.
 */
struct ThreadWrapperParam
{
    std::unique_ptr<ThreadWrapper> thread_instance = nullptr;
    std::string thread_instance_name;
    int device_id = 0;
    int thread_instance_id = INVALID_INSTANCE_ID;
    uint32_t queue_size = 256;
};

#endif // THREADWRAPPER_HPP