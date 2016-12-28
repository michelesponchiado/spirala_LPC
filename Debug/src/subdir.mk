################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Anybus.c \
../src/I2C_Master.c \
../src/az4186_can.c \
../src/az4186_dac.c \
../src/az4186_eeprom_24c04.c \
../src/az4186_emc.c \
../src/az4186_files.c \
../src/az4186_initHw.c \
../src/az4186_program_codes.c \
../src/az4186_temperature_compensation.c \
../src/az4186_window_temperature_compensation.c \
../src/codelist.c \
../src/crp.c \
../src/diskio.c \
../src/dma.c \
../src/fat_time.c \
../src/file_list.c \
../src/gui.c \
../src/i2c.c \
../src/inverse_v1.c \
../src/lcd.c \
../src/main.c \
../src/mci.c \
../src/params.c \
../src/popup.c \
../src/rtcDS3231_newer.c \
../src/spi.c \
../src/spidbg.c \
../src/spiglb.c \
../src/spimis.c \
../src/spiutils.c \
../src/spiwin2.c \
../src/target.c \
../src/vdip.c \
../src/xxtea.c 

S_SRCS += \
../src/cr_startup_lpc23.s 

OBJS += \
./src/Anybus.o \
./src/I2C_Master.o \
./src/az4186_can.o \
./src/az4186_dac.o \
./src/az4186_eeprom_24c04.o \
./src/az4186_emc.o \
./src/az4186_files.o \
./src/az4186_initHw.o \
./src/az4186_program_codes.o \
./src/az4186_temperature_compensation.o \
./src/az4186_window_temperature_compensation.o \
./src/codelist.o \
./src/cr_startup_lpc23.o \
./src/crp.o \
./src/diskio.o \
./src/dma.o \
./src/fat_time.o \
./src/file_list.o \
./src/gui.o \
./src/i2c.o \
./src/inverse_v1.o \
./src/lcd.o \
./src/main.o \
./src/mci.o \
./src/params.o \
./src/popup.o \
./src/rtcDS3231_newer.o \
./src/spi.o \
./src/spidbg.o \
./src/spiglb.o \
./src/spimis.o \
./src/spiutils.o \
./src/spiwin2.o \
./src/target.o \
./src/vdip.o \
./src/xxtea.o 

C_DEPS += \
./src/Anybus.d \
./src/I2C_Master.d \
./src/az4186_can.d \
./src/az4186_dac.d \
./src/az4186_eeprom_24c04.d \
./src/az4186_emc.d \
./src/az4186_files.d \
./src/az4186_initHw.d \
./src/az4186_program_codes.d \
./src/az4186_temperature_compensation.d \
./src/az4186_window_temperature_compensation.d \
./src/codelist.d \
./src/crp.d \
./src/diskio.d \
./src/dma.d \
./src/fat_time.d \
./src/file_list.d \
./src/gui.d \
./src/i2c.d \
./src/inverse_v1.d \
./src/lcd.d \
./src/main.d \
./src/mci.d \
./src/params.d \
./src/popup.d \
./src/rtcDS3231_newer.d \
./src/spi.d \
./src/spidbg.d \
./src/spiglb.d \
./src/spimis.d \
./src/spiutils.d \
./src/spiwin2.d \
./src/target.d \
./src/vdip.d \
./src/xxtea.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DDEBUG -D__CODE_RED -D__REDLIB__ -DARM9platform=1 -Dxdata="" -Ddata="" -DdefEnable_Anybus_Module=1 -Ddef_canopen_enable -Dcode=const -I"/home/michele/LPCXpresso/workspace/spiralatrice/inc" -I"/home/michele/LPCXpresso/workspace/spiralatrice/CANopen_stack" -I"/home/michele/LPCXpresso/workspace/spiralatrice/CANopen_stack/LPC2388" -I"/home/michele/LPCXpresso/workspace/spiralatrice/ff9/src" -Os -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mthumb-interwork -fshort-enums -fpack-struct -Wno-pointer-sign -Wno-unused-local-typedefs -fdata-sections -ffunction-sections -mcpu=arm7tdmi -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU Assembler'
	arm-none-eabi-gcc -c -x assembler-with-cpp -DDEBUG -D__CODE_RED -D__REDLIB__ -g3 -mcpu=arm7tdmi -specs=redlib.specs -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


