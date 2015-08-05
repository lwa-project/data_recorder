################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Threads/ThreadManager.cpp 

OBJS += \
./Threads/ThreadManager.o 

CPP_DEPS += \
./Threads/ThreadManager.d 


# Each subdirectory must supply rules for building sources it contributes
Threads/%.o: ../Threads/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


