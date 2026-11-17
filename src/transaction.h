#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "common.h"

void         initList(TransactionList *list);
void         freeList(TransactionList *list);
int          addTransaction(TransactionList *list, const char *desc, const char *cat,
                            double amount, time_t date, int isRecurring);
int          deleteTransaction(TransactionList *list, int id);
Transaction *findTransaction(TransactionList *list, int id);
void         setNextId(int n);

#endif /* TRANSACTION_H */
