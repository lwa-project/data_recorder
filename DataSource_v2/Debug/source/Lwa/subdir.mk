################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../source/Lwa/DrxFrameGenerator.cpp \
../source/Lwa/TbnFrameGenerator.cpp \
../source/Lwa/TbwFrameGenerator.cpp \
../source/Lwa/TbfFrameGenerator.cpp \
../source/Lwa/CorFrameGenerator.cpp

OBJS += \
./source/Lwa/DrxFrameGenerator.o \
./source/Lwa/TbnFrameGenerator.o \
./source/Lwa/TbwFrameGenerator.o \
./source/Lwa/TbfFrameGenerator.o \
./source/Lwa/CorFrameGenerator.o

CPP_DEPS += \
./source/Lwa/DrxFrameGenerator.d \
./source/Lwa/TbnFrameGenerator.d \
./source/Lwa/TbwFrameGenerator.d \
./source/Lwa/TbfFrameGenerator.d \
./source/Lwa/CorFrameGenerator.d


# Each subdirectory must supply rules for building sources it contributes
source/Lwa/%.o: ../source/Lwa/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


