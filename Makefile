# Combined Makefile (Windows + Linux)

CXX := g++
CC := gcc

TARGET_DEBUG := bin/dbg/u8-emu-frontend-cpp.exe
OBJDIR_DEBUG := obj/dbg/
CXXFLAGS_DEBUG := -Wall -std=c++23 -fconcepts-diagnostics-depth=2 $(CXXFLAGS_EX) -g -O0 -DBCDDEBUG -DESSTOPDEBUG
LDFLAGS_DEBUG := -static-libstdc++ -lSDL3 -lSDL3_image -luser32 -lgdi32 -ldxguid -ldbghelp -lwinmm

SRCS_CPP_DEBUG := \
    config/settings.cpp \
    disas/disas.cpp \
    gui/disasdisp.cpp \
    gui/rominfo.cpp \
    gui/startupui.cpp \
    imgui/imgui.cpp \
    imgui/imgui_draw.cpp \
    imgui/imgui_impl_sdl3.cpp \
    imgui/imgui_impl_sdlrenderer3.cpp \
    imgui/imgui_tables.cpp \
    imgui/imgui_widgets.cpp \
    labeltool/labeltool.cpp \
    main.cpp \
    mcu/datalabels.cpp \
    mcu/mcu.cpp \
    peripheral/battery.cpp \
    peripheral/bcd.cpp \
    peripheral/interrupts.cpp \
    peripheral/keyboard.cpp \
    peripheral/ltb.cpp \
    peripheral/screen.cpp \
    peripheral/standby.cpp \
    peripheral/timer.cpp \
    peripheral/wdt.cpp \

SRCS_C_DEBUG := \
    u8_emu/src/core/core.c \
    u8_emu/src/core/instr.c \
    u8_emu/src/core/instr_impl.c \
    u8_emu/src/core/mem.c \
    u8_emu/src/core/regs.c \

OBJS_DEBUG := $(patsubst %.cpp,$(OBJDIR_DEBUG)/%.o,$(SRCS_CPP_DEBUG)) \
        $(patsubst %.c,$(OBJDIR_DEBUG)/%.o,$(SRCS_C_DEBUG))

$(TARGET_DEBUG): $(OBJS_DEBUG)
	@mkdir -p bin/dbg
	$(CXX) $(OBJS_DEBUG) -o $(TARGET_DEBUG) ${LDFLAGS_DEBUG}

TARGET_RELEASE := bin/release/u8-emu-frontend-cpp.exe
OBJDIR_RELEASE := obj/release/
CXXFLAGS_RELEASE := -Wall -std=c++23 -fconcepts-diagnostics-depth=2 $(CXXFLAGS_EX) -O2
LDFLAGS_RELEASE := -static-libstdc++ -s -lSDL3 -lSDL3_image -luser32 -lgdi32 -lwinmm -ldbghelp -ldxguid

SRCS_CPP_RELEASE := \
    config/settings.cpp \
    disas/disas.cpp \
    gui/disasdisp.cpp \
    gui/rominfo.cpp \
    gui/startupui.cpp \
    imgui/imgui.cpp \
    imgui/imgui_draw.cpp \
    imgui/imgui_impl_sdl3.cpp \
    imgui/imgui_impl_sdlrenderer3.cpp \
    imgui/imgui_tables.cpp \
    imgui/imgui_widgets.cpp \
    labeltool/labeltool.cpp \
    main.cpp \
    mcu/datalabels.cpp \
    mcu/mcu.cpp \
    peripheral/battery.cpp \
    peripheral/bcd.cpp \
    peripheral/interrupts.cpp \
    peripheral/keyboard.cpp \
    peripheral/ltb.cpp \
    peripheral/screen.cpp \
    peripheral/standby.cpp \
    peripheral/timer.cpp \
    peripheral/wdt.cpp \

SRCS_C_RELEASE := \
    u8_emu/src/core/core.c \
    u8_emu/src/core/instr.c \
    u8_emu/src/core/instr_impl.c \
    u8_emu/src/core/mem.c \
    u8_emu/src/core/regs.c \

OBJS_RELEASE := $(patsubst %.cpp,$(OBJDIR_RELEASE)/%.o,$(SRCS_CPP_RELEASE)) \
        $(patsubst %.c,$(OBJDIR_RELEASE)/%.o,$(SRCS_C_RELEASE))

$(TARGET_RELEASE): $(OBJS_RELEASE)
	@mkdir -p bin/release
	$(CXX) $(OBJS_RELEASE) -o $(TARGET_RELEASE) ${LDFLAGS_RELEASE}

TARGET_RELEASE_LINUX := bin/release_linux/u8-emu-frontend-cpp
OBJDIR_RELEASE_LINUX := obj/release_linux/
CXXFLAGS_RELEASE_LINUX := -Wall -std=c++23 -fconcepts-diagnostics-depth=2 $(CXXFLAGS_EX) -O2
LDFLAGS_RELEASE_LINUX := -static-libstdc++ -s -static-libstdc++ -static-libgcc -lSDL3 -lSDL3_image

SRCS_CPP_RELEASE_LINUX := \
    config/settings.cpp \
    disas/disas.cpp \
    gui/disasdisp.cpp \
    gui/rominfo.cpp \
    gui/startupui.cpp \
    imgui/imgui.cpp \
    imgui/imgui_draw.cpp \
    imgui/imgui_impl_sdl3.cpp \
    imgui/imgui_impl_sdlrenderer3.cpp \
    imgui/imgui_tables.cpp \
    imgui/imgui_widgets.cpp \
    labeltool/labeltool.cpp \
    main.cpp \
    mcu/datalabels.cpp \
    mcu/mcu.cpp \
    peripheral/battery.cpp \
    peripheral/bcd.cpp \
    peripheral/interrupts.cpp \
    peripheral/keyboard.cpp \
    peripheral/ltb.cpp \
    peripheral/screen.cpp \
    peripheral/standby.cpp \
    peripheral/timer.cpp \
    peripheral/wdt.cpp \

SRCS_C_RELEASE_LINUX := \
    u8_emu/src/core/core.c \
    u8_emu/src/core/instr.c \
    u8_emu/src/core/instr_impl.c \
    u8_emu/src/core/mem.c \
    u8_emu/src/core/regs.c \

OBJS_RELEASE_LINUX := $(patsubst %.cpp,$(OBJDIR_RELEASE_LINUX)/%.o,$(SRCS_CPP_RELEASE_LINUX)) \
        $(patsubst %.c,$(OBJDIR_RELEASE_LINUX)/%.o,$(SRCS_C_RELEASE_LINUX))

$(TARGET_RELEASE_LINUX): $(OBJS_RELEASE_LINUX)
	@mkdir -p bin/release_linux
	$(CXX) $(OBJS_RELEASE_LINUX) -o $(TARGET_RELEASE_LINUX) ${LDFLAGS_RELEASE_LINUX}

debug: $(TARGET_DEBUG)

release: $(TARGET_RELEASE)

release_linux: $(TARGET_RELEASE_LINUX)

all: debug release release_linux

$(OBJDIR_DEBUG)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_DEBUG) -c $< -o $@

$(OBJDIR_DEBUG)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS_DEBUG) -c $< -o $@

$(OBJDIR_RELEASE)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_RELEASE) -c $< -o $@

$(OBJDIR_RELEASE)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS_RELEASE) -c $< -o $@

$(OBJDIR_RELEASE_LINUX)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_RELEASE_LINUX) -c $< -o $@

$(OBJDIR_RELEASE_LINUX)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS_RELEASE_LINUX) -c $< -o $@

clean:
	rm -rf $(OBJDIR_DEBUG)
	rm -rf $(OBJDIR_RELEASE)
	rm -rf $(OBJDIR_RELEASE_LINUX)

.PHONY: all clean debug release release_linux
