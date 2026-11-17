#include "budget.h"
#include "insights.h"
#include <string.h>

int setBudget(BudgetList *budgets, const char *category, double limit) {
    for (int i = 0; i < budgets->count; i++) {
        if (strcmp(budgets->items[i].category, category) == 0) {
            budgets->items[i].limit = limit;
            return 1;
        }
    }
    if (budgets->count >= MAX_CATEGORIES) return 0;
    strncpy(budgets->items[budgets->count].category, category, MAX_CAT_LEN - 1);
    budgets->items[budgets->count].category[MAX_CAT_LEN - 1] = '\0';
    budgets->items[budgets->count].limit = limit;
    budgets->count++;
    return 1;
}

double getBudgetLimit(const BudgetList *budgets, const char *category) {
    for (int i = 0; i < budgets->count; i++) {
        if (strcmp(budgets->items[i].category, category) == 0)
            return budgets->items[i].limit;
    }
    return 0.0;
}

int isOverBudget(const BudgetList *budgets, const TransactionList *txns,
                 const char *category) {
    double limit = getBudgetLimit(budgets, category);
    if (limit <= 0.0) return 0;
    return spentThisMonthByCategory(txns, category) > limit;
}
