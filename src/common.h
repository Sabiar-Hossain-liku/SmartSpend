#ifndef COMMON_H
#define COMMON_H

#include <time.h>

#define MAX_DESC_LEN   128
#define MAX_CAT_LEN    32
#define MAX_CATEGORIES 20
#define INITIAL_CAP    64
#define DATA_FILE      "data/transactions.dat"
#define BUDGET_FILE    "data/budgets.dat"

typedef struct {
    int    id;
    char   description[MAX_DESC_LEN];
    char   category[MAX_CAT_LEN];
    double amount;
    time_t date;
    int    isRecurring;
} Transaction;

typedef struct {
    char   category[MAX_CAT_LEN];
    double limit;
} Budget;

typedef struct {
    Transaction *items;
    int          count;
    int          capacity;
} TransactionList;

typedef struct {
    Budget items[MAX_CATEGORIES];
    int    count;
} BudgetList;

/* Date utilities shared across modules */
void formatDate(time_t t, char *buf, int bufLen);
int  sameMonth(time_t a, time_t b);
int  inMonth(time_t t, int month, int year);

#endif /* COMMON_H */
