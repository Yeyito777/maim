# Local convenience wrapper for maim's CMake build.
#
# Defaults are chosen so this source checkout is the one installed in the
# user's PATH: `make` builds maim and `make install` installs to ~/.local/bin.

PREFIX ?= $(HOME)/.local
BUILD_DIR ?= build
BUILD_TYPE ?= Release

.PHONY: all configure install clean distclean

all: configure
	cmake --build $(BUILD_DIR)

configure:
	cmake -S . -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
		-DCMAKE_INSTALL_BINDIR=bin

install: all
	cmake --install $(BUILD_DIR)

clean:
	cmake --build $(BUILD_DIR) --target clean

distclean:
	rm -rf $(BUILD_DIR) bin
