#ifndef BRANCHING_STRATEGY_FACTORY_H
#define BRANCHING_STRATEGY_FACTORY_H

#include <memory>
#include <string>

#include "branching_strategy.h"

///@brief Фабрика создания стратегий ветвления.
class BranchingStrategyFactory
{
public:
	///@brief Создаёт стратегию ветвления по имени.
	///@param name Имя стратегии (например: "min-dont-care" или "first-free").
	///@return Умный указатель на созданную стратегию.
	///@throws std::invalid_argument Если передано неподдерживаемое имя.
	static std::shared_ptr<BranchingStrategy> CreateByName(const std::string &name);
};

#endif // BRANCHING_STRATEGY_FACTORY_H
