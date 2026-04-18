#include "branching_strategy_factory.h"

#include <stdexcept>

#include "first_free_column_branching_strategy.h"
#include "min_dont_care_branching_strategy.h"

std::unique_ptr<BranchingStrategy> BranchingStrategyFactory::CreateByName(const std::string &name)
{
	if (name == "min-dont-care" || name == "min") {
		return std::unique_ptr<BranchingStrategy>(new MinDontCareBranchingStrategy());
	}

	if (name == "first-free" || name == "first") {
		return std::unique_ptr<BranchingStrategy>(new FirstFreeColumnBranchingStrategy());
	}

	throw std::invalid_argument(
		"Unknown branching strategy. Available values: min-dont-care, first-free.");
}
