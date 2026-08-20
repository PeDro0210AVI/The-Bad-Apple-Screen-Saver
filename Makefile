SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
BIN_DIR := bin

BIN = $(shell echo $(BIN_NAME))
SOURCES := $(shell find src -name "*.c")
OBJECTS := $(SOURCES:.c=.o)

CFLAGS := -I$(INCLUDE_DIR)  -g

build: dir $(OBJECTS)
	@$(CC) -o $(BIN_DIR)/$(BIN) $(addprefix $(BUILD_DIR)/,$(OBJECTS))


$(OBJECTS): %.o : %.c
	@$(CC) $(CFLAGS) -c $< -o $(BUILD_DIR)/$@

clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR)

dir:
	find $(SRC_DIR) -type d | xargs -I{} mkdir -p $(BUILD_DIR)/{}
	mkdir -p $(BIN_DIR)
