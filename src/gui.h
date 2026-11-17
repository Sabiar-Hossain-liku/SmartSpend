#ifndef GUI_H
#define GUI_H

#define WIN32_LEAN_AND_MEAN
#undef  UNICODE
#undef  _UNICODE
#include <windows.h>
#include <commctrl.h>
#include "common.h"

/* ── Page identifiers ─────────────────────────────────── */
#define PAGE_DASHBOARD    0
#define PAGE_TRANSACTIONS 1
#define PAGE_BUDGET       2
#define PAGE_INSIGHTS     3
#define PAGE_FORECAST     4   /* Forecast & What-If Savings Simulator */
#define PAGE_EXPORT       5
#define PAGE_COUNT        6

/* Legacy alias */
#define PAGE_WHATIF       PAGE_FORECAST

/* ── Control IDs ──────────────────────────────────────── */
#define IDC_NAV_FIRST         100   /* 100-105 reserved for nav buttons */
#define IDC_BTN_ADD           201
#define IDC_BTN_DELETE        202
#define IDC_BTN_SETBUDGET     203
#define IDC_BTN_SIMULATE      204
#define IDC_BTN_EXPORTCSV     205
#define IDC_BTN_EXPORTRPT     206

/* Forecast / Simulator interactive controls */
#define IDC_COMBO_FC_CAT      210
#define IDC_EDIT_FC_CUT       211
#define IDC_BTN_FC_CALC       212
#define IDC_BTN_FC_10         213
#define IDC_BTN_FC_20         214
#define IDC_BTN_FC_30         215
#define IDC_BTN_FC_50         216

#define IDC_LIST_TRANS        301

/* ── Colours ──────────────────────────────────────────── */
#define CLR_BG          RGB(248, 250, 252)
#define CLR_SIDEBAR     RGB(15,  23,  42)
#define CLR_SIDEBAR_HOV RGB(30,  41,  59)
#define CLR_ACCENT      RGB(79,  70,  229)
#define CLR_ACCENT_HOV  RGB(67,  56,  202)
#define CLR_GREEN       RGB(22,  163, 74)
#define CLR_GREEN_LIGHT RGB(220, 252, 231)
#define CLR_RED         RGB(220, 38,  38)
#define CLR_RED_LIGHT   RGB(254, 226, 226)
#define CLR_ORANGE      RGB(234, 88,  12)
#define CLR_ORANGE_LGT  RGB(254, 243, 199)
#define CLR_WHITE       RGB(255, 255, 255)
#define CLR_TEXT_DARK   RGB(15,  23,  42)
#define CLR_TEXT_MUTED  RGB(100, 116, 139)
#define CLR_TEXT_LIGHT  RGB(148, 163, 184)
#define CLR_CARD        RGB(255, 255, 255)
#define CLR_BORDER      RGB(226, 232, 240)
#define CLR_CHART1      RGB(99,  102, 241)
#define CLR_CHART2      RGB(167, 139, 250)

/* ── Layout ───────────────────────────────────────────── */
#define SIDEBAR_W  185
#define HEADER_H    60
#define STATUS_H    26
#define NAV_BTN_H   46
#define CARD_R       8

/* ── Global app state ─────────────────────────────────── */
typedef struct {
    TransactionList txns;
    BudgetList      budgets;
    char            categories[MAX_CATEGORIES][MAX_CAT_LEN];
    int             catCount;
    int             currentPage;
    HWND            hMain;
    HWND            hContent;
    HWND            hList;
    HWND            hStatus;
    HWND            hNavBtns[PAGE_COUNT];

    /* Action buttons inside hContent */
    HWND            hBtnAdd;
    HWND            hBtnDel;
    HWND            hBtnSetBudget;
    HWND            hBtnSimulate;
    HWND            hBtnExportCSV;
    HWND            hBtnExportRpt;

    /* Interactive Forecast & Simulator controls inside hContent */
    HWND            hFcCatCombo;
    HWND            hFcCutEdit;
    HWND            hFcCalcBtn;
    HWND            hFcBtn10;
    HWND            hFcBtn20;
    HWND            hFcBtn30;
    HWND            hFcBtn50;

    /* Forecast interactive state */
    char            fcSelectedCat[MAX_CAT_LEN];
    double          fcCutPercent;
    double          fcMonthlySavings;
    double          fcAnnualSavings;
    int             fcCalculated;

    /* Reusable shared fonts */
    HFONT           hFontRegular;
    HFONT           hFontBold;
    HFONT           hFontTitle;
    HFONT           hFontHeading;
    HFONT           hFontSmall;
    HFONT           hFontStat;
} AppState;

extern AppState g_app;

/* ── Shared helpers ───────────────────────────────────── */
void  initFonts(void);
void  cleanupFonts(void);
HFONT makeFont(int size, int bold);
void  drawRoundRect(HDC hdc, int x, int y, int w, int h, int r,
                    COLORREF fill, COLORREF border);
void  drawCard(HDC hdc, int x, int y, int w, int h,
               const char *title, const char *value, const char *sub,
               COLORREF accent);
void  setStatus(const char *msg);
void  refreshList(void);
void  updatePageLayout(void);
void  runForecastCalculation(void);

#endif /* GUI_H */
