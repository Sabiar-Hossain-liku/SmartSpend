CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LIBS = -mwindows -lcomctl32 -lgdi32

CORE_SRCS = src/common.c src/transaction.c src/category.c \
            src/insights.c src/budget.c src/storage.c src/export.c \
            src/cli.c

GUI_SRCS = src/main.c src/gui.c src/chart.c $(CORE_SRCS)
CLI_SRCS = src/cli_main.c $(CORE_SRCS)

TARGET = smartspend.exe
CLI_TARGET = smartspend_cli.exe
TEST_TARGET = test_runner.exe

all: $(TARGET) $(CLI_TARGET)

$(TARGET): $(GUI_SRCS)
	$(CC) $(CFLAGS) $(GUI_SRCS) $(LIBS) -o $(TARGET)

$(CLI_TARGET): $(CLI_SRCS)
	$(CC) $(CFLAGS) $(CLI_SRCS) -o $(CLI_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): tests/test_main.c $(CORE_SRCS)
	$(CC) $(CFLAGS) tests/test_main.c $(CORE_SRCS) -o $(TEST_TARGET)

sample: $(TARGET)
	./$(TARGET) --sample

clean:
	-cmd /c del /f /q $(TARGET) $(CLI_TARGET) $(TEST_TARGET) *.o 2>nul

.PHONY: all test sample clean
