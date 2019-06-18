#include <atomic>
#include <stdint.h>

class BlockState {
public:
    int32_t assigned_index() {
        return assigned_index_.load();
    }
    void increase() {
        assigned_index_.fetch_add(1);
    }
private:
    std::atomic<int32_t> assigned_index_ = {0};
};

class BlockLock {
public:
    static const int32_t write_state_ = 1;
    static const int32_t idle_state_ = 0;
    static const int32_t read_state_ = -1;

    bool get_writelock() {
        int32_t exp_lock = idle_state_;
        if (!lock_state_.compare_exchange_weak(exp_lock, write_state_,
                                              std::memory_order_acq_rel,
                                              std::memory_order_relaxed)) {
            return false;
        }
        return true;
    }
    bool get_readlock() {
        int32_t cur_lock = lock_state_.load();
        if (cur_lock == write_state_) {
            std::cout << "segment is being writen" << std::endl;
            return false;
        }

        if (!lock_state_.compare_exchange_weak(cur_lock, read_state_,
                                               std::memory_order_acq_rel,
                                               std::memory_order_relaxed)) {
            return false;
        }
        return true;
    }

    void release_writelock() {
        lock_state_.fetch_sub(1); 
    }
    void release_readlock() {
        lock_state_.fetch_add(1);
    }
private:
    std::atomic<int32_t> lock_state_;
};