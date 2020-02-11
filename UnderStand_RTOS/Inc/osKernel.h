#ifndef __OS_KERNEL_H
#define __OS_KERNEL_H
#include<stdint.h>
#include"stm32f4xx_hal.h"
#include"core_cm4.h"

void osKernelLanch(uint32_t quanta);
void osKernelInit(void);
uint8_t osKernelAddThreads(void(*task0)(void),
                           void(*task1)(void),
                           void(*task2)(void));
void osSchedulerRoundRobin(void);
void osSemaphoreInit(int32_t *semaphore, int32_t value);
void osSignalWait(int32_t *semaphore);
void osSignalSet(int32_t *semaphore);
void osThreadYield(void);
#endif