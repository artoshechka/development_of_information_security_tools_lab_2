#ifndef FIRST_FREE_COLUMN_BRANCHING_STRATEGY_H
#define FIRST_FREE_COLUMN_BRANCHING_STRATEGY_H

#include "branching_strategy.h"
#include "Allocator.h"

///@brief Стратегия выбора первого свободного столбца слева направо.
class FirstFreeColumnBranchingStrategy : public BranchingStrategy
{
public:
	///@brief Выбирает первый доступный столбец.
	///@param equation Текущее состояние булева уравнения.
	///@return Индекс столбца с нуля или -1, если свободных столбцов нет.
	int ChooseColumn(BoolEquation &equation) const;

	///@brief Возвращает идентификатор стратегии.
	///@return C-строка с именем стратегии.
	const char *GetName() const;

	DECLARE_ALLOCATOR
};

#endif // FIRST_FREE_COLUMN_BRANCHING_STRATEGY_H
