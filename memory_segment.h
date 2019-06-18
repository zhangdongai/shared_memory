#pragma once

#include <iostream>
#include <map>
#include <stdint.h>
#include <unordered_map>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "block.h"

static const int32_t g_MemBlockCount = 1024;

static const std::unordered_map<int32_t, const char*> g_errors= {
    {EACCES, "Operation not permitted"},
    {EEXIST, "memory segment already exists"},
    {EINVAL, "size is less than SHMMIN"
              " or greater than SHMMAX"},
    {ENOENT, "No segment exists for the given key"},
    {ENOMEM, "No memory could be allocated for segment overhead"},
    {ENOSPC, "No space left on device"},
};

const char* get_error(int32_t err) {
    if (g_errors.find(err) != g_errors.end())
        return g_errors.at(err);
    return "unknown error";
}

template <typename T>
class MemorySegment {
public:
    static_assert(std::is_class<T>::value, "T should be a class type!");

    /**
        S_IRUSR  00400 user has read permission
        S_IWUSR  00200 user has write permission
        S_IRGRP  00040 group has read permission
        S_IROTH  00004 others have read permission
        http://man7.org/linux/man-pages/man2/open.2.html
    */
    enum class AccessMode {
        READ_ONLY = S_IRUSR | S_IRGRP | S_IROTH,
        FULLACCESS = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
    };

    MemorySegment(AccessMode access_mode = AccessMode::FULLACCESS)
        : access_mode_(access_mode) {
        mem_id_ = static_cast<key_t>(T::hash());
        init();
    }
    ~MemorySegment() {
        if (!detach() || !destroy())
            std::cout << "errno occured when remove sm, check the output" << std::endl;
        else
            std::cout << "remove shared memory success" << std::endl;
    }

    void init() {
        if (init_)
            return;
        if (!try_create()) {
            if (errno == EEXIST) {
                    if (!try_open()) {
                        return;
                    }
            }
        }
        std::cout << "shared memeory created or open successfully!" << std::endl;

        //attach memory to process
        mem_ptr_ = shmat(shm_id_, nullptr, 0);
        if (mem_ptr_ == reinterpret_cast<void*>(-1)) {
            std::cout << "attach memory failed, errno is "
                      << errno << std::endl;
            shmctl(shm_id_, IPC_RMID, nullptr);
            return;
        }

        // assign memory
        int32_t block_size = T::size();
        for (int32_t i = 0; i < g_MemBlockCount; ++i) {
            uint8_t* mem = new (static_cast<char*>(mem_ptr_) + block_size * i)
                               uint8_t[block_size];
            block_mems_[i] = mem;
        }
        
        init_ = true;
    }

    bool write_mem(const T& raw_data) {
        int32_t block_index = get_block();
        if (!block_lock_[block_index].get_writelock()) {
            std::cout << "get write lock faild, index is "
                      << block_index << std::endl;
            return false;
        }
        uint8_t* mem = block_mems_[block_index];
        raw_data.serialize(reinterpret_cast<char*>(mem));
        elem_refers_[raw_data.key()] = block_index;

        block_lock_[block_index].release_writelock();
        return true;
    }

    bool read_mem(T* const raw_data) {
        if (elem_refers_.find(raw_data->key()) == elem_refers_.end()) {
            std::cout << "can not find " << raw_data->key() << std::endl;
            return false;
        }

        int32_t block_index = elem_refers_[raw_data->key()];
        if (!block_lock_[block_index].get_readlock()) {
            std::cout << "get read lock faild, index is "
                      << block_index << std::endl;
            return false;
        }
        uint8_t* mem = block_mems_[block_index];
        raw_data->deserialize(reinterpret_cast<char*>(mem));

        block_lock_[block_index].release_readlock();
        return true;
    }

    void remap() {
        if (!detach())
            return;
        try_open();
    }

private:
    bool try_create() {
        shm_id_ = shmget(mem_id_,
                         allocate_size_,
                         static_cast<int32_t>(access_mode_) | IPC_CREAT | IPC_EXCL);
        if (shm_id_ == -1) {
            std::cout << "create failed with error: "
                      << get_error(errno) << std::endl;
            return false;
        }
        
        return true;
    }
    bool try_open() {
        shm_id_ = shmget(mem_id_, 0, static_cast<int32_t>(access_mode_));
        if (shm_id_ == -1) {
            std::cout << "open failed with error: "
                      << get_error(errno) << std::endl;
            return false;
        }
        std::cout << "shared memeory open successfully!" << std::endl;
        return true;
    }

    bool detach() {
        int32_t ret = shmdt(mem_ptr_);
        if (ret == -1) {
            std::cout << "detach failed! errno is "
                      << get_error(errno) << std::endl;
            return false;
        }
        return true;
    }
    bool destroy() {
        int32_t ret = shmctl(shm_id_, IPC_RMID, 0);
        if (ret == -1) {
            std::cout << "remove failed! errno is "
                      << get_error(errno) << std::endl;
            return false;
        }
        return true;
    }

    int32_t get_block() {
        int32_t assigned_index = block_state_.assigned_index();
        if (assigned_index >= g_MemBlockCount)
            assigned_index &= (g_MemBlockCount - 1);

        block_state_.increase();
        return assigned_index;
    }

    bool init_ = false;
    key_t mem_id_ = -1;
    int32_t shm_id_ = -1;
    void* mem_ptr_ = nullptr;
    AccessMode access_mode_ = AccessMode::READ_ONLY;

    BlockState block_state_;
    BlockLock block_lock_[g_MemBlockCount];
    std::map<uint64_t, int32_t> elem_refers_;
    std::unordered_map<int32_t, uint8_t*> block_mems_;

    static constexpr int32_t allocate_size_ = g_MemBlockCount * T::size();
};