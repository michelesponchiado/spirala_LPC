################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ff9/src/option/ccsbcs.c \
../ff9/src/option/syscall.c 

OBJS += \
./ff9/src/option/ccsbcs.o \
./ff9/src/option/syscall.o 

C_DEPS += \
./ff9/src/option/ccsbcs.d \
./ff9/src/option/syscall.d 


# Each subdirectory must supply rules for building sources it contributes
ff9/src/option/%.o: ../ff9/src/option/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DDEBUG -D__CODE_RED -D__REDLIB__ -DARM9platform=1 -Dxdata="" -Ddata="" -DdefEnable_Anybus_Module=1 -Ddef_canopen_enable -Dcode=const -I"/home/michele/LPCXpresso/workspace/spiralatrice/inc" -I"/home/michele/LPCXpresso/workspace/spiralatrice/CANopen_stack" -I"/home/michele/LPCXpresso/workspace/spiralatrice/CANopen_stack/LPC2388" -I"/home/michele/LPCXpresso/workspace/spiralatrice/ff9/src" -Os -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mthumb-interwork -fshort-enums -fpack-struct -Wno-pointer-sign -Wno-unused-local-typedefs -fdata-sections -ffunction-sections -mcpu=arm7tdmi -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


