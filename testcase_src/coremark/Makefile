TOOLCHAIN_7_2 = /opt/riscv-gnu-toolchain_20171107_rv32im/bin/riscv32-unknown-elf-
TOOLCHAIN_12_2 = /opt/riscv-gnu-toolchain_20221011_rv32im/bin/riscv32-unknown-elf-
src_dir = .

SRC_LIST = printf.c cmark.c mem_map.h printf.h startup.S
COMPILE_NAME = coremark_10

RISCV_LINK_OPTS ?= -static -nostdlib -nostartfiles -lm -lgcc -T$(src_dir)/link.ld

all: build_7_2/$(COMPILE_NAME)_7_2.bin build_12_2/$(COMPILE_NAME)_12_2.bin build_7_2/$(COMPILE_NAME)_7_2.dump build_12_2/$(COMPILE_NAME)_12_2.dump build_7_2/$(COMPILE_NAME)_7_2.elf build_12_2/$(COMPILE_NAME)_12_2.elf

build_7_2/%.bin: build_7_2/%.elf
	$(TOOLCHAIN_7_2)objcopy -O binary $^ $@

build_12_2/%.bin: build_12_2/%.elf
	$(TOOLCHAIN_12_2)objcopy -O binary $^ $@

build_7_2/%.dump: build_7_2/%.elf
	$(TOOLCHAIN_7_2)objdump --disassemble-all --disassemble-zeroes $^ > $@

build_12_2/%.dump: build_12_2/%.elf
	$(TOOLCHAIN_12_2)objdump --disassemble-all --disassemble-zeroes $^ > $@

build_7_2/%.elf: $(SRC_LIST)
	mkdir -p ./build_7_2
	$(TOOLCHAIN_7_2)gcc -Wl,-Map=build_7_2/output.map -g -O3 -funroll-all-loops -I$(src_dir) -mabi=ilp32 -march=rv32im -o $@ startup.S $(src_dir)/*.c $(RISCV_LINK_OPTS)
	
build_12_2/%.elf: $(SRC_LIST)
	mkdir -p ./build_12_2
	$(TOOLCHAIN_7_2)gcc -Wl,-Map=build_12_2/output.map -g -O3 -funroll-all-loops -I$(src_dir) -mabi=ilp32 -march=rv32im -o $@ startup.S $(src_dir)/*.c $(RISCV_LINK_OPTS)

copy: all
	cp build_7_2/$(COMPILE_NAME)_7_2.bin ../../testcase/benchmark/$(COMPILE_NAME)_7_2.bin
	cp build_7_2/$(COMPILE_NAME)_7_2.bin ../../testcase/benchmark/$(COMPILE_NAME)_7_2.dump
	cp build_12_2/$(COMPILE_NAME)_12_2.bin ../../testcase/benchmark/$(COMPILE_NAME)_12_2.bin
	cp build_12_2/$(COMPILE_NAME)_12_2.bin ../../testcase/benchmark/$(COMPILE_NAME)_12_2.dump

clean:
	rm build_7_2/*
	rm build_12_2/*
