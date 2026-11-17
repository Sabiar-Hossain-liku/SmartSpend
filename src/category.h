#ifndef CATEGORY_H
#define CATEGORY_H

#include "common.h"

void initCategories(char categories[][MAX_CAT_LEN], int *count);
int  addCategory(char categories[][MAX_CAT_LEN], int *count, const char *name);
int  findCategory(const char categories[][MAX_CAT_LEN], int count, const char *name);
void collectCategories(const TransactionList *list,
                       char categories[][MAX_CAT_LEN], int *count);

#endif /* CATEGORY_H */
