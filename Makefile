CXX ?= g++
CXXFLAGS ?= -O3 -fPIC -std=c++17 -Wall -Wextra
LDFLAGS ?= -shared
LV2_CFLAGS := $(shell pkg-config --cflags lv2 2>/dev/null)
LV2_LIBS := $(shell pkg-config --libs lv2 2>/dev/null)

PLUGIN_BUNDLE := drum-synth.lv2
PLUGIN_SO := $(PLUGIN_BUNDLE)/drumsynth.so
SRC := src/drumsynth.cpp

all: $(PLUGIN_SO)

$(PLUGIN_SO): $(SRC)
	$(CXX) $(CXXFLAGS) $(LV2_CFLAGS) $< -o $@ $(LDFLAGS) $(LV2_LIBS)

arm64:
	$(MAKE) CXX=aarch64-linux-gnu-g++ CXXFLAGS="$(CXXFLAGS) -march=armv8-a" all

install: all
	mkdir -p $(HOME)/.lv2/$(PLUGIN_BUNDLE)
	cp -a $(PLUGIN_BUNDLE)/* $(HOME)/.lv2/$(PLUGIN_BUNDLE)/

clean:
	rm -f $(PLUGIN_SO)

.PHONY: all arm64 install clean
