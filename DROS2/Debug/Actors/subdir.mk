################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Actors/MessageListener.cpp \
../Actors/MessageProcessor.cpp \
../Actors/Schedule.cpp 

OBJS_L += \
./Actors/MessageListener_L.o \
./Actors/MessageProcessor_L.o \
./Actors/Schedule_L.o 

OBJS_S += \
./Actors/MessageListener_S.o \
./Actors/MessageProcessor_S.o \
./Actors/Schedule_S.o 

CPP_DEPS += \
./Actors/MessageListener.d \
./Actors/MessageProcessor.d \
./Actors/Schedule.d 


# Each subdirectory must supply rules for building sources it contributes
Actors/%_L.o: ../Actors/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -DDROS_LIVE_BUFFER -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Actors/%_S.o: ../Actors/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
