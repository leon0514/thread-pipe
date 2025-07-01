#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP
#include <queue>
#include <mutex>
#include <condition_variable> // 新增: 用于支持阻塞操作，如果未来需要的话

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
     * @return true: success to push data; false: fail to push data (queue full)
     */
    bool push(T input_value)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // check current size is less than capacity
        if (queue_.size() < queue_capacity_) {
            queue_.push(input_value);
            // 如果您有消费者在等待队列非空，可以在这里通知它们
            // condition_consumer_.notify_one();
            return true;
        }

        return false; // Queue is full
    }

    /**
     * @brief pop data from queue
     * @return the data popped from queue, or a default-constructed T if queue is empty.
     * NOTE: For pointer/smart pointer types (like std::shared_ptr),
     * this might return nullptr if empty.
     * It's recommended to use a try_pop or wait_and_pop
     * pattern for robust error handling.
     */
    T pop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) { // check the queue is empty
            // 警告: 直接返回 nullptr 只有在 T 是指针类型或可从 nullptr 构造时才有效。
            // 对于 std::shared_ptr<Message>，它将返回一个空的 shared_ptr。
            // 对于其他非指针类型，这将是编译错误或未定义行为。
            // 建议使用 try_pop 或 wait_and_pop 模式来避免此问题。
            return T{}; // 返回 T 的默认构造值，对于指针类型是 nullptr
        }

        T tmp_ptr = queue_.front();
        queue_.pop();
        // 如果您有生产者在等待队列有空间，可以在这里通知它们
        // condition_producer_.notify_one();
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

    /**
     * @brief Extends the capacity of the queue.
     * This operation is now thread-safe.
     * @param [in] new_size: The new capacity for the queue.
     */
    void extend_capacity(uint32_t new_size)
    {
        // 修复：添加互斥锁以确保线程安全
        std::lock_guard<std::mutex> lock(mutex_);
        queue_capacity_ = new_size;
        max_queue_capacity_ = new_size > max_queue_capacity_ ? new_size : max_queue_capacity_;
    }

private:
    std::queue<T> queue_;               // the queue
    uint32_t queue_capacity_;           // queue capacity
    mutable std::mutex mutex_;          // the mutex value

    const uint32_t min_queue_capacity_ = 1;     // the minimum queue capacity
    const uint32_t max_queue_capacity_ = 10000; // the maximum queue capacity
    const uint32_t default_queue_capacity_ = 10; // default queue capacity
};

#endif /* THREAD_SAFE_QUEUE_HPP__ */