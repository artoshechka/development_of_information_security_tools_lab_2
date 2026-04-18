#ifndef BRANCHING_STRATEGY_H
#define BRANCHING_STRATEGY_H

class BoolEquation;

///@brief Интерфейс стратегий выбора столбца ветвления для SAT-решателя.
class BranchingStrategy
{
public:
	///@brief Виртуальный деструктор для корректного полиморфного удаления.
	virtual ~BranchingStrategy() {}

	///@brief Выбирает индекс столбца для ветвления.
	///@param equation Текущее состояние булева уравнения.
	///@return Индекс столбца с нуля или -1, если свободных столбцов нет.
	virtual int ChooseColumn(BoolEquation &equation) const = 0;

	///@brief Возвращает человекочитаемый идентификатор стратегии.
	///@return C-строка с именем стратегии.
	virtual const char *GetName() const = 0;
};

#endif // BRANCHING_STRATEGY_H
