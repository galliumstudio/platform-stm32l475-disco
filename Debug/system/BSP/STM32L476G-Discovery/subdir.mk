################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../system/BSP/STM32L476G-Discovery/stm32l476g_discovery.c \
../system/BSP/STM32L476G-Discovery/stm32l476g_discovery_audio.c \
../system/BSP/STM32L476G-Discovery/stm32l476g_discovery_compass.c \
../system/BSP/STM32L476G-Discovery/stm32l476g_discovery_glass_lcd.c \
../system/BSP/STM32L476G-Discovery/stm32l476g_discovery_gyroscope.c \
../system/BSP/STM32L476G-Discovery/stm32l476g_discovery_idd.c \
../system/BSP/STM32L476G-Discovery/stm32l476g_discovery_qspi.c 

OBJS += \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery.o \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery_audio.o \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery_compass.o \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery_glass_lcd.o \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery_gyroscope.o \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery_idd.o \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery_qspi.o 

C_DEPS += \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery.d \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery_audio.d \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery_compass.d \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery_glass_lcd.d \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery_gyroscope.d \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery_idd.d \
./system/BSP/STM32L476G-Discovery/stm32l476g_discovery_qspi.d 


# Each subdirectory must supply rules for building sources it contributes
system/BSP/STM32L476G-Discovery/%.o: ../system/BSP/STM32L476G-Discovery/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O3 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -ffreestanding -fno-move-loop-invariants -Wall -Wextra  -g3 -DDEBUG -DTRACE -DSTM32L476xx -I"../include" -I"../system/include" -I"../system/include/cmsis" -I"../system/include/stm32l4xx" -I"../system/BSP/STM32L476G-Discovery" -I"../system/BSP/Components" -I"../framework/include" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


