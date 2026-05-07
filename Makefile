CXX ?= g++
CC ?= gcc
CXXFLAGS ?= -O3 -fPIC -std=c++17 -Wall -Wextra -fno-exceptions -fno-rtti -fvisibility=hidden -Iinclude
LDFLAGS ?= -shared -Wl,--as-needed
LDLIBS ?= -lm
LV2_CFLAGS := $(shell pkg-config --cflags lv2 2>/dev/null)
LV2_LIBS := $(shell pkg-config --libs lv2 2>/dev/null)

PLUGIN_BUNDLE := drum-synth.lv2
PLUGIN_SO := $(PLUGIN_BUNDLE)/drumsynth.so
BUILD_DIR := build
OBJ := $(BUILD_DIR)/drumsynth.o
SRC := src/drumsynth.cpp

S2400_DIR := s2400-lv2
S2400_TEMPLATE := templates/s2400-plugin.ttl.in
VOICE_SLUGS := kick snare hihat tom clap sub808
VOICE_NAMES := Kick Snare HiHat Tom Clap Sub808
BASE_URI := https://github.com/AsierT/drumsynth

all: $(PLUGIN_SO)

$(BUILD_DIR):
	mkdir -p $@

$(OBJ): $(SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LV2_CFLAGS) -c $< -o $@

$(PLUGIN_SO): $(OBJ)
	$(CC) $< -o $@ $(LDFLAGS) $(LV2_LIBS) $(LDLIBS)

arm64:
	$(MAKE) CXX=aarch64-linux-gnu-g++ CC=aarch64-linux-gnu-gcc CXXFLAGS="$(CXXFLAGS) -march=armv8-a" all

s2400: $(SRC) $(S2400_TEMPLATE)
	mkdir -p $(S2400_DIR)
	set -- $(VOICE_NAMES); \
	i=0; \
	for slug in $(VOICE_SLUGS); do \
		name="$$1"; shift; \
		uri="$(BASE_URI)#$$slug"; \
		bundle="$(S2400_DIR)/drumsynth-$$slug.lv2"; \
		binary="drumsynth_$$slug.so"; \
		ttl="drumsynth_$$slug.ttl"; \
		obj="$$bundle/drumsynth_$$slug.o"; \
		mkdir -p "$$bundle"; \
		$(CXX) $(CXXFLAGS) $(LV2_CFLAGS) -DDRUMSYNTH_INSERT_PORTS -DDRUMSYNTH_SINGLE_INDEX=$$i -c $(SRC) -o "$$obj"; \
		$(CC) "$$obj" -o "$$bundle/$$binary" $(LDFLAGS) $(LV2_LIBS) $(LDLIBS); \
		sed -e "s|@URI@|$$uri|g" -e "s|@NAME@|$$name|g" -e "s|@BINARY@|$$binary|g" "$(S2400_TEMPLATE)" > "$$bundle/$$ttl"; \
		printf '@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n\n<%s>\n    a lv2:Plugin ;\n    lv2:binary <%s> ;\n    rdfs:seeAlso <%s> .\n' "$$uri" "$$binary" "$$ttl" > "$$bundle/manifest.ttl"; \
		rm -f "$$obj"; \
		i=$$((i + 1)); \
	done

arm64-s2400:
	$(MAKE) CXX=aarch64-linux-gnu-g++ CC=aarch64-linux-gnu-gcc CXXFLAGS="$(CXXFLAGS) -march=armv8-a" s2400

check-abi:
	file $(PLUGIN_SO)
	readelf -d $(PLUGIN_SO) | grep NEEDED || true
	strings -a $(PLUGIN_SO) | grep -E 'GLIBC_|GLIBCXX_|GCC_' | sort -V | uniq || true

install: all
	mkdir -p $(HOME)/.lv2/$(PLUGIN_BUNDLE)
	cp -a $(PLUGIN_BUNDLE)/* $(HOME)/.lv2/$(PLUGIN_BUNDLE)/

clean:
	rm -rf $(BUILD_DIR) $(PLUGIN_SO) $(S2400_DIR)

.PHONY: all arm64 s2400 arm64-s2400 check-abi install clean
