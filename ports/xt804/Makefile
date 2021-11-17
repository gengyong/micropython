########################################################################################
CROSS = 1
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
# Setup sdk path
SDKPath =./sdk/WM_SDK_$(TargetBoard)
########################################################################################
ifeq ($(CROSS), 1)
	CROSS_COMPILE ?= $(ToolChainPath)/bin/$(ToolChainName)-
endif

include ../../py/mkenv.mk

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

########################################################################################
# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1
MICROPY_PY_USSL = 1
MICROPY_SSL_AXTLS = 1


AXTLS_DEFS_EXTRA = -Dabort=abort_ -DRT_MAX_PLAIN_LENGTH=1024 -DRT_EXTRA=4096
BTREE_DEFS_EXTRA = -DDEFPSIZE=1024 -DMINCACHE=3

FROZEN_MANIFEST = boards/$(TargetBoard)/manifest.py

# include py core make definitions
include $(TOP)/py/py.mk

########################################################################################
# setup submodules, run 'make submodules' to get.
GIT_SUBMODULES = lib/axtls lib/berkeley-db-1.xx lib/lwip

########################################################################################
INC += -I. -I./include
INC += -I$(TOP)
INC += -I$(BUILD)

LIBDIRS = .
LIBS +=

COPT = -O2 -g3 -Wall

ifeq ($(CROSS), 1)
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


INC += $(addprefix -I,$(PLATFORM_INCDIR))
#INC += -I$(TOP)/lib/lwip/src/include -I$(TOP)/extmod/lwip-include

LIBDIRS += $(PLATFORM_LIBDIR)

CC_DEFAULT_OPTIONS = $(TargetCCOption) -DGCC_COMPILE=1 -ffunction-sections -fdata-sections
LD_MAP_OPTION = -Wl,--ckmap=$(BUILD)/firmware.map
LD_LIB_OPTION = -Wl,--start-group -Wl,--end-group
LD_DEFAULT_OPTIONS = $(ExtraLinkOption) $(LD_MAP_OPTION) $(TargetLDOption) -nostartfiles -Wl,--gc-sections -T"$(SDKPath)/ld/$(TargetBoard)/gcc_csky.ld" $(LD_LIB_OPTION) 

DFU = $(TOP)/tools/dfu.py
PYDFU = $(TOP)/tools/pydfu.py

CFLAGS = $(INC) $(CC_DEFAULT_OPTIONS) $(COPT) $(CFLAGS_MOD)
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

#-ljson -llwip_open -lpp -lnet80211 -lwpa -lphy -lnet80211
#LIBS += dsp c cc-rt m semi ssp stdc++ supc++
LIBS += semi

SRC_DIR = \
	src

SRC_C = \
	$(wildcard $(addsuffix /*.c,$(SRC_DIR))) \
	shared/libc/printf.c \
	shared/libc/string0.c \
	shared/libc/__errno.c \
	shared/libc/abort_.o \
	shared/netutils/netutils.c \
	shared/readline/readline.c \
	shared/timeutils/timeutils.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \
	shared/runtime/interrupt_char.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \
	shared/runtime/sys_stdio_mphal.c \
	$(BUILD)/frozen_content.c


SRC_CXX += \
	$(wildcard $(addsuffix /*.cc,$(SRC_DIR))) \
	$(wildcard $(addsuffix /*.cpp,$(SRC_DIR))) \
	$(wildcard $(addsuffix /*.cxx,$(SRC_DIR)))

SRC_ASM =

#	modlwip.c 

EXTMOD_SRC_C +=extmod/modonewire.c
EXTMOD_SRC_C +=extmod/modurandom.c
EXTMOD_SRC_C +=extmod/moduasyncio.c
EXTMOD_SRC_C +=extmod/moductypes.c
EXTMOD_SRC_C +=extmod/moduzlib.c
EXTMOD_SRC_C +=extmod/modujson.c
EXTMOD_SRC_C +=extmod/modure.c
EXTMOD_SRC_C +=extmod/moduheapq.c
EXTMOD_SRC_C +=extmod/moduhashlib.c
EXTMOD_SRC_C +=extmod/modubinascii.c
EXTMOD_SRC_C +=extmod/moduselect.c
EXTMOD_SRC_C +=extmod/modframebuf.c
EXTMOD_SRC_C +=extmod/modussl_axtls.c
EXTMOD_SRC_C +=extmod/vfs.c
EXTMOD_SRC_C +=extmod/vfs_reader.c
EXTMOD_SRC_C +=extmod/vfs_fat.c
EXTMOD_SRC_C +=extmod/vfs_fat_file.c
EXTMOD_SRC_C +=extmod/vfs_fat_diskio.c
EXTMOD_SRC_C +=extmod/vfs_blockdev.c
EXTMOD_SRC_C +=extmod/uos_dupterm.c


DRIVERS_SRC_C +=drivers/bus/softspi.c
DRIVERS_SRC_C +=drivers/dht/dht.c

LIB_SRC_C = $(addprefix lib/,\
	oofatfs/ff.c \
	oofatfs/ffunicode.c \
	crypto-algorithms/sha256.c \
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

ifeq ($(CROSS), 1)
SRC_C += $(wildcard $(addsuffix /*.c,$(PLATFORM_SRCDIR)))
SRC_ASM += src/__rt_entry.S
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
OBJ += $(addprefix $(BUILD)/, $(EXTMOD_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(DRIVERS_SRC_C:.c=.o))

ifeq ($(CROSS), 1)
OBJ += $(addprefix $(BUILD)/, $(SRC_ASM:.S=.o))
endif

ifneq ($(FROZEN_MANIFEST),)
CFLAGS += -DMICROPY_MODULE_FROZEN_STR
CFLAGS += -DMICROPY_MODULE_FROZEN_MPY
CFLAGS += -DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
endif

ifeq ($(CROSS), 1)
all: $(BUILD)/firmware.dfu
else
all: $(BUILD)/firmware.elf
endif

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(addprefix -L,$(LIBDIRS)) $(addprefix -l,$(LIBS))
	$(Q)$(SIZE) $@


zip := $(if $(zip),$(zip),0)
sign := $(if $(sign),$(sign),0)
encrypt := $(if $(encrypt),$(encrypt),0)
$(BUILD)/firmware.dfu: $(BUILD)/firmware.elf
	@$(OBJCOPY) -O binary $(BUILD)/firmware.elf $(BUILD)/firmware.bin
	@ls --color=auto -Ugo $(BUILD)/firmware.elf $(BUILD)/firmware.bin
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
