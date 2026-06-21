CC := gcc

# Only apply ARMv6 tuning when building on ARM (Pi 3/4/Zero).
# On x86 (dry-run development), omit so the build succeeds.
UNAME_M := $(shell uname -m)
ifneq ($(filter arm%,$(UNAME_M)),)
  PIZERO_CFLAGS := -march=armv6 -mfpu=vfp -mfloat-abi=hard
else ifneq ($(filter aarch64,$(UNAME_M)),)
  PIZERO_CFLAGS :=
else
  PIZERO_CFLAGS :=
endif

CFLAGS := -std=c99 -O3 -Wall -Wextra -Wpedantic \
          -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L \
          -Iinclude $(PIZERO_CFLAGS)
LDFLAGS :=

TARGET := build/oledtty
CTL_TARGET := build/oledtty-ctl
TEST_VCSA := tests/sample.vcsa

SRC := src/main.c \
       src/i2c.c \
       src/ssd1306.c \
       src/framebuffer.c \
       src/font5x7.c \
       src/terminal.c \
       src/viewport.c \
       src/render.c \
       src/history.c \
       src/control.c \
       src/keyboard.c \
       src/timer.c \
       src/log.c

CTL_SRC := src/oledtty_ctl.c

OBJ := $(SRC:src/%.c=build/%.o)

.PHONY: all clean run dirs check-arch test sample-vcsa

all: dirs $(TARGET) $(CTL_TARGET)
	@$(MAKE) --no-print-directory check-arch

dirs:
	@mkdir -p build

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

$(CTL_TARGET): src/oledtty_ctl.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

sample-vcsa:
	@command -v python3 >/dev/null 2>&1 || { echo "python3 required for sample.vcsa"; exit 1; }
	python3 tests/gen_sample_vcsa.py $(TEST_VCSA)

test: $(TARGET) sample-vcsa
	@echo "Running offline smoke test..."
	@./$(TARGET) --dry-run --vcsa $(TEST_VCSA) --once -q > /tmp/oledtty_test_out.txt
	@grep -q '#' /tmp/oledtty_test_out.txt && echo "OK: rendered a non-empty frame" || \
		{ echo "FAIL: empty frame"; exit 1; }

check-arch:
	@if command -v file >/dev/null 2>&1; then \
		echo "Binary info: $$(file $(TARGET))"; \
	fi

run: $(TARGET)
	sudo ./$(TARGET)

clean:
	rm -rf build
