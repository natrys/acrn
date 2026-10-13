DEBUG ?= 0
NAME := tinnitus_therapy
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
LIB_DIR := lib
BIN_DIR := bin

LIB := $(wildcard $(LIB_DIR)/*.c)
SRC := $(wildcard $(SRC_DIR)/*.c)

OBJS := $(patsubst $(LIB_DIR)/%.c,$(BUILD_DIR)/%.o, $(LIB))
DEPS := $(OBJS:.o=.d)

CC := gcc
FORMATTER := clang-format

DVARS := -DMA_NO_ENCODING=1 -DMA_NO_DECODING=1
DEPFLAGS := -MMD -MP
CFLAGS := -D _GNU_SOURCE -D __STDC_WANT_LIB_EXT1__ -Wall -Wextra -Werror -Wno-error=stringop-overflow -pedantic -pipe -I include/
LDFLAGS := -ldl -lm -lpthread

ifeq ($(DEBUG), 1)
	CFLAGS := $(CFLAGS) -g -O0 -Oz -DVERBOSE=1
else
	CFLAGS := $(CFLAGS) -O3 -DVERBOSE=1
endif

$(BIN_DIR)/$(NAME): $(SRC) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(LIB_DIR)/%.c
	$(CC) $(CFLAGS) $(DVARS) $(DEPFLAGS) -o $@ -c $<

format:
	$(FORMATTER) -style=file -i $(SRC_DIR)/*

clean:
	rm -f $(BIN_DIR)/$(NAME) $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d

all: $(BIN_DIR)/$(NAME)

.PHONY: all clean

-include $(DEPS)
