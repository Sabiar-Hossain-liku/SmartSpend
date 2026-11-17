#include "transaction.h"
#include <stdlib.h>
#include <string.h>

static int nextId = 1;

void setNextId(int n) {
    nextId = n;
}

void initList(TransactionList *list) {
    list->items    = malloc(INITIAL_CAP * sizeof(Transaction));
    list->count    = 0;
    list->capacity = (list->items != NULL) ? INITIAL_CAP : 0;
}

void freeList(TransactionList *list) {
    free(list->items);
    list->items    = NULL;
    list->count    = 0;
    list->capacity = 0;
}

int addTransaction(TransactionList *list, const char *desc, const char *cat,
                   double amount, time_t date, int isRecurring) {
    if (list->count >= list->capacity) {
        int newCap = (list->capacity > 0) ? list->capacity * 2 : INITIAL_CAP;
        Transaction *tmp = realloc(list->items, newCap * sizeof(Transaction));
        if (tmp == NULL) return 0;
        list->items    = tmp;
        list->capacity = newCap;
    }

    Transaction *t = &list->items[list->count];
    t->id          = nextId++;
    t->amount      = amount;
    t->date        = date;
    t->isRecurring = isRecurring;

    strncpy(t->description, desc ? desc : "", MAX_DESC_LEN - 1);
    t->description[MAX_DESC_LEN - 1] = '\0';

    strncpy(t->category, cat ? cat : "Other", MAX_CAT_LEN - 1);
    t->category[MAX_CAT_LEN - 1] = '\0';

    list->count++;
    return 1;
}

int deleteTransaction(TransactionList *list, int id) {
    for (int i = 0; i < list->count; i++) {
        if (list->items[i].id == id) {
            for (int j = i; j < list->count - 1; j++)
                list->items[j] = list->items[j + 1];
            list->count--;
            return 1;
        }
    }
    return 0;
}

Transaction *findTransaction(TransactionList *list, int id) {
    for (int i = 0; i < list->count; i++) {
        if (list->items[i].id == id)
            return &list->items[i];
    }
    return NULL;
}
