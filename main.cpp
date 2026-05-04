#include "Allocator.h"
#include "BBV.h"
#include "NodeBoolTree.h"
#include "boolequation.h"
#include "boolinterval.h"
#include "branching_strategy_factory.h"
#include "first_free_column_branching_strategy.h"
#include "min_dont_care_branching_strategy.h"
#include <benchmarks.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <ostream>
#include <stack>
#include <stdexcept>
#include <string>
#include <tests.h>
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

    bool runBench = false;
    bool runTest = false;
    std::string inputFile;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--bench")
        {
            runBench = true;
        }
        else if (arg == "--test")
        {
            runTest = true;
        }
        else if (arg[0] != '-')
        {
            inputFile = arg;
        }
    }

    if (runBench)
    {
        RunAllocatorBenchmarks();
    }

    if (runTest)
    {
        RunMemoryTests();
    }

    if (!runBench && !runTest && inputFile.empty())
    {
        std::cout << "Usage: " << argv[0] << " <input_file> [strategy_name]\n";
        std::cout << "   or: " << argv[0] << " --bench\n";
        std::cout << "   or: " << argv[0] << " --test\n";
        std::cout << "   or: " << argv[0] << " --bench --test <input_file> [strategy_name]\n";
        PrintAvailableStrategies();
        return 1;
    }

    if (!inputFile.empty())
    {
        std::string strategyName = "min-dont-care";

        for (int i = 1; i < argc; i++)
        {
            if (argv[i] == inputFile && i + 1 < argc && argv[i + 1][0] != '-')
            {
                strategyName = argv[i + 1];
                break;
            }
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

        SolveBooleanEquation(inputFile, *branchingStrategy);
    }

    return 0;
}
