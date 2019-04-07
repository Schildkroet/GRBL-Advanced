#--------------------------------------------------------------------------------
# Makefile
#
# Patrick F.
# GRBL Advanced
#
#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
# Location of gcc-arm-none-eabi toolchain
GCC_BASE	= 	/opt/gcc-arm-none-eabi-8-2018-q4-major/bin

CC          =   ${GCC_BASE}/arm-none-eabi-gcc
CXX         =   ${GCC_BASE}/arm-none-eabi-g++
OBJCPY		=	${GCC_BASE}/arm-none-eabi-objcopy
SIZE		=	${GCC_BASE}/arm-none-eabi-size
OBJDUMP		= 	${GCC_BASE}/arm-none-eabi-objdump

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	GRBL_Advanced
BUILD       :=	build
SOURCES		:=	./ cmsis/ grbl/ HAL/ HAL/EXTI HAL/FLASH HAL/GPIO HAL/I2C HAL/SPI HAL/STM32 HAL/TIM HAL/USART SPL/src Src/

INCLUDES    :=	$(SOURCES) SPL/inc

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
FLAGS       := 	-mfloat-abi=hard -mcpu=cortex-m4 -gdwarf-2 -mfpu=fpv4-sp-d16 -mthumb
CFLAGS      := 	-O2 -g1 -std=c11 -Wall -Wextra $(INCLUDE) -fno-common -fsingle-precision-constant -fdata-sections -ffunction-sections -fomit-frame-pointer -mlittle-endian  -DUSE_STDPERIPH_DRIVER -DSTM32F411xE -DSTM32F411RE -D__FPU_USED -DARM_MATH_CM4 -Wimplicit-fallthrough=0
CXXFLAGS    :=  $(CFLAGS)

LDFLAGS		:=	-lm -flto -Wl,--gc-sections -T../stm32f411re_flash.ld -Wl,-M=$(OUTPUT).map --specs=nosys.specs -nostartfiles --specs=nano.specs

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
# the order can-be/is critical
#---------------------------------------------------------------------------------
LIBS        :=

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS		:=

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES			:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES		:= 	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES			:= 	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(sort $(CFILES:.c=.c.o) \
							$(CPPFILES:.cpp=.cpp.o) \
							$(SFILES:.S=.S.o))

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES), -I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export OUTPUT	:=	$(CURDIR)/$(TARGET)

.PHONY: $(BUILD) clean flash

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) $(OUTPUT).elf $(OUTPUT).bin $(OUTPUT).hex $(OUTPUT).lst -f $(CURDIR)/Makefile -j2
	@$(SIZE) $(OUTPUT).elf

#---------------------------------------------------------------------------------
clean:
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).bin $(OUTPUT).hex $(OUTPUT).map $(OUTPUT).lst

#---------------------------------------------------------------------------------
flash: $(OUTPUT).bin
	st-flash write $(OUTPUT).bin 0x8000000

#---------------------------------------------------------------------------------
else

DEPENDS     :=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).elf: $(OFILES)
	@echo Linking executable...
	@$(LD) -o $@ $^ $(FLAGS) $(LDFLAGS)

$(OUTPUT).bin: $(OUTPUT).elf
	@echo Creating bin...
	@$(OBJCPY) -O binary $< $@

$(OUTPUT).hex: $(OUTPUT).elf
	@echo Creating hex...
	@$(OBJCPY) -O ihex $< $@

$(OUTPUT).lst: $(OUTPUT).elf
	@$(OBJDUMP) -d $< > $@

#---------------------------------------------------------------------------------
# This rule links in binary data with the .c extension
#---------------------------------------------------------------------------------
%.c.o	:	%.c
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(CC) $(FLAGS) $(CFLAGS) -c $^ -o $@

#---------------------------------------------------------------------------------
# This rule links in binary data with the .cpp extension
#---------------------------------------------------------------------------------
%.cpp.o	:	%.cpp
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(CXX) $(FLAGS) $(CXXFLAGS) -c $^ -o $@

#---------------------------------------------------------------------------------
# This rule links in binary data with the .S extension
#---------------------------------------------------------------------------------
%.S.o	:	%.S
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(CC) $(FLAGS) $(CFLAGS) -c $^ -o $@


-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
