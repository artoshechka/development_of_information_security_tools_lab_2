#ifndef MIN_DONT_CARE_BRANCHING_STRATEGY_H
#define MIN_DONT_CARE_BRANCHING_STRATEGY_H

#include "branching_strategy.h"

///@brief Стратегия выбора свободного столбца с минимальным числом символов '-'.
class MinDontCareBranchingStrategy : public BranchingStrategy
{
public:
	///@brief Выбирает столбец ветвления по эвристике минимального числа don't-care.
	///@param equation Текущее состояние булева уравнения.
	///@return Индекс столбца с нуля или -1, если свободных столбцов нет.
	int ChooseColumn(BoolEquation &equation) const;
};

#endif // MIN_DONT_CARE_BRANCHING_STRATEGY_H
