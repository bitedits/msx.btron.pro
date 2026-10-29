# Makefile — 8-bit BTRON Operating System for MSX turbo R / MegaROM
# Compatible with BSD make and GNU make on macOS, FreeBSD, and Linux.

CC ?= clang
CFLAGS ?= -O2 -Wall -Wextra -Werror

SDCC ?= sdcc
SDASZ80 ?= sdasz80

SDCC_FLAGS = -mz80 --opt-code-speed -Isrc -Isrc/kernel -Isrc/msxgl -Isrc/font -Isrc/tad -Isrc/gui -Isrc/apps -Isrc/data
SDLD_FLAGS = -mz80 --no-std-crt0 --code-loc 0x4010 --data-loc 0xC000

BUILD_DIR = build

IHX2ROM = $(BUILD_DIR)/ihx2rom
TARGET_IHX = $(BUILD_DIR)/btron8.ihx
TARGET_ROM = $(BUILD_DIR)/btron8.rom

OBJS = \
	$(BUILD_DIR)/crt0_btron.rel \
	$(BUILD_DIR)/main.rel \
	$(BUILD_DIR)/core.rel \
	$(BUILD_DIR)/vdp.rel \
	$(BUILD_DIR)/megarom.rel \
	$(BUILD_DIR)/input.rel \
	$(BUILD_DIR)/itron_task.rel \
	$(BUILD_DIR)/bmsx_body.rel \
	$(BUILD_DIR)/font_latin.rel \
	$(BUILD_DIR)/font_kanji.rel \
	$(BUILD_DIR)/font_tibetan.rel \
	$(BUILD_DIR)/tad_segment.rel \
	$(BUILD_DIR)/tad_render.rel \
	$(BUILD_DIR)/window.rel \
	$(BUILD_DIR)/menu.rel \
	$(BUILD_DIR)/app_editor.rel \
	$(BUILD_DIR)/app_terminal.rel \
	$(BUILD_DIR)/app_browser.rel \
	$(BUILD_DIR)/app_cabinet.rel \
	$(BUILD_DIR)/doc_heart_sutra.rel \
	$(BUILD_DIR)/doc_nortia_letter.rel

all: $(TARGET_ROM)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(IHX2ROM): src/sdcc/ihx2rom.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/crt0_btron.rel: src/sdcc/crt0_btron.s | $(BUILD_DIR)
	$(SDASZ80) -o $@ $<

$(BUILD_DIR)/main.rel: src/main.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/core.rel: src/msxgl/core.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/vdp.rel: src/msxgl/vdp.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/megarom.rel: src/msxgl/megarom.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/input.rel: src/msxgl/input.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/itron_task.rel: src/kernel/itron_task.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/bmsx_body.rel: src/kernel/bmsx_body.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/font_latin.rel: src/font/font_latin.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/font_kanji.rel: src/font/font_kanji.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/font_tibetan.rel: src/font/font_tibetan.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/tad_segment.rel: src/tad/tad_segment.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/tad_render.rel: src/tad/tad_render.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/window.rel: src/gui/window.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/menu.rel: src/gui/menu.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/app_editor.rel: src/apps/app_editor.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/app_terminal.rel: src/apps/app_terminal.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/app_browser.rel: src/apps/app_browser.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/app_cabinet.rel: src/apps/app_cabinet.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/doc_heart_sutra.rel: src/data/doc_heart_sutra.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/doc_nortia_letter.rel: src/data/doc_nortia_letter.c | $(BUILD_DIR)
	$(SDCC) $(SDCC_FLAGS) -c $< -o $@

$(TARGET_IHX): $(OBJS)
	$(SDCC) $(SDLD_FLAGS) -o $@ $(OBJS)

$(TARGET_ROM): $(TARGET_IHX) $(IHX2ROM)
	$(IHX2ROM) $(TARGET_IHX) $@ 1024

test: $(TARGET_ROM)
	python3 tests/verify_rom.py

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test clean

