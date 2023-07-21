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

OBJS_L += \
./System/Config_L.o \
./System/CpuInfo_L.o \
./System/HddInfo_L.o \
./System/Log_L.o \
./System/Shell_L.o \
./System/Storage_L.o \
./System/System_L.o \
./System/Time_L.o 

OBJS_S += \
./System/Config_S.o \
./System/CpuInfo_S.o \
./System/HddInfo_S.o \
./System/Log_S.o \
./System/Shell_S.o \
./System/Storage_S.o \
./System/System_S.o \
./System/Time_S.o 

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
System/%_L.o: ../System/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -DDROS_LIVE_BUFFER -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

System/%_S.o: ../System/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
