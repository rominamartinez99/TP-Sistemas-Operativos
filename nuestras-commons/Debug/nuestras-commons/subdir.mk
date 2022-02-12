################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../nuestras-commons/conexion.c \
../nuestras-commons/mensajes.c 

OBJS += \
./nuestras-commons/conexion.o \
./nuestras-commons/mensajes.o 

C_DEPS += \
./nuestras-commons/conexion.d \
./nuestras-commons/mensajes.d 


# Each subdirectory must supply rules for building sources it contributes
nuestras-commons/%.o: ../nuestras-commons/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


