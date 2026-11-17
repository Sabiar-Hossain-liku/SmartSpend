#include "insights.h"
#include "transaction.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>

double totalSpent(const TransactionList *list) {
    double total = 0.0;
    for (int i = 0; i < list->count; i++)
        total += list->items[i].amount;
    return total;
}

double spentThisMonth(const TransactionList *list) {
    time_t now = time(NULL);
    double total = 0.0;
    for (int i = 0; i < list->count; i++) {
        if (sameMonth(list->items[i].date, now))
            total += list->items[i].amount;
    }
    return total;
}

double spentByCategory(const TransactionList *list, const char *category) {
    double total = 0.0;
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].category, category) == 0)
            total += list->items[i].amount;
    }
    return total;
}

double spentThisMonthByCategory(const TransactionList *list, const char *category) {
    time_t now = time(NULL);
    double total = 0.0;
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].category, category) == 0 &&
            sameMonth(list->items[i].date, now))
            total += list->items[i].amount;
    }
    return total;
}

void monthlyTotals(const TransactionList *list, double totals[12], int year) {
    for (int m = 0; m < 12; m++) totals[m] = 0.0;
    int tmYear = year - 1900;
    for (int i = 0; i < list->count; i++) {
        struct tm *t = localtime(&list->items[i].date);
        if (t->tm_year == tmYear)
            totals[t->tm_mon] += list->items[i].amount;
    }
}

const char *topCategory(const TransactionList *list) {
    static char catNames[MAX_CATEGORIES][MAX_CAT_LEN];
    static double catTotals[MAX_CATEGORIES];
    int catCount = 0;

    for (int i = 0; i < MAX_CATEGORIES; i++) {
        catNames[i][0] = '\0';
        catTotals[i]   = 0.0;
    }

    for (int i = 0; i < list->count; i++) {
        const char *cat = list->items[i].category;
        int found = -1;
        for (int j = 0; j < catCount; j++) {
            if (strcmp(catNames[j], cat) == 0) { found = j; break; }
        }
        if (found < 0 && catCount < MAX_CATEGORIES) {
            strncpy(catNames[catCount], cat, MAX_CAT_LEN - 1);
            catNames[catCount][MAX_CAT_LEN - 1] = '\0';
            found = catCount++;
        }
        if (found >= 0) catTotals[found] += list->items[i].amount;
    }

    if (catCount == 0) return "None";

    int best = 0;
    for (int j = 1; j < catCount; j++) {
        if (catTotals[j] > catTotals[best]) best = j;
    }
    return catNames[best];
}

/* Linear regression over the last 6 months to forecast next month. */
double forecastNextMonth(const TransactionList *list) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int curYear  = t->tm_year;
    int curMonth = t->tm_mon;

    double monthly[6] = {0.0};
    for (int k = 0; k < 6; k++) {
        int m = curMonth - 5 + k;
        int y = curYear;
        while (m < 0) { m += 12; y--; }
        for (int i = 0; i < list->count; i++) {
            if (inMonth(list->items[i].date, m, y))
                monthly[k] += list->items[i].amount;
        }
    }

    /* Simple linear regression y = a*x + b over x in {0..5} */
    double sumX = 15.0, sumY = 0.0, sumXX = 55.0, sumXY = 0.0;
    for (int i = 0; i < 6; i++) {
        sumY  += monthly[i];
        sumXY += i * monthly[i];
    }
    double denom = 6.0 * sumXX - sumX * sumX;
    if (denom == 0.0) return sumY / 6.0;
    double a = (6.0 * sumXY - sumX * sumY) / denom;
    double b = (sumY - a * sumX) / 6.0;
    double forecast = a * 6.0 + b;
    return forecast > 0.0 ? forecast : 0.0;
}

/* Annual savings if spending on category is cut by cutPercent percent. */
double whatIfSavings(const TransactionList *list, const char *category,
                     double cutPercent) {
    if (list->count == 0) return 0.0;

    double total = spentByCategory(list, category);

    /* Estimate monthly average over the data span */
    time_t minDate = list->items[0].date;
    time_t maxDate = list->items[0].date;
    for (int i = 1; i < list->count; i++) {
        if (list->items[i].date < minDate) minDate = list->items[i].date;
        if (list->items[i].date > maxDate) maxDate = list->items[i].date;
    }
    double spanMonths = (double)(maxDate - minDate) / (30.44 * 24.0 * 3600.0);
    if (spanMonths < 1.0) spanMonths = 1.0;

    double monthlyAvg = total / spanMonths;
    return monthlyAvg * (cutPercent / 100.0) * 12.0;
}

/* Add one copy of each recurring transaction for the current month if not already there. */
void applyRecurring(TransactionList *list) {
    time_t now    = time(NULL);
    int snapshot  = list->count;

    for (int i = 0; i < snapshot; i++) {
        if (!list->items[i].isRecurring) continue;
        if (sameMonth(list->items[i].date, now)) continue;

        /* Check whether we already added it this month */
        int found = 0;
        for (int j = 0; j < list->count; j++) {
            if (strcmp(list->items[j].description, list->items[i].description) == 0 &&
                strcmp(list->items[j].category,    list->items[i].category)    == 0 &&
                sameMonth(list->items[j].date, now)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            addTransaction(list,
                           list->items[i].description,
                           list->items[i].category,
                           list->items[i].amount,
                           now, 0);
        }
    }
}
