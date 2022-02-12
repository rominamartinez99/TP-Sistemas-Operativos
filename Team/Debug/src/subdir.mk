################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/funcionesUtilesTeam.c \
../src/logger.c \
../src/planificador.c \
../src/team.c 

OBJS += \
./src/funcionesUtilesTeam.o \
./src/logger.o \
./src/planificador.o \
./src/team.o 

C_DEPS += \
./src/funcionesUtilesTeam.d \
./src/logger.d \
./src/planificador.d \
./src/team.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2020-1c-Fran-co/nuestras-commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


