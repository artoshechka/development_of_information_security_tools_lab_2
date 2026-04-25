#include "first_free_column_branching_strategy.h"

#include "boolequation.h"

IMPLEMENT_ALLOCATOR(FirstFreeColumnBranchingStrategy, 1, 0)

int FirstFreeColumnBranchingStrategy::ChooseColumn(
    BoolEquation &equation) const {
  for (int i = 0; i < equation.mask.getSize(); i++) {
    if (equation.mask[i] == 0) {
      return i;
    }
  }

  return -1;
}
