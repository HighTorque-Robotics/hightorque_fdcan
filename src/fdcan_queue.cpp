#include "fdcan_queue.h"
#include <cstdlib>
#include <cstring>


fdcan_queue::fdcan_queue(size_t capacity)
    : capacity_(capacity), head_(0), tail_(0), count_(0)
{
    if (capacity_ == 0)
    {
        buffer_ = nullptr;
        return;
    }
    buffer_ = static_cast<cdc_tr_fdcan_pass_s *>(std::malloc(capacity_ * sizeof(cdc_tr_fdcan_pass_s)));
    if (!buffer_)
    {
        PRINT_ERROR("fdcan_queue malloc error!!!");
        throw std::bad_alloc();
    }
}


fdcan_queue::~fdcan_queue()
{
    if (buffer_)
    {
        std::free(buffer_);
        buffer_ = nullptr;
    }
}


size_t fdcan_queue::next_index(size_t index) const
{
    return (index + 1) % capacity_;
}


bool fdcan_queue::push(const cdc_tr_fdcan_pass_s &item)
{
    std::lock_guard<std::mutex> lock(mtx_);

    if (!buffer_ || capacity_ == 0)
    {
        return false;
    }

    if (count_ >= capacity_)
    {
        head_ = next_index(head_);
        --count_;
    }

    buffer_[tail_] = item;
    tail_ = next_index(tail_);
    ++count_;

    return true;
}


bool fdcan_queue::pop(cdc_tr_fdcan_pass_s &item)
{
    std::lock_guard<std::mutex> lock(mtx_);

    if (!buffer_ || count_ == 0)
    {
        return false;
    }

    item = buffer_[head_];
    head_ = next_index(head_);
    --count_;

    return true;
}


void fdcan_queue::clear()
{
    std::lock_guard<std::mutex> lock(mtx_);
    head_ = 0;
    tail_ = 0;
    count_ = 0;
}


bool fdcan_queue::empty() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return count_ == 0;
}


bool fdcan_queue::full() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return count_ >= capacity_;
}


size_t fdcan_queue::size() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return count_;
}
