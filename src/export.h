#ifndef EXPORT_H
#define EXPORT_H

#include "common.h"

int exportCSV(const TransactionList *list, const char *filename);
int exportReport(const TransactionList *list, const BudgetList *budgets,
                 const char *filename);

#endif /* EXPORT_H */
