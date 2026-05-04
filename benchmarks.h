#ifndef BENCHMARKS_H
#define BENCHMARKS_H

#include "BBV.h"
#include "NodeBoolTree.h"
#include "boolequation.h"
#include "boolinterval.h"
#include "first_free_column_branching_strategy.h"
#include "min_dont_care_branching_strategy.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <new>
#include <vector>

/// @brief Измеряет время выполнения функции в микросекундах.
/// @tparam Func Тип функции для измерения.
/// @param[in] func Функция для измерения времени выполнения.
/// @return Время выполнения в микросекундах.
template <typename Func> long long MeasureMicroseconds(Func &&func)
{
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    func();
    const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

inline void PrintBenchmarkSectionHeader(const char *title)
{
    std::cout << "\n\t" << title << "\t\t\t\n";
}

inline void PrintBenchmarkModeHeader(const char *modeName)
{
    std::cout << "  [" << modeName << "]\n";
}

/// @brief Тестирует один режим выделения памяти для блоков заданного размера (только полные блоки).
/// @tparam AllocFunc Тип функции выделения памяти.
/// @tparam DeallocFunc Тип функции освобождения памяти.
/// @param[in] modeName Название режима для вывода.
/// @param[in] blockCount Количество блоков для тестирования.
/// @param[in] blockSize Полный размер блока.
/// @param[out] memoryPtrs Вектор указателей на выделенную память.
/// @param[in] allocFunc Функция выделения памяти.
/// @param[in] deallocFunc Функция освобождения памяти.
/// @return Общее время тестирования в микросекундах.
template <typename AllocFunc, typename DeallocFunc>
long long BenchmarkAllocatorMode(const char *modeName, int blockCount, int blockSize,
                                 std::vector<void *> &memoryPtrs, AllocFunc allocFunc, DeallocFunc deallocFunc)
{
    PrintBenchmarkModeHeader(modeName);

    const long long allocMicroseconds = MeasureMicroseconds([&]() {
        for (int i = 0; i < blockCount; ++i)
        {
            memoryPtrs[i] = allocFunc(blockSize);
        }
    });

    const long long deallocMicroseconds = MeasureMicroseconds([&]() {
        for (int i = 0; i < blockCount; ++i)
        {
            deallocFunc(memoryPtrs[i]);
        }
    });

    const long long totalMicroseconds = allocMicroseconds + deallocMicroseconds;

    std::cout << "    allocate (" << blockCount << " elements x " << blockSize << " Bytes): " << allocMicroseconds << " us\n";
    std::cout << "    deallocate (" << blockCount << " elements x " << blockSize << " Bytes): " << deallocMicroseconds
              << " us\n";
    std::cout << "    total (" << blockCount << " elements x " << blockSize << " Bytes): " << totalMicroseconds << " us\n";
    return totalMicroseconds;
}

/// @brief Запускает бенчмарки выделения памяти для одного типа с разными режимами.
/// @tparam T Тип объекта для тестирования.
/// @param[in] typeName Название типа для вывода результатов.
template <typename T> void BenchmarkType(const char *typeName)
{
    const int blockCount = 1000000;
    const int blockSize = static_cast<int>(sizeof(T));

    std::vector<void *> memoryPtrs(blockCount);
    std::vector<char> staticMemoryPool(static_cast<std::size_t>(blockSize) * blockCount);

    Allocator allocatorHeapBlocks(blockSize);
    Allocator allocatorHeapPool(blockSize, blockCount);
    Allocator allocatorStaticPool(blockSize, blockCount, staticMemoryPool.data());

    PrintBenchmarkSectionHeader(typeName);

    BenchmarkAllocatorMode(
        "Heap", blockCount, blockSize, memoryPtrs,
        [](int size) { return static_cast<void *>(new char[size]); },
        [](void *ptr) { delete[] static_cast<char *>(ptr); });

    BenchmarkAllocatorMode(
        "Heap Blocks", blockCount, blockSize, memoryPtrs,
        [&](int size) { return allocatorHeapBlocks.Allocate(size); },
        [&](void *ptr) { allocatorHeapBlocks.Deallocate(ptr); });

    BenchmarkAllocatorMode(
        "Heap Pool", blockCount, blockSize, memoryPtrs,
        [&](int size) { return allocatorHeapPool.Allocate(size); },
        [&](void *ptr) { allocatorHeapPool.Deallocate(ptr); });

    BenchmarkAllocatorMode(
        "Static Pool", blockCount, blockSize, memoryPtrs,
        [&](int size) { return allocatorStaticPool.Allocate(size); },
        [&](void *ptr) { allocatorStaticPool.Deallocate(ptr); });

}

/// @brief Запускает бенчмарк, который намеренно доводит выделение памяти до bad_alloc.
/// @tparam AllocFunc Тип функции выделения памяти.
/// @tparam DeallocFunc Тип функции освобождения памяти.
/// @param[in] modeName Название режима для вывода.
/// @param[in] allocationSize Размер каждого блока памяти.
/// @param[in] allocationLimit Безопасный лимит попыток выделения.
/// @param[in] allocFunc Функция выделения памяти.
/// @param[in] deallocFunc Функция освобождения памяти.
template <typename AllocFunc, typename DeallocFunc>
void BenchmarkAllocatorBadAllocMode(const char *modeName, std::size_t allocationSize, std::size_t allocationLimit,
                                    AllocFunc allocFunc, DeallocFunc deallocFunc)
{
    std::vector<void *> allocatedBlocks;
    allocatedBlocks.reserve(allocationLimit);

    bool badAllocCaught = false;
    const long long elapsedMicroseconds = MeasureMicroseconds([&]() {
        try
        {
            for (std::size_t i = 0; i < allocationLimit; ++i)
            {
                void *block = allocFunc(allocationSize);
                std::memset(block, 0xA5, allocationSize);
                allocatedBlocks.push_back(block);
            }
        }
        catch (const std::bad_alloc &)
        {
            badAllocCaught = true;
        }
    });

    std::cout << "  [" << modeName << "]\n";
    std::cout << "    allocations before bad_alloc: " << allocatedBlocks.size() << "\n";
    std::cout << "    time until bad_alloc: " << elapsedMicroseconds << " us\n";
    std::cout << "    bad_alloc caught: " << (badAllocCaught ? "yes" : "no") << "\n";

    for (auto it = allocatedBlocks.rbegin(); it != allocatedBlocks.rend(); ++it)
    {
        deallocFunc(*it);
    }
}

/// @brief Запускает набор бенчмарков, который намеренно исчерпывает память.
inline void RunAllocatorBadAllocBenchmarks()
{
    constexpr std::size_t allocationSize = 8u * 1024u * 1024u;
    constexpr std::size_t allocationLimit = 64u;
    constexpr std::size_t poolObjects = 32u;

    std::vector<char> staticMemoryPool(allocationSize * poolObjects);

    Allocator allocatorHeapBlocks(allocationSize);
    Allocator allocatorHeapPool(allocationSize, static_cast<UINT>(poolObjects));
    Allocator allocatorStaticPool(allocationSize, static_cast<UINT>(poolObjects), staticMemoryPool.data());

    PrintBenchmarkSectionHeader("Allocator bad_alloc benchmarks");

    BenchmarkAllocatorBadAllocMode(
        "Heap", allocationSize, allocationLimit,
        [](std::size_t size) { return static_cast<void *>(new char[size]); },
        [](void *ptr) { delete[] static_cast<char *>(ptr); });

    BenchmarkAllocatorBadAllocMode(
        "Heap Blocks", allocationSize, allocationLimit,
        [&](std::size_t size) { return allocatorHeapBlocks.Allocate(size); },
        [&](void *ptr) { allocatorHeapBlocks.Deallocate(ptr); });

    BenchmarkAllocatorBadAllocMode(
        "Heap Pool", allocationSize, allocationLimit,
        [&](std::size_t size) { return allocatorHeapPool.Allocate(size); },
        [&](void *ptr) { allocatorHeapPool.Deallocate(ptr); });

    BenchmarkAllocatorBadAllocMode(
        "Static Pool", allocationSize, allocationLimit,
        [&](std::size_t size) { return allocatorStaticPool.Allocate(size); },
        [&](void *ptr) { allocatorStaticPool.Deallocate(ptr); });
}

/// @brief Запускает полный набор бенчмарков выделения памяти для всех типов проекта.
inline void RunAllocatorBenchmarks()
{
    std::cout << "Allocator benchmarks" << std::endl;

    BenchmarkType<BBV>("BBV");
    BenchmarkType<BoolInterval>("BoolInterval");
    BenchmarkType<BoolEquation>("BoolEquation");
    BenchmarkType<NodeBoolTree>("NodeBoolTree");
    BenchmarkType<MinDontCareBranchingStrategy>("MinDontCareBranchingStrategy");
    BenchmarkType<FirstFreeColumnBranchingStrategy>("FirstFreeColumnBranchingStrategy");

    RunAllocatorBadAllocBenchmarks();
}

#endif // BENCHMARKS_H
