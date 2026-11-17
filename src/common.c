#include "common.h"
#include <stdio.h>
#include <string.h>

/* Format a time_t as "DD/MM/YYYY" into buf (must be at least 11 bytes). */
void formatDate(time_t t, char *buf, int bufLen) {
    struct tm *tm_info = localtime(&t);
    if (tm_info == NULL) {
        strncpy(buf, "00/00/0000", bufLen - 1);
        buf[bufLen - 1] = '\0';
        return;
    }
    snprintf(buf, bufLen, "%02d/%02d/%04d",
             tm_info->tm_mday,
             tm_info->tm_mon + 1,
             tm_info->tm_year + 1900);
}

/* Return 1 if two time_t values fall in the same calendar month. */
int sameMonth(time_t a, time_t b) {
    struct tm *ta = localtime(&a);
    int ma = ta->tm_mon, ya = ta->tm_year;
    struct tm *tb = localtime(&b);
    return (ma == tb->tm_mon && ya == tb->tm_year);
}

/* Return 1 if t is in the given month (0-based) and year (tm_year). */
int inMonth(time_t t, int month, int year) {
    struct tm *tm_info = localtime(&t);
    return (tm_info->tm_mon == month && tm_info->tm_year == year);
}
