CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LIBS = -mwindows -lcomctl32 -lgdi32

SRCS = src/main.c src/common.c src/transaction.c src/category.c \
       src/insights.c src/budget.c src/storage.c src/export.c \
       src/gui.c src/chart.c

CORE_SRCS = src/common.c src/transaction.c src/category.c \
            src/insights.c src/budget.c src/storage.c src/export.c

TARGET = smartspend.exe
TEST_TARGET = test_runner.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LIBS) -o $(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): tests/test_main.c $(CORE_SRCS)
	$(CC) $(CFLAGS) tests/test_main.c $(CORE_SRCS) -o $(TEST_TARGET)

sample: $(TARGET)
	./$(TARGET) --sample

clean:
	-cmd /c del /f /q $(TARGET) $(TEST_TARGET) *.o 2>nul

.PHONY: all test sample clean
