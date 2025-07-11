#ifndef BLOCKING_QUEUE_H
#define BLOCKING_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class BlockingQueue {
public:
    BlockingQueue() = default;

    // Push an item to the queue.
    void push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(std::move(item));
        // Notify one waiting thread that an item is available.
        cond_var_.notify_one();
    }

    // Pop an item from the queue. Blocks if the queue is empty.
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        // Wait until the queue is not empty.
        cond_var_.wait(lock, [this] { return !queue_.empty(); });
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    // Try to pop an item from the queue without blocking.
    // Returns true if an item was popped, false otherwise.
    bool try_pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    // Check if the queue is empty.
    bool empty() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    // Get the current size of the queue.
    size_t size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_; // mutable to allow locking in const methods like empty() and size()
    std::condition_variable cond_var_;
};

#endif // BLOCKING_QUEUE_H
