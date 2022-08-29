################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Messaging/IpSpec.cpp \
../Messaging/Message.cpp \
../Messaging/WaitQueueManager.cpp 

OBJS_L += \
./Messaging/IpSpec_L.o \
./Messaging/Message_L.o \
./Messaging/WaitQueueManager_L.o 

OBJS_S += \
./Messaging/IpSpec_S.o \
./Messaging/Message_S.o \
./Messaging/WaitQueueManager_S.o 

CPP_DEPS += \
./Messaging/IpSpec.d \
./Messaging/Message.d \
./Messaging/WaitQueueManager.d 


# Each subdirectory must supply rules for building sources it contributes
Messaging/%_L.o: ../Messaging/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -DDROS_LIVE_BUFFER -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Messaging/%_S.o: ../Messaging/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
