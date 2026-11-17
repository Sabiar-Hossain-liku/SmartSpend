#ifndef CLI_H
#define CLI_H

#include "common.h"

/* Runs the interactive terminal menu loop for SmartSpend */
void runCliMenu(TransactionList *list, BudgetList *budgets,
                char categories[][MAX_CAT_LEN], int *catCount);

#endif /* CLI_H */
