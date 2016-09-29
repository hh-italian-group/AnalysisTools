/*! Definiton of a thread-safe entry queue.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <mutex>
#include <queue>
#include <condition_variable>

namespace run {

template<typename Entry>
class EntryQueue {
public:
    using Queue = std::queue<Entry>;

public:
    explicit EntryQueue(size_t _max_queue_size) : max_queue_size(_max_queue_size), all_done(false) {}

    void Push(const Entry& entry)
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            cond_var.wait(lock, [&] { return queue.size() < max_queue_size; });
            queue.push(entry);
        }
        cond_var.notify_all();
    }

    bool Pop(Entry& entry)
    {
        bool need_to_loop;
        {
            std::unique_lock<std::mutex> lock(mutex);
            cond_var.wait(lock, [&] { return queue.size() || all_done; });
            need_to_loop = queue.size();
            if(queue.size()) {
                entry = queue.front();
                queue.pop();
            }
        }
        cond_var.notify_all();
        return need_to_loop;
    }

    void SetAllDone()
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            all_done = true;
        }
        cond_var.notify_all();
    }

private:
    Queue queue;
    size_t max_queue_size;
    bool all_done;
    std::mutex mutex;
    std::condition_variable cond_var;
};

} // namespace analysis
