# Automatically generated Makefile

CXX := g++
CC := gcc

OBJDIR := obj/release/
BINDIR := bin/release

TARGET := $(BINDIR)/u8-emu-frontend-cpp.exe

CXXFLAGS := -Wall -O3 -IC:\msys64\mingw32\include\SDL2 -std=gnu++20 -fconcepts-diagnostics-depth=2 -D__LITTLE_ENDIAN__
LDFLAGS := -s -lmingw32 -lSDL2main -lSDL2.dll -luser32 -lgdi32 -lwinmm -ldxguid -lSDL2_image

SRCS_CPP := \
    imgui/imgui.cpp \
    imgui/imgui_draw.cpp \
    imgui/imgui_impl_sdl2.cpp \
    imgui/imgui_impl_sdlrenderer2.cpp \
    imgui/imgui_tables.cpp \
    imgui/imgui_widgets.cpp \
    labeltool/labeltool.cpp \
    main.cpp \
    mcu/datalabels.cpp \
    mcu/mcu.cpp \
    peripheral/battery.cpp \
    peripheral/interrupts.cpp \
    peripheral/keyboard.cpp \
    peripheral/ltb.cpp \
    peripheral/screen.cpp \
    peripheral/standby.cpp \
    peripheral/timer.cpp \
    peripheral/wdt.cpp \
    startupui/rominfo.cpp \
    startupui/startupui.cpp \

SRCS_C := \
    u8_emu/src/core/core.c \
    u8_emu/src/core/instr.c \
    u8_emu/src/core/instr_impl.c \
    u8_emu/src/core/mem.c \
    u8_emu/src/core/regs.c \

OBJS := $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRCS_CPP)) \
        $(patsubst %.c, $(OBJDIR)/%.o, $(SRCS_C))

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BINDIR)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean
