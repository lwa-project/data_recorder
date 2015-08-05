################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../ChirpGenerator.cpp \
../DrxFrameGenerator.cpp \
../GaussianGenerator.cpp \
../SignalGenerator.cpp \
../SineGenerator.cpp \
../Socket.cpp \
../TimeKeeper.cpp \
../main.cpp 

OBJS += \
./ChirpGenerator.o \
./DrxFrameGenerator.o \
./GaussianGenerator.o \
./SignalGenerator.o \
./SineGenerator.o \
./Socket.o \
./TimeKeeper.o \
./main.o 

CPP_DEPS += \
./ChirpGenerator.d \
./DrxFrameGenerator.d \
./GaussianGenerator.d \
./SignalGenerator.d \
./SineGenerator.d \
./Socket.d \
./TimeKeeper.d \
./main.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


