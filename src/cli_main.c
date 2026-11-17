/*
 * cli_main.c — Standalone console entry point for SmartSpend.
 *
 * Runs 100% in the terminal across Windows, Linux, and macOS.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "transaction.h"
#include "category.h"
#include "insights.h"
#include "budget.h"
#include "storage.h"
#include "export.h"
#include "cli.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    TransactionList txns;
    BudgetList      budgets;
    char            categories[MAX_CATEGORIES][MAX_CAT_LEN];
    int             catCount = 0;

    initList(&txns);
    budgets.count = 0;

    loadTransactions(&txns);
    loadBudgets(&budgets);
    initCategories(categories, &catCount);
    collectCategories(&txns, categories, &catCount);

    /* Process automated recurring bills for month rollover */
    applyRecurring(&txns);
    saveTransactions(&txns);

    /* Launch interactive terminal menu */
    runCliMenu(&txns, &budgets, categories, &catCount);

    freeList(&txns);
    return 0;
}
