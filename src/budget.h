#ifndef BUDGET_H
#define BUDGET_H

#include "common.h"

int    setBudget(BudgetList *budgets, const char *category, double limit);
double getBudgetLimit(const BudgetList *budgets, const char *category);
int    isOverBudget(const BudgetList *budgets, const TransactionList *txns,
                    const char *category);

#endif /* BUDGET_H */
