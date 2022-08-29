################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Common/CommonICD.cpp \
../Common/Utility.cpp \
../Common/misc.cpp 

OBJS_L += \
./Common/CommonICD_L.o \
./Common/Utility_L.o \
./Common/misc_L.o 

OBJS_S += \
./Common/CommonICD_S.o \
./Common/Utility_S.o \
./Common/misc_S.o 

CPP_DEPS += \
./Common/CommonICD.d \
./Common/Utility.d \
./Common/misc.d 


# Each subdirectory must supply rules for building sources it contributes
Common/%_L.o: ../Common/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -DDROS_LIVE_BUFFER -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Common/%_S.o: ../Common/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ $(CPPFLAGS) -D__SSE3__ -O0 -g3 -p -pg -Wall -c -fmessage-length=0 -pthread -march=native -msse3 -rdynamic -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
