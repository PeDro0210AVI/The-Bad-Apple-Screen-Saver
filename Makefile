SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
BIN_DIR := bin

BIN_NAME ?= screensaver
BIN = $(shell echo $(BIN_NAME))
SOURCES := $(shell find src -name "*.c")
OBJECTS := $(SOURCES:.c=.o)

UNAME_S := $(shell uname -s)
OMPFLAGS := -fopenmp

ifeq ($(UNAME_S),Darwin)
	# El clang de Apple no acepta -fopenmp directo: necesita libomp de
	# Homebrew (brew install libomp) y pasar la bandera al preprocesador.
	LIBOMP ?= $(shell brew --prefix libomp 2>/dev/null)
  ifeq ($(LIBOMP),)
    $(error No se encontro libomp. Instalalo con: brew install libomp)
  endif
	OMPFLAGS := -Xpreprocessor -fopenmp -I$(LIBOMP)/include
	LDLIBS := -framework OpenGL -framework GLUT -L$(LIBOMP)/lib -lomp
else ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S)))
	LDLIBS := -lfreeglut -lopengl32 -lglu32 -lgomp
else
	LDLIBS := -lGL -lGLU -lglut -lgomp -lm
endif

CFLAGS := -I$(INCLUDE_DIR) -O2 -g $(OMPFLAGS) -Wall -Wextra -DGL_SILENCE_DEPRECATION

# Parametros por defecto para "make run"
N ?= 3000
MODO ?= par2

build: dir $(OBJECTS)
	@$(CC) -o $(BIN_DIR)/$(BIN) $(addprefix $(BUILD_DIR)/,$(OBJECTS)) $(LDLIBS)

$(OBJECTS): %.o : %.c $(wildcard $(INCLUDE_DIR)/*.h)
	@$(CC) $(CFLAGS) -c $< -o $(BUILD_DIR)/$@

run: build
	./$(BIN_DIR)/$(BIN) $(N) --modo $(MODO)

# Bitacora de pruebas: 10 corridas por configuracion -> results/benchmark.csv
bench: build
	./scripts/benchmark.sh

clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR)

dir:
	@find $(SRC_DIR) -type d | xargs -I{} mkdir -p $(BUILD_DIR)/{}
	@mkdir -p $(BIN_DIR)

.PHONY: build run bench clean dir
