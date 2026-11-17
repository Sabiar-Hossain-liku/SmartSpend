#ifndef INSIGHTS_H
#define INSIGHTS_H

#include "common.h"

double      totalSpent(const TransactionList *list);
double      spentThisMonth(const TransactionList *list);
double      spentByCategory(const TransactionList *list, const char *category);
double      spentThisMonthByCategory(const TransactionList *list, const char *category);
void        monthlyTotals(const TransactionList *list, double totals[12], int year);
const char *topCategory(const TransactionList *list);
double      forecastNextMonth(const TransactionList *list);
double      whatIfSavings(const TransactionList *list, const char *category,
                          double cutPercent);
void        applyRecurring(TransactionList *list);

#endif /* INSIGHTS_H */
