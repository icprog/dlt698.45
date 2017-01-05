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
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
cjcomm.bin \

SECONDARY_LIST += \
cjcomm.lst \

SECONDARY_SIZE += \
cjcomm.siz \


# All Target
all: cjcomm secondary-outputs

# Tool invocations
cjcomm: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Sourcery Linux GCC C++ Linker'
	arm-none-linux-gnueabi-gcc -L"/home/z/Desktop/Task/698/libAccess/Release" -L"/home/z/Desktop/Task/698/lib698/Release" -L"/home/z/Desktop/Task/698/libBase/Release" -Wl,-Map,cjcomm.map -mcpu=arm926ej-s -o "cjcomm" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

cjcomm.bin: cjcomm
	@echo 'Invoking: ARM Sourcery Linux GNU Create Flash Image'
	arm-none-linux-gnueabi-objcopy -O binary cjcomm  "cjcomm.bin"
	@echo 'Finished building: $@'
	@echo ' '

cjcomm.lst: cjcomm
	@echo 'Invoking: ARM Sourcery Linux GNU Create Listing'
	arm-none-linux-gnueabi-objdump -h -S cjcomm > "cjcomm.lst"
	@echo 'Finished building: $@'
	@echo ' '

cjcomm.siz: cjcomm
	@echo 'Invoking: ARM Sourcery Linux GNU Print Size'
	arm-none-linux-gnueabi-size  --format=berkeley cjcomm
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(C_UPPER_DEPS)$(SECONDARY_LIST)$(SECONDARY_SIZE)$(C_DEPS)$(CC_DEPS)$(C++_DEPS)$(EXECUTABLES)$(OBJS)$(CXX_DEPS)$(SECONDARY_FLASH)$(ASM_DEPS)$(S_UPPER_DEPS)$(CPP_DEPS) cjcomm
	-@echo ' '

post-build:
	-arm-none-linux-gnueabi-strip cjcomm && cp cjcomm ../../bin_arm
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
