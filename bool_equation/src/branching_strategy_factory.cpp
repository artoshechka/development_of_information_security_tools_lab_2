#include "branching_strategy_factory.h"

#include <stdexcept>

#include "first_free_column_branching_strategy.h"
#include "min_dont_care_branching_strategy.h"

std::shared_ptr<BranchingStrategy>
BranchingStrategyFactory::GetStrategy(const std::string &name) {
  if (name == "min-dont-care" || name == "min") {
    static std::shared_ptr<BranchingStrategy> instance =
        std::shared_ptr<BranchingStrategy>(new MinDontCareBranchingStrategy());
    return instance;
  }

  if (name == "first-free" || name == "first") {
    static std::shared_ptr<BranchingStrategy> instance =
        std::shared_ptr<BranchingStrategy>(
            new FirstFreeColumnBranchingStrategy());
    return instance;
  }

  throw std::invalid_argument("Unknown branching strategy. Available values: "
                              "min-dont-care, first-free.");
}
