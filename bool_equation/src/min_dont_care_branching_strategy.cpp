#include "min_dont_care_branching_strategy.h"

#include <algorithm>
#include <vector>

#include "boolequation.h"

int MinDontCareBranchingStrategy::ChooseColumn(BoolEquation &equation) const {
  std::vector<int> indexes;
  std::vector<int> values;
  bool rezInit = false;

  // Собираем индексы свободных (незафиксированных) столбцов
  for (int i = 0; i < equation.mask.getSize(); i++) {
    if (equation.mask[i] == 0) {
      indexes.push_back(i);
    }
  }

  if (indexes.empty()) {
    return -1; // Свободных столбцов нет — ветвиться некуда
  }

  // Для каждого свободного столбца считаем кол-во символов '-' по всем
  // интервалам КНФ
  for (int i = 0; i < equation.cnfSize; i++) {
    BoolInterval *interval = equation.cnf[i];

    if (interval != nullptr) {
      if (!rezInit) {
        // Инициализируем счётчики на первом живом интервале
        for (int k = 0; k < static_cast<int>(indexes.size()); k++) {
          if (interval->getValue(indexes.at(k)) == '-') {
            values.push_back(1);
          } else {
            values.push_back(0);
          }
        }

        rezInit = true;
      } else {
        // Накапливаем don't-care по остальным интервалам
        for (int k = 0; k < static_cast<int>(indexes.size()); k++) {
          if (interval->getValue(indexes.at(k)) == '-') {
            values.at(k)++;
          }
        }
      }
    }
  }

  if (values.empty()) {
    return indexes.front(); // КНФ пуста — берём первый свободный столбец
  }

  // Возвращаем столбец с наименьшим числом don't-care
  int minElementIndex = static_cast<int>(
      std::min_element(values.begin(), values.end()) - values.begin());
  return indexes.at(minElementIndex);
}
