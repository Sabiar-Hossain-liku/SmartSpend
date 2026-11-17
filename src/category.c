#include "category.h"
#include <string.h>

static const char *defaultCats[] = {
    "Food", "Transport", "Utilities", "Housing",
    "Entertainment", "Healthcare", "Education",
    "Shopping", "Personal", "Other"
};

void initCategories(char categories[][MAX_CAT_LEN], int *count) {
    *count = 0;
    int n = (int)(sizeof(defaultCats) / sizeof(defaultCats[0]));
    for (int i = 0; i < n && *count < MAX_CATEGORIES; i++) {
        strncpy(categories[*count], defaultCats[i], MAX_CAT_LEN - 1);
        categories[(*count)++][MAX_CAT_LEN - 1] = '\0';
    }
}

int addCategory(char categories[][MAX_CAT_LEN], int *count, const char *name) {
    if (*count >= MAX_CATEGORIES) return 0;
    if (findCategory(categories, *count, name) >= 0) return 1;
    strncpy(categories[*count], name, MAX_CAT_LEN - 1);
    categories[(*count)++][MAX_CAT_LEN - 1] = '\0';
    return 1;
}

int findCategory(const char categories[][MAX_CAT_LEN], int count, const char *name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(categories[i], name) == 0) return i;
    }
    return -1;
}

/* Add any categories from transactions that are not already in the list. */
void collectCategories(const TransactionList *list,
                       char categories[][MAX_CAT_LEN], int *count) {
    for (int i = 0; i < list->count; i++) {
        const char *cat = list->items[i].category;
        if (findCategory(categories, *count, cat) < 0)
            addCategory(categories, count, cat);
    }
}
