################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Common/CommonICD.cpp \
../Common/Utility.cpp \
../Common/misc.cpp 

OBJS += \
./Common/CommonICD.o \
./Common/Utility.o \
./Common/misc.o 

CPP_DEPS += \
./Common/CommonICD.d \
./Common/Utility.d \
./Common/misc.d 


# Each subdirectory must supply rules for building sources it contributes
Common/%.o: ../Common/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


