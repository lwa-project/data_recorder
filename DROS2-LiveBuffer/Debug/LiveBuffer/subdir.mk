################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../LiveBuffer/LiveBuffer.cpp 

OBJS += \
./LiveBuffer/LiveBuffer.o 

CPP_DEPS += \
./LiveBuffer/LiveBuffer.d 


# Each subdirectory must supply rules for building sources it contributes
LiveBuffer/%.o: ../LiveBuffer/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
