#ifndef FDCAN_QUEUE_H
#define FDCAN_QUEUE_H

#include <cstddef>
#include <cstdint>
#include <mutex>
#include "serial_struct.h"


class fdcan_queue
{
public:
    explicit fdcan_queue(size_t capacity = 20);

    fdcan_queue(const fdcan_queue &) = delete;
    fdcan_queue &operator=(const fdcan_queue &) = delete;

    ~fdcan_queue();

    bool push(const cdc_tr_fdcan_s &item);
    bool pop(cdc_tr_fdcan_s &item);
    void clear();

    bool empty() const;
    bool full() const;
    size_t size() const;
    size_t capacity() const { return capacity_; }

private:
    size_t next_index(size_t index) const;

    cdc_tr_fdcan_s *buffer_;
    size_t capacity_;
    size_t head_;            // 出队位置
    size_t tail_;            // 入队位置
    size_t count_;           // 当前元素个数
    mutable std::mutex mtx_; // 互斥锁
};

#endif
