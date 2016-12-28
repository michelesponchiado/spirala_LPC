################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../CANopen_stack/dsPIC30F/CO_driver.c \
../CANopen_stack/dsPIC30F/eeprom.c \
../CANopen_stack/dsPIC30F/memcpyram2flash.c 

OBJS += \
./CANopen_stack/dsPIC30F/CO_driver.o \
./CANopen_stack/dsPIC30F/eeprom.o \
./CANopen_stack/dsPIC30F/memcpyram2flash.o 

C_DEPS += \
./CANopen_stack/dsPIC30F/CO_driver.d \
./CANopen_stack/dsPIC30F/eeprom.d \
./CANopen_stack/dsPIC30F/memcpyram2flash.d 


# Each subdirectory must supply rules for building sources it contributes
CANopen_stack/dsPIC30F/%.o: ../CANopen_stack/dsPIC30F/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DDEBUG -D__CODE_RED -D__REDLIB__ -DARM9platform=1 -Dxdata="" -Ddata="" -DdefEnable_Anybus_Module=1 -Ddef_canopen_enable -DRemovable=1 -Dcode=const -I"/home/michele/LPCXpresso/workspace/spiralatrice/inc" -I"/home/michele/LPCXpresso/workspace/spiralatrice/CANopen_stack" -I"/home/michele/LPCXpresso/workspace/spiralatrice/CANopen_stack/LPC2388" -I"/home/michele/LPCXpresso/workspace/spiralatrice/ff9/src" -O0 -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=arm7tdmi -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


