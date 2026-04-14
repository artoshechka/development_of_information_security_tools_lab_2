#include <iostream>
#include <QFile>
#include <QTextStream>
#include <QStack>
#include <string>
#include <cstring>
#include <stack>
#include <ostream>
#include "NodeBoolTree.h"
#include "boolinterval.h"
#include "boolequation.h"
#include "BBV.h"


int main(int argc, char *argv[])
{
	QStringList full_file_list;
	QList<QStringList> Elements;
	std::string filepath;
	QStringList inputs;
	//std::cout << "Input file path...\n";
	//std::cin >> filepath;
	// Hardcode input
	//	filepath = "sat_ex_2.pla";
	//filepath = "Sat_ex11_3.pla";
	filepath = "Sat_ex30_3.pla";
	QFile file(QString::fromUtf8(filepath.c_str()));

	//считываем весь файл
	if ((file.exists()) && (file.open(QIODevice::ReadOnly))) {
		while (!file.atEnd()) {
			full_file_list << file.readLine().replace("\r\n", "");
		}

		int cnfSize = full_file_list.length();
		BoolInterval **CNF = new BoolInterval*[cnfSize];
		int rangInterval = -1; // error

		if (cnfSize) {
			rangInterval = full_file_list[0].toUtf8().trimmed().length();
		}

		for (int i = 0; i < cnfSize; i++) { // Заполняем массив
			QString strv = full_file_list[i];
			CNF[i] = new BoolInterval(strv.toUtf8().trimmed().data());
		}

		QString rootvec = "";
		QString rootdnc = "";

		//Строим интервал в которм все компоненты принимают значение '-',
		//который представляет собой корень уравнения, пока пустой.
		//В процессе поиска корня, компоненты интервала буду заменены на конкретные значения.

		for (int i = 0; i < rangInterval; i++) {
			rootvec += "0";
			rootdnc += "1";
		}

		QByteArray v = rootvec.toUtf8();

		BBV vec(v.data());
		QByteArray d = rootdnc.toUtf8();
		BBV dnc(d.data());

		// Создаем пустой корень уравнения;
		BoolInterval *root = new BoolInterval(vec, dnc);

		BoolEquation *boolequation = new BoolEquation(CNF, root, cnfSize, cnfSize, vec);

		// Алгоритм поиска корня. Работаем всегда с верхушкой стека.
		// Шаг 1. Правила выполняются? Нет - Ветвление Шаг 5. Да - Упрощаем Шаг 2.
		// Шаг 2. Строки закончились? Нет - Шаг1, Да - Корень найден? Да - Успех КОНЕЦ, Нет - Шаг 3.
		// Шаг 3. Кол-во узлов в стеке > 1? Нет - Корня нет КОНЕЦ, Да - Шаг 4.
		// Шаг 4. Текущий узел выталкиваем из стека, попадаем в новый узел. У нового узла lt rt отличны от NULL? Нет - Шаг 1. Да - Шаг 3.
		// Шаг 5. Выбор компоненты ветвления, создание двух новых узлов, добавление их в стек сначала с 1 потом с 0. Шаг 1.

		// Алгоритм CheckRules.
		// Цикл по строкам КНФ.
		// 1. Проверка правила 2. Выполнилось? Да - Корня нет, Нет - Идем дальше.
		// 2. Проверка правила 1. Выполнилось? Да - Упрощаем, Нет - Идем дальше.

		// Создаем стек под узлы булева дерева
		// QStack<NodeBoolTree> BoolTree;

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

						case 1: { // Правило выполнилось, корень найден или продолжаем упрощать.
							if (currentEquation->count == 0 ||
									currentEquation->mask.getWeight() ==
									currentEquation->mask.getSize()) { // Если кончились строки или столбцы, корень найден.
								flag = false;
								rootIsFinded =
									true; // Полагаем, что корень найден, выполняем проверку корня

								for (int i = 0; i < cnfSize; i++) {

									if (!CNF[i]->isEqualComponent(*currentEquation->root)) {
										rootIsFinded = false;//Корень не найден. Продолжаем искать дальше.
										BoolTree.pop();
										break;
									}
								}
							}

							break;
						}

						case 2: { // Правила не выполнились, ветвление.
							// Ветвление, создание новых узлов.

							int indexBranching = currentEquation->ChooseColForBranching();

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

	} else {
		std::cout << "File does not exists.\n";
	}

	return 0;

}
