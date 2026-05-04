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
#include <iostream>
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

/// @brief Тестирует один режим выделения памяти для блоков заданного размера.
/// @tparam AllocFunc Тип функции выделения памяти.
/// @tparam DeallocFunc Тип функции освобождения памяти.
/// @param[in] modeName Название режима для вывода.
/// @param[in] blockCount Количество блоков для тестирования.
/// @param[in] halfBlockSize Размер половинного блока.
/// @param[in] blockSize Полный размер блока.
/// @param[out] memoryPtrs Вектор указателей на выделенную память (часть 1).
/// @param[out] memoryPtrs2 Вектор указателей на выделенную память (часть 2).
/// @param[in] allocFunc Функция выделения памяти.
/// @param[in] deallocFunc Функция освобождения памяти.
/// @return Общее время тестирования в микросекундах.
template <typename AllocFunc, typename DeallocFunc>
long long BenchmarkAllocatorMode(const char *modeName, int blockCount, int halfBlockSize, int blockSize,
                                 std::vector<void *> &memoryPtrs, std::vector<void *> &memoryPtrs2, AllocFunc allocFunc,
                                 DeallocFunc deallocFunc)
{
    long long totalElapsedMicroseconds = 0;

    long long elapsedMicroseconds = MeasureMicroseconds([&]() {
        for (int i = 0; i < blockCount; ++i)
        {
            memoryPtrs[i] = allocFunc(halfBlockSize);
        }
    });
    std::cout << modeName << " allocate half block time: " << elapsedMicroseconds << "\n";
    totalElapsedMicroseconds += elapsedMicroseconds;

    elapsedMicroseconds = MeasureMicroseconds([&]() {
        for (int i = 0; i < blockCount; i += 2)
        {
            deallocFunc(memoryPtrs[i]);
        }
    });
    std::cout << modeName << " deallocate half block time: " << elapsedMicroseconds << "\n";
    totalElapsedMicroseconds += elapsedMicroseconds;

    elapsedMicroseconds = MeasureMicroseconds([&]() {
        for (int i = 0; i < blockCount; ++i)
        {
            memoryPtrs2[i] = allocFunc(blockSize);
        }
    });
    std::cout << modeName << " allocate full block time: " << elapsedMicroseconds << "\n";
    totalElapsedMicroseconds += elapsedMicroseconds;

    elapsedMicroseconds = MeasureMicroseconds([&]() {
        for (int i = 1; i < blockCount; i += 2)
        {
            deallocFunc(memoryPtrs[i]);
        }
    });
    std::cout << modeName << " deallocate half remaining time: " << elapsedMicroseconds << "\n";
    totalElapsedMicroseconds += elapsedMicroseconds;

    elapsedMicroseconds = MeasureMicroseconds([&]() {
        for (int i = blockCount - 1; i >= 0; --i)
        {
            deallocFunc(memoryPtrs2[i]);
        }
    });
    std::cout << modeName << " deallocate full block time: " << elapsedMicroseconds << "\n";
    totalElapsedMicroseconds += elapsedMicroseconds;

    std::cout << modeName << " TOTAL TIME: " << totalElapsedMicroseconds << "\n";
    return totalElapsedMicroseconds;
}

/// @brief Запускает бенчмарки выделения памяти для одного типа с разными режимами.
/// @tparam T Тип объекта для тестирования.
/// @param[in] typeName Название типа для вывода результатов.
template <typename T> void BenchmarkType(const char *typeName)
{
    const int blockCount = 10000;
    const int blockSize = static_cast<int>(sizeof(T));
    const int halfBlockSize = std::max(1, blockSize / 2);

    std::vector<void *> memoryPtrs(blockCount);
    std::vector<void *> memoryPtrs2(blockCount);
    std::vector<char> staticMemoryPool(static_cast<std::size_t>(blockSize) * blockCount * 2);

    Allocator allocatorHeapBlocks(blockSize);
    Allocator allocatorHeapPool(blockSize, blockCount * 2);
    Allocator allocatorStaticPool(blockSize, blockCount * 2, staticMemoryPool.data());

    std::cout << "Benchmark for " << typeName << "\n";

    BenchmarkAllocatorMode(
        "Heap", blockCount, halfBlockSize, blockSize, memoryPtrs, memoryPtrs2,
        [](int size) { return static_cast<void *>(new char[size]); },
        [](void *ptr) { delete[] static_cast<char *>(ptr); });

    BenchmarkAllocatorMode(
        "Heap Blocks", blockCount, halfBlockSize, blockSize, memoryPtrs, memoryPtrs2,
        [&](int size) { return allocatorHeapBlocks.Allocate(size); },
        [&](void *ptr) { allocatorHeapBlocks.Deallocate(ptr); });

    BenchmarkAllocatorMode(
        "Heap Pool", blockCount, halfBlockSize, blockSize, memoryPtrs, memoryPtrs2,
        [&](int size) { return allocatorHeapPool.Allocate(size); },
        [&](void *ptr) { allocatorHeapPool.Deallocate(ptr); });

    BenchmarkAllocatorMode(
        "Static Pool", blockCount, halfBlockSize, blockSize, memoryPtrs, memoryPtrs2,
        [&](int size) { return allocatorStaticPool.Allocate(size); },
        [&](void *ptr) { allocatorStaticPool.Deallocate(ptr); });
}

/// @brief Запускает полный набор бенчмарков выделения памяти для всех типов проекта.
inline void RunAllocatorBenchmarks()
{
    std::cout << "Allocator benchmarks\n";
    BenchmarkType<BBV>("BBV");
    BenchmarkType<BoolInterval>("BoolInterval");
    BenchmarkType<BoolEquation>("BoolEquation");
    BenchmarkType<NodeBoolTree>("NodeBoolTree");
    BenchmarkType<MinDontCareBranchingStrategy>("MinDontCareBranchingStrategy");
    BenchmarkType<FirstFreeColumnBranchingStrategy>("FirstFreeColumnBranchingStrategy");
}

#endif // BENCHMARKS_H
