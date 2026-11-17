#include "chart.h"
#include "gui.h"
#include "insights.h"
#include "budget.h"
#include <stdio.h>
#include <string.h>

void drawBarChart(HDC hdc, RECT rect, double *values, const char **labels,
                  int count, const char *title) {
    if (count <= 0) return;

    SetBkMode(hdc, TRANSPARENT);

    /* Title */
    HFONT hBold = makeFont(11, 1);
    HFONT hOld  = (HFONT)SelectObject(hdc, hBold);
    SetTextColor(hdc, CLR_TEXT_DARK);
    TextOut(hdc, rect.left + 10, rect.top + 8, title, (int)strlen(title));
    SelectObject(hdc, hOld);
    DeleteObject(hBold);

    int topPad  = 34, botPad  = 32;
    int leftPad = 10, rightPad = 10;
    int cLeft   = rect.left  + leftPad;
    int cRight  = rect.right - rightPad;
    int cTop    = rect.top   + topPad;
    int cBottom = rect.bottom - botPad;
    int cH      = cBottom - cTop;
    int cW      = cRight  - cLeft;
    if (cH <= 0 || cW <= 0 || count <= 0) return;

    double maxVal = 0.0;
    for (int i = 0; i < count; i++)
        if (values[i] > maxVal) maxVal = values[i];
    if (maxVal == 0.0) maxVal = 1.0;

    int barW = cW / count;
    int gap  = barW / 5;
    if (gap < 2) gap = 2;

    COLORREF barColors[2] = {CLR_CHART1, CLR_CHART2};
    HFONT hSmall = makeFont(9, 0);

    for (int i = 0; i < count; i++) {
        int barH = (int)((values[i] / maxVal) * cH);
        int x    = cLeft + i * barW;
        RECT bar = {x + gap, cBottom - barH, x + barW - gap, cBottom};

        HBRUSH hBr = CreateSolidBrush(barColors[i % 2]);
        SelectObject(hdc, hBr);
        SelectObject(hdc, GetStockObject(NULL_PEN));
        Rectangle(hdc, bar.left, bar.top, bar.right, bar.bottom);
        DeleteObject(hBr);

        /* Amount above bar */
        if (values[i] > 0.0) {
            char amtBuf[20];
            snprintf(amtBuf, sizeof(amtBuf), "$%.0f", values[i]);
            hOld = (HFONT)SelectObject(hdc, hSmall);
            SetTextColor(hdc, CLR_TEXT_DARK);
            int tx = x + barW / 2 - 14;
            TextOut(hdc, tx, bar.top - 15, amtBuf, (int)strlen(amtBuf));
            SelectObject(hdc, hOld);
        }

        /* Label below chart */
        if (labels && labels[i]) {
            hOld = (HFONT)SelectObject(hdc, hSmall);
            SetTextColor(hdc, CLR_TEXT_MUTED);
            int tx = x + barW / 2 - 10;
            TextOut(hdc, tx, cBottom + 4, labels[i], (int)strlen(labels[i]));
            SelectObject(hdc, hOld);
        }
    }
    DeleteObject(hSmall);

    /* Baseline */
    HPEN hPen = CreatePen(PS_SOLID, 1, CLR_BORDER);
    SelectObject(hdc, hPen);
    MoveToEx(hdc, cLeft, cBottom, NULL);
    LineTo(hdc, cRight, cBottom);
    DeleteObject(hPen);
}

void drawBudgetBars(HDC hdc, RECT rect, const TransactionList *list,
                    const BudgetList *budgets) {
    int x  = rect.left  + 12;
    int y  = rect.top   + 10;
    int w  = rect.right - rect.left - 24;
    int rH = 50;

    HFONT hFont = makeFont(11, 0);
    HFONT hBold = makeFont(11, 1);
    SetBkMode(hdc, TRANSPARENT);

    for (int i = 0; i < budgets->count && y + rH < rect.bottom - 10; i++) {
        const char *cat   = budgets->items[i].category;
        double      limit = budgets->items[i].limit;
        double      spent = spentThisMonthByCategory(list, cat);
        double      pct   = (limit > 0.0) ? (spent / limit) : 0.0;
        if (pct > 1.0) pct = 1.0;

        COLORREF col = (pct >= 1.0) ? CLR_RED : (pct >= 0.75) ? CLR_ORANGE : CLR_GREEN;

        /* Row card */
        drawRoundRect(hdc, x, y, w, rH - 4, 6, CLR_CARD, CLR_BORDER);

        /* Category name */
        HFONT hOld = (HFONT)SelectObject(hdc, hBold);
        SetTextColor(hdc, CLR_TEXT_DARK);
        TextOut(hdc, x + 12, y + 8, cat, (int)strlen(cat));
        SelectObject(hdc, hOld);

        /* Amount text */
        char amtBuf[48];
        snprintf(amtBuf, sizeof(amtBuf), "$%.0f / $%.0f", spent, limit);
        int amtX = x + w - 110;
        hOld = (HFONT)SelectObject(hdc, hFont);
        SetTextColor(hdc, col);
        TextOut(hdc, amtX, y + 8, amtBuf, (int)strlen(amtBuf));
        SelectObject(hdc, hOld);

        /* Progress bar track */
        int barX = x + 160, barY = y + 10;
        int barW = amtX - barX - 16, barH2 = 10;
        if (barW > 10) {
            HBRUSH bgBr = CreateSolidBrush(CLR_BORDER);
            RECT bgR    = {barX, barY, barX + barW, barY + barH2};
            FillRect(hdc, &bgR, bgBr);
            DeleteObject(bgBr);

            int filled = (int)(pct * barW);
            if (filled > 0) {
                HBRUSH fillBr = CreateSolidBrush(col);
                RECT fillR    = {barX, barY, barX + filled, barY + barH2};
                FillRect(hdc, &fillR, fillBr);
                DeleteObject(fillBr);
            }

            /* Percentage */
            char pctBuf[12];
            snprintf(pctBuf, sizeof(pctBuf), "%.0f%%", pct * 100.0);
            hOld = (HFONT)SelectObject(hdc, hFont);
            SetTextColor(hdc, col);
            TextOut(hdc, barX + barW + 4, barY, pctBuf, (int)strlen(pctBuf));
            SelectObject(hdc, hOld);
        }

        y += rH;
    }
    DeleteObject(hFont);
    DeleteObject(hBold);
}
