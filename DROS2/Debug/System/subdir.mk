################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../System/Config.cpp \
../System/CpuInfo.cpp \
../System/HddInfo.cpp \
../System/Log.cpp \
../System/Shell.cpp \
../System/Storage.cpp \
../System/System.cpp \
../System/Time.cpp 

OBJS += \
./System/Config.o \
./System/CpuInfo.o \
./System/HddInfo.o \
./System/Log.o \
./System/Shell.o \
./System/Storage.o \
./System/System.o \
./System/Time.o 

CPP_DEPS += \
./System/Config.d \
./System/CpuInfo.d \
./System/HddInfo.d \
./System/Log.d \
./System/Shell.d \
./System/Storage.d \
./System/System.d \
./System/Time.d 


# Each subdirectory must supply rules for building sources it contributes
System/%.o: ../System/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


