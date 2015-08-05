################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../source/DataSource_v2.cpp \
../source/Socket.cpp \
../source/TimeKeeper.cpp 

OBJS += \
./source/DataSource_v2.o \
./source/Socket.o \
./source/TimeKeeper.o 

CPP_DEPS += \
./source/DataSource_v2.d \
./source/Socket.d \
./source/TimeKeeper.d 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


