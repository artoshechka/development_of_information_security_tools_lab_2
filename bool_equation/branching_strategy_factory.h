#ifndef BRANCHING_STRATEGY_FACTORY_H
#define BRANCHING_STRATEGY_FACTORY_H

#include <memory>
#include <string>

#include "branching_strategy.h"

///@brief Фабрика создания стратегий ветвления с кэшированием синглтонов.
class BranchingStrategyFactory {
public:
  ///@brief Возвращает синглтон стратегии ветвления по имени.
  ///@param name Имя стратегии (например: "min-dont-care" или "first-free").
  ///@return Умный указатель на синглтон стратегии.
  ///@throws std::invalid_argument Если передано неподдерживаемое имя.
  static std::shared_ptr<BranchingStrategy>
  GetStrategy(const std::string &name);
};

#endif // BRANCHING_STRATEGY_FACTORY_H
