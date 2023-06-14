CC = gcc
CPP = g++
GLSLC = glslc
STRIP = strip
CFLAGS = -Ofast -g -Wstrict-aliasing -Ideps -I/usr/include/freetype2
CPPFLAGS =                               \
	-std=gnu++20                         \
	-Wno-deprecated-enum-enum-conversion \
	-DDEBUG                              \
	-DGLM_FORCE_LEFT_HANDED              \
	-DGLM_FORCE_DEPTH_ZERO_TO_ONE        \
	-DGLM_EXT_INCLUDED                   \
	$(CFLAGS)
LDFLAGS = -lglfw -lvulkan -lfreetype -lnoise -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lm
GLSLCFLAGS = -O --target-env=vulkan1.2
BIN = build/voksel

# ifeq ($(CROSS), mingw-w64)
# 	CC = x86_64-w64-mingw32-gcc
# 	CPP = x86_64-w64-mingw32-g++
# 	STRIP = x86_64-w64-mingw32-strip
# 	LDFLAGS = -static -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32 -lssp
# 	BIN = voksel.exe
# endif

SRC_CPP_FILES := $(shell find ./src -type f -name '*.cpp')
DEPS_CPP_FILES := $(shell find ./deps -type f -name '*.cpp')
DEPS_C_FILES := $(shell find ./deps -type f -name '*.c')
RES_VERT_GLSL_FILES := $(shell find ./res -type f -name '*.vert.glsl')
RES_FRAG_GLSL_FILES := $(shell find ./res -type f -name '*.frag.glsl')
RES_COMP_GLSL_FILES := $(shell find ./res -type f -name '*.comp.glsl')
OBJ := $(SRC_CPP_FILES:./src/%.cpp=build/src/%.o)
OBJ += $(DEPS_CPP_FILES:./deps/%.cpp=build/deps/%.o)
OBJ += $(DEPS_C_FILES:./deps/%.c=build/deps/%.o)
SPV := $(RES_VERT_GLSL_FILES:./res/%.vert.glsl=build/res/%.vert.spv)
SPV += $(RES_FRAG_GLSL_FILES:./res/%.frag.glsl=build/res/%.frag.spv)
SPV += $(RES_COMP_GLSL_FILES:./res/%.comp.glsl=build/res/%.comp.spv)

.PHONY: all run res clean cleansrc

all: $(BIN) res

run: $(BIN) res
	@echo "[RUN] $@"
	@cd build && mangohud ../$(BIN)

res: $(SPV)
	@echo "[RES]"
	@cp -r res/textures build/res
	@cp -r res/thirdparty build/res

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

build/res/%.vert.spv: res/%.vert.glsl
	@echo "[GLSLC]  $< | $@"
	@mkdir -p $(shell dirname $@)
	@$(GLSLC) $(GLSLCFLAGS) -fshader-stage=vert $< -o $@

build/res/%.frag.spv: res/%.frag.glsl
	@echo "[GLSLC]  $< | $@"
	@mkdir -p $(shell dirname $@)
	@$(GLSLC) $(GLSLCFLAGS) -fshader-stage=frag $< -o $@

build/res/%.comp.spv: res/%.comp.glsl
	@echo "[GLSLC]  $< | $@"
	@mkdir -p $(shell dirname $@)
	@$(GLSLC) $(GLSLCFLAGS) -fshader-stage=comp $< -o $@

clean:
	@echo "[CLEAN]"
	@rm -rf build $(BIN)

cleansrc:
	@echo "[CLEAN SOURCES]"
	@rm -rf build/src $(BIN)

cleanres:
	@echo "[CLEAN RESOURCES]"
	@rm -rf build/res
