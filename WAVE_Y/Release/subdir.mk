################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../BeamCorrelator.c \
../Commands.c \
../Config.c \
../DataFormats.c \
../Disk.c \
../FileSystem.c \
../HostInterface.c \
../LiveCapture.c \
../Log.c \
../MIB.c \
../Main.c \
../Message.c \
../MessageQueue.c \
../OpJup.c \
../OpSpec.c \
../OpXcp.c \
../Operations.c \
../Persistence.c \
../Profiler.c \
../RingQueue.c \
../Schedule.c \
../Socket.c \
../Spectrometer.c \
../TempFile.c \
../Time.c 

OBJS += \
./BeamCorrelator.o \
./Commands.o \
./Config.o \
./DataFormats.o \
./Disk.o \
./FileSystem.o \
./HostInterface.o \
./LiveCapture.o \
./Log.o \
./MIB.o \
./Main.o \
./Message.o \
./MessageQueue.o \
./OpJup.o \
./OpSpec.o \
./OpXcp.o \
./Operations.o \
./Persistence.o \
./Profiler.o \
./RingQueue.o \
./Schedule.o \
./Socket.o \
./Spectrometer.o \
./TempFile.o \
./Time.o 

C_DEPS += \
./BeamCorrelator.d \
./Commands.d \
./Config.d \
./DataFormats.d \
./Disk.d \
./FileSystem.d \
./HostInterface.d \
./LiveCapture.d \
./Log.d \
./MIB.d \
./Main.d \
./Message.d \
./MessageQueue.d \
./OpJup.d \
./OpSpec.d \
./OpXcp.d \
./Operations.d \
./Persistence.d \
./Profiler.d \
./RingQueue.d \
./Schedule.d \
./Socket.d \
./Spectrometer.d \
./TempFile.d \
./Time.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -D_GNU_SOURCE -O3 -Wall -c -fmessage-length=0 -v -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


