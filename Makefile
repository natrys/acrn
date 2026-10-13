DEBUG ?= 0
NAME := tinnitus_therapy
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
LIB_DIR := lib
BIN_DIR := bin

LIB := $(wildcard $(LIB_DIR)/*.c)
SRC := $(wildcard $(SRC_DIR)/*.c)

LIB_OBJS := $(patsubst $(LIB_DIR)/%.c,$(BUILD_DIR)/%.o, $(LIB))
SRC_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o, $(SRC))
OBJS := $(SRC_OBJS) $(LIB_OBJS)
DEPS := $(OBJS:.o=.d)

CC := gcc
FORMATTER := clang-format

DVARS := -DMA_NO_ENCODING=1 -DMA_NO_DECODING=1
DEPFLAGS := -MMD -MP
# GCC reports a bogus stringop-overflow in include/miniaudio.h at -O3; do not remove.
CFLAGS := -D_GNU_SOURCE -D__STDC_WANT_LIB_EXT1__ -Wall -Wextra -Werror -Wno-error=stringop-overflow -pedantic -pipe -I$(INCLUDE_DIR)
LDFLAGS := -ldl -lm -lpthread

ifeq ($(DEBUG), 1)
	CFLAGS := $(CFLAGS) -g -O0 -DVERBOSE=1
else
	CFLAGS := $(CFLAGS) -O3 -DVERBOSE=1
endif

.DELETE_ON_ERROR:

all: $(BIN_DIR)/$(NAME)

$(BIN_DIR)/$(NAME): $(OBJS) | $(BIN_DIR) $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(LIB_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DVARS) $(DEPFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DVARS) $(DEPFLAGS) -o $@ -c $<

$(BIN_DIR) $(BUILD_DIR):
	mkdir -p $@

format:
	$(FORMATTER) -style=file -i $(SRC_DIR)/*

clean:
	rm -f $(BIN_DIR)/$(NAME) $(OBJS) $(DEPS)

.PHONY: all clean format

-include $(DEPS)
