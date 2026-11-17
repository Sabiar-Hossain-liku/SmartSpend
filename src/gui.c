#include "gui.h"
#include "common.h"
#include <stdio.h>
#include <string.h>

AppState g_app;

HFONT makeFont(int size, int bold) {
    return CreateFont(
        -size, 0, 0, 0,
        bold ? FW_SEMIBOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        "Segoe UI"
    );
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
    drawRoundRect(hdc, x + 2, y + 2, w, h, CARD_R, RGB(220, 225, 235), RGB(220, 225, 235));
    /* Card body */
    drawRoundRect(hdc, x, y, w, h, CARD_R, CLR_CARD, CLR_BORDER);
    /* Accent strip */
    HBRUSH hAcc = CreateSolidBrush(accent);
    RECT   ar   = {x, y, x + 5, y + h};
    FillRect(hdc, &ar, hAcc);
    DeleteObject(hAcc);

    SetBkMode(hdc, TRANSPARENT);

    /* Title */
    HFONT hT = makeFont(10, 0), hOld = (HFONT)SelectObject(hdc, hT);
    SetTextColor(hdc, CLR_TEXT_MUTED);
    TextOut(hdc, x + 14, y + 10, title, (int)strlen(title));
    SelectObject(hdc, hOld);
    DeleteObject(hT);

    /* Value */
    HFONT hV = makeFont(18, 1);
    hOld = (HFONT)SelectObject(hdc, hV);
    SetTextColor(hdc, accent);
    TextOut(hdc, x + 14, y + 26, value, (int)strlen(value));
    SelectObject(hdc, hOld);
    DeleteObject(hV);

    /* Subtitle */
    if (sub && sub[0]) {
        HFONT hS = makeFont(9, 0);
        hOld = (HFONT)SelectObject(hdc, hS);
        SetTextColor(hdc, CLR_TEXT_MUTED);
        TextOut(hdc, x + 14, y + h - 18, sub, (int)strlen(sub));
        SelectObject(hdc, hOld);
        DeleteObject(hS);
    }
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
        char dateBuf[16], amtBuf[24];
        formatDate(t->date, dateBuf, sizeof(dateBuf));
        snprintf(amtBuf, sizeof(amtBuf), "$%.2f", t->amount);

        LVITEM lvi   = {0};
        lvi.mask     = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem    = i;
        lvi.lParam   = (LPARAM)t->id;
        lvi.pszText  = dateBuf;
        int row = ListView_InsertItem(hList, &lvi);

        ListView_SetItemText(hList, row, 1, (LPSTR)t->description);
        ListView_SetItemText(hList, row, 2, (LPSTR)t->category);
        ListView_SetItemText(hList, row, 3, amtBuf);
        ListView_SetItemText(hList, row, 4, (LPSTR)(t->isRecurring ? "Yes" : "No"));
    }
}

void updatePageLayout(void) {
    int pg = g_app.currentPage;

    /* Show/hide the ListView */
    ShowWindow(g_app.hList, (pg == PAGE_TRANSACTIONS) ? SW_SHOW : SW_HIDE);

    /* Show/hide per-page buttons */
    ShowWindow(g_app.hBtnAdd,       (pg == PAGE_TRANSACTIONS) ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hBtnDel,       (pg == PAGE_TRANSACTIONS) ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hBtnSetBudget, (pg == PAGE_BUDGET)       ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hBtnSimulate,  (pg == PAGE_WHATIF)       ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hBtnExportCSV, (pg == PAGE_EXPORT)       ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.hBtnExportRpt, (pg == PAGE_EXPORT)       ? SW_SHOW : SW_HIDE);

    InvalidateRect(g_app.hContent, NULL, TRUE);
}
