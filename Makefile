CC = gcc
CPP = g++
STRIP = strip
CFLAGS = -Ofast -g -Wstrict-aliasing -Ideps -Ideps/luajit-2.1 -DSOL_LUAJIT=1 
CPPFLAGS = -std=gnu++20 -Wno-deprecated-enum-enum-conversion -DGLM_MESSAGES $(CFLAGS)
LDFLAGS = -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -lluajit-5.1
BIN = voksel

ifeq ($(CROSS), mingw-w64)
	CC = x86_64-w64-mingw32-gcc
	CPP = x86_64-w64-mingw32-g++
	STRIP = x86_64-w64-mingw32-strip
	LDFLAGS = -static -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32 -lssp -lluajit
	BIN = voksel.exe
endif

SRC_CPP_FILES := $(shell find ./src -type f -name '*.cpp')
DEPS_CPP_FILES := $(shell find ./deps -type f -name '*.cpp')
DEPS_C_FILES := $(shell find ./deps -type f -name '*.c')
OBJ := $(SRC_CPP_FILES:./src/%.cpp=build/src/%.o)
OBJ += $(DEPS_CPP_FILES:./deps/%.cpp=build/deps/%.o)
OBJ += $(DEPS_C_FILES:./deps/%.c=build/deps/%.o)

.PHONY: all run clean cleansrc

all: $(BIN)

run: $(BIN)
	@echo "[RUN] $@"
	@GALLIUM_HUD="simple,fps" ./$(BIN)

$(BIN): $(OBJ)
	@echo "[LD]  $@"
	@$(CPP) $^ -o $@ $(LDFLAGS)

build/src/%.o: src/%.cpp
	@echo "[CPP] $< | $@"
	@mkdir -p $(shell dirname $@)
	@$(CPP) $(CPPFLAGS) -c $< -o $@

build/deps/%.o: deps/%.cpp
	@echo "[CPP] $< | $@"
	@mkdir -p $(shell dirname $@)
	@$(CPP) $(CPPFLAGS) -c $< -o $@

build/deps/%.o: deps/%.c
	@echo "[CC]  $< | $@"
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "[CLEAN]"
	@rm -rf build $(BIN)

cleansrc:
	@echo "[CLEAN SOURCES]"
	@rm -rf build/src $(BIN)
