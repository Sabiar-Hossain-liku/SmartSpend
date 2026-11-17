#ifndef STORAGE_H
#define STORAGE_H

#include "common.h"

int loadTransactions(TransactionList *list);
int saveTransactions(const TransactionList *list);
int loadBudgets(BudgetList *list);
int saveBudgets(const BudgetList *list);

#endif /* STORAGE_H */
