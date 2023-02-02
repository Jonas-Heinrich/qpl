/*******************************************************************************
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#pragma once

#include <utility.hpp>
#include <ops/ops.hpp>

#include <benchmark/benchmark.h>

//#define PER_THREAD_STAT
#ifdef PER_THREAD_STAT
#include <chrono>
#include <mutex>
#endif

#include <stdexcept>

namespace bench::details
{
static inline void set_thread_affinity(const benchmark::State &state)
{
    thread_local bool is_set{false};
    if(!is_set)
    {
        auto &info = get_sys_info();
        std::uint32_t devices  = info.accelerators.total_devices;
        if(devices)
        {
            pthread_t current_thread = pthread_self();
            int cpu_index = ((state.thread_index()%devices)*info.cpu_physical_per_cluster+state.thread_index()/devices)%info.cpu_physical_per_socket;
            cpu_set_t cpus;
            CPU_ZERO(&cpus);
            CPU_SET(cpu_index, &cpus);
            pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpus);
            //printf("Thread: %d - %d\n", state.thread_index(), cpu_index);
        }

        is_set = true;
    }
}

template <path_e path, typename OperationT, typename ParamsT>
static statistics_t measure_async(benchmark::State &state, const case_params_t &common_params, OperationT &&operations, ParamsT &&params)
{
    statistics_t res;
    auto threads = state.threads();
    if constexpr(path == path_e::cpu)
    {
        res.queue_size = 1;
        res.operations = threads;
    }
    else
    {
        auto devices   = get_current_numa_accels();
        res.queue_size = common_params.queue_size_;
        res.operations = res.queue_size*devices;
    }

    res.operations_per_thread = res.operations/threads;
    if (res.operations_per_thread < 1)
        throw std::runtime_error("Operation pool is too small for given threads");

    set_thread_affinity(state);

    operations.resize(res.operations_per_thread);
    for (auto &operation: operations)
    {
        operation.init(params, get_mem_cc(common_params.out_mem_), common_params.full_time_, cmd::FLAGS_node);
        operation.mem_control(common_params.in_mem_, mem_loc_mask_e::src);
    }

    // Strategies:
    // - File at once. Each operation works on same file independently.
    // - Chunk at once. Measure each chunk independently one by one, gather aggregate in the end. Is this reasonable?
    // - File by chunks. Measure for the whole file processing different chunks in parallel (map file before processing). Like normal processing

    size_t completion_limit = res.operations_per_thread; // Do at least qdepth tasks for each iteration
    bool   first_iteration  = true;
#ifdef PER_THREAD_STAT
    std::chrono::high_resolution_clock::time_point timer_start;
    std::chrono::high_resolution_clock::time_point timer;
    timer_start = std::chrono::high_resolution_clock::now();
    std::uint64_t polls = 0;
#endif

    for (auto _ : state)
    {
        if(first_iteration)
        {
            for (auto &operation : operations)
            {
                operation.async_submit();
            }
            first_iteration = false;
        }

        size_t completed = 0;
        while(completed < completion_limit)
        {
            for (size_t idx = 0; idx < operations.size(); ++idx)
            {
                auto task_status_e = operations[idx].async_poll();

#ifdef PER_THREAD_STAT
                if(task_status_e != task_status_e::retired)
                    polls++;
#endif

                if(task_status_e == task_status_e::completed)
                {
                    completed++;
                    res.completed_operations++;
                    res.data_read    += operations[idx].get_bytes_read();
                    res.data_written += operations[idx].get_bytes_written();

                    operations[idx].light_reset();
                    operations[idx].async_submit();
                }
            }
        }
    }
#ifdef PER_THREAD_STAT
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - timer_start).count()*1000000000;
#endif

    for (auto &operation : operations)
    {
        operation.async_wait();
    }

    res.completed_operations /= state.iterations();
    res.data_read            /= state.iterations();
    res.data_written         /= state.iterations();

#ifdef PER_THREAD_STAT
    static std::mutex guard;
    guard.lock();
    auto per_op = elapsed_seconds/(state.iterations()*res.completed_operations);
    printf("Thread: %3d; iters: %6lu; ops: %3u; completed/iter: %3u; polls/op: %6u; time/op: %5.0f ns\n", state.thread_index(), state.iterations(), res.operations_per_thread, (std::uint32_t)res.completed_operations, (std::uint32_t)(polls/(state.iterations()*res.completed_operations)), per_op);
    guard.unlock();
#endif

    return res;
}
}