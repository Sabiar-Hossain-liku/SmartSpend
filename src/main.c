/*
 * main.c — SmartSpend Win32 GUI entry point.
 *
 * Owns: WinMain, main window proc, content window proc, all dialog windows.
 * Data layer is in storage/transaction/category/insights/budget/export.
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

/* ── Window class names ───────────────────────────────── */
#define CLS_MAIN    "SmartSpendMain"
#define CLS_CONTENT "SmartSpendContent"
#define CLS_DIALOG  "SmartSpendDialog"

/* ── Forward declarations ─────────────────────────────── */
static LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK ContentWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK AddTransWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK SetBudgetWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK WhatIfWndProc(HWND, UINT, WPARAM, LPARAM);

/* ── Dialog control IDs ───────────────────────────────── */
#define DLG_DESC      1001
#define DLG_AMOUNT    1002
#define DLG_CATEGORY  1003
#define DLG_DATE      1004
#define DLG_RECURRING 1005
#define DLG_OK        1006
#define DLG_CANCEL    1007
#define DLG_CATCOMBO  1008
#define DLG_CUTPCT    1009
#define DLG_RESULT    1010

/* ── Dialog data structs ──────────────────────────────── */
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

typedef struct {
    char   category[MAX_CAT_LEN];
    double cutPercent;
    int    confirmed;
} WhatIfData;

static AddTransData   g_addData;
static SetBudgetData  g_budgetData;
static WhatIfData     g_whatIfData;

/* ── Sample data ──────────────────────────────────────── */
static void addSampleData(TransactionList *list) {
    time_t now    = time(NULL);
    time_t oneDay = 86400;

    addTransaction(list, "Grocery shopping",    "Food",          2500.0, now - oneDay * 2,  0);
    addTransaction(list, "Monthly rent",        "Housing",      12000.0, now - oneDay * 5,  1);
    addTransaction(list, "Bus pass",            "Transport",      800.0, now - oneDay * 3,  1);
    addTransaction(list, "Internet bill",       "Utilities",      700.0, now - oneDay * 1,  1);
    addTransaction(list, "Movie tickets",       "Entertainment",  350.0, now - oneDay * 4,  0);
    addTransaction(list, "Doctor visit",        "Healthcare",    1200.0, now - oneDay * 10, 0);
    addTransaction(list, "Textbook purchase",   "Education",     1800.0, now - oneDay * 15, 0);
    addTransaction(list, "Restaurant lunch",    "Food",           480.0, now - oneDay * 6,  0);
    addTransaction(list, "Gym membership",      "Personal",       600.0, now - oneDay * 8,  1);
    addTransaction(list, "Online shopping",     "Shopping",      3200.0, now - oneDay * 12, 0);
    addTransaction(list, "Electricity bill",    "Utilities",      950.0, now - oneDay * 20, 1);
    addTransaction(list, "Coffee & snacks",     "Food",           320.0, now - oneDay * 7,  0);
}

/* ── Modal dialog helper ──────────────────────────────── */
static HWND showModal(HWND parent, WNDPROC proc, const char *title,
                      int w, int h) {
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
    /* Replace the window proc with the specific dialog proc */
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
    return NULL;
}

/* ── Add Transaction dialog ───────────────────────────── */
static LRESULT CALLBACK AddTransWndProc(HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam) {
    static HWND hDesc, hAmt, hCat, hDate, hRecur;
    (void)lParam;

    switch (msg) {
    case WM_CREATE: {
        HINSTANCE hInst = GetModuleHandle(NULL);
        int y = 16, lw = 110, ex = 130, ew = 220;

        CreateWindowEx(0,"STATIC","Description:",
                       WS_CHILD|WS_VISIBLE,16,y,lw,20,hwnd,NULL,hInst,NULL);
        hDesc = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",
                       WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
                       ex,y,ew,24,hwnd,(HMENU)DLG_DESC,hInst,NULL);
        y += 34;

        CreateWindowEx(0,"STATIC","Category:",
                       WS_CHILD|WS_VISIBLE,16,y,lw,20,hwnd,NULL,hInst,NULL);
        hCat = CreateWindowEx(0,"COMBOBOX","",
                       WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
                       ex,y,ew,200,hwnd,(HMENU)DLG_CATEGORY,hInst,NULL);
        for (int i = 0; i < g_app.catCount; i++)
            SendMessage(hCat, CB_ADDSTRING, 0, (LPARAM)g_app.categories[i]);
        SendMessage(hCat, CB_SETCURSEL, 0, 0);
        y += 34;

        CreateWindowEx(0,"STATIC","Amount (Tk):",
                       WS_CHILD|WS_VISIBLE,16,y,lw,20,hwnd,NULL,hInst,NULL);
        hAmt = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",
                       WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
                       ex,y,ew,24,hwnd,(HMENU)DLG_AMOUNT,hInst,NULL);
        y += 34;

        CreateWindowEx(0,"STATIC","Date (DD/MM/YYYY):",
                       WS_CHILD|WS_VISIBLE,16,y,lw,20,hwnd,NULL,hInst,NULL);
        char todayBuf[16];
        formatDate(time(NULL), todayBuf, sizeof(todayBuf));
        hDate = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT",todayBuf,
                       WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
                       ex,y,ew,24,hwnd,(HMENU)DLG_DATE,hInst,NULL);
        y += 34;

        hRecur = CreateWindowEx(0,"BUTTON","Recurring every month",
                       WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
                       ex,y,ew,22,hwnd,(HMENU)DLG_RECURRING,hInst,NULL);
        y += 38;

        CreateWindowEx(0,"BUTTON","Add Transaction",
                       WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
                       ex,y,145,32,hwnd,(HMENU)DLG_OK,hInst,NULL);
        CreateWindowEx(0,"BUTTON","Cancel",
                       WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                       ex+150,y,70,32,hwnd,(HMENU)DLG_CANCEL,hInst,NULL);
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
                MessageBox(hwnd, "Please enter a description.", "Missing Field",
                           MB_OK | MB_ICONWARNING);
                return 0;
            }
            double amount = atof(amtBuf);
            if (amount <= 0.0) {
                MessageBox(hwnd, "Please enter a positive amount.", "Missing Field",
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
        int y = 20, lw = 100, ex = 120, ew = 200;

        CreateWindowEx(0,"STATIC","Category:",
                       WS_CHILD|WS_VISIBLE,16,y,lw,20,hwnd,NULL,hInst,NULL);
        hCat = CreateWindowEx(0,"COMBOBOX","",
                       WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
                       ex,y,ew,200,hwnd,(HMENU)DLG_CATCOMBO,hInst,NULL);
        for (int i = 0; i < g_app.catCount; i++)
            SendMessage(hCat, CB_ADDSTRING, 0, (LPARAM)g_app.categories[i]);
        SendMessage(hCat, CB_SETCURSEL, 0, 0);
        y += 36;

        CreateWindowEx(0,"STATIC","Monthly Limit (Tk):",
                       WS_CHILD|WS_VISIBLE,16,y,lw,20,hwnd,NULL,hInst,NULL);
        hLimit = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",
                       WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
                       ex,y,ew,24,hwnd,(HMENU)DLG_AMOUNT,hInst,NULL);
        y += 40;

        CreateWindowEx(0,"BUTTON","Set Budget",
                       WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
                       ex,y,120,32,hwnd,(HMENU)DLG_OK,hInst,NULL);
        CreateWindowEx(0,"BUTTON","Cancel",
                       WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                       ex+126,y,70,32,hwnd,(HMENU)DLG_CANCEL,hInst,NULL);
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
                MessageBox(hwnd, "Please enter a positive limit.", "Missing Field",
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

/* ── What-If dialog ───────────────────────────────────── */
static LRESULT CALLBACK WhatIfWndProc(HWND hwnd, UINT msg,
                                      WPARAM wParam, LPARAM lParam) {
    static HWND hCat, hPct, hResult;
    (void)lParam;

    switch (msg) {
    case WM_CREATE: {
        HINSTANCE hInst = GetModuleHandle(NULL);
        int y = 16, lw = 130, ex = 150, ew = 180;

        CreateWindowEx(0,"STATIC","Select Category:",
                       WS_CHILD|WS_VISIBLE,16,y,lw,20,hwnd,NULL,hInst,NULL);
        hCat = CreateWindowEx(0,"COMBOBOX","",
                       WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
                       ex,y,ew,200,hwnd,(HMENU)DLG_CATCOMBO,hInst,NULL);
        for (int i = 0; i < g_app.catCount; i++)
            SendMessage(hCat, CB_ADDSTRING, 0, (LPARAM)g_app.categories[i]);
        SendMessage(hCat, CB_SETCURSEL, 0, 0);
        y += 36;

        CreateWindowEx(0,"STATIC","Cut spending by (%):",
                       WS_CHILD|WS_VISIBLE,16,y,lw,20,hwnd,NULL,hInst,NULL);
        hPct = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","20",
                       WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
                       ex,y,ew,24,hwnd,(HMENU)DLG_CUTPCT,hInst,NULL);
        y += 36;

        hResult = CreateWindowEx(0,"STATIC","",
                       WS_CHILD|WS_VISIBLE|SS_CENTER,
                       16,y,314,30,hwnd,(HMENU)DLG_RESULT,hInst,NULL);
        y += 40;

        CreateWindowEx(0,"BUTTON","Calculate",
                       WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
                       ex,y,100,32,hwnd,(HMENU)DLG_OK,hInst,NULL);
        CreateWindowEx(0,"BUTTON","Close",
                       WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                       ex+106,y,70,32,hwnd,(HMENU)DLG_CANCEL,hInst,NULL);
        SetFocus(hPct);
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case DLG_OK: {
            char pctBuf[32];
            GetWindowText(hPct, pctBuf, sizeof(pctBuf));
            double cut = atof(pctBuf);

            int sel = (int)SendMessage(hCat, CB_GETCURSEL, 0, 0);
            const char *cat = (sel >= 0 && sel < g_app.catCount)
                              ? g_app.categories[sel] : "Other";

            double savings = whatIfSavings(&g_app.txns, cat, cut);
            char resBuf[128];
            snprintf(resBuf, sizeof(resBuf),
                     "Cutting %s by %.0f%% saves ~Tk %.2f / year",
                     cat, cut, savings);
            SetWindowText(hResult, resBuf);

            /* Also store in global for content area display */
            strncpy(g_app.whatIfResult, resBuf, sizeof(g_app.whatIfResult) - 1);
            g_app.whatIfResult[sizeof(g_app.whatIfResult) - 1] = '\0';
            return 0;
        }
        case DLG_CANCEL:
            g_whatIfData.confirmed = 0;
            DestroyWindow(hwnd);
            return 0;
        }
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* ── Content window – draws the right-hand page ───────── */
static LRESULT CALLBACK ContentWndProc(HWND hwnd, UINT msg,
                                       WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {
    case WM_ERASEBKGND: {
        HDC   hdc = (HDC)wParam;
        RECT  rc;
        GetClientRect(hwnd, &rc);
        HBRUSH hBr = CreateSolidBrush(CLR_BG);
        FillRect(hdc, &rc, hBr);
        DeleteObject(hBr);
        return 1;
    }

    case WM_SIZE: {
        int cw = LOWORD(lParam), ch = HIWORD(lParam);
        int btnY = ch - 50;
        int topY = 50;    /* below page title bar */

        /* Reposition per-page buttons at the top of content */
        SetWindowPos(g_app.hBtnAdd,       NULL, cw - 290, 10, 130, 30, SWP_NOZORDER);
        SetWindowPos(g_app.hBtnDel,       NULL, cw - 155, 10, 130, 30, SWP_NOZORDER);
        SetWindowPos(g_app.hBtnSetBudget, NULL, cw - 170, 10, 155, 30, SWP_NOZORDER);
        SetWindowPos(g_app.hBtnSimulate,  NULL, cw - 160, 10, 145, 30, SWP_NOZORDER);
        SetWindowPos(g_app.hBtnExportCSV, NULL, cw - 310, 10, 145, 30, SWP_NOZORDER);
        SetWindowPos(g_app.hBtnExportRpt, NULL, cw - 160, 10, 145, 30, SWP_NOZORDER);

        /* ListView fills the rest of the content area */
        SetWindowPos(g_app.hList, NULL, 0, topY, cw, ch - topY, SWP_NOZORDER);

        (void)btnY;
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        /* Page title bar */
        static const char *pageTitles[PAGE_COUNT] = {
            "Dashboard", "Transactions", "Budget",
            "Insights",  "What-If Simulator", "Export"
        };
        int pg = g_app.currentPage;
        HBRUSH hBg = CreateSolidBrush(CLR_BG);
        FillRect(hdc, &rc, hBg);
        DeleteObject(hBg);

        /* Title strip */
        RECT titleBar = {rc.left, rc.top, rc.right, rc.top + 46};
        HBRUSH hTBg = CreateSolidBrush(CLR_WHITE);
        FillRect(hdc, &titleBar, hTBg);
        DeleteObject(hTBg);
        HPEN hLine = CreatePen(PS_SOLID, 1, CLR_BORDER);
        SelectObject(hdc, hLine);
        MoveToEx(hdc, rc.left, rc.top + 46, NULL);
        LineTo(hdc, rc.right, rc.top + 46);
        DeleteObject(hLine);

        HFONT hBig = makeFont(15, 1);
        HFONT hOld = (HFONT)SelectObject(hdc, hBig);
        SetTextColor(hdc, CLR_TEXT_DARK);
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc, 16, 13, pageTitles[pg], (int)strlen(pageTitles[pg]));
        SelectObject(hdc, hOld);
        DeleteObject(hBig);

        /* Page content below the title bar */
        RECT content = {rc.left, rc.top + 54, rc.right, rc.bottom};

        switch (pg) {
        case PAGE_DASHBOARD: {
            int x = content.left + 16;
            int y = content.top  + 12;
            int cw3 = (content.right - content.left - 48) / 3;
            int ch3 = 88;
            char valBuf[64];

            snprintf(valBuf, sizeof(valBuf), "Tk %.2f", spentThisMonth(&g_app.txns));
            drawCard(hdc, x,                 y, cw3, ch3,
                     "Spent This Month", valBuf, "total so far", CLR_ACCENT);

            snprintf(valBuf, sizeof(valBuf), "Tk %.2f", forecastNextMonth(&g_app.txns));
            drawCard(hdc, x + cw3 + 8,      y, cw3, ch3,
                     "Forecast", valBuf, "next month", CLR_ORANGE);

            drawCard(hdc, x + 2*(cw3 + 8),  y, cw3, ch3,
                     "Top Category", topCategory(&g_app.txns), "most spending", CLR_GREEN);

            /* Monthly bar chart */
            int chartY = y + ch3 + 16;
            int chartH = 220;
            drawRoundRect(hdc,
                          content.left + 16, chartY,
                          content.right - content.left - 32, chartH,
                          CARD_R, CLR_CARD, CLR_BORDER);

            time_t now2  = time(NULL);
            struct tm *tmNow = localtime(&now2);
            int curYear = tmNow->tm_year + 1900;
            double totals[12];
            monthlyTotals(&g_app.txns, totals, curYear);

            static const char *months[12] = {
                "Jan","Feb","Mar","Apr","May","Jun",
                "Jul","Aug","Sep","Oct","Nov","Dec"
            };
            RECT chartInner = {
                content.left + 26, chartY + 4,
                content.right - 26, chartY + chartH - 4
            };
            drawBarChart(hdc, chartInner, totals, months, 12, "Monthly Spending");

            /* Recent transactions list */
            int listY = chartY + chartH + 16;
            HFONT hLbl = makeFont(11, 1);
            hOld = (HFONT)SelectObject(hdc, hLbl);
            SetTextColor(hdc, CLR_TEXT_DARK);
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc, x, listY, "Recent Transactions", 19);
            SelectObject(hdc, hOld);
            DeleteObject(hLbl);

            listY += 22;
            HFONT hRows = makeFont(10, 0);
            hOld = (HFONT)SelectObject(hdc, hRows);

            int shown = 0;
            for (int i = g_app.txns.count - 1; i >= 0 && shown < 6; i--, shown++) {
                const Transaction *tr = &g_app.txns.items[i];
                char dBuf[16];
                formatDate(tr->date, dBuf, sizeof(dBuf));
                char line[256];
                snprintf(line, sizeof(line), "%s   %-22s  %-12s  Tk %.2f%s",
                         dBuf, tr->description, tr->category, tr->amount,
                         tr->isRecurring ? "  [R]" : "");
                COLORREF rowClr = (shown % 2 == 0) ? CLR_TEXT_DARK : CLR_TEXT_MUTED;
                SetTextColor(hdc, rowClr);
                TextOut(hdc, x, listY, line, (int)strlen(line));
                listY += 19;
            }
            if (g_app.txns.count == 0) {
                SetTextColor(hdc, CLR_TEXT_MUTED);
                TextOut(hdc, x, listY,
                        "No transactions yet — add one in the Transactions page.", 54);
            }
            SelectObject(hdc, hOld);
            DeleteObject(hRows);
            break;
        }

        case PAGE_BUDGET: {
            if (g_app.budgets.count == 0) {
                HFONT hF = makeFont(11, 0);
                hOld = (HFONT)SelectObject(hdc, hF);
                SetTextColor(hdc, CLR_TEXT_MUTED);
                SetBkMode(hdc, TRANSPARENT);
                TextOut(hdc, content.left + 16, content.top + 16,
                        "No budgets set yet. Click 'Set Budget' to begin.", 48);
                SelectObject(hdc, hOld);
                DeleteObject(hF);
            } else {
                drawBudgetBars(hdc, content, &g_app.txns, &g_app.budgets);
            }
            break;
        }

        case PAGE_INSIGHTS: {
            int x2 = content.left + 16, y2 = content.top + 12;
            SetBkMode(hdc, TRANSPARENT);

            struct { const char *lbl; char val[64]; } stats[] = {
                {"Total Spent (All Time)",  {0}},
                {"Spent This Month",        {0}},
                {"Forecast Next Month",     {0}},
                {"Top Spending Category",   {0}},
                {"Total Transactions",      {0}},
            };
            snprintf(stats[0].val, 64, "Tk %.2f", totalSpent(&g_app.txns));
            snprintf(stats[1].val, 64, "Tk %.2f", spentThisMonth(&g_app.txns));
            snprintf(stats[2].val, 64, "Tk %.2f", forecastNextMonth(&g_app.txns));
            snprintf(stats[3].val, 64, "%s",       topCategory(&g_app.txns));
            snprintf(stats[4].val, 64, "%d",       g_app.txns.count);

            int nStats = 5;
            int cw2 = (content.right - content.left - 48) / nStats;
            for (int i = 0; i < nStats; i++) {
                drawCard(hdc, x2 + i * (cw2 + 8), y2, cw2, 86,
                         stats[i].lbl, stats[i].val, "", CLR_ACCENT);
            }

            /* Category bar chart */
            int cy2 = y2 + 86 + 20;
            int chartH2 = 200;
            drawRoundRect(hdc,
                          content.left + 16, cy2,
                          content.right - content.left - 32, chartH2,
                          CARD_R, CLR_CARD, CLR_BORDER);

            if (g_app.catCount > 0) {
                double catTotals[MAX_CATEGORIES] = {0.0};
                const char *catLabels[MAX_CATEGORIES];
                for (int i = 0; i < g_app.catCount; i++) {
                    catTotals[i]  = spentByCategory(&g_app.txns, g_app.categories[i]);
                    catLabels[i]  = g_app.categories[i];
                }
                RECT cInner = {
                    content.left + 26, cy2 + 4,
                    content.right - 26, cy2 + chartH2 - 4
                };
                drawBarChart(hdc, cInner, catTotals, catLabels,
                             g_app.catCount, "Spending by Category");
            }
            break;
        }

        case PAGE_WHATIF: {
            int x3 = content.left + 16, y3 = content.top + 16;
            SetBkMode(hdc, TRANSPARENT);

            HFONT hF = makeFont(11, 0);
            hOld = (HFONT)SelectObject(hdc, hF);
            SetTextColor(hdc, CLR_TEXT_MUTED);
            TextOut(hdc, x3, y3,
                    "Pick a category, enter a cut percentage, and click Simulate.", 61);
            y3 += 24;
            TextOut(hdc, x3, y3,
                    "The result shows how much you would save in one year.", 53);

            if (g_app.whatIfResult[0] != '\0') {
                y3 += 40;
                HFONT hBig2 = makeFont(14, 1);
                SelectObject(hdc, hBig2);
                SetTextColor(hdc, CLR_GREEN);
                TextOut(hdc, x3, y3, g_app.whatIfResult,
                        (int)strlen(g_app.whatIfResult));
                SelectObject(hdc, hF);
                DeleteObject(hBig2);
            }
            SelectObject(hdc, hOld);
            DeleteObject(hF);
            break;
        }

        case PAGE_EXPORT: {
            int x4 = content.left + 16, y4 = content.top + 16;
            SetBkMode(hdc, TRANSPARENT);
            HFONT hF = makeFont(11, 0);
            hOld = (HFONT)SelectObject(hdc, hF);
            SetTextColor(hdc, CLR_TEXT_MUTED);
            TextOut(hdc, x4, y4,
                    "'Export CSV' — saves all transactions to exports/transactions.csv", 65);
            y4 += 22;
            TextOut(hdc, x4, y4,
                    "'Export Report' — saves a full spending report to exports/report.txt", 70);
            SelectObject(hdc, hOld);
            DeleteObject(hF);
            break;
        }

        default:
            break;
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_COMMAND:
        /* Forward to main window */
        SendMessage(g_app.hMain, WM_COMMAND, wParam, lParam);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* ── Main window ──────────────────────────────────────── */
static void createNavButton(HINSTANCE hInst, int idx,
                            const char *label, int x, int y) {
    g_app.hNavBtns[idx] = CreateWindowEx(
        0, "BUTTON", label,
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        x, y, SIDEBAR_W - 20, NAV_BTN_H,
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

        /* Nav buttons */
        static const char *navLabels[PAGE_COUNT] = {
            "  Dashboard", "  Transactions", "  Budget",
            "  Insights",  "  What-If",      "  Export"
        };
        for (int i = 0; i < PAGE_COUNT; i++)
            createNavButton(hInst, i, navLabels[i], 10, HEADER_H + 10 + i * (NAV_BTN_H + 6));

        /* Content window */
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

        /* Status bar */
        g_app.hStatus = CreateWindowEx(
            0, STATUSCLASSNAME, "Ready",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0, hwnd, (HMENU)IDOK, hInst, NULL
        );

        /* Transaction ListView (child of hContent) */
        g_app.hList = CreateWindowEx(
            WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
            WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER,
            0, 50, cw, ch - 50,
            g_app.hContent, (HMENU)IDC_LIST_TRANS, hInst, NULL
        );
        ListView_SetExtendedListViewStyle(g_app.hList,
            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        /* ListView columns */
        LVCOLUMN lvc = {0};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        struct { const char *name; int w; } cols[] = {
            {"Date", 90}, {"Description", 210}, {"Category", 110},
            {"Amount", 90}, {"Recurring", 75}
        };
        for (int i = 0; i < 5; i++) {
            lvc.iSubItem = i;
            lvc.pszText  = (LPSTR)cols[i].name;
            lvc.cx       = cols[i].w;
            ListView_InsertColumn(g_app.hList, i, &lvc);
        }

        /* Per-page action buttons (children of hContent) */
        g_app.hBtnAdd = CreateWindowEx(0,"BUTTON","+ Add Transaction",
            WS_CHILD|BS_PUSHBUTTON, 0,10,130,30,
            g_app.hContent,(HMENU)IDC_BTN_ADD,hInst,NULL);
        g_app.hBtnDel = CreateWindowEx(0,"BUTTON","Delete Selected",
            WS_CHILD|BS_PUSHBUTTON, 0,10,130,30,
            g_app.hContent,(HMENU)IDC_BTN_DELETE,hInst,NULL);
        g_app.hBtnSetBudget = CreateWindowEx(0,"BUTTON","+ Set Budget",
            WS_CHILD|BS_PUSHBUTTON, 0,10,155,30,
            g_app.hContent,(HMENU)IDC_BTN_SETBUDGET,hInst,NULL);
        g_app.hBtnSimulate = CreateWindowEx(0,"BUTTON","Simulate Savings",
            WS_CHILD|BS_PUSHBUTTON, 0,10,145,30,
            g_app.hContent,(HMENU)IDC_BTN_SIMULATE,hInst,NULL);
        g_app.hBtnExportCSV = CreateWindowEx(0,"BUTTON","Export CSV",
            WS_CHILD|BS_PUSHBUTTON, 0,10,145,30,
            g_app.hContent,(HMENU)IDC_BTN_EXPORTCSV,hInst,NULL);
        g_app.hBtnExportRpt = CreateWindowEx(0,"BUTTON","Export Report",
            WS_CHILD|BS_PUSHBUTTON, 0,10,145,30,
            g_app.hContent,(HMENU)IDC_BTN_EXPORTRPT,hInst,NULL);

        /* Load data */
        initList(&g_app.txns);
        g_app.budgets.count = 0;
        loadTransactions(&g_app.txns);
        loadBudgets(&g_app.budgets);
        initCategories(g_app.categories, &g_app.catCount);
        collectCategories(&g_app.txns, g_app.categories, &g_app.catCount);
        applyRecurring(&g_app.txns);
        saveTransactions(&g_app.txns);
        refreshList();

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

        /* Reposition nav buttons */
        for (int i = 0; i < PAGE_COUNT; i++) {
            SetWindowPos(g_app.hNavBtns[i], NULL,
                         10, HEADER_H + 10 + i * (NAV_BTN_H + 6),
                         SIDEBAR_W - 20, NAV_BTN_H, SWP_NOZORDER);
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        /* Sidebar background */
        RECT sideRect = {0, 0, SIDEBAR_W, rc.bottom};
        FillRect(hdc, &sideRect, hSidebarBrush);

        /* App title in sidebar */
        SetBkMode(hdc, TRANSPARENT);
        HFONT hTitle = makeFont(16, 1);
        HFONT hOld   = (HFONT)SelectObject(hdc, hTitle);
        SetTextColor(hdc, CLR_WHITE);
        TextOut(hdc, 16, 16, "SmartSpend", 10);
        SelectObject(hdc, hOld);
        DeleteObject(hTitle);

        HFONT hSub = makeFont(9, 0);
        hOld = (HFONT)SelectObject(hdc, hSub);
        SetTextColor(hdc, RGB(100, 116, 139));
        TextOut(hdc, 16, 36, "Personal Finance", 16);
        SelectObject(hdc, hOld);
        DeleteObject(hSub);

        /* Divider under title */
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(30, 41, 59));
        SelectObject(hdc, hPen);
        MoveToEx(hdc, 10, HEADER_H + 2, NULL);
        LineTo(hdc, SIDEBAR_W - 10, HEADER_H + 2);
        DeleteObject(hPen);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DRAWITEM: {
        /* Owner-draw the sidebar nav buttons */
        DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
        int idx = (int)(dis->CtlID - IDC_NAV_FIRST);
        if (idx < 0 || idx >= PAGE_COUNT) break;

        int isActive = (idx == g_app.currentPage);
        COLORREF bgColor = isActive ? CLR_ACCENT : CLR_SIDEBAR;

        HBRUSH hBr = CreateSolidBrush(bgColor);
        FillRect(dis->hDC, &dis->rcItem, hBr);
        DeleteObject(hBr);

        /* Accent line on left for active */
        if (isActive) {
            HBRUSH hLine2 = CreateSolidBrush(CLR_WHITE);
            RECT lineR = {dis->rcItem.left, dis->rcItem.top,
                          dis->rcItem.left + 3, dis->rcItem.bottom};
            FillRect(dis->hDC, &lineR, hLine2);
            DeleteObject(hLine2);
        }

        /* Button text */
        char btnText[64];
        GetWindowText(dis->hwndItem, btnText, sizeof(btnText));
        SetBkMode(dis->hDC, TRANSPARENT);
        HFONT hBtnFont = makeFont(11, isActive ? 1 : 0);
        HFONT hOldFont = (HFONT)SelectObject(dis->hDC, hBtnFont);
        SetTextColor(dis->hDC, isActive ? CLR_WHITE : RGB(148, 163, 184));
        RECT textRect = dis->rcItem;
        textRect.left += 10;
        DrawText(dis->hDC, btnText, -1, &textRect,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dis->hDC, hOldFont);
        DeleteObject(hBtnFont);
        return TRUE;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);

        /* Navigation */
        if (id >= IDC_NAV_FIRST && id < IDC_NAV_FIRST + PAGE_COUNT) {
            g_app.currentPage = id - IDC_NAV_FIRST;
            updatePageLayout();
            /* Redraw sidebar for active state change */
            RECT sideRect = {0, 0, SIDEBAR_W,
                             10000 /* large enough */ };
            InvalidateRect(hwnd, &sideRect, FALSE);
            return 0;
        }

        switch (id) {

        case IDC_BTN_ADD: {
            g_addData.confirmed = 0;
            showModal(hwnd, AddTransWndProc, "Add Transaction", 380, 270);
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
                setStatus("Transaction added.");
                InvalidateRect(g_app.hContent, NULL, TRUE);
            }
            return 0;
        }

        case IDC_BTN_DELETE: {
            int sel = ListView_GetNextItem(g_app.hList, -1, LVNI_SELECTED);
            if (sel < 0) {
                MessageBox(hwnd, "Select a transaction to delete.", "Delete",
                           MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            LVITEM lvi = {0};
            lvi.mask    = LVIF_PARAM;
            lvi.iItem   = sel;
            ListView_GetItem(g_app.hList, &lvi);
            int txId = (int)lvi.lParam;
            if (MessageBox(hwnd, "Delete this transaction?", "Confirm Delete",
                           MB_YESNO | MB_ICONQUESTION) == IDYES) {
                deleteTransaction(&g_app.txns, txId);
                saveTransactions(&g_app.txns);
                refreshList();
                setStatus("Transaction deleted.");
                InvalidateRect(g_app.hContent, NULL, TRUE);
            }
            return 0;
        }

        case IDC_BTN_SETBUDGET: {
            g_budgetData.confirmed = 0;
            showModal(hwnd, SetBudgetWndProc, "Set Monthly Budget", 360, 190);
            if (g_budgetData.confirmed) {
                setBudget(&g_app.budgets, g_budgetData.category, g_budgetData.limit);
                saveBudgets(&g_app.budgets);
                setStatus("Budget updated.");
                InvalidateRect(g_app.hContent, NULL, TRUE);
            }
            return 0;
        }

        case IDC_BTN_SIMULATE: {
            g_whatIfData.confirmed = 0;
            showModal(hwnd, WhatIfWndProc, "What-If Simulator", 360, 220);
            InvalidateRect(g_app.hContent, NULL, TRUE);
            return 0;
        }

        case IDC_BTN_EXPORTCSV: {
            system("if not exist exports mkdir exports 2>nul");
            if (exportCSV(&g_app.txns, "exports/transactions.csv"))
                MessageBox(hwnd, "Exported to exports/transactions.csv", "Export",
                           MB_OK | MB_ICONINFORMATION);
            else
                MessageBox(hwnd, "Export failed.", "Error", MB_OK | MB_ICONERROR);
            return 0;
        }

        case IDC_BTN_EXPORTRPT: {
            system("if not exist exports mkdir exports 2>nul");
            if (exportReport(&g_app.txns, &g_app.budgets, "exports/report.txt"))
                MessageBox(hwnd, "Report saved to exports/report.txt", "Export",
                           MB_OK | MB_ICONINFORMATION);
            else
                MessageBox(hwnd, "Export failed.", "Error", MB_OK | MB_ICONERROR);
            return 0;
        }
        }
        return 0;
    }

    case WM_DESTROY:
        saveTransactions(&g_app.txns);
        saveBudgets(&g_app.budgets);
        freeList(&g_app.txns);
        DeleteObject(hSidebarBrush);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* ── WinMain ──────────────────────────────────────────── */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev,
                   LPSTR lpCmd, int nCmdShow) {
    (void)hPrev;

    /* Initialise Common Controls (ListView) */
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);

    /* Sample data mode */
    if (strstr(lpCmd, "--sample") != NULL) {
        TransactionList tmp;
        initList(&tmp);
        addSampleData(&tmp);
        saveTransactions(&tmp);
        freeList(&tmp);
        return 0;
    }

    /* Register main window class */
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

    /* Register content child window class */
    WNDCLASSEX wcContent    = {0};
    wcContent.cbSize        = sizeof(wcContent);
    wcContent.lpfnWndProc   = ContentWndProc;
    wcContent.hInstance     = hInst;
    wcContent.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcContent.hbrBackground = CreateSolidBrush(CLR_BG);
    wcContent.lpszClassName = CLS_CONTENT;
    RegisterClassEx(&wcContent);

    /* Register dialog class (placeholder proc, overwritten by showModal) */
    WNDCLASSEX wcDlg    = {0};
    wcDlg.cbSize        = sizeof(wcDlg);
    wcDlg.lpfnWndProc   = DefWindowProc;
    wcDlg.hInstance     = hInst;
    wcDlg.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcDlg.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcDlg.lpszClassName = CLS_DIALOG;
    RegisterClassEx(&wcDlg);

    /* Create main window */
    HWND hMain = CreateWindowEx(
        0, CLS_MAIN, "SmartSpend — Personal Finance Tracker",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 720,
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
