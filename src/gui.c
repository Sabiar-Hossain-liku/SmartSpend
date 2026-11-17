#include "gui.h"
#include "common.h"
#include "insights.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AppState g_app;

HFONT makeFont(int size, int bold) {
    return CreateFont(
        -size, 0, 0, 0,
        bold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        "Segoe UI"
    );
}

void initFonts(void) {
    g_app.hFontSmall   = makeFont(11, 0);
    g_app.hFontRegular = makeFont(13, 0);
    g_app.hFontBold    = makeFont(13, 1);
    g_app.hFontTitle   = makeFont(16, 1);
    g_app.hFontHeading = makeFont(20, 1);
    g_app.hFontStat    = makeFont(24, 1);
}

void cleanupFonts(void) {
    if (g_app.hFontSmall)   DeleteObject(g_app.hFontSmall);
    if (g_app.hFontRegular) DeleteObject(g_app.hFontRegular);
    if (g_app.hFontBold)    DeleteObject(g_app.hFontBold);
    if (g_app.hFontTitle)   DeleteObject(g_app.hFontTitle);
    if (g_app.hFontHeading) DeleteObject(g_app.hFontHeading);
    if (g_app.hFontStat)    DeleteObject(g_app.hFontStat);
}

void drawRoundRect(HDC hdc, int x, int y, int w, int h, int r,
                   COLORREF fill, COLORREF border) {
    HBRUSH hBrush = CreateSolidBrush(fill);
    HPEN   hPen;
    if (border == fill)
        hPen = (HPEN)GetStockObject(NULL_PEN);
    else
        hPen = CreatePen(PS_SOLID, 1, border);

    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    HPEN   oldPen   = (HPEN)SelectObject(hdc, hPen);
    RoundRect(hdc, x, y, x + w, y + h, r * 2, r * 2);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(hBrush);
    if (border != fill) DeleteObject(hPen);
}

void drawCard(HDC hdc, int x, int y, int w, int h,
              const char *title, const char *value, const char *sub,
              COLORREF accent) {
    /* Shadow */
    drawRoundRect(hdc, x + 1, y + 2, w, h, CARD_R, RGB(235, 240, 246), RGB(235, 240, 246));
    /* Card body */
    drawRoundRect(hdc, x, y, w, h, CARD_R, CLR_CARD, CLR_BORDER);

    /* Left accent vertical pill */
    HBRUSH hAcc = CreateSolidBrush(accent);
    RECT   ar   = {x + 1, y + 10, x + 5, y + h - 10};
    FillRect(hdc, &ar, hAcc);
    DeleteObject(hAcc);

    SetBkMode(hdc, TRANSPARENT);

    /* Title */
    HFONT hOld = (HFONT)SelectObject(hdc, g_app.hFontSmall);
    SetTextColor(hdc, CLR_TEXT_MUTED);
    TextOut(hdc, x + 16, y + 12, title, (int)strlen(title));

    /* Value */
    SelectObject(hdc, g_app.hFontStat);
    SetTextColor(hdc, accent);
    TextOut(hdc, x + 16, y + 28, value, (int)strlen(value));

    /* Subtitle */
    if (sub && sub[0]) {
        SelectObject(hdc, g_app.hFontSmall);
        SetTextColor(hdc, CLR_TEXT_MUTED);
        TextOut(hdc, x + 16, y + h - 20, sub, (int)strlen(sub));
    }
    SelectObject(hdc, hOld);
}

void setStatus(const char *msg) {
    if (g_app.hStatus)
        SetWindowText(g_app.hStatus, msg);
}

void refreshList(void) {
    HWND hList = g_app.hList;
    if (!hList) return;
    ListView_DeleteAllItems(hList);

    for (int i = 0; i < g_app.txns.count; i++) {
        const Transaction *t = &g_app.txns.items[i];
        char dateBuf[16], amtBuf[32];
        formatDate(t->date, dateBuf, sizeof(dateBuf));
        snprintf(amtBuf, sizeof(amtBuf), "Tk %.2f", t->amount);

        LVITEM lvi   = {0};
        lvi.mask     = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem    = i;
        lvi.lParam   = (LPARAM)t->id;
        lvi.pszText  = dateBuf;
        int row = ListView_InsertItem(hList, &lvi);

        ListView_SetItemText(hList, row, 1, (LPSTR)t->description);
        ListView_SetItemText(hList, row, 2, (LPSTR)t->category);
        ListView_SetItemText(hList, row, 3, amtBuf);
        ListView_SetItemText(hList, row, 4, (LPSTR)(t->isRecurring ? "Yes (Auto)" : "No"));
    }
}

void runForecastCalculation(void) {
    if (!g_app.hFcCatCombo || !g_app.hFcCutEdit) return;

    int sel = (int)SendMessage(g_app.hFcCatCombo, CB_GETCURSEL, 0, 0);
    if (sel >= 0 && sel < g_app.catCount) {
        strncpy(g_app.fcSelectedCat, g_app.categories[sel], MAX_CAT_LEN - 1);
        g_app.fcSelectedCat[MAX_CAT_LEN - 1] = '\0';
    } else {
        strncpy(g_app.fcSelectedCat, "Food", MAX_CAT_LEN - 1);
        g_app.fcSelectedCat[MAX_CAT_LEN - 1] = '\0';
    }

    char cutBuf[32];
    GetWindowText(g_app.hFcCutEdit, cutBuf, sizeof(cutBuf));
    double cut = atof(cutBuf);
    if (cut <= 0.0) cut = 20.0;
    if (cut > 100.0) cut = 100.0;
    g_app.fcCutPercent = cut;

    double annual = whatIfSavings(&g_app.txns, g_app.fcSelectedCat, cut);
    g_app.fcAnnualSavings  = annual;
    g_app.fcMonthlySavings = annual / 12.0;
    g_app.fcCalculated     = 1;

    if (g_app.hContent)
        InvalidateRect(g_app.hContent, NULL, TRUE);
}

void updatePageLayout(void) {
    int pg = g_app.currentPage;

    /* Show/hide the ListView */
    ShowWindow(g_app.hList, (pg == PAGE_TRANSACTIONS) ? SW_SHOW : SW_HIDE);

    /* Show/hide per-page buttons */
    ShowWindow(g_app.hBtnAdd,       (pg == PAGE_TRANSACTIONS) ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hBtnDel,       (pg == PAGE_TRANSACTIONS) ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hBtnSetBudget, (pg == PAGE_BUDGET)       ? SW_SHOW : SW_HIDE);

    /* Forecast & Simulator controls */
    int isForecast = (pg == PAGE_FORECAST);
    ShowWindow(g_app.hFcCatCombo, isForecast ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hFcCutEdit,  isForecast ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hFcCalcBtn,  isForecast ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hFcBtn10,    isForecast ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hFcBtn20,    isForecast ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hFcBtn30,    isForecast ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hFcBtn50,    isForecast ? SW_SHOW : SW_HIDE);

    /* Export buttons */
    ShowWindow(g_app.hBtnExportCSV, (pg == PAGE_EXPORT) ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hBtnExportRpt, (pg == PAGE_EXPORT) ? SW_SHOW : SW_HIDE);

    if (isForecast && !g_app.fcCalculated) {
        runForecastCalculation();
    }

    InvalidateRect(g_app.hContent, NULL, TRUE);
}
