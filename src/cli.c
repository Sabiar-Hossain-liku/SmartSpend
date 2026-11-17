#include "cli.h"
#include "transaction.h"
#include "category.h"
#include "insights.h"
#include "budget.h"
#include "storage.h"
#include "export.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void readLine(char *buf, int maxLen) {
    if (fgets(buf, maxLen, stdin) == NULL) {
        buf[0] = '\0';
        return;
    }
    int len = (int)strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
}

static void showTransactionsCli(const TransactionList *list) {
    printf("\n--- All Transactions (%d entries) ---\n", list->count);
    if (list->count == 0) {
        printf("No transactions recorded yet.\n");
        return;
    }

    printf("%-5s %-12s %-28s %-16s %-14s %-10s\n",
           "ID", "Date", "Description", "Category", "Amount", "Recurring");
    printf("------------------------------------------------------------------------------------\n");

    for (int i = 0; i < list->count; i++) {
        const Transaction *t = &list->items[i];
        char dBuf[16];
        formatDate(t->date, dBuf, sizeof(dBuf));
        printf("%-5d %-12s %-28s %-16s Tk %-11.2f %-10s\n",
               t->id, dBuf, t->description, t->category,
               t->amount, t->isRecurring ? "Yes (Auto)" : "No");
    }
}

static void addTransactionCli(TransactionList *list, char categories[][MAX_CAT_LEN], int *catCount) {
    char desc[MAX_DESC_LEN];
    char amtStr[32];
    char catStr[MAX_CAT_LEN];
    char recurStr[16];

    printf("\n--- Add New Transaction ---\n");
    printf("Enter description: ");
    readLine(desc, sizeof(desc));
    if (desc[0] == '\0') {
        printf("Description cannot be empty.\n");
        return;
    }

    printf("Available categories:\n  ");
    for (int i = 0; i < *catCount; i++) {
        printf("%s%s", categories[i], (i < *catCount - 1) ? ", " : "\n");
    }
    printf("Enter category (or type a new one): ");
    readLine(catStr, sizeof(catStr));
    if (catStr[0] == '\0') strncpy(catStr, "Other", sizeof(catStr) - 1);

    printf("Enter amount (Tk): ");
    readLine(amtStr, sizeof(amtStr));
    double amount = atof(amtStr);
    if (amount <= 0.0) {
        printf("Invalid amount. Must be positive.\n");
        return;
    }

    printf("Is this a recurring monthly bill? (y/n): ");
    readLine(recurStr, sizeof(recurStr));
    int isRecurring = (recurStr[0] == 'y' || recurStr[0] == 'Y') ? 1 : 0;

    time_t now = time(NULL);
    if (addTransaction(list, desc, catStr, amount, now, isRecurring)) {
        addCategory(categories, catCount, catStr);
        saveTransactions(list);
        printf("✓ Transaction added successfully! (ID: %d)\n", list->items[list->count - 1].id);
    } else {
        printf("Failed to add transaction (memory error).\n");
    }
}

static void deleteTransactionCli(TransactionList *list) {
    char idStr[32];
    printf("\n--- Delete Transaction ---\n");
    printf("Enter transaction ID to delete: ");
    readLine(idStr, sizeof(idStr));
    int id = atoi(idStr);

    Transaction *t = findTransaction(list, id);
    if (t == NULL) {
        printf("Transaction with ID %d not found.\n", id);
        return;
    }

    printf("Deleting '%s' (Tk %.2f). Confirm? (y/n): ", t->description, t->amount);
    char conf[16];
    readLine(conf, sizeof(conf));
    if (conf[0] == 'y' || conf[0] == 'Y') {
        deleteTransaction(list, id);
        saveTransactions(list);
        printf("✓ Transaction %d deleted.\n", id);
    } else {
        printf("Deletion canceled.\n");
    }
}

static void showBudgetStatusCli(const TransactionList *txns, const BudgetList *budgets) {
    printf("\n--- Monthly Budget Status ---\n");
    if (budgets->count == 0) {
        printf("No monthly budgets set. Choose option 5 to set category limits.\n");
        return;
    }

    printf("%-18s %-16s %-16s %-12s %s\n",
           "Category", "Spent This Month", "Monthly Limit", "Usage", "Status");
    printf("--------------------------------------------------------------------------\n");

    for (int i = 0; i < budgets->count; i++) {
        const char *cat   = budgets->items[i].category;
        double      limit = budgets->items[i].limit;
        double      spent = spentThisMonthByCategory(txns, cat);
        double      pct   = (limit > 0.0) ? (spent / limit * 100.0) : 0.0;
        double      diff  = limit - spent;

        char bar[16];
        int filled = (int)(pct / 10.0);
        if (filled > 10) filled = 10;
        for (int b = 0; b < 10; b++) {
            bar[b] = (b < filled) ? '=' : '-';
        }
        bar[10] = '\0';

        char status[32];
        if (diff < 0.0)
            snprintf(status, sizeof(status), "OVER by Tk %.0f!", -diff);
        else
            snprintf(status, sizeof(status), "Tk %.0f left", diff);

        printf("%-18s Tk %-13.2f Tk %-13.2f [%s] %3.0f%%  %s\n",
               cat, spent, limit, bar, pct, status);
    }
}

static void setBudgetCli(BudgetList *budgets, char categories[][MAX_CAT_LEN], int count) {
    char catStr[MAX_CAT_LEN];
    char limStr[32];

    printf("\n--- Set Category Monthly Budget ---\n");
    printf("Available categories:\n  ");
    for (int i = 0; i < count; i++) {
        printf("%s%s", categories[i], (i < count - 1) ? ", " : "\n");
    }
    printf("Enter category name: ");
    readLine(catStr, sizeof(catStr));

    printf("Enter monthly spending limit (Tk): ");
    readLine(limStr, sizeof(limStr));
    double limit = atof(limStr);
    if (limit <= 0.0) {
        printf("Invalid limit. Must be greater than 0.\n");
        return;
    }

    setBudget(budgets, catStr, limit);
    saveBudgets(budgets);
    printf("✓ Budget limit of Tk %.2f set for '%s'.\n", limit, catStr);
}

static void showInsightsCli(const TransactionList *list) {
    printf("\n--- Financial Insights & Analytics ---\n");
    printf("Total Lifetime Spending:    Tk %.2f\n", totalSpent(list));
    printf("Spent This Calendar Month:  Tk %.2f\n", spentThisMonth(list));
    printf("Top Spending Category:      %s\n", topCategory(list));
    printf("Total Recorded Entries:     %d\n", list->count);

    time_t now = time(NULL);
    struct tm *tmNow = localtime(&now);
    int curYear = tmNow ? (tmNow->tm_year + 1900) : 2026;
    double totals[12];
    monthlyTotals(list, totals, curYear);

    static const char *mNames[12] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };

    printf("\nMonthly Breakdown (%d):\n", curYear);
    for (int m = 0; m < 12; m++) {
        if (totals[m] > 0.0) {
            printf("  %-4s: Tk %10.2f\n", mNames[m], totals[m]);
        }
    }
}

static void showForecastCli(const TransactionList *list) {
    double nextMonth = forecastNextMonth(list);
    printf("\n--- Expenditure Forecasting (Linear Regression) ---\n");
    printf("Based on your past 6-month spending slope:\n");
    printf("  • Projected Spending Next Month:  Tk %.2f\n", nextMonth);
    printf("  • 3-Month Estimated Outflow:      Tk %.2f\n", nextMonth * 3.0);
    printf("  • Projected 12-Month Run-Rate:    Tk %.2f\n", nextMonth * 12.0);
}

static void runWhatIfSimulatorCli(const TransactionList *list, char categories[][MAX_CAT_LEN], int count) {
    char catStr[MAX_CAT_LEN];
    char cutStr[32];

    printf("\n--- Interactive 'What-If' Savings Simulator ---\n");
    printf("Test lifestyle changes to project your annual cash savings.\n");
    printf("Available categories:\n  ");
    for (int i = 0; i < count; i++) {
        printf("%s%s", categories[i], (i < count - 1) ? ", " : "\n");
    }
    printf("Select category to trim: ");
    readLine(catStr, sizeof(catStr));

    printf("Enter cut percentage (e.g. 10, 20, 30): ");
    readLine(cutStr, sizeof(cutStr));
    double cut = atof(cutStr);
    if (cut <= 0.0) cut = 20.0;

    double annualSavings = whatIfSavings(list, catStr, cut);
    double monthlySavings = annualSavings / 12.0;

    printf("\n=======================================================\n");
    printf(" SCENARIO: Cut '%s' spending by %.0f%%\n", catStr, cut);
    printf(" • Monthly Cash Saved:  + Tk %.2f / month\n", monthlySavings);
    printf(" • Annual Cash Retained: + Tk %.2f / year\n", annualSavings);
    printf("=======================================================\n");
}

static void exportDataCli(const TransactionList *txns, const BudgetList *budgets) {
    system("if not exist exports mkdir exports 2>nul");
    int csvOk = exportCSV(txns, "exports/transactions.csv");
    int rptOk = exportReport(txns, budgets, "exports/report.txt");

    printf("\n--- Exporting Financial Data ---\n");
    if (csvOk) printf("✓ Transactions exported to exports/transactions.csv\n");
    else       printf("✗ Failed to write exports/transactions.csv\n");

    if (rptOk) printf("✓ Audit report generated at exports/report.txt\n");
    else       printf("✗ Failed to write exports/report.txt\n");
}

void runCliMenu(TransactionList *list, BudgetList *budgets,
                char categories[][MAX_CAT_LEN], int *catCount) {
    char choiceStr[32];
    int running = 1;

    while (running) {
        printf("\n=====================================================\n");
        printf("         SmartSpend — Terminal Expense Tracker       \n");
        printf("=====================================================\n");
        printf("  [1] View All Transactions (Table View)\n");
        printf("  [2] Add New Transaction\n");
        printf("  [3] Delete Transaction (by ID)\n");
        printf("  [4] Budget Tracker & Limit Alerts\n");
        printf("  [5] Set Monthly Category Budget\n");
        printf("  [6] Financial Insights & Analytics\n");
        printf("  [7] Linear Regression Forecast (Next Month)\n");
        printf("  [8] 'What-If' Lifestyle Savings Simulator\n");
        printf("  [9] Export Data (CSV & Audit Report)\n");
        printf("  [0] Save & Exit\n");
        printf("=====================================================\n");
        printf("Enter your choice [0-9]: ");
        readLine(choiceStr, sizeof(choiceStr));

        int choice = (choiceStr[0] >= '0' && choiceStr[0] <= '9') ? (choiceStr[0] - '0') : -1;

        switch (choice) {
        case 1: showTransactionsCli(list); break;
        case 2: addTransactionCli(list, categories, catCount); break;
        case 3: deleteTransactionCli(list); break;
        case 4: showBudgetStatusCli(list, budgets); break;
        case 5: setBudgetCli(budgets, categories, *catCount); break;
        case 6: showInsightsCli(list); break;
        case 7: showForecastCli(list); break;
        case 8: runWhatIfSimulatorCli(list, categories, *catCount); break;
        case 9: exportDataCli(list, budgets); break;
        case 0:
            saveTransactions(list);
            saveBudgets(budgets);
            printf("\nData saved. Goodbye!\n");
            running = 0;
            break;
        default:
            printf("Invalid selection. Please choose a number from 0 to 9.\n");
            break;
        }
    }
}
