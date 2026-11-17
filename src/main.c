/*
 * main.c — SmartSpend Win32 GUI Application
 *
 * Professional native desktop expense tracker in pure C.
 * Features:
 *  - Dashboard with real-time stats and 12-month GDI bar chart
 *  - Transactions registry with sorting and ListView
 *  - Budget Enforcer with color-coded threshold progress bars
 *  - Analytics & Insights breakdown
 *  - Predictive Forecasting & Interactive What-If Savings Simulator
 *  - Automated Recurring Bills engine
 *  - CSV and plaintext financial audit report export
 */

#define WIN32_LEAN_AND_MEAN
#undef  UNICODE
#undef  _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "gui.h"
#include "storage.h"
#include "transaction.h"
#include "category.h"
#include "insights.h"
#include "budget.h"
#include "chart.h"
#include "export.h"
#include "cli.h"

#define CLS_MAIN    "SmartSpendMain"
#define CLS_CONTENT "SmartSpendContent"
#define CLS_DIALOG  "SmartSpendDialog"

/* ── Forward declarations ─────────────────────────────── */
static LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK ContentWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK AddTransWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK SetBudgetWndProc(HWND, UINT, WPARAM, LPARAM);

/* ── Dialog control IDs ───────────────────────────────── */
#define DLG_DESC      1001
#define DLG_AMOUNT    1002
#define DLG_CATEGORY  1003
#define DLG_DATE      1004
#define DLG_RECURRING 1005
#define DLG_OK        1006
#define DLG_CANCEL    1007
#define DLG_CATCOMBO  1008

typedef struct {
    char   description[MAX_DESC_LEN];
    char   category[MAX_CAT_LEN];
    double amount;
    time_t date;
    int    isRecurring;
    int    confirmed;
} AddTransData;

typedef struct {
    char   category[MAX_CAT_LEN];
    double limit;
    int    confirmed;
} SetBudgetData;

static AddTransData   g_addData;
static SetBudgetData  g_budgetData;

/* ── Sample data ──────────────────────────────────────── */
static void addSampleData(TransactionList *list) {
    time_t now    = time(NULL);
    time_t oneDay = 86400;

    addTransaction(list, "Grocery shopping",    "Food",          2500.0, now - oneDay * 2,  0);
    addTransaction(list, "Monthly rent",        "Housing",      12000.0, now - oneDay * 5,  1);
    addTransaction(list, "Bus & Metro pass",    "Transport",      800.0, now - oneDay * 3,  1);
    addTransaction(list, "High-speed Internet", "Utilities",      700.0, now - oneDay * 1,  1);
    addTransaction(list, "Movie tickets",       "Entertainment",  350.0, now - oneDay * 4,  0);
    addTransaction(list, "Doctor prescription", "Healthcare",    1200.0, now - oneDay * 10, 0);
    addTransaction(list, "Textbook purchase",   "Education",     1800.0, now - oneDay * 15, 0);
    addTransaction(list, "Restaurant dinner",   "Food",           650.0, now - oneDay * 6,  0);
    addTransaction(list, "Gym membership",      "Personal",       600.0, now - oneDay * 8,  1);
    addTransaction(list, "Online tech shopping","Shopping",      3200.0, now - oneDay * 12, 0);
    addTransaction(list, "Electricity bill",    "Utilities",      950.0, now - oneDay * 20, 1);
    addTransaction(list, "Coffee & breakfast",  "Food",           320.0, now - oneDay * 7,  0);
}

/* ── Modal dialog helper ──────────────────────────────── */
static void showModal(HWND parent, WNDPROC proc, const char *title, int w, int h) {
    RECT pr;
    GetWindowRect(parent, &pr);
    int cx = pr.left + (pr.right  - pr.left - w) / 2;
    int cy = pr.top  + (pr.bottom - pr.top  - h) / 2;

    HWND hDlg = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        CLS_DIALOG, title,
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        cx, cy, w, h,
        parent, NULL, GetModuleHandle(NULL), NULL
    );
    SetWindowLongPtr(hDlg, GWLP_WNDPROC, (LONG_PTR)proc);
    SendMessage(hDlg, WM_CREATE, 0, 0);

    EnableWindow(parent, FALSE);

    MSG msg;
    while (IsWindow(hDlg) && GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);
}

/* ── Add Transaction dialog ───────────────────────────── */
static LRESULT CALLBACK AddTransWndProc(HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam) {
    static HWND hDesc, hAmt, hCat, hDate, hRecur;
    (void)lParam;

    switch (msg) {
    case WM_CREATE: {
        HINSTANCE hInst = GetModuleHandle(NULL);
        int y = 20, lw = 130, ex = 150, ew = 210;

        HWND hL1 = CreateWindowEx(0,"STATIC","Description:",
                       WS_CHILD|WS_VISIBLE,18,y,lw,20,hwnd,NULL,hInst,NULL);
        hDesc = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",
                       WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
                       ex,y,ew,24,hwnd,(HMENU)DLG_DESC,hInst,NULL);
        y += 36;

        HWND hL2 = CreateWindowEx(0,"STATIC","Category:",
                       WS_CHILD|WS_VISIBLE,18,y,lw,20,hwnd,NULL,hInst,NULL);
        hCat = CreateWindowEx(0,"COMBOBOX","",
                       WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
                       ex,y,ew,200,hwnd,(HMENU)DLG_CATEGORY,hInst,NULL);
        for (int i = 0; i < g_app.catCount; i++)
            SendMessage(hCat, CB_ADDSTRING, 0, (LPARAM)g_app.categories[i]);
        SendMessage(hCat, CB_SETCURSEL, 0, 0);
        y += 36;

        HWND hL3 = CreateWindowEx(0,"STATIC","Amount (Tk):",
                       WS_CHILD|WS_VISIBLE,18,y,lw,20,hwnd,NULL,hInst,NULL);
        hAmt = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",
                       WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
                       ex,y,ew,24,hwnd,(HMENU)DLG_AMOUNT,hInst,NULL);
        y += 36;

        HWND hL4 = CreateWindowEx(0,"STATIC","Date (DD/MM/YYYY):",
                       WS_CHILD|WS_VISIBLE,18,y,lw,20,hwnd,NULL,hInst,NULL);
        char todayBuf[16];
        formatDate(time(NULL), todayBuf, sizeof(todayBuf));
        hDate = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT",todayBuf,
                       WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
                       ex,y,ew,24,hwnd,(HMENU)DLG_DATE,hInst,NULL);
        y += 36;

        hRecur = CreateWindowEx(0,"BUTTON","Recurring bill (repeats monthly)",
                       WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
                       ex,y,ew,22,hwnd,(HMENU)DLG_RECURRING,hInst,NULL);
        y += 42;

        HWND hBtnOk = CreateWindowEx(0,"BUTTON","Add Transaction",
                       WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
                       ex,y,130,32,hwnd,(HMENU)DLG_OK,hInst,NULL);
        HWND hBtnCc = CreateWindowEx(0,"BUTTON","Cancel",
                       WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                       ex+140,y,70,32,hwnd,(HMENU)DLG_CANCEL,hInst,NULL);

        SendMessage(hL1, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);
        SendMessage(hL2, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);
        SendMessage(hL3, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);
        SendMessage(hL4, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);
        SendMessage(hDesc, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);
        SendMessage(hCat, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);
        SendMessage(hAmt, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);
        SendMessage(hDate, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);
        SendMessage(hRecur, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);
        SendMessage(hBtnOk, WM_SETFONT, (WPARAM)g_app.hFontBold, TRUE);
        SendMessage(hBtnCc, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);

        SetFocus(hDesc);
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case DLG_OK: {
            char descBuf[MAX_DESC_LEN], amtBuf[32], dateBuf[16];
            GetWindowText(hDesc, descBuf, MAX_DESC_LEN);
            GetWindowText(hAmt,  amtBuf,  sizeof(amtBuf));
            GetWindowText(hDate, dateBuf, sizeof(dateBuf));

            if (descBuf[0] == '\0') {
                MessageBox(hwnd, "Please enter an expense description.", "Missing Information",
                           MB_OK | MB_ICONWARNING);
                return 0;
            }
            double amount = atof(amtBuf);
            if (amount <= 0.0) {
                MessageBox(hwnd, "Please enter a valid positive amount.", "Invalid Amount",
                           MB_OK | MB_ICONWARNING);
                return 0;
            }

            strncpy(g_addData.description, descBuf, MAX_DESC_LEN - 1);
            g_addData.description[MAX_DESC_LEN - 1] = '\0';
            g_addData.amount = amount;

            int sel = (int)SendMessage(hCat, CB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < g_app.catCount)
                strncpy(g_addData.category, g_app.categories[sel], MAX_CAT_LEN - 1);
            else
                strncpy(g_addData.category, "Other", MAX_CAT_LEN - 1);
            g_addData.category[MAX_CAT_LEN - 1] = '\0';

            int d = 1, m = 1, y = 2026;
            if (sscanf(dateBuf, "%d/%d/%d", &d, &m, &y) == 3) {
                struct tm t2 = {0};
                t2.tm_mday = d; t2.tm_mon = m - 1; t2.tm_year = y - 1900;
                g_addData.date = mktime(&t2);
            } else {
                g_addData.date = time(NULL);
            }
            g_addData.isRecurring =
                (SendMessage(hRecur, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
            g_addData.confirmed = 1;
            DestroyWindow(hwnd);
            return 0;
        }
        case DLG_CANCEL:
            g_addData.confirmed = 0;
            DestroyWindow(hwnd);
            return 0;
        }
        return 0;

    case WM_CLOSE:
        g_addData.confirmed = 0;
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* ── Set Budget dialog ────────────────────────────────── */
static LRESULT CALLBACK SetBudgetWndProc(HWND hwnd, UINT msg,
                                         WPARAM wParam, LPARAM lParam) {
    static HWND hCat, hLimit;
    (void)lParam;

    switch (msg) {
    case WM_CREATE: {
        HINSTANCE hInst = GetModuleHandle(NULL);
        int y = 20, lw = 130, ex = 150, ew = 190;

        HWND hL1 = CreateWindowEx(0,"STATIC","Select Category:",
                       WS_CHILD|WS_VISIBLE,18,y,lw,20,hwnd,NULL,hInst,NULL);
        hCat = CreateWindowEx(0,"COMBOBOX","",
                       WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
                       ex,y,ew,200,hwnd,(HMENU)DLG_CATCOMBO,hInst,NULL);
        for (int i = 0; i < g_app.catCount; i++)
            SendMessage(hCat, CB_ADDSTRING, 0, (LPARAM)g_app.categories[i]);
        SendMessage(hCat, CB_SETCURSEL, 0, 0);
        y += 40;

        HWND hL2 = CreateWindowEx(0,"STATIC","Monthly Limit (Tk):",
                       WS_CHILD|WS_VISIBLE,18,y,lw,20,hwnd,NULL,hInst,NULL);
        hLimit = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",
                       WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
                       ex,y,ew,24,hwnd,(HMENU)DLG_AMOUNT,hInst,NULL);
        y += 46;

        HWND hBtnOk = CreateWindowEx(0,"BUTTON","Save Budget",
                       WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
                       ex,y,110,32,hwnd,(HMENU)DLG_OK,hInst,NULL);
        HWND hBtnCc = CreateWindowEx(0,"BUTTON","Cancel",
                       WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                       ex+116,y,70,32,hwnd,(HMENU)DLG_CANCEL,hInst,NULL);

        SendMessage(hL1, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);
        SendMessage(hL2, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);
        SendMessage(hCat, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);
        SendMessage(hLimit, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);
        SendMessage(hBtnOk, WM_SETFONT, (WPARAM)g_app.hFontBold, TRUE);
        SendMessage(hBtnCc, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);

        SetFocus(hLimit);
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case DLG_OK: {
            char limBuf[32];
            GetWindowText(hLimit, limBuf, sizeof(limBuf));
            double limit = atof(limBuf);
            if (limit <= 0.0) {
                MessageBox(hwnd, "Please enter a valid monthly spending limit.", "Invalid Limit",
                           MB_OK | MB_ICONWARNING);
                return 0;
            }
            int sel = (int)SendMessage(hCat, CB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < g_app.catCount)
                strncpy(g_budgetData.category, g_app.categories[sel], MAX_CAT_LEN - 1);
            else
                strncpy(g_budgetData.category, "Other", MAX_CAT_LEN - 1);
            g_budgetData.category[MAX_CAT_LEN - 1] = '\0';
            g_budgetData.limit     = limit;
            g_budgetData.confirmed = 1;
            DestroyWindow(hwnd);
            return 0;
        }
        case DLG_CANCEL:
            g_budgetData.confirmed = 0;
            DestroyWindow(hwnd);
            return 0;
        }
        return 0;

    case WM_CLOSE:
        g_budgetData.confirmed = 0;
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* ── Content Window Procedure ─────────────────────────── */
static LRESULT CALLBACK ContentWndProc(HWND hwnd, UINT msg,
                                       WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1; /* Eliminates flicker — double-buffering handled in WM_PAINT */

    case WM_SIZE: {
        int cw = LOWORD(lParam), ch = HIWORD(lParam);

        /* Top right action buttons */
        SetWindowPos(g_app.hBtnAdd,       NULL, cw - 290, 14, 130, 32, SWP_NOZORDER);
        SetWindowPos(g_app.hBtnDel,       NULL, cw - 150, 14, 130, 32, SWP_NOZORDER);
        SetWindowPos(g_app.hBtnSetBudget, NULL, cw - 160, 14, 140, 32, SWP_NOZORDER);

        /* ListView on Transactions page with clean margins */
        SetWindowPos(g_app.hList, NULL, 20, HEADER_H + 14, cw - 40, ch - HEADER_H - 28, SWP_NOZORDER);

        /* Interactive controls for Forecast page */
        int fcTop = HEADER_H + 215;
        SetWindowPos(g_app.hFcCatCombo, NULL, 40, fcTop + 36, 180, 200, SWP_NOZORDER);
        SetWindowPos(g_app.hFcBtn10,    NULL, 230, fcTop + 34, 46, 28, SWP_NOZORDER);
        SetWindowPos(g_app.hFcBtn20,    NULL, 282, fcTop + 34, 46, 28, SWP_NOZORDER);
        SetWindowPos(g_app.hFcBtn30,    NULL, 334, fcTop + 34, 46, 28, SWP_NOZORDER);
        SetWindowPos(g_app.hFcBtn50,    NULL, 386, fcTop + 34, 46, 28, SWP_NOZORDER);
        SetWindowPos(g_app.hFcCutEdit,  NULL, 442, fcTop + 36, 48, 25, SWP_NOZORDER);
        SetWindowPos(g_app.hFcCalcBtn,  NULL, 510, fcTop + 33, 140, 30, SWP_NOZORDER);

        /* Export page buttons */
        int cardW = (cw - 60) / 2;
        SetWindowPos(g_app.hBtnExportCSV, NULL, 36, HEADER_H + 180, cardW - 32, 38, SWP_NOZORDER);
        SetWindowPos(g_app.hBtnExportRpt, NULL, 36 + cardW + 20, HEADER_H + 180, cardW - 32, 38, SWP_NOZORDER);

        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdcReal = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        /* Create Double-Buffer Memory DC */
        HDC hdc = CreateCompatibleDC(hdcReal);
        HBITMAP hMemBmp = CreateCompatibleBitmap(hdcReal, rc.right, rc.bottom);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(hdc, hMemBmp);

        /* Fill background */
        HBRUSH hBg = CreateSolidBrush(CLR_BG);
        FillRect(hdc, &rc, hBg);
        DeleteObject(hBg);

        int pg = g_app.currentPage;
        static const char *titles[PAGE_COUNT] = {
            "Dashboard", "Transactions Registry", "Budget Enforcer",
            "Analytics & Insights", "Forecast & Savings Simulator", "Exports & Reports"
        };
        static const char *subtitles[PAGE_COUNT] = {
            "Real-time overview of your finances and monthly spending trends",
            "Record, categorize, and organize your expenses",
            "Monitor spending limits and keep your budget in check",
            "Deep breakdown of spending behavior and category distribution",
            "Predict upcoming month expenses and calculate lifestyle savings",
            "Export data to CSV spreadsheets and comprehensive plaintext reports"
        };

        /* Top Header Strip */
        RECT headerRect = {rc.left, rc.top, rc.right, rc.top + HEADER_H};
        HBRUSH hHdrBg = CreateSolidBrush(CLR_WHITE);
        FillRect(hdc, &headerRect, hHdrBg);
        DeleteObject(hHdrBg);

        HPEN hSep = CreatePen(PS_SOLID, 1, CLR_BORDER);
        SelectObject(hdc, hSep);
        MoveToEx(hdc, rc.left, rc.top + HEADER_H, NULL);
        LineTo(hdc, rc.right, rc.top + HEADER_H);
        DeleteObject(hSep);

        SetBkMode(hdc, TRANSPARENT);
        SelectObject(hdc, g_app.hFontHeading);
        SetTextColor(hdc, CLR_TEXT_DARK);
        TextOut(hdc, 24, 10, titles[pg], (int)strlen(titles[pg]));

        SelectObject(hdc, g_app.hFontSmall);
        SetTextColor(hdc, CLR_TEXT_MUTED);
        TextOut(hdc, 25, 36, subtitles[pg], (int)strlen(subtitles[pg]));

        /* Render Active Page Content */
        RECT content = {rc.left + 20, rc.top + HEADER_H + 14, rc.right - 20, rc.bottom - 14};

        switch (pg) {
        case PAGE_DASHBOARD: {
            int cx = content.left;
            int cy = content.top;
            int cardW = (content.right - content.left - 24) / 3;
            int cardH = 92;
            char valBuf[64];

            snprintf(valBuf, sizeof(valBuf), "Tk %.2f", spentThisMonth(&g_app.txns));
            drawCard(hdc, cx, cy, cardW, cardH,
                     "SPENT THIS MONTH", valBuf, "Current calendar month", CLR_ACCENT);

            snprintf(valBuf, sizeof(valBuf), "Tk %.2f", forecastNextMonth(&g_app.txns));
            drawCard(hdc, cx + cardW + 12, cy, cardW, cardH,
                     "NEXT MONTH FORECAST", valBuf, "Predicted via linear regression", CLR_ORANGE);

            drawCard(hdc, cx + 2 * (cardW + 12), cy, cardW, cardH,
                     "TOP SPENDING CATEGORY", topCategory(&g_app.txns), "Highest expenditure source", CLR_GREEN);

            /* Monthly Spending Chart Card */
            int chartY = cy + cardH + 16;
            int chartH = 220;
            drawRoundRect(hdc, cx, chartY, content.right - content.left, chartH,
                          CARD_R, CLR_CARD, CLR_BORDER);

            time_t now2 = time(NULL);
            struct tm *tmNow = localtime(&now2);
            int curYear = tmNow ? (tmNow->tm_year + 1900) : 2026;
            double totals[12];
            monthlyTotals(&g_app.txns, totals, curYear);

            static const char *months[12] = {
                "Jan","Feb","Mar","Apr","May","Jun",
                "Jul","Aug","Sep","Oct","Nov","Dec"
            };
            char chartTitle[64];
            snprintf(chartTitle, sizeof(chartTitle), "Monthly Expenditure Trends (%d)", curYear);
            RECT cInner = {cx + 12, chartY + 6, content.right - 12, chartY + chartH - 8};
            drawBarChart(hdc, cInner, totals, months, 12, chartTitle);

            /* Recent Transactions Card Table */
            int listY = chartY + chartH + 16;
            int listH = content.bottom - listY;
            if (listH > 60) {
                drawRoundRect(hdc, cx, listY, content.right - content.left, listH,
                              CARD_R, CLR_CARD, CLR_BORDER);

                SelectObject(hdc, g_app.hFontTitle);
                SetTextColor(hdc, CLR_TEXT_DARK);
                TextOut(hdc, cx + 16, listY + 12, "Recent Activity Log", 19);

                /* Table Header */
                int thY = listY + 38;
                HBRUSH hThBg = CreateSolidBrush(RGB(248, 250, 252));
                RECT thRect = {cx + 14, thY, content.right - 14, thY + 24};
                FillRect(hdc, &thRect, hThBg);
                DeleteObject(hThBg);

                SelectObject(hdc, g_app.hFontBold);
                SetTextColor(hdc, CLR_TEXT_MUTED);
                TextOut(hdc, cx + 24, thY + 4, "Date", 4);
                TextOut(hdc, cx + 130, thY + 4, "Description", 11);
                TextOut(hdc, cx + 380, thY + 4, "Category", 8);
                TextOut(hdc, cx + 520, thY + 4, "Amount", 6);
                TextOut(hdc, cx + 640, thY + 4, "Type", 4);

                /* Table Rows */
                int rowY = thY + 28;
                SelectObject(hdc, g_app.hFontRegular);
                int shown = 0;
                for (int i = g_app.txns.count - 1; i >= 0 && rowY < listY + listH - 24 && shown < 5; i--, shown++) {
                    const Transaction *tr = &g_app.txns.items[i];
                    char dBuf[16], amtBuf[32];
                    formatDate(tr->date, dBuf, sizeof(dBuf));
                    snprintf(amtBuf, sizeof(amtBuf), "Tk %.2f", tr->amount);

                    /* Row divider */
                    HPEN hRowPen = CreatePen(PS_SOLID, 1, RGB(241, 245, 249));
                    SelectObject(hdc, hRowPen);
                    MoveToEx(hdc, cx + 14, rowY, NULL);
                    LineTo(hdc, content.right - 14, rowY);
                    DeleteObject(hRowPen);

                    SetTextColor(hdc, CLR_TEXT_MUTED);
                    TextOut(hdc, cx + 24, rowY + 4, dBuf, (int)strlen(dBuf));

                    SetTextColor(hdc, CLR_TEXT_DARK);
                    TextOut(hdc, cx + 130, rowY + 4, tr->description, (int)strlen(tr->description));

                    SetTextColor(hdc, CLR_ACCENT);
                    TextOut(hdc, cx + 380, rowY + 4, tr->category, (int)strlen(tr->category));

                    SetTextColor(hdc, CLR_TEXT_DARK);
                    SelectObject(hdc, g_app.hFontBold);
                    TextOut(hdc, cx + 520, rowY + 4, amtBuf, (int)strlen(amtBuf));
                    SelectObject(hdc, g_app.hFontRegular);

                    if (tr->isRecurring) {
                        SetTextColor(hdc, CLR_GREEN);
                        TextOut(hdc, cx + 640, rowY + 4, "Recurring (Auto)", 16);
                    } else {
                        SetTextColor(hdc, CLR_TEXT_MUTED);
                        TextOut(hdc, cx + 640, rowY + 4, "Standard", 8);
                    }
                    rowY += 28;
                }
            }
            break;
        }

        case PAGE_TRANSACTIONS:
            /* Managed via native ListView control hList with modern margins */
            break;

        case PAGE_BUDGET: {
            if (g_app.budgets.count == 0) {
                drawRoundRect(hdc, content.left, content.top, content.right - content.left, 160,
                              CARD_R, CLR_CARD, CLR_BORDER);
                SelectObject(hdc, g_app.hFontTitle);
                SetTextColor(hdc, CLR_TEXT_DARK);
                TextOut(hdc, content.left + 24, content.top + 24, "No Active Budgets Defined", 25);
                SelectObject(hdc, g_app.hFontRegular);
                SetTextColor(hdc, CLR_TEXT_MUTED);
                TextOut(hdc, content.left + 24, content.top + 52,
                        "Setting monthly spending targets helps keep your personal expenses within boundaries.", 86);
                TextOut(hdc, content.left + 24, content.top + 76,
                        "Click the '+ Set Budget' button in the top right to assign your first category limit.", 86);
            } else {
                drawBudgetBars(hdc, content, &g_app.txns, &g_app.budgets);
            }
            break;
        }

        case PAGE_INSIGHTS: {
            int cx = content.left, cy = content.top;
            int nStats = 4;
            int cardW = (content.right - content.left - (nStats - 1) * 12) / nStats;
            int cardH = 90;
            char b1[64], b2[64], b3[64], b4[64];

            snprintf(b1, sizeof(b1), "Tk %.2f", totalSpent(&g_app.txns));
            drawCard(hdc, cx, cy, cardW, cardH, "ALL-TIME SPENT", b1, "Total tracked expenditure", CLR_ACCENT);

            snprintf(b2, sizeof(b2), "Tk %.2f", spentThisMonth(&g_app.txns));
            drawCard(hdc, cx + cardW + 12, cy, cardW, cardH, "THIS MONTH", b2, "Current monthly total", CLR_ORANGE);

            double avgVal = (g_app.txns.count > 0) ? (totalSpent(&g_app.txns) / g_app.txns.count) : 0.0;
            snprintf(b3, sizeof(b3), "Tk %.2f", avgVal);
            drawCard(hdc, cx + 2*(cardW + 12), cy, cardW, cardH, "AVG TRANSACTION", b3, "Mean per-expense amount", CLR_GREEN);

            snprintf(b4, sizeof(b4), "%d entries", g_app.txns.count);
            drawCard(hdc, cx + 3*(cardW + 12), cy, cardW, cardH, "TRANSACTIONS", b4, "Total recorded transactions", RGB(14, 165, 233));

            /* Category Spending Chart */
            int chartY = cy + cardH + 16;
            int chartH = 240;
            drawRoundRect(hdc, cx, chartY, content.right - content.left, chartH,
                          CARD_R, CLR_CARD, CLR_BORDER);

            if (g_app.catCount > 0) {
                double catTotals[MAX_CATEGORIES] = {0.0};
                const char *catLabels[MAX_CATEGORIES];
                for (int i = 0; i < g_app.catCount; i++) {
                    catTotals[i] = spentByCategory(&g_app.txns, g_app.categories[i]);
                    catLabels[i] = g_app.categories[i];
                }
                RECT cInner = {cx + 12, chartY + 6, content.right - 12, chartY + chartH - 8};
                drawBarChart(hdc, cInner, catTotals, catLabels, g_app.catCount, "Expenditure by Category (All Time)");
            }
            break;
        }

        case PAGE_FORECAST: {
            int cx = content.left, cy = content.top;

            /* ── Section 1: Predictive Forecast Cards ── */
            double nextMonthFc = forecastNextMonth(&g_app.txns);
            double qtrOutlook   = nextMonthFc * 3.0;
            double annualRunRate= nextMonthFc * 12.0;

            int nCards = 3;
            int cardW = (content.right - content.left - (nCards - 1) * 12) / nCards;
            int cardH = 92;
            char f1[64], f2[64], f3[64];

            snprintf(f1, sizeof(f1), "Tk %.2f", nextMonthFc);
            drawCard(hdc, cx, cy, cardW, cardH,
                     "PROJECTED NEXT MONTH", f1, "Based on 6-month linear regression", CLR_ORANGE);

            snprintf(f2, sizeof(f2), "Tk %.2f", qtrOutlook);
            drawCard(hdc, cx + cardW + 12, cy, cardW, cardH,
                     "QUARTERLY OUTLOOK (3 MO)", f2, "Estimated 90-day cash outflow", CLR_ACCENT);

            snprintf(f3, sizeof(f3), "Tk %.2f", annualRunRate);
            drawCard(hdc, cx + 2*(cardW + 12), cy, cardW, cardH,
                     "ANNUAL RUN-RATE", f3, "Projected 12-month expenditure", RGB(14, 165, 233));

            /* ── Section 2: Interactive What-If Simulator Card ── */
            int simY = cy + cardH + 16;
            int simH = 150;
            drawRoundRect(hdc, cx, simY, content.right - content.left, simH,
                          CARD_R, CLR_CARD, CLR_BORDER);

            SelectObject(hdc, g_app.hFontTitle);
            SetTextColor(hdc, CLR_TEXT_DARK);
            TextOut(hdc, cx + 20, simY + 12, "Interactive \"What-If\" Lifestyle Savings Simulator", 50);

            SelectObject(hdc, g_app.hFontSmall);
            SetTextColor(hdc, CLR_TEXT_MUTED);
            TextOut(hdc, cx + 20, simY + 34,
                    "Select a spending category and test percentage cuts to see immediate monthly & annual savings:", 95);

            /* Labels for controls */
            SelectObject(hdc, g_app.hFontBold);
            SetTextColor(hdc, CLR_TEXT_DARK);
            TextOut(hdc, cx + 20, simY + 58, "1. Category:", 12);
            TextOut(hdc, cx + 210, simY + 58, "2. Spending Cut (%):", 20);

            /* Savings Outcome Banner on the Right */
            int resX = cx + (content.right - content.left) - 340;
            int resW = 320;
            int resH = 110;
            int resY = simY + 24;

            drawRoundRect(hdc, resX, resY, resW, resH, 8, CLR_GREEN_LIGHT, RGB(187, 247, 208));

            SelectObject(hdc, g_app.hFontSmall);
            SetTextColor(hdc, RGB(22, 101, 52));
            char scenarioHeader[128];
            snprintf(scenarioHeader, sizeof(scenarioHeader), "Scenario: Cut %s by %.0f%%",
                     g_app.fcSelectedCat[0] ? g_app.fcSelectedCat : "Food",
                     g_app.fcCutPercent > 0 ? g_app.fcCutPercent : 20.0);
            TextOut(hdc, resX + 16, resY + 12, scenarioHeader, (int)strlen(scenarioHeader));

            char annBuf[64], moBuf[64];
            snprintf(annBuf, sizeof(annBuf), "+ Tk %.2f / year", g_app.fcAnnualSavings);
            snprintf(moBuf, sizeof(moBuf), "Saves ~ Tk %.2f every month", g_app.fcMonthlySavings);

            SelectObject(hdc, g_app.hFontHeading);
            SetTextColor(hdc, CLR_GREEN);
            TextOut(hdc, resX + 16, resY + 32, annBuf, (int)strlen(annBuf));

            SelectObject(hdc, g_app.hFontBold);
            SetTextColor(hdc, RGB(21, 128, 61));
            TextOut(hdc, resX + 16, resY + 66, moBuf, (int)strlen(moBuf));

            SelectObject(hdc, g_app.hFontSmall);
            SetTextColor(hdc, CLR_TEXT_MUTED);
            TextOut(hdc, resX + 16, resY + 86, "Pure cash retained in your pocket!", 34);

            /* ── Section 3: Fixed Recurring Commitments Table ── */
            int recY = simY + simH + 16;
            int recH = content.bottom - recY;
            if (recH > 60) {
                drawRoundRect(hdc, cx, recY, content.right - content.left, recH,
                              CARD_R, CLR_CARD, CLR_BORDER);

                SelectObject(hdc, g_app.hFontTitle);
                SetTextColor(hdc, CLR_TEXT_DARK);
                TextOut(hdc, cx + 20, recY + 12, "Fixed Monthly Recurring Commitments", 36);

                SelectObject(hdc, g_app.hFontSmall);
                SetTextColor(hdc, CLR_TEXT_MUTED);
                TextOut(hdc, cx + 20, recY + 34,
                        "These expenses repeat automatically every month and represent your baseline budget floor.", 90);

                int rY = recY + 58;
                double recurringTotal = 0.0;
                SelectObject(hdc, g_app.hFontRegular);
                for (int i = 0; i < g_app.txns.count && rY < recY + recH - 24; i++) {
                    if (g_app.txns.items[i].isRecurring) {
                        const Transaction *rt = &g_app.txns.items[i];
                        recurringTotal += rt->amount;

                        char line[256];
                        snprintf(line, sizeof(line), "• %-26s [%-12s]   Tk %.2f / month",
                                 rt->description, rt->category, rt->amount);

                        SetTextColor(hdc, CLR_TEXT_DARK);
                        TextOut(hdc, cx + 24, rY, line, (int)strlen(line));
                        rY += 24;
                    }
                }

                if (recurringTotal > 0.0) {
                    char totBuf[128];
                    snprintf(totBuf, sizeof(totBuf), "Total Fixed Commitments: Tk %.2f / month (Tk %.2f / year)",
                             recurringTotal, recurringTotal * 12.0);
                    SelectObject(hdc, g_app.hFontBold);
                    SetTextColor(hdc, CLR_ACCENT);
                    TextOut(hdc, cx + 24, rY + 4, totBuf, (int)strlen(totBuf));
                }
            }
            break;
        }

        case PAGE_EXPORT: {
            int cx = content.left, cy = content.top + 10;
            int cardW = (content.right - content.left - 24) / 2;
            int cardH = 260;

            /* Left Card: CSV Export */
            drawRoundRect(hdc, cx, cy, cardW, cardH, CARD_R, CLR_CARD, CLR_BORDER);
            SelectObject(hdc, g_app.hFontHeading);
            SetTextColor(hdc, CLR_TEXT_DARK);
            TextOut(hdc, cx + 24, cy + 24, "Export to CSV Spreadsheet", 25);

            SelectObject(hdc, g_app.hFontRegular);
            SetTextColor(hdc, CLR_TEXT_MUTED);
            TextOut(hdc, cx + 24, cy + 58, "Exports all recorded transactions into a clean CSV format", 57);
            TextOut(hdc, cx + 24, cy + 80, "compatible with Microsoft Excel, Google Sheets, or Numbers.", 58);

            SelectObject(hdc, g_app.hFontBold);
            SetTextColor(hdc, CLR_ACCENT);
            TextOut(hdc, cx + 24, cy + 120, "Output Destination: exports/transactions.csv", 44);

            /* Right Card: Full Audit Report */
            int rx = cx + cardW + 24;
            drawRoundRect(hdc, rx, cy, cardW, cardH, CARD_R, CLR_CARD, CLR_BORDER);
            SelectObject(hdc, g_app.hFontHeading);
            SetTextColor(hdc, CLR_TEXT_DARK);
            TextOut(hdc, rx + 24, cy + 24, "Generate Audit Report", 21);

            SelectObject(hdc, g_app.hFontRegular);
            SetTextColor(hdc, CLR_TEXT_MUTED);
            TextOut(hdc, rx + 24, cy + 58, "Generates a comprehensive financial statement including", 55);
            TextOut(hdc, rx + 24, cy + 80, "all-time totals, monthly forecasts, budget limits, and history.", 63);

            SelectObject(hdc, g_app.hFontBold);
            SetTextColor(hdc, CLR_ACCENT);
            TextOut(hdc, rx + 24, cy + 120, "Output Destination: exports/report.txt", 37);
            break;
        }

        default:
            break;
        }

        /* Blit double-buffer to actual screen */
        BitBlt(hdcReal, 0, 0, rc.right, rc.bottom, hdc, 0, 0, SRCCOPY);

        SelectObject(hdc, hOldBmp);
        DeleteObject(hMemBmp);
        DeleteDC(hdc);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_COMMAND:
        /* Forward all commands to main window */
        SendMessage(g_app.hMain, WM_COMMAND, wParam, lParam);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* ── Main Window Procedure ────────────────────────────── */
static void createNavButton(HINSTANCE hInst, int idx, const char *label, int x, int y) {
    g_app.hNavBtns[idx] = CreateWindowEx(
        0, "BUTTON", label,
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        x, y, SIDEBAR_W - 24, NAV_BTN_H,
        g_app.hMain, (HMENU)(UINT_PTR)(IDC_NAV_FIRST + idx),
        hInst, NULL
    );
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam) {
    static HBRUSH hSidebarBrush = NULL;
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

    switch (msg) {

    case WM_CREATE: {
        hSidebarBrush = CreateSolidBrush(CLR_SIDEBAR);
        g_app.hMain   = hwnd;

        initFonts();

        /* Navigation Buttons */
        static const char *navLabels[PAGE_COUNT] = {
            "  Dashboard",
            "  Transactions",
            "  Budget Tracker",
            "  Insights & Trends",
            "  Forecast & Simulator",
            "  Export Data"
        };
        for (int i = 0; i < PAGE_COUNT; i++)
            createNavButton(hInst, i, navLabels[i], 12, HEADER_H + 16 + i * (NAV_BTN_H + 6));

        /* Content Window */
        RECT cr;
        GetClientRect(hwnd, &cr);
        int cw = cr.right - cr.left - SIDEBAR_W;
        int ch = cr.bottom - cr.top - STATUS_H;
        g_app.hContent = CreateWindowEx(
            0, CLS_CONTENT, "",
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
            SIDEBAR_W, 0, cw, ch,
            hwnd, NULL, hInst, NULL
        );

        /* Status Bar */
        g_app.hStatus = CreateWindowEx(
            0, STATUSCLASSNAME, "Ready",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0, hwnd, (HMENU)IDOK, hInst, NULL
        );
        SendMessage(g_app.hStatus, WM_SETFONT, (WPARAM)g_app.hFontSmall, TRUE);

        /* Transaction ListView */
        g_app.hList = CreateWindowEx(
            WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
            WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER,
            20, HEADER_H + 14, cw - 40, ch - HEADER_H - 28,
            g_app.hContent, (HMENU)IDC_LIST_TRANS, hInst, NULL
        );
        ListView_SetExtendedListViewStyle(g_app.hList,
            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        SendMessage(g_app.hList, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);

        HWND hHdr = ListView_GetHeader(g_app.hList);
        if (hHdr) SendMessage(hHdr, WM_SETFONT, (WPARAM)g_app.hFontBold, TRUE);

        /* ListView Columns */
        LVCOLUMN lvc = {0};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        struct { const char *name; int w; } cols[] = {
            {"Date", 110}, {"Description", 260}, {"Category", 140},
            {"Amount (Tk)", 120}, {"Recurring Type", 140}
        };
        for (int i = 0; i < 5; i++) {
            lvc.iSubItem = i;
            lvc.pszText  = (LPSTR)cols[i].name;
            lvc.cx       = cols[i].w;
            ListView_InsertColumn(g_app.hList, i, &lvc);
        }

        /* Top Action Buttons inside Content window */
        g_app.hBtnAdd = CreateWindowEx(0,"BUTTON","+ Add Transaction",
            WS_CHILD|BS_PUSHBUTTON, 0,0,130,32,
            g_app.hContent,(HMENU)IDC_BTN_ADD,hInst,NULL);
        SendMessage(g_app.hBtnAdd, WM_SETFONT, (WPARAM)g_app.hFontBold, TRUE);

        g_app.hBtnDel = CreateWindowEx(0,"BUTTON","Delete Selected",
            WS_CHILD|BS_PUSHBUTTON, 0,0,130,32,
            g_app.hContent,(HMENU)IDC_BTN_DELETE,hInst,NULL);
        SendMessage(g_app.hBtnDel, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);

        g_app.hBtnSetBudget = CreateWindowEx(0,"BUTTON","+ Set Budget",
            WS_CHILD|BS_PUSHBUTTON, 0,0,140,32,
            g_app.hContent,(HMENU)IDC_BTN_SETBUDGET,hInst,NULL);
        SendMessage(g_app.hBtnSetBudget, WM_SETFONT, (WPARAM)g_app.hFontBold, TRUE);

        /* Interactive Forecast & Simulator Controls */
        g_app.hFcCatCombo = CreateWindowEx(0,"COMBOBOX","",
            WS_CHILD|CBS_DROPDOWNLIST|WS_VSCROLL, 0,0,180,200,
            g_app.hContent,(HMENU)IDC_COMBO_FC_CAT,hInst,NULL);
        SendMessage(g_app.hFcCatCombo, WM_SETFONT, (WPARAM)g_app.hFontRegular, TRUE);

        g_app.hFcBtn10 = CreateWindowEx(0,"BUTTON","10%",
            WS_CHILD|BS_PUSHBUTTON, 0,0,46,28,
            g_app.hContent,(HMENU)IDC_BTN_FC_10,hInst,NULL);
        SendMessage(g_app.hFcBtn10, WM_SETFONT, (WPARAM)g_app.hFontSmall, TRUE);

        g_app.hFcBtn20 = CreateWindowEx(0,"BUTTON","20%",
            WS_CHILD|BS_PUSHBUTTON, 0,0,46,28,
            g_app.hContent,(HMENU)IDC_BTN_FC_20,hInst,NULL);
        SendMessage(g_app.hFcBtn20, WM_SETFONT, (WPARAM)g_app.hFontSmall, TRUE);

        g_app.hFcBtn30 = CreateWindowEx(0,"BUTTON","30%",
            WS_CHILD|BS_PUSHBUTTON, 0,0,46,28,
            g_app.hContent,(HMENU)IDC_BTN_FC_30,hInst,NULL);
        SendMessage(g_app.hFcBtn30, WM_SETFONT, (WPARAM)g_app.hFontSmall, TRUE);

        g_app.hFcBtn50 = CreateWindowEx(0,"BUTTON","50%",
            WS_CHILD|BS_PUSHBUTTON, 0,0,46,28,
            g_app.hContent,(HMENU)IDC_BTN_FC_50,hInst,NULL);
        SendMessage(g_app.hFcBtn50, WM_SETFONT, (WPARAM)g_app.hFontSmall, TRUE);

        g_app.hFcCutEdit = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","20",
            WS_CHILD|ES_AUTOHSCROLL|ES_NUMBER, 0,0,48,25,
            g_app.hContent,(HMENU)IDC_EDIT_FC_CUT,hInst,NULL);
        SendMessage(g_app.hFcCutEdit, WM_SETFONT, (WPARAM)g_app.hFontBold, TRUE);

        g_app.hFcCalcBtn = CreateWindowEx(0,"BUTTON","Calculate Savings",
            WS_CHILD|BS_DEFPUSHBUTTON, 0,0,140,30,
            g_app.hContent,(HMENU)IDC_BTN_FC_CALC,hInst,NULL);
        SendMessage(g_app.hFcCalcBtn, WM_SETFONT, (WPARAM)g_app.hFontBold, TRUE);

        /* Export Page Buttons */
        g_app.hBtnExportCSV = CreateWindowEx(0,"BUTTON","📥 Export to CSV Spreadsheet",
            WS_CHILD|BS_PUSHBUTTON, 0,0,240,38,
            g_app.hContent,(HMENU)IDC_BTN_EXPORTCSV,hInst,NULL);
        SendMessage(g_app.hBtnExportCSV, WM_SETFONT, (WPARAM)g_app.hFontBold, TRUE);

        g_app.hBtnExportRpt = CreateWindowEx(0,"BUTTON","📄 Generate Full Audit Report",
            WS_CHILD|BS_PUSHBUTTON, 0,0,240,38,
            g_app.hContent,(HMENU)IDC_BTN_EXPORTRPT,hInst,NULL);
        SendMessage(g_app.hBtnExportRpt, WM_SETFONT, (WPARAM)g_app.hFontBold, TRUE);

        /* Load Data */
        initList(&g_app.txns);
        g_app.budgets.count = 0;
        loadTransactions(&g_app.txns);
        loadBudgets(&g_app.budgets);
        initCategories(g_app.categories, &g_app.catCount);
        collectCategories(&g_app.txns, g_app.categories, &g_app.catCount);
        applyRecurring(&g_app.txns);
        saveTransactions(&g_app.txns);

        /* Populate Category Dropdown */
        for (int i = 0; i < g_app.catCount; i++)
            SendMessage(g_app.hFcCatCombo, CB_ADDSTRING, 0, (LPARAM)g_app.categories[i]);
        SendMessage(g_app.hFcCatCombo, CB_SETCURSEL, 0, 0);

        refreshList();
        runForecastCalculation();

        g_app.currentPage = PAGE_DASHBOARD;
        updatePageLayout();
        return 0;
    }

    case WM_SIZE: {
        int W = LOWORD(lParam), H = HIWORD(lParam);
        int contentW = W - SIDEBAR_W;
        int contentH = H - STATUS_H;
        if (contentW < 1) contentW = 1;
        if (contentH < 1) contentH = 1;

        SetWindowPos(g_app.hContent, NULL,
                     SIDEBAR_W, 0, contentW, contentH, SWP_NOZORDER);
        SendMessage(g_app.hStatus, WM_SIZE, 0, 0);

        for (int i = 0; i < PAGE_COUNT; i++) {
            SetWindowPos(g_app.hNavBtns[i], NULL,
                         12, HEADER_H + 16 + i * (NAV_BTN_H + 6),
                         SIDEBAR_W - 24, NAV_BTN_H, SWP_NOZORDER);
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        /* Sidebar Background */
        RECT sideRect = {0, 0, SIDEBAR_W, rc.bottom};
        FillRect(hdc, &sideRect, hSidebarBrush);

        /* App Branding in Sidebar */
        SetBkMode(hdc, TRANSPARENT);
        SelectObject(hdc, g_app.hFontHeading);
        SetTextColor(hdc, CLR_WHITE);
        TextOut(hdc, 20, 14, "SmartSpend", 10);

        SelectObject(hdc, g_app.hFontSmall);
        SetTextColor(hdc, RGB(148, 163, 184));
        TextOut(hdc, 20, 38, "Personal Finance Tracker", 24);

        /* Sidebar separator line */
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(30, 41, 59));
        SelectObject(hdc, hPen);
        MoveToEx(hdc, 12, HEADER_H, NULL);
        LineTo(hdc, SIDEBAR_W - 12, HEADER_H);
        DeleteObject(hPen);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DRAWITEM: {
        /* Owner-draw sidebar navigation buttons */
        DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
        int idx = (int)(dis->CtlID - IDC_NAV_FIRST);
        if (idx < 0 || idx >= PAGE_COUNT) break;

        int isActive = (idx == g_app.currentPage);
        COLORREF bgColor = isActive ? CLR_ACCENT : CLR_SIDEBAR;

        HBRUSH hBr = CreateSolidBrush(bgColor);
        FillRect(dis->hDC, &dis->rcItem, hBr);
        DeleteObject(hBr);

        /* Accent bar on the left of the active tab */
        if (isActive) {
            HBRUSH hLine = CreateSolidBrush(CLR_WHITE);
            RECT lineR = {dis->rcItem.left, dis->rcItem.top + 4,
                          dis->rcItem.left + 4, dis->rcItem.bottom - 4};
            FillRect(dis->hDC, &lineR, hLine);
            DeleteObject(hLine);
        }

        /* Button Text */
        char btnText[64];
        GetWindowText(dis->hwndItem, btnText, sizeof(btnText));
        SetBkMode(dis->hDC, TRANSPARENT);
        HFONT hOldFont = (HFONT)SelectObject(dis->hDC, isActive ? g_app.hFontBold : g_app.hFontRegular);
        SetTextColor(dis->hDC, isActive ? CLR_WHITE : RGB(148, 163, 184));
        RECT textRect = dis->rcItem;
        textRect.left += 14;
        DrawText(dis->hDC, btnText, -1, &textRect,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dis->hDC, hOldFont);
        return TRUE;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);

        /* Navigation Buttons */
        if (id >= IDC_NAV_FIRST && id < IDC_NAV_FIRST + PAGE_COUNT) {
            g_app.currentPage = id - IDC_NAV_FIRST;
            updatePageLayout();
            RECT sideRect = {0, 0, SIDEBAR_W, 10000};
            InvalidateRect(hwnd, &sideRect, FALSE);
            return 0;
        }

        /* Interactive Forecast Quick Percentage Buttons */
        if (id == IDC_BTN_FC_10) { SetWindowText(g_app.hFcCutEdit, "10"); runForecastCalculation(); return 0; }
        if (id == IDC_BTN_FC_20) { SetWindowText(g_app.hFcCutEdit, "20"); runForecastCalculation(); return 0; }
        if (id == IDC_BTN_FC_30) { SetWindowText(g_app.hFcCutEdit, "30"); runForecastCalculation(); return 0; }
        if (id == IDC_BTN_FC_50) { SetWindowText(g_app.hFcCutEdit, "50"); runForecastCalculation(); return 0; }
        if (id == IDC_BTN_FC_CALC) { runForecastCalculation(); return 0; }
        if (id == IDC_COMBO_FC_CAT && HIWORD(wParam) == CBN_SELCHANGE) { runForecastCalculation(); return 0; }

        switch (id) {
        case IDC_BTN_ADD: {
            g_addData.confirmed = 0;
            showModal(hwnd, AddTransWndProc, "Add New Transaction", 410, 290);
            if (g_addData.confirmed) {
                addTransaction(&g_app.txns,
                               g_addData.description,
                               g_addData.category,
                               g_addData.amount,
                               g_addData.date,
                               g_addData.isRecurring);
                collectCategories(&g_app.txns, g_app.categories, &g_app.catCount);
                saveTransactions(&g_app.txns);
                refreshList();
                runForecastCalculation();
                setStatus("Transaction successfully added.");
                InvalidateRect(g_app.hContent, NULL, TRUE);
            }
            return 0;
        }

        case IDC_BTN_DELETE: {
            int sel = ListView_GetNextItem(g_app.hList, -1, LVNI_SELECTED);
            if (sel < 0) {
                MessageBox(hwnd, "Please select a transaction to delete.", "Delete",
                           MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            LVITEM lvi = {0};
            lvi.mask  = LVIF_PARAM;
            lvi.iItem = sel;
            ListView_GetItem(g_app.hList, &lvi);
            int txId = (int)lvi.lParam;
            if (MessageBox(hwnd, "Are you sure you want to delete this transaction?", "Confirm Delete",
                           MB_YESNO | MB_ICONQUESTION) == IDYES) {
                deleteTransaction(&g_app.txns, txId);
                saveTransactions(&g_app.txns);
                refreshList();
                runForecastCalculation();
                setStatus("Transaction deleted.");
                InvalidateRect(g_app.hContent, NULL, TRUE);
            }
            return 0;
        }

        case IDC_BTN_SETBUDGET: {
            g_budgetData.confirmed = 0;
            showModal(hwnd, SetBudgetWndProc, "Set Category Budget Limit", 390, 200);
            if (g_budgetData.confirmed) {
                setBudget(&g_app.budgets, g_budgetData.category, g_budgetData.limit);
                saveBudgets(&g_app.budgets);
                setStatus("Category budget limit updated.");
                InvalidateRect(g_app.hContent, NULL, TRUE);
            }
            return 0;
        }

        case IDC_BTN_EXPORTCSV: {
            system("if not exist exports mkdir exports 2>nul");
            if (exportCSV(&g_app.txns, "exports/transactions.csv")) {
                MessageBox(hwnd, "Transactions successfully exported to exports/transactions.csv", "Export Successful",
                           MB_OK | MB_ICONINFORMATION);
                setStatus("Exported to exports/transactions.csv");
            } else {
                MessageBox(hwnd, "Failed to write exports/transactions.csv", "Export Failed", MB_OK | MB_ICONERROR);
            }
            return 0;
        }

        case IDC_BTN_EXPORTRPT: {
            system("if not exist exports mkdir exports 2>nul");
            if (exportReport(&g_app.txns, &g_app.budgets, "exports/report.txt")) {
                MessageBox(hwnd, "Full financial report successfully generated at exports/report.txt", "Report Generated",
                           MB_OK | MB_ICONINFORMATION);
                setStatus("Report generated at exports/report.txt");
            } else {
                MessageBox(hwnd, "Failed to write exports/report.txt", "Export Failed", MB_OK | MB_ICONERROR);
            }
            return 0;
        }
        }
        return 0;
    }

    case WM_DESTROY:
        saveTransactions(&g_app.txns);
        saveBudgets(&g_app.budgets);
        freeList(&g_app.txns);
        cleanupFonts();
        DeleteObject(hSidebarBrush);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* ── WinMain Entry Point ──────────────────────────────── */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev,
                   LPSTR lpCmd, int nCmdShow) {
    (void)hPrev;

    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);

    /* Sample Data Commandline Mode */
    if (strstr(lpCmd, "--sample") != NULL) {
        TransactionList tmp;
        initList(&tmp);
        addSampleData(&tmp);
        saveTransactions(&tmp);
        freeList(&tmp);
        return 0;
    }

    /* Interactive CLI Terminal Mode */
    if (strstr(lpCmd, "--cli") != NULL) {
        if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
            AllocConsole();
        }
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);

        TransactionList txns;
        BudgetList      budgets;
        char            categories[MAX_CATEGORIES][MAX_CAT_LEN];
        int             catCount = 0;

        initList(&txns);
        budgets.count = 0;
        loadTransactions(&txns);
        loadBudgets(&budgets);
        initCategories(categories, &catCount);
        collectCategories(&txns, categories, &catCount);
        applyRecurring(&txns);
        saveTransactions(&txns);

        runCliMenu(&txns, &budgets, categories, &catCount);

        freeList(&txns);
        return 0;
    }

    /* Register Main Window Class */
    WNDCLASSEX wcMain    = {0};
    wcMain.cbSize        = sizeof(wcMain);
    wcMain.lpfnWndProc   = MainWndProc;
    wcMain.hInstance     = hInst;
    wcMain.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcMain.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcMain.lpszClassName = CLS_MAIN;
    wcMain.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wcMain.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassEx(&wcMain);

    /* Register Content Child Window Class */
    WNDCLASSEX wcContent    = {0};
    wcContent.cbSize        = sizeof(wcContent);
    wcContent.lpfnWndProc   = ContentWndProc;
    wcContent.hInstance     = hInst;
    wcContent.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcContent.hbrBackground = CreateSolidBrush(CLR_BG);
    wcContent.lpszClassName = CLS_CONTENT;
    RegisterClassEx(&wcContent);

    /* Register Dialog Class */
    WNDCLASSEX wcDlg    = {0};
    wcDlg.cbSize        = sizeof(wcDlg);
    wcDlg.lpfnWndProc   = DefWindowProc;
    wcDlg.hInstance     = hInst;
    wcDlg.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcDlg.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcDlg.lpszClassName = CLS_DIALOG;
    RegisterClassEx(&wcDlg);

    /* Create Main Application Window */
    HWND hMain = CreateWindowEx(
        0, CLS_MAIN, "SmartSpend — Personal Finance Tracker",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1150, 750,
        NULL, NULL, hInst, NULL
    );
    if (!hMain) return 1;

    ShowWindow(hMain, (nCmdShow == SW_HIDE || nCmdShow == 0) ? SW_SHOWNORMAL : nCmdShow);
    UpdateWindow(hMain);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
