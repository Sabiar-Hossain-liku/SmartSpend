#include "storage.h"
#include "transaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ensureDataDir(void) {
    /* Create data/ silently if it does not exist */
    system("if not exist data mkdir data 2>nul");
}

int loadTransactions(TransactionList *list) {
    FILE *f = fopen(DATA_FILE, "r");
    if (f == NULL) return 0;

    char line[640];
    int maxId = 0;

    while (fgets(line, sizeof(line), f)) {
        int    id, isRecurring;
        double amount;
        long long dateLL;
        char desc[MAX_DESC_LEN] = {0};
        char cat[MAX_CAT_LEN]   = {0};

        /* Format: id,dateLL,amount,isRecurring,category,description */
        if (sscanf(line, "%d,%lld,%lf,%d,", &id, &dateLL, &amount, &isRecurring) != 4)
            continue;

        /* Find the 4th and 5th comma to split category and description */
        int commas = 0, catStart = -1, descStart = -1;
        for (int i = 0; line[i] != '\0'; i++) {
            if (line[i] == ',') {
                commas++;
                if (commas == 4) catStart = i + 1;
                if (commas == 5) { descStart = i + 1; break; }
            }
        }
        if (catStart < 0) continue;

        if (descStart >= 0) {
            int catLen = descStart - catStart - 1;
            if (catLen >= MAX_CAT_LEN) catLen = MAX_CAT_LEN - 1;
            strncpy(cat, line + catStart, catLen);
            cat[catLen] = '\0';

            char *dp  = line + descStart;
            int   dLen = (int)strlen(dp);
            while (dLen > 0 && (dp[dLen-1] == '\n' || dp[dLen-1] == '\r')) dLen--;
            if (dLen >= MAX_DESC_LEN) dLen = MAX_DESC_LEN - 1;
            strncpy(desc, dp, dLen);
            desc[dLen] = '\0';
        } else {
            char *cp  = line + catStart;
            int   cLen = (int)strlen(cp);
            while (cLen > 0 && (cp[cLen-1] == '\n' || cp[cLen-1] == '\r')) cLen--;
            if (cLen >= MAX_CAT_LEN) cLen = MAX_CAT_LEN - 1;
            strncpy(cat, cp, cLen);
            cat[cLen] = '\0';
        }

        addTransaction(list, desc, cat, amount, (time_t)dateLL, isRecurring);
        list->items[list->count - 1].id = id;
        if (id > maxId) maxId = id;
    }
    fclose(f);
    setNextId(maxId + 1);
    return 1;
}

int saveTransactions(const TransactionList *list) {
    ensureDataDir();
    FILE *f = fopen(DATA_FILE, "w");
    if (f == NULL) return 0;

    for (int i = 0; i < list->count; i++) {
        const Transaction *t = &list->items[i];
        fprintf(f, "%d,%lld,%.2f,%d,%s,%s\n",
                t->id, (long long)t->date, t->amount, t->isRecurring,
                t->category, t->description);
    }
    fclose(f);
    return 1;
}

int loadBudgets(BudgetList *list) {
    list->count = 0;
    FILE *f = fopen(BUDGET_FILE, "r");
    if (f == NULL) return 0;

    char line[256];
    while (fgets(line, sizeof(line), f) && list->count < MAX_CATEGORIES) {
        char   cat[MAX_CAT_LEN];
        double lim;
        if (sscanf(line, "%31[^,],%lf", cat, &lim) == 2) {
            strncpy(list->items[list->count].category, cat, MAX_CAT_LEN - 1);
            list->items[list->count].category[MAX_CAT_LEN - 1] = '\0';
            list->items[list->count].limit = lim;
            list->count++;
        }
    }
    fclose(f);
    return 1;
}

int saveBudgets(const BudgetList *list) {
    ensureDataDir();
    FILE *f = fopen(BUDGET_FILE, "w");
    if (f == NULL) return 0;

    for (int i = 0; i < list->count; i++)
        fprintf(f, "%s,%.2f\n", list->items[i].category, list->items[i].limit);

    fclose(f);
    return 1;
}
