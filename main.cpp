#include "Allocator.h"
#include "BBV.h"
#include "NodeBoolTree.h"
#include "boolequation.h"
#include "boolinterval.h"
#include "branching_strategy_factory.h"
#include "first_free_column_branching_strategy.h"
#include "min_dont_care_branching_strategy.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <new>
#include <ostream>
#include <stack>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static void out_of_memory()
{
    throw std::bad_alloc();
}

static std::string Trim(const std::string &value)
{
    const std::string whitespace = " \t\n\r";
    const std::size_t begin = value.find_first_not_of(whitespace);

    if (begin == std::string::npos)
    {
        return "";
    }

    const std::size_t end = value.find_last_not_of(whitespace);
    return value.substr(begin, end - begin + 1);
}

static void PrintAvailableStrategies()
{
    std::cout << "Available branching strategies:\n";
    std::cout << "  min-dont-care (aliases: min)\n";
    std::cout << "  first-free (aliases: first)\n";
}

static void DestroyTree(NodeBoolTree *root)
{
    if (root == nullptr)
    {
        return;
    }

    std::stack<NodeBoolTree *> nodes;
    nodes.push(root);

    while (!nodes.empty())
    {
        NodeBoolTree *node = nodes.top();
        nodes.pop();

        if (node->lt != nullptr)
        {
            nodes.push(node->lt);
        }

        if (node->rt != nullptr)
        {
            nodes.push(node->rt);
        }

        delete node->eq;
        delete node;
    }
}

namespace
{

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
void RunAllocatorBenchmarks()
{
    std::cout << "Allocator benchmarks\n";
    BenchmarkType<BBV>("BBV");
    BenchmarkType<BoolInterval>("BoolInterval");
    BenchmarkType<BoolEquation>("BoolEquation");
    BenchmarkType<NodeBoolTree>("NodeBoolTree");
    BenchmarkType<MinDontCareBranchingStrategy>("MinDontCareBranchingStrategy");
    BenchmarkType<FirstFreeColumnBranchingStrategy>("FirstFreeColumnBranchingStrategy");
}

/// @brief Тестирует создание объектов BoolInterval с обработкой ошибок памяти.
void TestBoolIntervalCreation()
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
void TestBBVCreation()
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
void TestAllocationFailure()
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
void TestNodeBoolTreeCreation()
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
void TestMemoryExhaustion()
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
                    std::cout << "[STATS] Elapsed: " << elapsed << "s | Objects created: " << total_count 
                              << " | Vector size: " << vectors.size() << " | Rate: " 
                              << (total_count > 0 ? total_count / elapsed : 0) << " obj/s\n";
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

        std::cout << "Memory limit reached after creating " << total_count << " objects across " << num_threads
                  << " threads in " << total_elapsed << " seconds\n";
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
void RunMemoryTests()
{
    std::cout << "Testing Memory Error Handling\n";
    TestMemoryExhaustion();
    TestBoolIntervalCreation();
    TestBBVCreation();
    TestAllocationFailure();
    TestNodeBoolTreeCreation();
    std::cout << "All memory tests completed\n\n";
}

/// @brief Решает булево уравнение с использованием алгоритма DPLL с выбранной стратегией ветвления.
/// @param[in] filepath Путь к входному файлу с булевыми интервалами.
/// @param[in] branchingStrategy Стратегия выбора колонки при ветвлении.
void SolveBooleanEquation(const std::string &filepath, const BranchingStrategy &branchingStrategy)
{
    std::vector<std::string> full_file_list;

    std::cout << "Input file: " << filepath << "\n";

    std::ifstream file(filepath.c_str());

    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            if (!line.empty() && line[line.size() - 1] == '\r')
            {
                line.erase(line.size() - 1);
            }
            full_file_list.push_back(line);
        }

        file.close();

        int cnfSize = static_cast<int>(full_file_list.size());
        BoolInterval **CNF = new BoolInterval *[cnfSize];
        int rangInterval = -1;

        if (cnfSize)
        {
            rangInterval = static_cast<int>(Trim(full_file_list[0]).length());
        }

        for (int i = 0; i < cnfSize; i++)
        {
            std::string strv = Trim(full_file_list[i]);
            CNF[i] = new BoolInterval(strv.c_str());
        }

        std::string rootvec;
        std::string rootdnc;

        for (int i = 0; i < rangInterval; i++)
        {
            rootvec += "0";
            rootdnc += "1";
        }

        BBV vec(rootvec.c_str());
        BBV dnc(rootdnc.c_str());

        BoolInterval *root = new BoolInterval(vec, dnc);

        BoolEquation *boolequation = new BoolEquation(CNF, root, cnfSize, cnfSize, vec);

        bool rootIsFinded = false;
        stack<NodeBoolTree *> BoolTree;
        NodeBoolTree *startNode = new NodeBoolTree(boolequation);
        BoolTree.push(startNode);

        do
        {
            NodeBoolTree *currentNode(BoolTree.top());

            if (currentNode->lt == nullptr && currentNode->rt == nullptr)
            {
                BoolEquation *currentEquation = currentNode->eq;
                bool flag = true;

                while (flag)
                {
                    int a = currentEquation->CheckRules();

                    switch (a)
                    {
                    case 0: {
                        BoolTree.pop();
                        flag = false;
                        break;
                    }

                    case 1: {
                        if (currentEquation->count == 0 ||
                            currentEquation->mask.getWeight() == currentEquation->mask.getSize())
                        {
                            flag = false;
                            rootIsFinded = true;

                            for (int i = 0; i < cnfSize; i++)
                            {

                                if (!CNF[i]->isEqualComponent(*currentEquation->root))
                                {
                                    rootIsFinded = false;
                                    BoolTree.pop();
                                    break;
                                }
                            }
                        }

                        break;
                    }

                    case 2: {
                        int indexBranching = currentEquation->ChooseColForBranching(branchingStrategy);

                        if (indexBranching < 0)
                        {
                            BoolTree.pop();
                            flag = false;
                            break;
                        }

                        BoolEquation *Equation0 = new BoolEquation(*currentEquation);
                        BoolEquation *Equation1 = new BoolEquation(*currentEquation);

                        Equation0->Simplify(indexBranching, '0');
                        Equation1->Simplify(indexBranching, '1');

                        NodeBoolTree *Node0 = new NodeBoolTree(Equation0);
                        NodeBoolTree *Node1 = new NodeBoolTree(Equation1);

                        currentNode->lt = Node0;
                        currentNode->rt = Node1;

                        BoolTree.push(Node1);
                        BoolTree.push(Node0);

                        flag = false;
                        break;
                    }
                    }
                }
            }
            else
            {
                BoolTree.pop();
            }

        } while (BoolTree.size() > 1 && !rootIsFinded);

        if (rootIsFinded)
        {
            cout << "Root is:\n ";
            BoolInterval *finded_root = BoolTree.top()->eq->root;
            cout << string(*finded_root);
        }
        else
        {
            cout << "Root is not exists!";
        }

        DestroyTree(startNode);

        for (int i = 0; i < cnfSize; i++)
        {
            delete CNF[i];
        }

        delete[] CNF;
    }
    else
    {
        std::cout << "File does not exists.\n";
    }
}

} // namespace

/// @brief Главная функция программы для решения булевых уравнений.
/// @param[in] argc Количество аргументов командной строки.
/// @param[in] argv Массив аргументов командной строки.
/// @return Код возврата: 0 в случае успеха, 1 в случае ошибки.
int main(int argc, char *argv[])
{
    std::set_new_handler(out_of_memory);

    if (argc >= 2 && std::string(argv[1]) == "--bench")
    {
        RunAllocatorBenchmarks();
        return 0;
    }

    RunMemoryTests();

    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <input_file> [strategy_name]\n";
        std::cout << "   or: " << argv[0] << " --bench\n";
        PrintAvailableStrategies();
        return 1;
    }

    std::string strategyName = "min-dont-care";
    if (argc > 2)
    {
        strategyName = argv[2];
    }

    std::shared_ptr<BranchingStrategy> branchingStrategy;
    try
    {
        branchingStrategy = BranchingStrategyFactory::GetStrategy(strategyName);
    }
    catch (const std::invalid_argument &error)
    {
        std::cout << error.what() << "\n";
        PrintAvailableStrategies();
        return 1;
    }

    SolveBooleanEquation(argv[1], *branchingStrategy);

    return 0;
}
