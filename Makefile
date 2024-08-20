ifeq ($(OS), Windows_NT)
	#WINDOWS USE THESE DEFINITIONS
	RM = -del /q
	SLASH = \\
else
	#LINUX USE THESE DEFINITIONS
	RM = -rm -f
	SLASH = /
endif 

Pi3: CFLAGS = -Wall -O3 -mcpu=cortex-a53 -mfpu=neon-vfpv4 -mfloat-abi=hard -ffreestanding -nostartfiles -std=c11 -mno-unaligned-access -fno-tree-loop-vectorize -fno-tree-slp-vectorize -Wno-nonnull-compare
Pi3: ARMGNU = arm-none-eabi
Pi3: LINKERFILE = rpi32.ld
Pi3: SMARTSTART = SmartStart32.S
Pi3: IMGFILE = kernel7.img

# The directory in which source files are stored.
SOURCE = ${CURDIR}/src
BUILD = Build

# The name of the assembler listing file to generate.
LIST = kernel.list

# The name of the map file to generate.
MAP = kernel.map

# The names of all object files that must be generated. Deduced from the 
# assembly code files in source.

ASMOBJS = $(patsubst $(SOURCE)/%.S,$(BUILD)/%.o,$(SMARTSTART))
COBJS = $(patsubst $(SOURCE)/%.c,$(BUILD)/%.o,$(wildcard $(SOURCE)/*.c))

Pi3: kernel.elf
BINARY = $(IMGFILE)
.PHONY: Pi3

$(BUILD)/%.o: $(SOURCE)/%.s
	$(ARMGNU)-gcc -MMD -MP -g $(CFLAGS) -c  $< -o $@ -lc -lm -lgcc

$(BUILD)/%.o: $(SOURCE)/%.S
	$(ARMGNU)-gcc -MMD -MP -g $(CFLAGS) -c  $< -o $@ -lc -lm -lgcc

$(BUILD)/%.o: $(SOURCE)/%.c
	$(ARMGNU)-gcc -MMD -MP -g $(CFLAGS) -c  $< -o $@ -lc -lm -lgcc

kernel.elf: $(ASMOBJS) $(COBJS) 
	$(ARMGNU)-gcc $(CFLAGS) $(ASMOBJS) $(COBJS) -T $(LINKERFILE) -Wl,--build-id=none -o kernel.elf -lc -lm -lgcc
	$(ARMGNU)-objdump -d kernel.elf > $(LIST)
	$(ARMGNU)-objcopy kernel.elf -O binary DiskImg/$(BINARY)
	$(ARMGNU)-nm -n kernel.elf > $(MAP)

# Control silent mode  .... we want silent in clean
.SILENT: clean

# cleanup temp files
clean:
	$(RM) $(MAP) 
	$(RM) kernel.elf 
	$(RM) $(LIST) 
	$(RM) $(BUILD)$(SLASH)*.o 
	$(RM) $(BUILD)$(SLASH)*.d 
	echo CLEAN COMPLETED
.PHONY: clean

