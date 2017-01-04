################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
cjmbus_water.bin \

SECONDARY_LIST += \
cjmbus_water.lst \

SECONDARY_SIZE += \
cjmbus_water.siz \


# All Target
all: cjmbus_water secondary-outputs

# Tool invocations
cjmbus_water: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Sourcery Linux GCC C++ Linker'
	arm-none-linux-gnueabi-gcc -L"/opt/develop/jzq/software/apps/cjpro/libBase/Release" -L"/opt/develop/jzq/software/apps/cjpro/libCjt188/Release" -Wl,-Map,cjmbus_water.map -mcpu=arm926ej-s -o "cjmbus_water" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

cjmbus_water.bin: cjmbus_water
	@echo 'Invoking: ARM Sourcery Linux GNU Create Flash Image'
	arm-none-linux-gnueabi-objcopy -O binary cjmbus_water  "cjmbus_water.bin"
	@echo 'Finished building: $@'
	@echo ' '

cjmbus_water.lst: cjmbus_water
	@echo 'Invoking: ARM Sourcery Linux GNU Create Listing'
	arm-none-linux-gnueabi-objdump -h -S cjmbus_water > "cjmbus_water.lst"
	@echo 'Finished building: $@'
	@echo ' '

cjmbus_water.siz: cjmbus_water
	@echo 'Invoking: ARM Sourcery Linux GNU Print Size'
	arm-none-linux-gnueabi-size  --format=berkeley cjmbus_water
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(SECONDARY_FLASH)$(CXX_DEPS)$(S_UPPER_DEPS)$(SECONDARY_LIST)$(SECONDARY_SIZE)$(C++_DEPS)$(ASM_DEPS)$(CC_DEPS)$(CPP_DEPS)$(EXECUTABLES)$(C_UPPER_DEPS) cjmbus_water
	-@echo ' '

post-build:
	-arm-none-linux-gnueabi-strip cjmbus_water && cp cjmbus_water ../../bin_arm
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
