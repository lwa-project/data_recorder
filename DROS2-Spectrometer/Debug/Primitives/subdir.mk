################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Primitives/EnumTypes.cpp \
../Primitives/StringStringMap.cpp 

OBJS += \
./Primitives/EnumTypes.o \
./Primitives/StringStringMap.o 

CPP_DEPS += \
./Primitives/EnumTypes.d \
./Primitives/StringStringMap.d 


# Each subdirectory must supply rules for building sources it contributes
Primitives/%.o: ../Primitives/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


