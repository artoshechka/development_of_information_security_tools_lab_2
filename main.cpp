#include "BBV.h"
#include "NodeBoolTree.h"
#include "boolequation.h"
#include "boolinterval.h"
#include "branching_strategy_factory.h"
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
#include <vector>

static void out_of_memory() {
  // Called by global new or by Allocator when pool allocation fails.
  throw std::bad_alloc();
}

static std::string trim(const std::string &value) {
  const std::string whitespace = " \t\n\r";
  const std::size_t begin = value.find_first_not_of(whitespace);

  if (begin == std::string::npos) {
    return "";
  }

  const std::size_t end = value.find_last_not_of(whitespace);
  return value.substr(begin, end - begin + 1);
}

static void PrintAvailableStrategies() {
  std::cout << "Available branching strategies:\n";
  std::cout << "  min-dont-care (aliases: min)\n";
  std::cout << "  first-free (aliases: first)\n";
}

static void DestroyTree(NodeBoolTree *root) {
  if (root == nullptr) {
    return;
  }

  std::stack<NodeBoolTree *> nodes;
  nodes.push(root);

  while (!nodes.empty()) {
    NodeBoolTree *node = nodes.top();
    nodes.pop();

    if (node->lt != nullptr) {
      nodes.push(node->lt);
    }

    if (node->rt != nullptr) {
      nodes.push(node->rt);
    }

    delete node->eq;
    delete node;
  }
}

int main(int argc, char *argv[]) {
  std::set_new_handler(out_of_memory);

  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <input_file> [strategy_name]\n";
    PrintAvailableStrategies();
    return 1;
  }

  std::string strategyName = "min-dont-care";
  if (argc > 2) {
    strategyName = argv[2];
  }

  std::unique_ptr<BranchingStrategy> branchingStrategy;
  try {
    branchingStrategy = BranchingStrategyFactory::CreateByName(strategyName);
  } catch (const std::invalid_argument &error) {
    std::cout << error.what() << "\n";
    PrintAvailableStrategies();
    return 1;
  }

  std::vector<std::string> full_file_list;
  std::string filepath = argv[1];

  std::cout << "Input file: " << filepath << "\n";

  std::ifstream file(filepath.c_str());

  // считываем весь файл
  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      if (!line.empty() && line[line.size() - 1] == '\r') {
        line.erase(line.size() - 1);
      }
      full_file_list.push_back(line);
    }

    file.close();

    int cnfSize = static_cast<int>(full_file_list.size());
    BoolInterval **CNF = new BoolInterval *[cnfSize];
    int rangInterval = -1; // error

    if (cnfSize) {
      rangInterval = static_cast<int>(trim(full_file_list[0]).length());
    }

    for (int i = 0; i < cnfSize; i++) { // Заполняем массив
      std::string strv = trim(full_file_list[i]);
      CNF[i] = new BoolInterval(strv.c_str());
    }

    std::string rootvec;
    std::string rootdnc;

    // Строим интервал в которм все компоненты принимают значение '-',
    // который представляет собой корень уравнения, пока пустой.
    // В процессе поиска корня, компоненты интервала буду заменены на конкретные
    // значения.

    for (int i = 0; i < rangInterval; i++) {
      rootvec += "0";
      rootdnc += "1";
    }

    BBV vec(rootvec.c_str());
    BBV dnc(rootdnc.c_str());

    // Создаем пустой корень уравнения;
    BoolInterval *root = new BoolInterval(vec, dnc);

    BoolEquation *boolequation =
        new BoolEquation(CNF, root, cnfSize, cnfSize, vec);

    // Алгоритм поиска корня. Работаем всегда с верхушкой стека.
    // Шаг 1. Правила выполняются? Нет - Ветвление Шаг 5. Да - Упрощаем Шаг 2.
    // Шаг 2. Строки закончились? Нет - Шаг1, Да - Корень найден? Да - Успех
    // КОНЕЦ, Нет - Шаг 3. Шаг 3. Кол-во узлов в стеке > 1? Нет - Корня нет
    // КОНЕЦ, Да - Шаг 4. Шаг 4. Текущий узел выталкиваем из стека, попадаем в
    // новый узел. У нового узла lt rt отличны от NULL? Нет - Шаг 1. Да - Шаг 3.
    // Шаг 5. Выбор компоненты ветвления, создание двух новых узлов, добавление
    // их в стек сначала с 1 потом с 0. Шаг 1.

    // Алгоритм CheckRules.
    // Цикл по строкам КНФ.
    // 1. Проверка правила 2. Выполнилось? Да - Корня нет, Нет - Идем дальше.
    // 2. Проверка правила 1. Выполнилось? Да - Упрощаем, Нет - Идем дальше.

    // Создаем стек под узлы булева дерева
    // Стек под узлы булева дерева.

    bool rootIsFinded = false;
    stack<NodeBoolTree *> BoolTree;
    NodeBoolTree *startNode = new NodeBoolTree(boolequation);
    BoolTree.push(startNode);

    do {
      NodeBoolTree *currentNode(BoolTree.top());

      if (currentNode->lt == nullptr &&
          currentNode->rt == nullptr) { // Если вернулись в обработанный узел
        BoolEquation *currentEquation = currentNode->eq;
        bool flag = true;

        // Цикл для упрощения по правилам.
        while (flag) {
          int a = currentEquation->CheckRules(); // Проверка выполнения правил

          switch (a) {
          case 0: { // Корня нет.
            BoolTree.pop();
            flag = false;
            break;
          }

          case 1: { // Правило выполнилось, корень найден или продолжаем
                    // упрощать.
            if (currentEquation->count == 0 ||
                currentEquation->mask.getWeight() ==
                    currentEquation->mask
                        .getSize()) { // Если кончились строки или столбцы,
                                      // корень найден.
              flag = false;
              rootIsFinded =
                  true; // Полагаем, что корень найден, выполняем проверку корня

              for (int i = 0; i < cnfSize; i++) {

                if (!CNF[i]->isEqualComponent(*currentEquation->root)) {
                  rootIsFinded =
                      false; // Корень не найден. Продолжаем искать дальше.
                  BoolTree.pop();
                  break;
                }
              }
            }

            break;
          }

          case 2: { // Правила не выполнились, ветвление.
            // Ветвление, создание новых узлов.

            int indexBranching =
                currentEquation->ChooseColForBranching(*branchingStrategy);

            if (indexBranching < 0) {
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
      } else {
        BoolTree.pop();
      }

    } while (BoolTree.size() > 1 && !rootIsFinded);

    if (rootIsFinded) {
      cout << "Root is:\n ";
      BoolInterval *finded_root = BoolTree.top()->eq->root;
      cout << string(*finded_root);
    } else {
      cout << "Root is not exists!";
    }

    DestroyTree(startNode);

    for (int i = 0; i < cnfSize; i++) {
      delete CNF[i];
    }

    delete[] CNF;

  } else {
    std::cout << "File does not exists.\n";
  }

  return 0;
}
