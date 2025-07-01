#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class ThreadSafeQueue {
public:
    /**
     * @brief Constructor with a specific capacity.
     * @param capacity The maximum number of items the queue can hold.
     */
    explicit ThreadSafeQueue(uint32_t capacity)
    {
        if (capacity >= MIN_QUEUE_CAPACITY && capacity <= MAX_QUEUE_CAPACITY) {
            queue_capacity_ = capacity;
        } else {
            queue_capacity_ = DEFAULT_QUEUE_CAPACITY;
        }
    }

    /**
     * @brief Default constructor with default capacity.
     */
    ThreadSafeQueue() : queue_capacity_(DEFAULT_QUEUE_CAPACITY) {}

    ~ThreadSafeQueue() = default;

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    /**
     * @brief Pushes a value onto the queue if there is capacity.
     * @param value The value to push.
     * @return true on success, false if the queue is full.
     */
    bool push(T value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() >= queue_capacity_) {
            return false; // Queue is full
        }
        queue_.push(std::move(value));
        cond_var_.notify_one(); // Notify one waiting consumer
        return true;
    }

    /**
     * @brief Tries to pop a value from the queue without blocking.
     * @param[out] value Reference to store the popped value.
     * @return true on success, false if the queue was empty.
     */
    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    /**
     * @brief Waits until an item is available and pops it from the queue.
     * @param[out] value Reference to store the popped value.
     */
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_var_.wait(lock, [this] { return !queue_.empty(); });
        value = std::move(queue_.front());
        queue_.pop();
    }

    /// @brief Checks if the queue is empty.
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /// @brief Returns the current number of items in the queue.
    uint32_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    std::queue<T> queue_;
    uint32_t queue_capacity_;
    mutable std::mutex mutex_;
    std::condition_variable cond_var_;

    static constexpr uint32_t MIN_QUEUE_CAPACITY = 1;
    static constexpr uint32_t MAX_QUEUE_CAPACITY = 10000;
    static constexpr uint32_t DEFAULT_QUEUE_CAPACITY = 256;
};

#endif /* THREAD_SAFE_QUEUE_HPP */