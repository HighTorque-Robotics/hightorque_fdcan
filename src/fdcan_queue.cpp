#include "fdcan_queue.h"
#include <cstdlib>
#include <cstring>


FdcanQueue::FdcanQueue(size_t capacity)
    : capacity(capacity), head(0), tail(0), count(0)
{
    if (capacity == 0)
    {
        buffer = nullptr;
        return;
    }
    buffer = static_cast<fdcan_msg_s *>(std::malloc(capacity * sizeof(fdcan_msg_s)));
    if (!buffer)
    {
        PRINT_ERROR("FdcanQueue malloc error!!!");
        throw std::bad_alloc();
    }
}


FdcanQueue::~FdcanQueue()
{
    if (buffer)
    {
        std::free(buffer);
        buffer = nullptr;
    }
}


size_t FdcanQueue::next_index(size_t index) const
{
    return (index + 1) % capacity;
}


bool FdcanQueue::push(const fdcan_msg_s &item)
{
    std::lock_guard<std::mutex> lock(mtx);

    if (!buffer || capacity == 0)
    {
        return false;
    }

    if (count >= capacity)
    {
        head = next_index(head);
        --count;
    }

    buffer[tail] = item;
    tail = next_index(tail);
    ++count;

    return true;
}


bool FdcanQueue::pop(fdcan_msg_s &item)
{
    std::lock_guard<std::mutex> lock(mtx);

    if (!buffer || count == 0)
    {
        return false;
    }

    item = buffer[head];
    head = next_index(head);
    --count;

    return true;
}


void FdcanQueue::clear()
{
    std::lock_guard<std::mutex> lock(mtx);
    head = 0;
    tail = 0;
    count = 0;
}


bool FdcanQueue::empty() const
{
    std::lock_guard<std::mutex> lock(mtx);
    return count == 0;
}


bool FdcanQueue::full() const
{
    std::lock_guard<std::mutex> lock(mtx);
    return count >= capacity;
}


size_t FdcanQueue::size() const
{
    std::lock_guard<std::mutex> lock(mtx);
    return count;
}
