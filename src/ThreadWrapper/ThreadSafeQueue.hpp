#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP
#include <queue>
#include <mutex>

template<typename T>
class ThreadSafeQueue {
public:

    /**
     * @brief ThreadSafeQueue constructor
     * @param [in] capacity: the queue capacity
     */
    ThreadSafeQueue(uint32_t capacity)
    {
        // check the input value: capacity is valid
        if (capacity >= min_queue_capacity_ && capacity <= max_queue_capacity_) {
            queue_capacity_ = capacity;
        } else { // the input value: capacity is invalid, set the default value
            queue_capacity_ = default_queue_capacity_;
        }
    }

    /**
     * @brief ThreadSafeQueue constructor
     */
    ThreadSafeQueue()
    {
        queue_capacity_ = default_queue_capacity_;
    }

    /**
     * @brief ThreadSafeQueue destructor
     */
    ~ThreadSafeQueue() = default;

    /**
     * @brief push data to queue
     * @param [in] input_value: the value will push to the queue
     * @return true: success to push data; false: fail to push data
     */
    bool push(T input_value)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // check current size is less than capacity
        if (queue_.size() < queue_capacity_) {
            queue_.push(input_value);
            return true;
        }

        return false;
    }

    /**
     * @brief pop data from queue
     * @return true: success to pop data; false: fail to pop data
     */
    T pop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) { // check the queue is empty
            return nullptr;
        }

        T tmp_ptr = queue_.front();
        queue_.pop();
        return tmp_ptr;
    }

    /**
     * @brief check the queue is empty
     * @return true: the queue is empty; false: the queue is not empty
     */
    bool empty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief get the queue size
     * @return the queue size
     */
    uint32_t size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    void extend_capacity(uint32_t new_size)
    {
        queue_capacity_ = new_size;
        max_queue_capacity_ = new_size > max_queue_capacity_ ? new_size : max_queue_capacity_;
    }

private:
    std::queue<T> queue_; // the queue
    uint32_t queue_capacity_; // queue capacity
    mutable std::mutex mutex_; // the mutex value
    const uint32_t min_queue_capacity_ = 1; // the minimum queue capacity
    const uint32_t max_queue_capacity_ = 10000; // the maximum queue capacity
    const uint32_t default_queue_capacity_ = 10; // default queue capacity
};

#endif /* THREAD_SAFE_QUEUE_HPP__ */