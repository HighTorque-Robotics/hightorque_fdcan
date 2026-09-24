#ifndef FDCAN_QUEUE_H
#define FDCAN_QUEUE_H

#include <cstddef>
#include <cstdint>
#include <mutex>
#include "serial_struct.h"


class FdcanQueue
{
public:
    explicit FdcanQueue(size_t capacity = 20);

    FdcanQueue(const FdcanQueue &) = delete;
    FdcanQueue &operator=(const FdcanQueue &) = delete;

    ~FdcanQueue();

    bool push(const fdcan_msg_s &item);
    bool pop(fdcan_msg_s &item);
    void clear();

    bool empty() const;
    bool full() const;
    size_t size() const;
    size_t get_capacity() const { return capacity; }

private:
    size_t next_index(size_t index) const;

    fdcan_msg_s *buffer;
    size_t capacity;
    size_t head;            // 出队位置
    size_t tail;            // 入队位置
    size_t count;           // 当前元素个数
    mutable std::mutex mtx; // 互斥锁
};

#endif
