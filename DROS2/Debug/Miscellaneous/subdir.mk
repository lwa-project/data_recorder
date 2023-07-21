################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Miscellaneous/BackTrace.cpp \
../Miscellaneous/SignalExceptionListener.cpp 

OBJS_L += \
./Miscellaneous/BackTrace_L.o \
./Miscellaneous/SignalExceptionListener_L.o 

OBJS_S += \
./Miscellaneous/BackTrace_S.o \
./Miscellaneous/SignalExceptionListener_S.o 

CPP_DEPS += \
./Miscellaneous/BackTrace.d \
./Miscellaneous/SignalExceptionListener.d 


# Each subdirectory must supply rules for building sources it contributes
Miscellaneous/%_L.o: ../Miscellaneous/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -DDROS_LIVE_BUFFER -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Miscellaneous/%_S.o: ../Miscellaneous/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
