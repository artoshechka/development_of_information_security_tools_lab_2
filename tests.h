#ifndef TESTS_H
#define TESTS_H

#include "BBV.h"
#include "NodeBoolTree.h"
#include "boolequation.h"
#include "boolinterval.h"
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

/// @brief Тестирует создание объектов BoolInterval с обработкой ошибок памяти.
inline void TestBoolIntervalCreation()
{
    std::cout << "Test 1 Creating BoolInterval objects\n";
    try
    {
        BoolInterval *interval1 = new BoolInterval("11001100");
        BoolInterval *interval2 = new BoolInterval("00110011");
        std::cout << "SUCCESS: BoolInterval objects created successfully\n";
        delete interval1;
        delete interval2;
    }
    catch (const std::bad_alloc &e)
    {
        std::cout << "ERROR: Memory allocation failed " << e.what() << "\n";
    }
}

/// @brief Тестирует создание объектов BBV с обработкой ошибок памяти.
inline void TestBBVCreation()
{
    std::cout << "Test 2 Creating BBV objects\n";
    try
    {
        BBV vec1("11110000");
        BBV vec2("00001111");
        std::cout << "SUCCESS: BBV objects created successfully\n";
    }
    catch (const std::bad_alloc &e)
    {
        std::cout << "ERROR: Memory allocation failed " << e.what() << "\n";
    }
}

/// @brief Тестирует обработку ошибок при попытке выделения огромного объема памяти.
inline void TestAllocationFailure()
{
    std::cout << "Test 3 Testing allocation failure handling\n";
    try
    {
        size_t huge_size = static_cast<size_t>(-1) / 2;
        char *huge_array = new char[huge_size];
        delete[] huge_array;
        std::cout << "WARNING: Huge allocation succeeded system has enough memory\n";
    }
    catch (const std::bad_alloc &e)
    {
        std::cout << "SUCCESS: Memory allocation failure caught correctly " << e.what() << "\n";
    }
}

/// @brief Тестирует создание объектов NodeBoolTree с обработкой ошибок памяти.
inline void TestNodeBoolTreeCreation()
{
    std::cout << "Test 4 Creating NodeBoolTree objects\n";
    try
    {
        BBV vec("11001100");
        BBV dnc("00110011");
        BoolInterval *root = new BoolInterval(vec, dnc);
        BoolInterval **cnf = new BoolInterval *[1];
        cnf[0] = new BoolInterval("10101010");

        BoolEquation *eq = new BoolEquation(cnf, root, 1, 1, vec);
        NodeBoolTree *node = new NodeBoolTree(eq);
        std::cout << "SUCCESS: NodeBoolTree created successfully\n";

        delete node;
        delete cnf[0];
        delete[] cnf;
    }
    catch (const std::bad_alloc &e)
    {
        std::cout << "ERROR: Memory allocation failed " << e.what() << "\n";
    }
}

/// @brief Тестирует создание большого количества объектов BBV до исчерпания памяти в нескольких потоках.
inline void TestMemoryExhaustion()
{
    std::cout << "Test 5 Creating many BBV objects until memory runs out\n";
    try
    {
        std::vector<BBV *> vectors;
        std::mutex vectors_mutex;

        long long total_count = 0;
        std::mutex count_mutex;
        bool should_stop = false;
        std::mutex stop_mutex;

        auto start_time = std::chrono::steady_clock::now();

        const int num_threads = std::max(1u, std::thread::hardware_concurrency());
        const int objects_per_iteration = 10000;
        const int stats_interval_seconds = 30;

        auto thread_func = [&]() {
            while (true)
            {
                try
                {
                    for (int i = 0; i < objects_per_iteration; i++)
                    {
                        BBV *vec = new BBV("11001100110011001100110011001100");
                        {
                            std::lock_guard<std::mutex> lock(vectors_mutex);
                            vectors.push_back(vec);
                            {
                                std::lock_guard<std::mutex> count_lock(count_mutex);
                                total_count++;
                            }
                        }
                    }
                }
                catch (const std::bad_alloc &e)
                {
                    {
                        std::lock_guard<std::mutex> stop_lock(stop_mutex);
                        should_stop = true;
                    }
                    break;
                }

                {
                    std::lock_guard<std::mutex> stop_lock(stop_mutex);
                    if (should_stop)
                        break;
                }
            }
        };

        auto stats_thread_func = [&]() {
            while (true)
            {
                std::this_thread::sleep_for(std::chrono::seconds(stats_interval_seconds));

                {
                    std::lock_guard<std::mutex> stop_lock(stop_mutex);
                    if (should_stop)
                        break;
                }

                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();

                {
                    std::lock_guard<std::mutex> count_lock(count_mutex);
                    std::lock_guard<std::mutex> vectors_lock(vectors_mutex);
                    std::cout << "[STATS] Elapsed: " << elapsed << "s | Objects created: " << total_count << " | Vector size: " << vectors.size() << " | Rate: " << (total_count > 0 ? total_count / elapsed : 0) << " obj/s\n";
                }
            }
        };

        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; i++)
        {
            threads.emplace_back(thread_func);
        }

        std::thread stats_thread(stats_thread_func);

        for (auto &t : threads)
        {
            t.join();
        }

        stats_thread.join();

        auto end_time = std::chrono::steady_clock::now();
        auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

        std::cout << "Memory limit reached after creating " << total_count << " objects across " << num_threads << " threads in " << total_elapsed << " seconds\n";
        std::cout << "Average rate: " << (total_count > 0 ? total_count / total_elapsed : 0) << " objects/second\n";
        std::cout << "SUCCESS: Caught bad_alloc as expected\n";

        for (auto v : vectors)
        {
            delete v;
        }
        vectors.clear();
    }
    catch (const std::exception &e)
    {
        std::cout << "ERROR: Unexpected exception " << e.what() << "\n";
    }
}

/// @brief Запускает полный набор тестов управления памятью.
inline void RunMemoryTests()
{
    std::cout << "Testing Memory Error Handling\n";
    TestMemoryExhaustion();
    TestBoolIntervalCreation();
    TestBBVCreation();
    TestAllocationFailure();
    TestNodeBoolTreeCreation();
    std::cout << "All memory tests completed\n\n";
}

#endif // TESTS_H
