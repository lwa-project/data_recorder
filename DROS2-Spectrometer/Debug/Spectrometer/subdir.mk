################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Spectrometer/DrxSpectrometer.cpp \
../Spectrometer/Spectrometer.cpp \
../Spectrometer/TestPattern.cpp 

OBJS_S += \
./Spectrometer/DrxSpectrometer_S.o \
./Spectrometer/Spectrometer_S.o \
./Spectrometer/TestPattern_S.o 

CPP_DEPS += \
./Spectrometer/DrxSpectrometer.d \
./Spectrometer/Spectrometer.d \
./Spectrometer/TestPattern.d 


# Each subdirectory must supply rules for building sources it contributes
Spectrometer/%_S.o: ../Spectrometer/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
