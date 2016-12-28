################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../CANopen_stack/CANopen.c \
../CANopen_stack/CO_Emergency.c \
../CANopen_stack/CO_HBconsumer.c \
../CANopen_stack/CO_NMT_Heartbeat.c \
../CANopen_stack/CO_PDO.c \
../CANopen_stack/CO_SDO.c \
../CANopen_stack/CO_SDOmaster.c \
../CANopen_stack/CO_SYNC.c 

OBJS += \
./CANopen_stack/CANopen.o \
./CANopen_stack/CO_Emergency.o \
./CANopen_stack/CO_HBconsumer.o \
./CANopen_stack/CO_NMT_Heartbeat.o \
./CANopen_stack/CO_PDO.o \
./CANopen_stack/CO_SDO.o \
./CANopen_stack/CO_SDOmaster.o \
./CANopen_stack/CO_SYNC.o 

C_DEPS += \
./CANopen_stack/CANopen.d \
./CANopen_stack/CO_Emergency.d \
./CANopen_stack/CO_HBconsumer.d \
./CANopen_stack/CO_NMT_Heartbeat.d \
./CANopen_stack/CO_PDO.d \
./CANopen_stack/CO_SDO.d \
./CANopen_stack/CO_SDOmaster.d \
./CANopen_stack/CO_SYNC.d 


# Each subdirectory must supply rules for building sources it contributes
CANopen_stack/%.o: ../CANopen_stack/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DDEBUG -D__CODE_RED -D__REDLIB__ -DARM9platform=1 -Dxdata="" -Ddata="" -DdefEnable_Anybus_Module=1 -Ddef_canopen_enable -Dcode=const -I"/home/michele/LPCXpresso/workspace/spiralatrice/inc" -I"/home/michele/LPCXpresso/workspace/spiralatrice/CANopen_stack" -I"/home/michele/LPCXpresso/workspace/spiralatrice/CANopen_stack/LPC2388" -I"/home/michele/LPCXpresso/workspace/spiralatrice/ff9/src" -Os -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mthumb-interwork -fshort-enums -fpack-struct -Wno-pointer-sign -Wno-unused-local-typedefs -fdata-sections -ffunction-sections -mcpu=arm7tdmi -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


