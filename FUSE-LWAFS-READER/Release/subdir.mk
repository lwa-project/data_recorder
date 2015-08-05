################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Disk.c \
../FileSystem.c \
../HostInterface.c \
../Time.c \
../fuse-lwafs.c 

OBJS += \
./Disk.o \
./FileSystem.o \
./HostInterface.o \
./Time.o \
./fuse-lwafs.o 

C_DEPS += \
./Disk.d \
./FileSystem.d \
./HostInterface.d \
./Time.d \
./fuse-lwafs.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -I/usr/include/fuse -O3 -g3 -Wall -pthread -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


