################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/broker.c \
../src/buddy_system.c \
../src/dynamic_partitions.c \
../src/logger.c \
../src/memory.c \
../src/memory_commons.c \
../src/messages_queues.c 

OBJS += \
./src/broker.o \
./src/buddy_system.o \
./src/dynamic_partitions.o \
./src/logger.o \
./src/memory.o \
./src/memory_commons.o \
./src/messages_queues.o 

C_DEPS += \
./src/broker.d \
./src/buddy_system.d \
./src/dynamic_partitions.d \
./src/logger.d \
./src/memory.d \
./src/memory_commons.d \
./src/messages_queues.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2020-1c-Fran-co/nuestras-commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


