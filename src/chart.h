#ifndef CHART_H
#define CHART_H

#define WIN32_LEAN_AND_MEAN
#undef  UNICODE
#undef  _UNICODE
#include <windows.h>
#include "common.h"

void drawBarChart(HDC hdc, RECT rect, double *values, const char **labels,
                  int count, const char *title);
void drawBudgetBars(HDC hdc, RECT rect, const TransactionList *list,
                    const BudgetList *budgets);

#endif /* CHART_H */
