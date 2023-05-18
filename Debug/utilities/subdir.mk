################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../utilities/fsl_assert.c \
../utilities/fsl_debug_console.c 

C_DEPS += \
./utilities/fsl_assert.d \
./utilities/fsl_debug_console.d 

OBJS += \
./utilities/fsl_assert.o \
./utilities/fsl_debug_console.o 


# Each subdirectory must supply rules for building sources it contributes
utilities/%.o: ../utilities/%.c utilities/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MK66FN2M0VMD18 -DCPU_MK66FN2M0VMD18_cm4 -DFLEXCAN_WAIT_TIMEOUT=1000 -DFRDM_K66F -DFREEDOM -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\noeas\Documents\MCUXpressoIDE_11.7.0_9198\workspace\frdmk66f_final_flexcan_loopback\source" -I"C:\Users\noeas\Documents\MCUXpressoIDE_11.7.0_9198\workspace\frdmk66f_final_flexcan_loopback\utilities" -I"C:\Users\noeas\Documents\MCUXpressoIDE_11.7.0_9198\workspace\frdmk66f_final_flexcan_loopback\drivers" -I"C:\Users\noeas\Documents\MCUXpressoIDE_11.7.0_9198\workspace\frdmk66f_final_flexcan_loopback\device" -I"C:\Users\noeas\Documents\MCUXpressoIDE_11.7.0_9198\workspace\frdmk66f_final_flexcan_loopback\component\uart" -I"C:\Users\noeas\Documents\MCUXpressoIDE_11.7.0_9198\workspace\frdmk66f_final_flexcan_loopback\component\lists" -I"C:\Users\noeas\Documents\MCUXpressoIDE_11.7.0_9198\workspace\frdmk66f_final_flexcan_loopback\CMSIS" -I"C:\Users\noeas\Documents\MCUXpressoIDE_11.7.0_9198\workspace\frdmk66f_final_flexcan_loopback\board" -I"C:\Users\noeas\Documents\MCUXpressoIDE_11.7.0_9198\workspace\frdmk66f_final_flexcan_loopback\frdmk66f\driver_examples\flexcan\loopback" -O0 -fno-common -g3 -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-utilities

clean-utilities:
	-$(RM) ./utilities/fsl_assert.d ./utilities/fsl_assert.o ./utilities/fsl_debug_console.d ./utilities/fsl_debug_console.o

.PHONY: clean-utilities

