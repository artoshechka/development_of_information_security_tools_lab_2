#ifndef BOOLEQUATION_H
#define BOOLEQUATION_H

#include "boolinterval.h"
#include "Allocator.h"
#include <cstddef>

class BranchingStrategy; // Forward declaration of BranchingStrategy

class BoolEquation
{
public:
	BoolInterval **cnf;//множество интервалов
	BoolInterval *root;//Корень уравнения
	int cnfSize; // Размер КНФ
	int count; //количество дизъюнкций
	BBV mask; //маска для столбцов
	BoolEquation(BoolInterval **cnf, BoolInterval *root, int cnfSize, int count, BBV mask);
	BoolEquation(BoolEquation &equation);
	~BoolEquation();
	int CheckRules();
	bool Rule1Row1(BoolInterval *interval);
	bool Rule2RowNull(BoolInterval *interval);
	void Rule3ColNull(BBV vector);
	bool Rule4Col0(BBV vector);
	bool Rule5Col1(BBV vector);
	void Simplify(int ixCol, char value);
	int ChooseColForBranching();
	int ChooseColForBranching(const BranchingStrategy &strategy);
	static void *operator new(std::size_t size);
	static void operator delete(void *pointer);

private:
	static Allocator allocator_;
};

#endif // BOOLEQUATION_H
