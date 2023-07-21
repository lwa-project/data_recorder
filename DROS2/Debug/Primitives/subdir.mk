################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Primitives/EnumTypes.cpp \
../Primitives/StringStringMap.cpp 

OBJS_L += \
./Primitives/EnumTypes_L.o \
./Primitives/StringStringMap_L.o 

OBJS_S += \
./Primitives/EnumTypes_S.o \
./Primitives/StringStringMap_S.o 

CPP_DEPS += \
./Primitives/EnumTypes.d \
./Primitives/StringStringMap.d 


# Each subdirectory must supply rules for building sources it contributes
Primitives/%_L.o: ../Primitives/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -DDROS_LIVE_BUFFER -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Primitives/%_S.o: ../Primitives/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
