


########################################################################################
CROSS = 1
########################################################################################
ToolChainName=csky-elfabiv2
ToolChainPath=/usr/local/lib/cskytools-minilibc
ExtraLinkOption=-Wl,-zmax-page-size=1024
#ToolChainName=csky-linux-uclibcabiv2
#ToolChainPath=/usr/local/lib/cskytools-uclibc
#ExtraLinkOption=-Wl,-zmax-page-size=1024
########################################################################################
SDKPath =./boards/W806/WM_SDK_W806
########################################################################################
ifeq ($(CROSS), 1)
CROSS_COMPILE ?= $(ToolChainPath)/bin/$(ToolChainName)-
endif

include ../../py/mkenv.mk

#AS = $(CROSS_COMPILE)as
#CC = $(CROSS_COMPILE)gcc
#CXX = $(CROSS_COMPILE)g++
#GDB = $(CROSS_COMPILE)gdb
#LD = $(CROSS_COMPILE)ld
#OBJCOPY = $(CROSS_COMPILE)objcopy
#SIZE = $(CROSS_COMPILE)size
#STRIP = $(CROSS_COMPILE)strip
#AR = $(CROSS_COMPILE)ar

OBJDUMP ?=$(CROSS_COMPILE)objdump
RANLIB ?= $(CROSS_COMPILE)ranlib

ifeq ($(CROSS), 1)
LD = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)gcc -E
CXX	= $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar rcu
endif

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# include py core make definitions
include $(TOP)/py/py.mk

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -Imain/inc

LIBDIRS = .
LIBS +=

COPT = -O2 -g3 -Wall

ifeq ($(CROSS), 1)
PLATFORM_SRCDIR = \
	$(SDKPath)/platform/drivers \
	$(SDKPath)/platform/arch/xt804/bsp \
	$(SDKPath)/platform/arch/xt804/libc

PLATFORM_INCDIR = \
	$(ToolChainPath)/$(ToolChainName)/include \
	$(SDKPath)/include \
	$(SDKPath)/include/arch/xt804 \
	$(SDKPath)/include/arch/xt804/csi_core \
	$(SDKPath)/include/arch/xt804/csi_dsp \
	$(SDKPath)/include/driver
	
PLATFORM_ASMDIR = \
	$(SDKPath)/include \
	$(SDKPath)/include/arch/xt804 \
	$(SDKPath)/include/arch/xt804/csi_core \
	$(SDKPath)/include/arch/xt804/csi_dsp \
	$(SDKPath)/include/os \
	$(SDKPath)/include/platform 

PLATFORM_ASMINC = \
	$(ToolChainPath)/$(ToolChainName)/include \
	$(SDKPath)/include \
	$(SDKPath)/include/arch/xt804 \
	$(SDKPath)/include/arch/xt804/csi_core \
	$(SDKPath)/include/arch/xt804/csi_dsp \
	$(SDKPath)/include/os \
	$(SDKPath)/include/platform 


PLATFORM_LIBDIR = \
	$(SDKPath)/lib/W806

INC += $(addprefix -I,$(PLATFORM_INCDIR))
LIBDIRS += $(PLATFORM_LIBDIR)

CC_DEFAULT_OPTIONS = -mcpu=ck804ef -mhard-float -DGCC_COMPILE=1 -DTLS_CONFIG_CPU_XT804=1 -ffunction-sections -fdata-sections
AS_DEFAULT_OPTIONS = -mcpu=ck804ef -mhard-float -Wa,--gdwarf2
LD_MAP_OPTION = -Wl,--ckmap=$(BUILD)/firmware.map
LD_LIB_OPTION = -Wl,--start-group -Wl,--end-group
LD_DEFAULT_OPTIONS = $(ExtraLinkOption) $(LD_MAP_OPTION) -mcpu=ck804ef -mhard-float -nostartfiles -Wl,--gc-sections -T"$(SDKPath)/ld/W806/gcc_csky.ld" $(LD_LIB_OPTION) 

DFU = $(TOP)/tools/dfu.py
PYDFU = $(TOP)/tools/pydfu.py

CFLAGS = $(INC) $(CC_DEFAULT_OPTIONS) $(COPT)
SFLAGS = $(INC) $(AS_DEFAULT_OPTIONS) $(COPT)
LDFLAGS = $(LD_DEFAULT_OPTIONS)
else
LD = gcc
CFLAGS = $(INC) -Wdouble-promotion -Wfloat-conversion -std=c99 $(COPT)
LDFLAGS = -Wl,-Map=$@.map,--cref -Wl,--gc-sections
# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif
endif

# Flags for optional C++ source code
CXXFLAGS += $(filter-out -std=c99,$(CFLAGS))
CXXFLAGS += $(CXXFLAGS_MOD)

# Flags for user C modules
CFLAGS += $(CFLAGS_MOD)
LDFLAGS += $(LDFLAGS_MOD)

LIBS += dsp

SRC_DIR = \
	main/src

SRC_C = \
	$(wildcard $(addsuffix /*.c,$(SRC_DIR))) \
	shared/libc/printf.c \
	shared/readline/readline.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \
	$(BUILD)/_frozen_mpy.c

#SRC_DIR = \
	boards/W806/WM_SDK_W806/app/src

#SRC_C = \
	$(wildcard $(addsuffix /*.c,$(SRC_DIR))) \


SRC_CXX += \
	$(wildcard $(addsuffix /*.cc,$(SRC_DIR))) \
	$(wildcard $(addsuffix /*.cpp,$(SRC_DIR))) \
	$(wildcard $(addsuffix /*.cxx,$(SRC_DIR)))

SRC_ASM =

ifeq ($(CROSS), 1)
SRC_C += shared/libc/string0.c
SRC_C += $(wildcard $(addsuffix /*.c,$(PLATFORM_SRCDIR)))
SRC_ASM += main/src/__rt_entry.S
SRC_ASM += $(wildcard $(addsuffix /*.S,$(PLATFORM_SRCDIR)))
SRC_ASM += $(wildcard $(addsuffix /*.S,$(PLATFORM_ASMDIR)))
endif

SRC_C += $(SRC_MOD)

SRC_CXX += $(SRC_MOD_CXX)

SRC_QSTR += $(SRC_MOD) $(SRC_MOD_CXX)

OBJ += $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cpp=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cxx=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cc=.o))

ifeq ($(CROSS), 1)
OBJ += $(addprefix $(BUILD)/, $(SRC_ASM:.S=.o))
endif

ifeq ($(CROSS), 1)
all: $(BUILD)/firmware.dfu
else
all: $(BUILD)/firmware.elf
endif

$(BUILD)/_frozen_mpy.c: frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@

#$(LD) $(LDFLAGS) -o $@ $^ $(addprefix -L,$(LIBDIRS)) $(addprefix -l,$(LIBS))
#$(BUILD)/firmware.elf: $(OBJ)
#	$(ECHO) "LINK $@"
#	$(LD) $(LDFLAGS) -o $@.new \
	test/app/src/wm_hal_msp.o \
	test/app/src/wm_it.o \
	test/app/src/main.o \
	test/app/src/__rt_entry.o \
	test/platform/drivers/wm_cpu.o \
	test/platform/drivers/wm_uart.o \
	test/platform/drivers/wm_spi_flash.o \
	test/platform/drivers/wm_pmu.o \
	test/platform/drivers/wm_touch.o \
	test/platform/drivers/wm_internal_flash.o \
	test/platform/drivers/wm_adc.o \
	test/platform/drivers/wm_spi.o \
	test/platform/drivers/wm_pwm.o \
	test/platform/drivers/wm_wdg.o \
	test/platform/drivers/wm_hal.o \
	test/platform/drivers/wm_tim.o \
	test/platform/drivers/wm_i2c.o \
	test/platform/arch/xt804/bsp/board_init.o \
	test/platform/arch/xt804/bsp/trap_c.o \
	test/platform/arch/xt804/bsp/system.o \
	test/platform/arch/xt804/libc/libc_port.o \
	test/platform/arch/xt804/bsp/startup.o \
	test/platform/arch/xt804/bsp/vectors.o \
	$(addprefix -L,$(LIBDIRS)) $(addprefix -l,$(LIBS))
#	$(Q)$(SIZE) $@

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(addprefix -L,$(LIBDIRS)) $(addprefix -l,$(LIBS))
	$(Q)$(SIZE) $@


#$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data $^ $(BUILD)/firmware.bin
zip := $(if $(zip),$(zip),0)
sign := $(if $(sign),$(sign),0)
encrypt := $(if $(encrypt),$(encrypt),0)
$(BUILD)/firmware.dfu: $(BUILD)/firmware.elf
	@$(OBJCOPY) -O binary $(BUILD)/firmware.elf $(BUILD)/firmware.bin
	@ls -Ugo $(BUILD)/firmware.elf -go $(BUILD)/firmware.bin
	@./tools/postbuild.sh firmware $(SDKPath) $(BUILD) $(zip) $(sign) $(encrypt)

deploy: $(BUILD)/firmware.dfu
	$(ECHO) "Writing $< to the board"
	$(Q)$(PYTHON) $(PYDFU) -u $<


# Run emulation build on a POSIX system with suitable terminal settings
run:
	stty raw opost -echo
	build/firmware.elf
	@echo Resetting terminal...
# This sleep is useful to spot segfaults
	sleep 1
	reset

test: $(BUILD)/firmware.elf
	$(Q)/bin/echo -e "print('hello world!', list(x+1 for x in range(10)), end='eol\\\\n')\\r\\n\\x04" | $(BUILD)/firmware.elf | tail -n2 | grep "^hello world! \\[1, 2, 3, 4, 5, 6, 7, 8, 9, 10\\]eol"


include $(TOP)/py/mkrules.mk
