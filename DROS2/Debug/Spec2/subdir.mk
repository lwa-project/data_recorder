################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Spec2/SpecBlock.cpp 

OBJS += \
./Spec2/SpecBlock.o 

CPP_DEPS += \
./Spec2/SpecBlock.d 


# Each subdirectory must supply rules for building sources it contributes
Spec2/%.o: ../Spec2/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++-4.7 -D__SSE3__ -I/usr/gcc_4_7/lib/gcc/x86_64-linux-gnu/4.7.2/include -I/usr/gcc_4_7/include/c++/4.7.2 -O3 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -std=c++11 -march=native -msse3 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


