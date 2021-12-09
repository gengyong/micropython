########################################################################################
# Board: W806
TargetBoard=W806
TargetArch=xt804
TargetArchFlag=ck803
TargetArchFPU=hard-fp
TargetCCOption=-mcpu=ck804ef -mhard-float -DTLS_CONFIG_CPU_XT804=1
TargetLDOption=-mcpu=ck804ef -mhard-float
########################################################################################
# Setup toolchain
ToolChainName=csky-elfabiv2
ToolChainPath=/usr/local/lib/cskytools-minilibc
ExtraLinkOption=-Wl,-zmax-page-size=1024
#ToolChainName=csky-linux-uclibcabiv2
#ToolChainPath=/usr/local/lib/cskytools-uclibc
#ExtraLinkOption=-Wl,-zmax-page-size=1024
########################################################################################
# Setup sdk
GIT_SUBMODULES = ports/xt804/sdk/WM_SDK_W806
SDKPath =./sdk/WM_SDK_W806
########################################################################################
WMTOOL=$(SDKPath)/tools/$(TargetBoard)/wm_tool
TARGET_ROM=micropython_xt804
########################################################################################
# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1
MICROPY_VFS_FAT = 0
MICROPY_VFS_LFS2 = 1
MICROPY_PY_USSL = 1
MICROPY_SSL_AXTLS = 1
MICROPY_PY_LWIP = 0
MICROPY_PY_BTREE = 1
AXTLS_DEFS_EXTRA = -Dabort=abort_ -DRT_MAX_PLAIN_LENGTH=1024 -DRT_EXTRA=4096
BTREE_DEFS_EXTRA = -DDEFPSIZE=1024 -DMINCACHE=3
########################################################################################
CROSS_COMPILE ?= $(ToolChainPath)/bin/$(ToolChainName)-

include ../../py/mkenv.mk

OBJDUMP ?=$(CROSS_COMPILE)objdump
RANLIB ?= $(CROSS_COMPILE)ranlib

LD = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)gcc -E
CXX	= $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar rcu

########################################################################################
# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h
########################################################################################
# frozen python scripts
FROZEN_MANIFEST = boards/$(TargetBoard)/manifest.py
########################################################################################
# include py core make definitions
include $(TOP)/py/py.mk
########################################################################################
# setup submodules, run 'make submodules' to get.
GIT_SUBMODULES += lib/axtls lib/berkeley-db-1.xx lib/lwip 
########################################################################################
COPT = -O2 -g3 -Wall
########################################################################################
INC += -I./boards/$(TargetBoard)/include 
INC += -I./include 
INC += -I. 
INC += -I./src
INC += -I$(TOP)
INC += -I$(BUILD)

PLATFORM_SRCDIR = \
	$(SDKPath)/platform/drivers \
	$(SDKPath)/platform/arch/$(TargetArch)/bsp \
	$(SDKPath)/platform/arch/$(TargetArch)/libc

PLATFORM_INCDIR = \
	$(ToolChainPath)/$(ToolChainName)/include \
	$(SDKPath)/include \
	$(SDKPath)/include/arch/$(TargetArch) \
	$(SDKPath)/include/arch/$(TargetArch)/csi_core \
	$(SDKPath)/include/arch/$(TargetArch)/csi_dsp \
	$(SDKPath)/include/driver
	
PLATFORM_ASMDIR = \
	$(SDKPath)/include \
	$(SDKPath)/include/arch/$(TargetArch) \
	$(SDKPath)/include/arch/$(TargetArch)/csi_core \
	$(SDKPath)/include/arch/$(TargetArch)/csi_dsp \
	$(SDKPath)/include/os \
	$(SDKPath)/include/platform 

PLATFORM_ASMINC = \
	$(ToolChainPath)/$(ToolChainName)/include \
	$(SDKPath)/include \
	$(SDKPath)/include/arch/$(TargetArch) \
	$(SDKPath)/include/arch/$(TargetArch)/csi_core \
	$(SDKPath)/include/arch/$(TargetArch)/csi_dsp \
	$(SDKPath)/include/os \
	$(SDKPath)/include/platform 

PLATFORM_LIBDIR = \
	$(SDKPath)/lib/$(TargetBoard) \
	$(ToolChainPath)/$(ToolChainName)/lib/$(TargetArchFlag)/$(TargetArchFPU) \

########################################################################################
CC_DEFAULT_OPTIONS = $(TargetCCOption) -DGCC_COMPILE=1 -ffunction-sections -fdata-sections
AS_DEFAULT_OPTIONS = $(TargetCCOption) -Wa,--gdwarf2
LD_MAP_OPTION = -Wl,--ckmap=$(BUILD)/firmware.map
LD_LIB_OPTION = -Wl,--start-group -Wl,--end-group
LD_DEFAULT_OPTIONS = $(ExtraLinkOption) $(LD_MAP_OPTION) $(TargetLDOption) -nostartfiles -Wl,--gc-sections -T"$(SDKPath)/ld/$(TargetBoard)/gcc_csky.ld" $(LD_LIB_OPTION) 
########################################################################################
ifeq ($(prod),)
CC_DEFAULT_OPTIONS +=-DDEBUG
endif
########################################################################################
CFLAGS = $(INC) $(addprefix -I,$(PLATFORM_INCDIR)) $(CC_DEFAULT_OPTIONS) $(COPT) $(CFLAGS_MOD)
SFLAGS = $(addprefix -I,$(PLATFORM_ASMINC)) $(AS_DEFAULT_OPTIONS)
LDFLAGS = $(LD_DEFAULT_OPTIONS)
########################################################################################
include $(TOP)/extmod/extmod.mk

# Flags from extmods
CFLAGS += $(CFLAGS_MOD)
LDFLAGS += $(LDFLAGS_MOD)

ifneq ($(FROZEN_MANIFEST),)
CFLAGS += -DMICROPY_MODULE_FROZEN_STR
CFLAGS += -DMICROPY_MODULE_FROZEN_MPY
CFLAGS += -DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
endif

CFLAGS += -include ./include/__fixstdheader.h

# Flags for optional C++ source code
CXXFLAGS += $(filter-out -std=c99,$(CFLAGS))
########################################################################################

SRC_DIR = \
	src

SRC_C = \
	$(wildcard $(addsuffix /*.c,$(SRC_DIR))) \
	$(wildcard $(addsuffix /*.c,$(PLATFORM_SRCDIR))) \
	shared/libc/printf.c \
	shared/libc/string0.c \
	shared/libc/__errno.c \
	shared/libc/abort_.o \
	shared/netutils/netutils.c \
	shared/readline/readline.c \
	shared/timeutils/timeutils.c \
	shared/runtime/pyexec.c \
	shared/runtime/mpirq.c \
	shared/runtime/interrupt_char.c \
	shared/runtime/stdout_helpers.c \
	shared/runtime/sys_stdio_mphal.c \
	$(SRC_MOD) \
	$(BUILD)/frozen_content.c \

SRC_CXX = \
	$(wildcard $(addsuffix /*.cc,$(SRC_DIR))) \
	$(wildcard $(addsuffix /*.cpp,$(SRC_DIR))) \
	$(wildcard $(addsuffix /*.cxx,$(SRC_DIR)))

SRC_ASM = \
	$(wildcard $(addsuffix /*.S,$(PLATFORM_SRCDIR))) \
	$(wildcard $(addsuffix /*.S,$(PLATFORM_ASMDIR)))

#if has bluetooth or wifi
#EXTMOD_SRC_C +=extmod/modbluetooth.c
#EXTMOD_SRC_C +=extmod/lwip.c
#endif

EXTMOD_SRC_C +=extmod/modbtree.c
EXTMOD_SRC_C +=extmod/modframebuf.c
EXTMOD_SRC_C +=extmod/modnetwork.c
EXTMOD_SRC_C +=extmod/modonewire.c
EXTMOD_SRC_C +=extmod/moduasyncio.c
EXTMOD_SRC_C +=extmod/modubinascii.c
EXTMOD_SRC_C +=extmod/moducryptolib.c
EXTMOD_SRC_C +=extmod/moductypes.c
EXTMOD_SRC_C +=extmod/moduhashlib.c
EXTMOD_SRC_C +=extmod/moduheapq.c
EXTMOD_SRC_C +=extmod/modujson.c
EXTMOD_SRC_C +=extmod/moduplatform.c
EXTMOD_SRC_C +=extmod/modurandom.c
EXTMOD_SRC_C +=extmod/modure.c
EXTMOD_SRC_C +=extmod/moduselect.c
EXTMOD_SRC_C +=extmod/modusocket.c
EXTMOD_SRC_C +=extmod/modussl_axtls.c
EXTMOD_SRC_C +=extmod/modutimeq.c
EXTMOD_SRC_C +=extmod/moduwebsocket.c
EXTMOD_SRC_C +=extmod/moduzlib.c
EXTMOD_SRC_C +=extmod/modwebrepl.c
EXTMOD_SRC_C +=extmod/vfs_blockdev.c
EXTMOD_SRC_C +=extmod/vfs_fat_diskio.c
EXTMOD_SRC_C +=extmod/vfs_fat_file.c
EXTMOD_SRC_C +=extmod/vfs_fat.c
EXTMOD_SRC_C +=extmod/vfs_lfs.c
EXTMOD_SRC_C +=extmod/vfs_reader.c
EXTMOD_SRC_C +=extmod/vfs.c
EXTMOD_SRC_C +=extmod/uos_dupterm.c
EXTMOD_SRC_C +=extmod/utime_mphal.c
EXTMOD_SRC_C +=extmod/machine_bitstream.c
EXTMOD_SRC_C +=extmod/machine_i2c.c
EXTMOD_SRC_C +=extmod/machine_mem.c
EXTMOD_SRC_C +=extmod/machine_pinbase.c
EXTMOD_SRC_C +=extmod/machine_pulse.c
EXTMOD_SRC_C +=extmod/machine_signal.c
EXTMOD_SRC_C +=extmod/machine_spi.c
EXTMOD_SRC_C +=extmod/virtpin.c

DRIVERS_SRC_C +=drivers/bus/softspi.c
DRIVERS_SRC_C +=drivers/bus/softqspi.c
DRIVERS_SRC_C +=drivers/dht/dht.c
DRIVERS_SRC_C +=drivers/memory/spiflash.c

LIB_SRC_C = $(addprefix lib/,\
	oofatfs/ff.c \
	oofatfs/ffunicode.c \
	littlefs/lfs2.c \
	littlefs/lfs2_util.c \
	libm/math.c \
	libm/fmodf.c \
	libm/nearbyintf.c \
	libm/ef_sqrt.c \
	libm/erf_lgamma.c \
	libm/kf_rem_pio2.c \
	libm/kf_sin.c \
	libm/kf_cos.c \
	libm/kf_tan.c \
	libm/ef_rem_pio2.c \
	libm/sf_erf.c \
	libm/sf_sin.c \
	libm/sf_cos.c \
	libm/sf_tan.c \
	libm/sf_frexp.c \
	libm/sf_modf.c \
	libm/sf_ldexp.c \
	libm/acoshf.c \
	libm/asinfacosf.c \
	libm/asinhf.c \
	libm/atanf.c \
	libm/atanhf.c \
	libm/atan2f.c \
	libm/log1pf.c \
	libm/roundf.c \
	libm/wf_lgamma.c \
	libm/wf_tgamma.c \
	)


OBJ += $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cpp=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cxx=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cc=.o))
OBJ += $(addprefix $(BUILD)/, $(EXTMOD_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(DRIVERS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_ASM:.S=.o))


#-ljson -llwip_open -lpp -lnet80211 -lwpa -lphy -lnet80211
#LIBS += dsp c cc-rt m semi ssp stdc++ supc++
LIBS += semi
########################################################################################

all: $(BUILD)/$(TARGET_ROM).fls

zip := $(if $(zip),$(zip),0)
sign := $(if $(sign),$(sign),0)
encrypt := $(if $(encrypt),$(encrypt),0)
$(BUILD)/$(TARGET_ROM).fls: $(BUILD)/firmware.elf
	@$(OBJCOPY) -O binary $(BUILD)/firmware.elf $(BUILD)/firmware.bin
	@ls --color=auto -Ugo $(BUILD)/firmware.elf $(BUILD)/firmware.bin
	@./tools/postbuild.sh $(BUILD)/$(TARGET_ROM) $(BUILD)/firmware $(WMTOOL) $(SDKPath) $(zip) $(sign) $(encrypt)

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(addprefix -L,$(PLATFORM_LIBDIR)) $(addprefix -l,$(LIBS))
	$(Q)$(SIZE) $@
	$(Q)$(ECHO) ========================================

sdk: submodules
	$(Q)cd $(SDKPath);make;cd -

ifeq ($(com),)
deploy: $(BUILD)/$(TARGET_ROM).fls
	$(ECHO) "please config Serial Port via com=<PORT>."
	$(ECHO) "  on Windows, port should be COM1, COM2, COM3... etc."
	$(ECHO) "  on Linux, port should be ttyS1, ttyS2... or ttyUSB0, ttyUSB1... etc."
else
WMTOOL_FLASH_OPTION =-rs at -ws 2M -ds 2M
ifeq ($(erase),1)
	WMTOOL_FLASH_OPTION += -eo all
endif
deploy: $(BUILD)/$(TARGET_ROM).fls
	$(ECHO) "Writing $< to the board"
	$(Q)$(WMTOOL) -c $(com) $(WMTOOL_FLASH_OPTION) -dl $<
endif

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

$(BUILD)/%.o: %.S
	$(ECHO) "ASM $<"
	$(Q)$(CC) $(SFLAGS) -c -o $@ $<