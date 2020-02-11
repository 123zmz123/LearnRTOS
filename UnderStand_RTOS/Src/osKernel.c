#include "osKernel.h"
// In the default mode ， Nucleao f401 Systic interrupt is 1khz. 1ms
// 对于 Nucleo F401 来说， 默认模式下 systic 已经被设置为 1khz , 周期为1ms因此，我不想在去搞什么os切换时间了，作为一个演示类操作系统没什么意义。
//Scheduler
//TCB
//Stack
#define NUM_OF_THREADS 3//操作系统所支持的任务数目
#define STACKSIZE 100//每个任务的stack大小
#define _1Mhz 1000000 
#define BUS_FREQ    16*_1Mhz
#define PERIOD 100
#define INTCTRL (*((volatile uint32_t *)0xE000ED04)) // cortex-m4 中断控制寄存器

uint32_t MILLIS_PRESCALER = 0;
uint32_t period_cnt=0;
void osSchedulerLauch();
void ToggleTask();
/*task control block 定义*/
struct tcb
{
    int32_t *stackPt;//指向当前任务的栈顶
    struct tcb *nextPt;//指向下一个任务的tcb control block
};

typedef struct tcb tcbType; 
tcbType tcbs[NUM_OF_THREADS];// 在内存中开辟空间来存储任务控制块
tcbType *currentPt; // 当前任务控制块
int32_t TCB_STACK[NUM_OF_THREADS][STACKSIZE];//内存中各个任务的堆栈

/*
    对第i个任务的堆栈进行初始化
*/
void osKernelStackInit(int i)
{
    tcbs[i].stackPt = &TCB_STACK[i][STACKSIZE-16];//tcb 指向堆栈顶端，一次任务切换开辟16个寄存器存储区域
    TCB_STACK[i][STACKSIZE-1]=0x01000000;//内存堆栈边缘被初始化为一个特殊值
}

/*
    向kernel中增加3个task或者thread. 
*/
uint8_t osKernelAddThreads(
void(*task0)(void),
void(*task1)(void),
void(*task2)(void))
{
    __disable_irq();// 关闭中断
    //初始化stack管理数组,nextptr 指向下一个tcb
    tcbs[0].nextPt = &tcbs[1];
    tcbs[1].nextPt = &tcbs[2];
    tcbs[2].nextPt = &tcbs[0];

    osKernelStackInit(0);//初始化第一个任务
    TCB_STACK[0][STACKSIZE-2]=(int32_t)(task0);// 当前堆栈中LR 寄存器的位置中 放入 task0 的地址

    osKernelStackInit(1);//初始化第二个任务
    TCB_STACK[1][STACKSIZE-2]=(int32_t)(task1);// 当前堆栈中LR 寄存器的位置中 放入 task1 的地址

    osKernelStackInit(2);//初始化第三个任务
    TCB_STACK[2][STACKSIZE-2]=(int32_t)(task2);// 当前堆栈中LR 寄存器的位置中 放入 task2 的地址
    currentPt = &tcbs[0];

    __enable_irq();

    return 1;
}

void osKernelInit(void)
{
    __disable_irq();
    MILLIS_PRESCALER = (BUS_FREQ/1000);
}
/* 堆栈中（某一块特定内存）与寄存器对应关系，调用 osSchedulerLauch后，堆栈内容按此规律被放到arm寄存器中，堆栈指针一步步指向新的栈顶
----------------------
|         R4         |------------------>StackPointer(堆栈指针)
----------------------  
|         R5         |
----------------------  
|         R6         |
----------------------  
|         R7         |
----------------------  
|         R8         |
----------------------  
|         R9         |
----------------------  
|         R10        |
----------------------  
|         R11        |
----------------------  
|         R0         |
----------------------  
|         R1         |
----------------------  
|         R2         |
----------------------  
|         R3         |
----------------------  
|         R12        |
----------------------  
|         SP         |
----------------------  
|         LR         |
----------------------  
|         PC         |
----------------------
|         New(R4)    |------------------->New StackPointer(堆栈指针)
----------------------

*/
__asm void osSchedulerLauch()                        
{
    extern currentPt

  CPSID I
        ldr r0,=currentPt
        ldr r2,[r0]
        ldr sp, [r2] ;SP=currentPtr
        pop {r4-r11}
        pop {r0-r3}
        pop {r12}
        add sp,sp,#4
        pop {lr}
        add sp,sp,#4
        CPSIE I
        bx lr
}
void osKernelLanch(uint32_t quanta)
{
    //HAL_SYSTICK_Config(MILLIS_PRESCALER*quanta);
    osSchedulerLauch();
}

void osSchedulerRoundRobin(void)
{
    period_cnt++;
    if (period_cnt == PERIOD)
    {
        (*ToggleTask)();
        period_cnt=0;
    }
    currentPt = currentPt->nextPt;
}

void osThreadYield(void)
{
    /*
     触发任务切换中断
    */
    SysTick->VAL = 0;
    INTCTRL = 0x04000000; // Trigger Systick
}

void osSemaphoreInit(int32_t *semaphore, int32_t value)
{
    (*semaphore)=value;
}
void osSignalSet(int32_t *semaphore)
{
    __disable_irq();
    (*semaphore)+=1;
    __enable_irq();
}

void osSignalWait(int32_t *semaphore)
{
    __disable_irq();
    while (*semaphore<=0)
    {
        __disable_irq();
        osThreadYield();//这里就是引发任务切换中断，使得其他任务可以继续.
        __enable_irq();
    }
    (*semaphore)-=1;
    __enable_irq();
    

}

void ToggleTask()
{
    GreenLedToggle();
}


