#include "chart.h"
#include "gui.h"
#include "insights.h"
#include "budget.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void drawBarChart(HDC hdc, RECT rect, double *values, const char **labels,
                  int count, const char *title) {
    if (count <= 0) return;

    SetBkMode(hdc, TRANSPARENT);

    /* Title */
    HFONT hOld = (HFONT)SelectObject(hdc, g_app.hFontTitle);
    SetTextColor(hdc, CLR_TEXT_DARK);
    TextOut(hdc, rect.left + 14, rect.top + 10, title, (int)strlen(title));

    int topPad  = 44, botPad  = 30;
    int leftPad = 16, rightPad = 16;
    int cLeft   = rect.left  + leftPad;
    int cRight  = rect.right - rightPad;
    int cTop    = rect.top   + topPad;
    int cBottom = rect.bottom - botPad;
    int cH      = cBottom - cTop;
    int cW      = cRight  - cLeft;
    if (cH <= 10 || cW <= 10) {
        SelectObject(hdc, hOld);
        return;
    }

    double maxVal = 0.0;
    for (int i = 0; i < count; i++)
        if (values[i] > maxVal) maxVal = values[i];
    if (maxVal <= 0.0) maxVal = 1000.0;

    /* Background guideline */
    HPEN hGridPen = CreatePen(PS_DOT, 1, RGB(226, 232, 240));
    SelectObject(hdc, hGridPen);
    int midY = cTop + cH / 2;
    MoveToEx(hdc, cLeft, midY, NULL);
    LineTo(hdc, cRight, midY);
    DeleteObject(hGridPen);

    int barW = cW / count;
    int gap  = barW / 4;
    if (gap < 4) gap = 4;

    time_t now = time(NULL);
    struct tm *tmNow = localtime(&now);
    int curMonth = tmNow ? tmNow->tm_mon : -1;

    SelectObject(hdc, g_app.hFontSmall);

    for (int i = 0; i < count; i++) {
        int barH = (int)((values[i] / maxVal) * cH);
        if (barH > cH) barH = cH;
        int x = cLeft + i * barW;
        int bx1 = x + gap;
        int bx2 = x + barW - gap;
        int by1 = cBottom - barH;
        int by2 = cBottom;

        int isCurrent = (count == 12 && i == curMonth);
        COLORREF barColor;
        if (isCurrent) {
            barColor = RGB(67, 56, 202); /* Deep Indigo for current month */
        } else if (i % 2 == 0) {
            barColor = CLR_CHART1;
        } else {
            barColor = CLR_CHART2;
        }

        if (barH > 0 && bx2 > bx1) {
            HBRUSH hBr = CreateSolidBrush(barColor);
            RECT barRect = {bx1, by1, bx2, by2};
            FillRect(hdc, &barRect, hBr);
            DeleteObject(hBr);

            /* Top accent highlight */
            HBRUSH hTopBr = CreateSolidBrush(RGB(255, 255, 255));
            RECT topHighlight = {bx1, by1, bx2, by1 + 2};
            FillRect(hdc, &topHighlight, hTopBr);
            DeleteObject(hTopBr);

            /* Amount above bar */
            char amtBuf[24];
            if (values[i] >= 1000.0)
                snprintf(amtBuf, sizeof(amtBuf), "%.1fk", values[i] / 1000.0);
            else
                snprintf(amtBuf, sizeof(amtBuf), "%.0f", values[i]);

            SetTextColor(hdc, isCurrent ? CLR_TEXT_DARK : CLR_TEXT_MUTED);
            int textW = (int)strlen(amtBuf) * 6;
            int tx = x + (barW - textW) / 2;
            TextOut(hdc, tx, by1 - 16, amtBuf, (int)strlen(amtBuf));
        }

        /* Label below chart */
        if (labels && labels[i]) {
            SetTextColor(hdc, isCurrent ? CLR_ACCENT : CLR_TEXT_MUTED);
            int lblW = (int)strlen(labels[i]) * 6;
            int tx = x + (barW - lblW) / 2;
            TextOut(hdc, tx, cBottom + 6, labels[i], (int)strlen(labels[i]));
        }
    }

    /* Baseline */
    HPEN hPen = CreatePen(PS_SOLID, 1, CLR_BORDER);
    SelectObject(hdc, hPen);
    MoveToEx(hdc, cLeft, cBottom, NULL);
    LineTo(hdc, cRight, cBottom);
    DeleteObject(hPen);

    SelectObject(hdc, hOld);
}

void drawBudgetBars(HDC hdc, RECT rect, const TransactionList *list,
                    const BudgetList *budgets) {
    int x  = rect.left  + 16;
    int y  = rect.top   + 12;
    int w  = rect.right - rect.left - 32;
    int rH = 64;

    SetBkMode(hdc, TRANSPARENT);
    HFONT hOld = (HFONT)SelectObject(hdc, g_app.hFontBold);

    for (int i = 0; i < budgets->count && y + rH < rect.bottom - 10; i++) {
        const char *cat   = budgets->items[i].category;
        double      limit = budgets->items[i].limit;
        double      spent = spentThisMonthByCategory(list, cat);
        double      pct   = (limit > 0.0) ? (spent / limit) : 0.0;
        double      diff  = limit - spent;
        int         isOver = (diff < 0.0);

        COLORREF col = (pct >= 1.0) ? CLR_RED : (pct >= 0.75) ? CLR_ORANGE : CLR_GREEN;
        COLORREF fillBg = (pct >= 1.0) ? CLR_RED_LIGHT : (pct >= 0.75) ? CLR_ORANGE_LGT : CLR_GREEN_LIGHT;

        /* Row card container */
        drawRoundRect(hdc, x, y, w, rH - 4, 8, CLR_CARD, CLR_BORDER);

        /* Category Name */
        SelectObject(hdc, g_app.hFontBold);
        SetTextColor(hdc, CLR_TEXT_DARK);
        TextOut(hdc, x + 16, y + 10, cat, (int)strlen(cat));

        /* Status badge text (Remaining / Exceeded) */
        char statusBuf[64];
        if (isOver)
            snprintf(statusBuf, sizeof(statusBuf), "⚠️ Exceeded by Tk %.2f", -diff);
        else
            snprintf(statusBuf, sizeof(statusBuf), "✓ Remaining: Tk %.2f", diff);

        SelectObject(hdc, g_app.hFontSmall);
        SetTextColor(hdc, col);
        TextOut(hdc, x + 16, y + 34, statusBuf, (int)strlen(statusBuf));

        /* Amount spent vs limit */
        char amtBuf[48];
        snprintf(amtBuf, sizeof(amtBuf), "Tk %.2f / Tk %.2f", spent, limit);
        int amtLen = (int)strlen(amtBuf);
        int amtX = x + w - 180;
        SelectObject(hdc, g_app.hFontBold);
        SetTextColor(hdc, col);
        TextOut(hdc, amtX, y + 10, amtBuf, amtLen);

        /* Progress bar track */
        int barX = x + 200, barY = y + 36;
        int barW = amtX - barX - 20;
        int barH = 10;
        if (barW > 20) {
            /* Background track */
            HBRUSH bgBr = CreateSolidBrush(fillBg);
            RECT bgR    = {barX, barY, barX + barW, barY + barH};
            FillRect(hdc, &bgR, bgBr);
            DeleteObject(bgBr);

            /* Fill */
            double displayPct = (pct > 1.0) ? 1.0 : pct;
            int filled = (int)(displayPct * barW);
            if (filled > 0) {
                HBRUSH fillBr = CreateSolidBrush(col);
                RECT fillR    = {barX, barY, barX + filled, barY + barH};
                FillRect(hdc, &fillR, fillBr);
                DeleteObject(fillBr);
            }

            /* Percentage pill */
            char pctBuf[16];
            snprintf(pctBuf, sizeof(pctBuf), "%.0f%%", pct * 100.0);
            SelectObject(hdc, g_app.hFontBold);
            SetTextColor(hdc, col);
            TextOut(hdc, barX + barW + 8, barY - 2, pctBuf, (int)strlen(pctBuf));
        }

        y += rH;
    }
    SelectObject(hdc, hOld);
}
