################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Buffers/GenericBuffer.cpp \
../Buffers/ParcelMemQueue.cpp \
../Buffers/ParcelQueue.cpp \
../Buffers/SimpleRingBuffer.cpp 

OBJS_L += \
./Buffers/GenericBuffer_L.o \
./Buffers/ParcelMemQueue_L.o \
./Buffers/ParcelQueue_L.o \
./Buffers/SimpleRingBuffer_L.o 

OBJS_S += \
./Buffers/GenericBuffer_S.o \
./Buffers/ParcelMemQueue_S.o \
./Buffers/ParcelQueue_S.o \
./Buffers/SimpleRingBuffer_S.o 

CPP_DEPS += \
./Buffers/GenericBuffer.d \
./Buffers/ParcelMemQueue.d \
./Buffers/ParcelQueue.d \
./Buffers/SimpleRingBuffer.d 


# Each subdirectory must supply rules for building sources it contributes
Buffers/%_L.o: ../Buffers/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -DDROS_LIVE_BUFFER -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Buffers/%_S.o: ../Buffers/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
