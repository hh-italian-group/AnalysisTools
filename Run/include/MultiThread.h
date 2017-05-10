/*! Definition of methods that provide multi-thread functionality.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>
#include <future>
#include <thread>

namespace run {

class ThreadPull {
public:
    ThreadPull(size_t n_threads, bool is_global = true)
        : work(io_service)
    {
        for(size_t n = 0; n < n_threads; ++n)
            thread_group.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));
        if(is_global)
            GlobalPull() = this;
    }

    ThreadPull(const ThreadPull&) = delete;

    ~ThreadPull()
    {
        io_service.stop();
        thread_group.join_all();
    }

    template<class Function, class... Args>
    std::future<std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>>
        run(Function&& f, Args&&... args)
    {
        using Result = std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>;
        auto task = std::make_shared<std::packaged_task<Result()>>(std::bind(f, std::forward<Args>(args)...));
        auto future = task->get_future();
        io_service.post([task = std::move(task)] { (*task)(); });
        return future;
    }

    template< class Function, class... Args>
    static std::future<std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>>
        run_global(Function&& f, Args&&... args)
    {
        if(!GlobalPull())
            throw std::runtime_error("Global thread pull not defined.");
        return GlobalPull()->run(f, std::forward<Args>(args)...);
    }

private:
    static ThreadPull*& GlobalPull() { static ThreadPull* global_pull = nullptr; return global_pull; }
private:
    boost::asio::io_service io_service;
    boost::asio::io_service::work work;
    boost::thread_group thread_group;
};

template< class Function, class... Args>
std::future<std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>>
    async(Function&& f, Args&&... args)
{
    return ThreadPull::run_global(f, std::forward<Args>(args)...);
}

} // namespace run
