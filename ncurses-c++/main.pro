# Project Configuration
CONFIG += c++23 console

QT += core

TARGET = Alpine-Btrfs-Installer

# Source Files
SOURCES += main.cpp

# Compiler and linker settings
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic
LIBS += -lncurses

# Build directories
DESTDIR = $$PWD/build
OBJECTS_DIR = $$DESTDIR/obj
