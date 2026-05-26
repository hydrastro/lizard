# dynsys — GLFW/OpenGL dynamical-system visualizer with Dear ImGui + TPCAS
#
# Native Linux workflow:
#   nix develop
#   make
#   make run
#
# Windows cross-build workflow from NixOS:
#   nix develop .#windows
#   make windows
#
# The Windows build writes build/windows/dynsys.exe so it does not reuse
# native Linux object files.

CC         ?= cc
CXX        ?= c++
AR         ?= ar
RM         ?= rm -f
RMDIR      ?= rm -rf
MKDIR_P    ?= mkdir -p
INSTALL    ?= install
PKG_CONFIG ?= pkg-config

# Set TARGET_OS=windows directly, or use `make windows` / `make build-windows`.
TARGET_OS ?= native
HOST_TRIPLE := $(shell $(CC) -dumpmachine 2>/dev/null || echo unknown)
WINDOWS_BUILD := 0
ifeq ($(TARGET_OS),windows)
  WINDOWS_BUILD := 1
else ifneq (,$(findstring mingw,$(HOST_TRIPLE)))
  WINDOWS_BUILD := 1
else ifneq (,$(findstring windows,$(HOST_TRIPLE)))
  WINDOWS_BUILD := 1
endif

BUILD_ROOT ?= build
ifeq ($(WINDOWS_BUILD),1)
  EXEEXT ?= .exe
  BUILD_DIR := $(BUILD_ROOT)/windows
else
  EXEEXT ?=
  BUILD_DIR := $(BUILD_ROOT)
endif

SRC_DIR    := src
TPCAS_DIR  := vendor/tpcas
DS_DIR     := $(TPCAS_DIR)/vendor/ds/lib
OBJ_DIR    := $(BUILD_DIR)/obj
C_OBJ_DIR  := $(BUILD_DIR)/obj-c
CXX_OBJ_DIR:= $(BUILD_DIR)/obj-cxx
DEP_DIR    := $(BUILD_DIR)/dep
C_DEP_DIR  := $(BUILD_DIR)/dep-c
CXX_DEP_DIR:= $(BUILD_DIR)/dep-cxx
TARGET     := $(BUILD_DIR)/dynsys$(EXEEXT)
IR_TEST_TARGET := $(BUILD_DIR)/ir_smoke$(EXEEXT)
TEST_OBJ_DIR := $(BUILD_DIR)/obj-test

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

MODE ?= debug
PKGS ?= glfw3 glew cglm
IMGUI_DIR ?=

# Used by the recursive `make windows` target. Override these if your MinGW
# toolchain uses different names.
WIN_TRIPLE ?= x86_64-w64-mingw32
WIN_CC ?= $(WIN_TRIPLE)-gcc
WIN_CXX ?= $(WIN_TRIPLE)-g++
WIN_AR ?= $(WIN_TRIPLE)-ar
WIN_PKG_CONFIG ?= pkg-config
WIN_BUILD_DIR ?= $(BUILD_ROOT)/windows
WIN_TARGET ?= $(WIN_BUILD_DIR)/dynsys.exe

WARNINGS_C   := -Wall -Wextra -Wpedantic
WARNINGS_CXX := -Wall -Wextra
CSTD         := -std=c11
CXXSTD       := -std=c++17

PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PKGS) 2>/dev/null)
PKG_LIBS   := $(shell $(PKG_CONFIG) --libs $(PKGS) 2>/dev/null)

CPPFLAGS ?=
CPPFLAGS += -I$(SRC_DIR) -I$(TPCAS_DIR)/src -I$(TPCAS_DIR)/vendor/ds $(PKG_CFLAGS)
ifneq ($(strip $(IMGUI_DIR)),)
  CPPFLAGS += -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
endif
ifeq ($(WINDOWS_BUILD),1)
  CPPFLAGS += -D_WIN32_WINNT=0x0601
endif

CFLAGS ?=
CFLAGS += $(CSTD) $(WARNINGS_C)
CXXFLAGS ?=
CXXFLAGS += $(CXXSTD) $(WARNINGS_CXX)
LDFLAGS ?=
LDLIBS ?=

ifeq ($(WINDOWS_BUILD),1)
  # MinGW/Windows OpenGL + GLFW backend system libraries. pkg-config will
  # usually provide most dependency flags, but these system libraries are cheap
  # and make the link robust across MinGW package layouts.
  LDLIBS += $(PKG_LIBS) -lopengl32 -lgdi32 -limm32 -lole32 -luuid -lwinmm -lm
  WIN_STATIC_RUNTIME ?= 1
  ifeq ($(WIN_STATIC_RUNTIME),1)
    LDFLAGS += -static-libgcc -static-libstdc++
  endif
else
  LDLIBS += $(PKG_LIBS) -lGL -ldl -lm
endif

ifeq ($(MODE),release)
  CFLAGS += -O2 -DNDEBUG
  CXXFLAGS += -O2 -DNDEBUG
else ifeq ($(MODE),asan)
  CFLAGS += -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined
  CXXFLAGS += -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined
  LDFLAGS += -fsanitize=address,undefined
else
  CFLAGS += -O0 -g3
  CXXFLAGS += -O0 -g3
endif

DYNSYS_CPP_SRCS := $(SRC_DIR)/dynsys.cpp $(SRC_DIR)/expr_ir.cpp
DYNSYS_OBJS := $(patsubst %.cpp,$(CXX_OBJ_DIR)/%.o,$(DYNSYS_CPP_SRCS))
DYNSYS_DEPS := $(patsubst %.cpp,$(CXX_DEP_DIR)/%.d,$(DYNSYS_CPP_SRCS))

IR_TEST_CPP_SRCS := $(SRC_DIR)/expr_ir.cpp test/ir_smoke.cpp
IR_TEST_CPP_OBJS := $(patsubst %.cpp,$(TEST_OBJ_DIR)/%.o,$(IR_TEST_CPP_SRCS))

TPCAS_SRCS := \
  $(TPCAS_DIR)/src/arena.c \
  $(TPCAS_DIR)/src/ast.c \
  $(TPCAS_DIR)/src/lex.c \
  $(TPCAS_DIR)/src/pratt.c
DS_SRCS := \
  $(DS_DIR)/common.c \
  $(DS_DIR)/status.c \
  $(DS_DIR)/error.c \
  $(DS_DIR)/diagnostic.c \
  $(DS_DIR)/context.c \
  $(DS_DIR)/allocators.c
C_SRCS := $(TPCAS_SRCS) $(DS_SRCS)
C_OBJS := $(patsubst %.c,$(C_OBJ_DIR)/%.o,$(C_SRCS))
C_DEPS := $(patsubst %.c,$(C_DEP_DIR)/%.d,$(C_SRCS))
IR_TEST_C_OBJS := $(patsubst %.c,$(TEST_OBJ_DIR)/%.o,$(C_SRCS))
IR_TEST_OBJS := $(IR_TEST_CPP_OBJS) $(IR_TEST_C_OBJS)

IMGUI_CORE_SRCS := imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp
IMGUI_BACKEND_SRCS := imgui_impl_glfw.cpp imgui_impl_opengl3.cpp
IMGUI_CORE_OBJS := $(addprefix $(CXX_OBJ_DIR)/imgui/,$(IMGUI_CORE_SRCS:.cpp=.o))
IMGUI_BACKEND_OBJS := $(addprefix $(CXX_OBJ_DIR)/imgui/backends/,$(IMGUI_BACKEND_SRCS:.cpp=.o))
IMGUI_OBJS := $(IMGUI_CORE_OBJS) $(IMGUI_BACKEND_OBJS)
IMGUI_DEPS := $(patsubst $(CXX_OBJ_DIR)/%.o,$(CXX_DEP_DIR)/%.d,$(IMGUI_OBJS))

OBJS := $(DYNSYS_OBJS) $(C_OBJS) $(IMGUI_OBJS)
DEPS := $(DYNSYS_DEPS) $(C_DEPS) $(IMGUI_DEPS)

.PHONY: all build check-deps check-legacy prune-legacy run headless headless-ast headless-smoke bench test ir-smoke debug release asan windows build-windows clean distclean install uninstall format print-vars help

all: check-deps check-legacy $(TARGET)

build: all

windows build-windows:
	$(MAKE) TARGET_OS=windows \
	  BUILD_DIR="$(WIN_BUILD_DIR)" \
	  TARGET="$(WIN_TARGET)" \
	  CC="$(WIN_CC)" \
	  CXX="$(WIN_CXX)" \
	  AR="$(WIN_AR)" \
	  PKG_CONFIG="$(WIN_PKG_CONFIG)"

check-deps:
	@$(PKG_CONFIG) --exists $(PKGS) || { \
	  echo "error: missing pkg-config dependencies: $(PKGS)" >&2; \
	  if [ "$(WINDOWS_BUILD)" = "1" ]; then \
	    echo "hint: run 'nix develop .#windows' first, then run 'make windows' again." >&2; \
	  else \
	    echo "hint: run 'nix develop' first, then run 'make' again." >&2; \
	  fi; \
	  exit 1; \
	}
	@if [ -z "$(strip $(IMGUI_DIR))" ]; then \
	  echo "error: IMGUI_DIR is not set" >&2; \
	  echo "hint: run 'nix develop' or 'nix develop .#windows' first; the flake exports IMGUI_DIR." >&2; \
	  echo "      or run: make IMGUI_DIR=/path/to/imgui" >&2; \
	  exit 1; \
	fi
	@test -f "$(IMGUI_DIR)/imgui.cpp" || { \
	  echo "error: IMGUI_DIR does not point at a Dear ImGui source tree: $(IMGUI_DIR)" >&2; \
	  exit 1; \
	}

check-legacy:
	@if [ -f "$(SRC_DIR)/dynsys.c" ]; then \
	  echo "warning: legacy $(SRC_DIR)/dynsys.c exists but is intentionally ignored; run 'make prune-legacy CLEAN_APPLY=1' to remove old FreeType code." >&2; \
	fi

prune-legacy:
	@if [ "$(CLEAN_APPLY)" = "1" ]; then \
	  scripts/prune-legacy.sh --apply; \
	else \
	  scripts/prune-legacy.sh; \
	fi

$(TARGET): $(OBJS)
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(C_OBJ_DIR)/%.o: %.c
	@$(MKDIR_P) $(dir $@) $(dir $(patsubst $(C_OBJ_DIR)/%.o,$(C_DEP_DIR)/%.d,$@))
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF $(patsubst $(C_OBJ_DIR)/%.o,$(C_DEP_DIR)/%.d,$@) -c $< -o $@

$(CXX_OBJ_DIR)/%.o: %.cpp
	@$(MKDIR_P) $(dir $@) $(dir $(patsubst $(CXX_OBJ_DIR)/%.o,$(CXX_DEP_DIR)/%.d,$@))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF $(patsubst $(CXX_OBJ_DIR)/%.o,$(CXX_DEP_DIR)/%.d,$@) -c $< -o $@

$(CXX_OBJ_DIR)/imgui/%.o: $(IMGUI_DIR)/%.cpp
	@$(MKDIR_P) $(dir $@) $(dir $(patsubst $(CXX_OBJ_DIR)/%.o,$(CXX_DEP_DIR)/%.d,$@))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF $(patsubst $(CXX_OBJ_DIR)/%.o,$(CXX_DEP_DIR)/%.d,$@) -c $< -o $@

$(CXX_OBJ_DIR)/imgui/backends/%.o: $(IMGUI_DIR)/backends/%.cpp
	@$(MKDIR_P) $(dir $@) $(dir $(patsubst $(CXX_OBJ_DIR)/%.o,$(CXX_DEP_DIR)/%.d,$@))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF $(patsubst $(CXX_OBJ_DIR)/%.o,$(CXX_DEP_DIR)/%.d,$@) -c $< -o $@

test: ir-smoke

ir-smoke: $(IR_TEST_TARGET)
	./$(IR_TEST_TARGET)

$(IR_TEST_TARGET): $(IR_TEST_OBJS)
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(IR_TEST_OBJS) -lm

$(TEST_OBJ_DIR)/%.o: %.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_OBJ_DIR)/%.o: %.cpp
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: all
	./$(TARGET)

headless: all
	./$(TARGET) --headless $(ARGS)

headless-ast: all
	./$(TARGET) --headless $(ARGS) --use-ast

headless-smoke: all
	./$(TARGET) --headless examples/lorenz.dyn --steps 10000

bench:
	$(MAKE) MODE=release
	./$(TARGET) --headless examples/lorenz.dyn --steps 200000
	./$(TARGET) --headless examples/lorenz.dyn --steps 200000 --use-ast

debug:
	$(MAKE) MODE=debug

release:
	$(MAKE) MODE=release

asan:
	$(MAKE) MODE=asan

install: all
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)/dynsys$(EXEEXT)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/dynsys $(DESTDIR)$(BINDIR)/dynsys.exe

clean:
	$(RMDIR) $(BUILD_ROOT)

distclean: clean
	$(RM) result

format:
	clang-format -i $(SRC_DIR)/*.cpp $(SRC_DIR)/*.h test/*.cpp

print-vars:
	@echo "CC=$(CC)"
	@echo "CXX=$(CXX)"
	@echo "AR=$(AR)"
	@echo "PKG_CONFIG=$(PKG_CONFIG)"
	@echo "HOST_TRIPLE=$(HOST_TRIPLE)"
	@echo "TARGET_OS=$(TARGET_OS)"
	@echo "WINDOWS_BUILD=$(WINDOWS_BUILD)"
	@echo "MODE=$(MODE)"
	@echo "BUILD_DIR=$(BUILD_DIR)"
	@echo "TARGET=$(TARGET)"
	@echo "C_OBJ_DIR=$(C_OBJ_DIR)"
	@echo "CXX_OBJ_DIR=$(CXX_OBJ_DIR)"
	@echo "PKGS=$(PKGS)"
	@echo "IMGUI_DIR=$(IMGUI_DIR)"
	@echo "PKG_CFLAGS=$(PKG_CFLAGS)"
	@echo "PKG_LIBS=$(PKG_LIBS)"
	@echo "LDLIBS=$(LDLIBS)"

help:
	@echo "Targets:"
	@echo "  make                 build native $(TARGET)"
	@echo "  make run             build and run native executable"
	@echo "  make windows         cross-build build/windows/dynsys.exe"
	@echo "  make build-windows   same as make windows"
	@echo "  make test            build and run standalone IR smoke test"
	@echo "  make headless        run headless; pass ARGS='examples/lorenz.dyn --steps 10000'"
	@echo "  make bench           release IR/AST headless comparison on Lorenz"
	@echo "  make release         optimized native build"

-include $(DEPS)
