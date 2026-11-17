#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "../src/common.h"
#include "../src/transaction.h"
#include "../src/category.h"
#include "../src/insights.h"
#include "../src/budget.h"
#include "../src/storage.h"
#include "../src/export.h"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define RUN_TEST(fn) do { \
    printf("Running %s... ", #fn); \
    g_tests_run++; \
    fn(); \
    g_tests_passed++; \
    printf("PASS\n"); \
} while(0)

void test_common_date_utils(void) {
    char buf[32];
    time_t now = time(NULL);
    formatDate(now, buf, sizeof(buf));
    assert(strlen(buf) == 10);
    assert(buf[2] == '/' && buf[5] == '/');

    struct tm tm_sample = {0};
    tm_sample.tm_year = 126; /* 2026 */
    tm_sample.tm_mon  = 10;  /* Nov */
    tm_sample.tm_mday = 17;
    time_t t1 = mktime(&tm_sample);

    tm_sample.tm_mday = 20;
    time_t t2 = mktime(&tm_sample);
    assert(sameMonth(t1, t2) == 1);
    assert(inMonth(t1, 10, 126) == 1);
    assert(inMonth(t1, 9, 126) == 0);

    tm_sample.tm_mon = 11; /* Dec */
    time_t t3 = mktime(&tm_sample);
    assert(sameMonth(t1, t3) == 0);
}

void test_transaction_crud(void) {
    TransactionList list;
    initList(&list);
    assert(list.count == 0);
    assert(list.capacity >= INITIAL_CAP);

    time_t now = time(NULL);
    int ok = addTransaction(&list, "Coffee", "Food", 150.0, now, 0);
    assert(ok == 1);
    assert(list.count == 1);
    assert(strcmp(list.items[0].description, "Coffee") == 0);
    assert(strcmp(list.items[0].category, "Food") == 0);
    assert(list.items[0].amount == 150.0);

    int id1 = list.items[0].id;
    Transaction *found = findTransaction(&list, id1);
    assert(found != NULL);
    assert(strcmp(found->description, "Coffee") == 0);

    addTransaction(&list, "Metro", "Transport", 80.0, now, 1);
    assert(list.count == 2);
    int id2 = list.items[1].id;

    assert(deleteTransaction(&list, id1) == 1);
    assert(list.count == 1);
    assert(findTransaction(&list, id1) == NULL);
    assert(findTransaction(&list, id2) != NULL);

    freeList(&list);
    assert(list.items == NULL);
    assert(list.count == 0);
}

void test_category_operations(void) {
    char cats[MAX_CATEGORIES][MAX_CAT_LEN];
    int count = 0;

    initCategories(cats, &count);
    assert(count > 0);
    assert(findCategory(cats, count, "Food") >= 0);
    assert(findCategory(cats, count, "NonExistentCat") == -1);

    int prevCount = count;
    addCategory(cats, &count, "CustomCat");
    assert(count == prevCount + 1);
    assert(findCategory(cats, count, "CustomCat") >= 0);

    /* duplicate add should not increment */
    addCategory(cats, &count, "CustomCat");
    assert(count == prevCount + 1);
}

void test_insights_and_whatif(void) {
    TransactionList list;
    initList(&list);
    time_t now = time(NULL);

    addTransaction(&list, "Meal 1", "Food", 500.0, now, 0);
    addTransaction(&list, "Meal 2", "Food", 300.0, now, 0);
    addTransaction(&list, "Bus", "Transport", 200.0, now, 1);

    assert(totalSpent(&list) == 1000.0);
    assert(spentThisMonth(&list) == 1000.0);
    assert(spentByCategory(&list, "Food") == 800.0);
    assert(spentByCategory(&list, "Transport") == 200.0);
    assert(strcmp(topCategory(&list), "Food") == 0);

    double savings = whatIfSavings(&list, "Food", 20.0);
    assert(savings > 0.0);

    /* Test recurring expansion */
    int beforeCount = list.count;
    applyRecurring(&list);
    /* already in this month, should not duplicate */
    assert(list.count == beforeCount);

    /* Add a recurring transaction from last month */
    time_t lastMonth = now - (40 * 86400);
    addTransaction(&list, "Internet", "Utilities", 700.0, lastMonth, 1);
    beforeCount = list.count;
    applyRecurring(&list);
    /* Should add an entry for this month */
    assert(list.count == beforeCount + 1);
    assert(strcmp(list.items[list.count - 1].description, "Internet") == 0);
    assert(sameMonth(list.items[list.count - 1].date, now) == 1);

    freeList(&list);
}

void test_budget_operations(void) {
    BudgetList budgets;
    budgets.count = 0;

    TransactionList txns;
    initList(&txns);
    time_t now = time(NULL);

    addTransaction(&txns, "Groceries", "Food", 4500.0, now, 0);
    setBudget(&budgets, "Food", 4000.0);

    assert(getBudgetLimit(&budgets, "Food") == 4000.0);
    assert(isOverBudget(&budgets, &txns, "Food") == 1);

    setBudget(&budgets, "Food", 5000.0);
    assert(isOverBudget(&budgets, &txns, "Food") == 0);

    freeList(&txns);
}

void test_export_operations(void) {
    TransactionList txns;
    initList(&txns);
    BudgetList budgets;
    budgets.count = 0;
    time_t now = time(NULL);

    addTransaction(&txns, "Book", "Education", 250.0, now, 0);
    setBudget(&budgets, "Education", 500.0);

    system("if not exist tests_output mkdir tests_output 2>nul");
    int csvOk = exportCSV(&txns, "tests_output/test.csv");
    assert(csvOk == 1);

    int rptOk = exportReport(&txns, &budgets, "tests_output/test_report.txt");
    assert(rptOk == 1);

    freeList(&txns);
}

int main(void) {
    printf("=== SmartSpend Unit Tests ===\n");
    RUN_TEST(test_common_date_utils);
    RUN_TEST(test_transaction_crud);
    RUN_TEST(test_category_operations);
    RUN_TEST(test_insights_and_whatif);
    RUN_TEST(test_budget_operations);
    RUN_TEST(test_export_operations);

    printf("=============================\n");
    printf("Summary: %d / %d tests passed successfully.\n", g_tests_passed, g_tests_run);
    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
