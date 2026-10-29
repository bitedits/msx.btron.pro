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
	$(BUILD_DIR)/compositor.rel \
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

$(BUILD_DIR)/compositor.rel: src/gui/compositor.c | $(BUILD_DIR)
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

BTRON_DIR = ../btron
EMU_DIR = $(BTRON_DIR)/src/emulators/msx
EMU_CORE_OBJS = \
	$(EMU_DIR)/tronmsx.test.o \
	$(EMU_DIR)/z80.test.o \
	$(EMU_DIR)/r800.test.o \
	$(EMU_DIR)/v9938.test.o \
	$(EMU_DIR)/vdp.test.o \
	$(EMU_DIR)/slots.test.o \
	$(EMU_DIR)/ppi.test.o \
	$(EMU_DIR)/psg.test.o \
	$(EMU_DIR)/opll.test.o \
	$(EMU_DIR)/scc.test.o \
	$(EMU_DIR)/y8950.test.o \
	$(EMU_DIR)/pcm.test.o \
	$(EMU_DIR)/audio.test.o \
	$(EMU_DIR)/kbd.test.o \
	$(EMU_DIR)/dsk.test.o \
	$(EMU_DIR)/s1985.test.o \
	$(EMU_DIR)/t9769b.test.o \
	$(EMU_DIR)/t9769c.test.o \
	$(EMU_DIR)/hd64180.test.o \
	$(EMU_DIR)/bios.test.o \
	$(EMU_DIR)/geo3d.test.o

# C Verification Test Binaries
$(BUILD_DIR)/test_itron_scheduler: verify/tests/test_itron_scheduler.c src/kernel/itron_task.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc $^ -o $@

$(BUILD_DIR)/test_bmsx_body: verify/tests/test_bmsx_body.c src/kernel/bmsx_body.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc $^ -o $@

$(BUILD_DIR)/test_tad_parser: verify/tests/test_tad_parser.c src/tad/tad_segment.c src/data/doc_heart_sutra.c src/data/doc_nortia_letter.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc $^ -o $@

$(BUILD_DIR)/test_multilingual_font: verify/tests/test_multilingual_font.c src/font/font_tibetan.c src/font/font_kanji.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc $^ -o $@

$(BUILD_DIR)/test_vdp_bitblt: verify/tests/test_vdp_bitblt.c $(EMU_DIR)/v9938.test.o | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(BTRON_DIR)/include -I$(EMU_DIR) $^ -o $@

$(BUILD_DIR)/test_btron_boot: verify/tests/test_btron_boot.c $(EMU_CORE_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(BTRON_DIR)/include -I$(EMU_DIR) $^ -o $@

$(BUILD_DIR)/test_btron_input: verify/tests/test_btron_input.c $(EMU_CORE_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(BTRON_DIR)/include -I$(EMU_DIR) $^ -o $@

test-c: $(BUILD_DIR)/test_itron_scheduler $(BUILD_DIR)/test_bmsx_body $(BUILD_DIR)/test_tad_parser $(BUILD_DIR)/test_multilingual_font $(BUILD_DIR)/test_vdp_bitblt $(BUILD_DIR)/test_btron_boot $(BUILD_DIR)/test_btron_input $(TARGET_ROM)
	@echo "=== Running C Verification Test Suite ==="
	./$(BUILD_DIR)/test_itron_scheduler
	./$(BUILD_DIR)/test_bmsx_body
	./$(BUILD_DIR)/test_tad_parser
	./$(BUILD_DIR)/test_multilingual_font
	./$(BUILD_DIR)/test_vdp_bitblt
	./$(BUILD_DIR)/test_btron_boot $(TARGET_ROM) 300 /tmp/btron_stabilization.ppm
	./$(BUILD_DIR)/test_btron_input $(TARGET_ROM)
	@echo "All C stabilization tests PASSED."

test: $(TARGET_ROM) test-c
	python3 tests/verify_rom.py

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test test-c clean

