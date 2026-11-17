#include "export.h"
#include "insights.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

int exportCSV(const TransactionList *list, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (f == NULL) return 0;

    fprintf(f, "ID,Date,Amount,Category,Description,Recurring\n");
    for (int i = 0; i < list->count; i++) {
        const Transaction *t = &list->items[i];
        char dateBuf[16];
        formatDate(t->date, dateBuf, sizeof(dateBuf));
        fprintf(f, "%d,%s,%.2f,%s,%s,%s\n",
                t->id, dateBuf, t->amount,
                t->category, t->description,
                t->isRecurring ? "Yes" : "No");
    }
    fclose(f);
    return 1;
}

int exportReport(const TransactionList *list, const BudgetList *budgets,
                 const char *filename) {
    FILE *f = fopen(filename, "w");
    if (f == NULL) return 0;

    time_t now = time(NULL);
    char   dateBuf[32];
    strftime(dateBuf, sizeof(dateBuf), "%d/%m/%Y %H:%M", localtime(&now));

    fprintf(f, "SmartSpend Financial Report\n");
    fprintf(f, "Generated: %s\n", dateBuf);
    fprintf(f, "============================================\n\n");
    fprintf(f, "Total Spent (All Time):  $%.2f\n",   totalSpent(list));
    fprintf(f, "Spent This Month:        $%.2f\n",   spentThisMonth(list));
    fprintf(f, "Top Category:            %s\n",       topCategory(list));
    fprintf(f, "Forecasted Next Month:   $%.2f\n\n", forecastNextMonth(list));

    if (budgets->count > 0) {
        fprintf(f, "Budget Overview (This Month)\n");
        fprintf(f, "----------------------------\n");
        for (int i = 0; i < budgets->count; i++) {
            const char *cat   = budgets->items[i].category;
            double      limit = budgets->items[i].limit;
            double      spent = spentThisMonthByCategory(list, cat);
            double      pct   = (limit > 0.0) ? (spent / limit * 100.0) : 0.0;
            fprintf(f, "%-18s $%8.2f / $%8.2f  (%5.1f%%)\n",
                    cat, spent, limit, pct);
        }
        fprintf(f, "\n");
    }

    fprintf(f, "All Transactions\n");
    fprintf(f, "----------------\n");
    for (int i = 0; i < list->count; i++) {
        const Transaction *t = &list->items[i];
        char dBuf[16];
        formatDate(t->date, dBuf, sizeof(dBuf));
        fprintf(f, "%s  %-22s  %-14s  $%8.2f%s\n",
                dBuf, t->description, t->category, t->amount,
                t->isRecurring ? "  [Recurring]" : "");
    }
    fclose(f);
    return 1;
}
