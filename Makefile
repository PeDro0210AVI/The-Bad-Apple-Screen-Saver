SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
BIN_DIR := bin

BIN_NAME ?= screensaver
BIN = $(shell echo $(BIN_NAME))
SOURCES := $(shell find src -name "*.c")
OBJECTS := $(SOURCES:.c=.o)

CFLAGS := -I$(INCLUDE_DIR) -g -fopenmp -Wall

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDLIBS := -framework OpenGL -framework GLUT -lomp
else ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S)))
	LDLIBS := -lfreeglut -lopengl32 -lglu32 -lgomp
else
	LDLIBS := -lGL -lGLU -lglut -lgomp -lm
endif

build: dir $(OBJECTS)
	@$(CC) -o $(BIN_DIR)/$(BIN) $(addprefix $(BUILD_DIR)/,$(OBJECTS)) $(LDLIBS)


$(OBJECTS): %.o : %.c
	@$(CC) $(CFLAGS) -c $< -o $(BUILD_DIR)/$@

clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR)

dir:
	find $(SRC_DIR) -type d | xargs -I{} mkdir -p $(BUILD_DIR)/{}
	mkdir -p $(BIN_DIR)
