# TOOLCHAIN=/opt/riscv/bin/riscv32-unknown-elf-
TOOLCHAIN=riscv64-unknown-elf-
#TOOLCHAIN=/home/eh1/Downloads/2020.12/riscv64-unknown-elf-toolchain-10.2.0-2020.12.8-x86_64-linux-ubuntu14/bin/riscv64-unknown-elf-
# TOOLCHAIN=/home/eh1/Downloads/riscv-gcc/gnu-mcu-eclipse-riscv-none-gcc-7.2.0-4-20180606-1631-centos64/gnu-mcu-eclipse/riscv-none-gcc/7.2.0-4-20180606-1631/bin/riscv-none-embed-
# TOOLCHAIN=/home/eh1/Downloads/riscv-gcc/7.2.0/gcc7.2_riscv_202111290316/build/bin/riscv64-unknown-elf-

src_dir = .

RISCV_GCC_OPTS ?= -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -g -O3 -ffast-math -fno-common -fno-builtin-printf -funroll-all-loops -march=rv32im -mabi=ilp32
RISCV_LINK_OPTS ?= -static -nostartfiles -lm -lgcc -T$(src_dir)/link.ld

all: dhrystone.elf
	$(TOOLCHAIN)objcopy -O binary dhrystone.elf dhrystone.bin
	$(TOOLCHAIN)objcopy -I binary -O ihex dhrystone.bin dhrystone.hex
	$(TOOLCHAIN)objdump --disassemble-all --disassemble-zeroes dhrystone.elf > dhrystone.dump

# build/coremark: 
# 	$(TOOLCHAIN)gcc -Wl,-Map=output.map -Wl,-V -I$(src_dir) $(RISCV_GCC_OPTS) -o $@ startup.S $(src_dir)/*.c $(RISCV_LINK_OPTS)
dhrystone.elf: printf.c dhrystone_main.c dhrystone.c dhrystone.h printf.h startup.S
	$(TOOLCHAIN)gcc -Wl,-Map=output.map -g -O3 -funroll-all-loops -I$(src_dir) -mabi=ilp32 -march=rv32im -o $@ startup.S $(src_dir)/*.c $(RISCV_LINK_OPTS)

#build/coremark: 
#	$(TOOLCHAIN)gcc -mtune=sifive-7-series -mbranch-cost=1 -Ofast -funroll-all-loops -march=rv32im -mabi=ilp32 --param=hot-bb-frequency-fraction=1 --param=max-jump-thread-duplication-stmts=2 --param=max-tail-merge-iterations=0 -fno-aggressive-loop-optimizations -fgcse-las --param=max-grow-copy-bb-insns=7 --param=unroll-jam-min-percent=0 --param=large-unit-insns=0 -fno-delete-null-pointer-checks -fno-rename-registers --param=max-partial-antic-length=1 --param=loop-max-datarefs-for-datadeps=0 -funroll-all-loops --param=uninlined-function-insns=16 -fno-tree-vrp -fwrapv -fno-toplevel-reorder --param=max-inline-insns-size=128 -fipa-pta --param=max-cse-path-length=3 --param=inline-min-speedup=1 --param=max-sched-region-insns=7 --param=max-goto-duplication-insns=0 --param=max-gcse-insertion-ratio=0 -fno-tree-ter --param=max-average-unrolled-insns=128 -fno-dce --param=max-loop-header-insns=2 -fno-tree-loop-ivcanon -fno-tree-loop-distribute-patterns -fno-sched-rank-heuristic -I$(src_dir) -o $@ startup.S $(src_dir)/*.c -static -nostdlib -nostartfiles -lm -lgcc -T$(src_dir)/link.ld


dasm: coremark.elf
	$(TOOLCHAIN)objdump -d $<

GDB_UPLOAD_ARGS ?= --batch
GDB_UPLOAD_CMDS += -ex "set remotetimeout 240"
GDB_UPLOAD_CMDS += -ex "target extended-remote localhost:3333"
GDB_UPLOAD_CMDS += -ex "monitor reset halt"
GDB_UPLOAD_CMDS += -ex "monitor flash protect 0 64 last off"
GDB_UPLOAD_CMDS += -ex "load"


upload: build/coremark
	$(TOOLCHAIN)gdb $< $(GDB_UPLOAD_ARGS) $(GDB_UPLOAD_CMDS) -ex "monitor resume" -ex "quit"

debug: build/coremark
	$(TOOLCHAIN)gdb $< -tui $(GDB_UPLOAD_CMDS)

clean:
	rm build/*
