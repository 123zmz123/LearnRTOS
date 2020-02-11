# LearnRTOS
此respo维护了一个展示用的最小RTOS，只作为展示RTOS工作原理之用。

this repo just contained an minimal rtos, that can give an good example to help you understand how rtos work.
But I do not believe it can work as a professional Rtos.
## 硬件（hardware）
基于 cortex-m4 
官方开发板 Nucleuo-F401RE . 开发板与硬件并非重点，此演示操作系统只用了Nucleo开发板上的一个外接LED(用于指示上下文切换)，剩余部分在任何一款cortex-m4 或者 cortex-m3 上都能轻松实现，此演示操作系统与芯片生产商基本无关，只是运用了cortex-m4上的资源。

based on Arm cortex-m4 ,and the developing board is nucleo-f401re, we just use one gpio port(as led control) on f401re.
the led blinking  will inform us that context switch is happened.SO  no matter which board (chip)you are using,as long as you are using cortex m4 , it will work property. 

## 软件(software)
stm32cubemx 生成工程， keil-mdk5 负责编译。
在使用cubemx时候,选择Nuleo-F401RE开发板一切选默认，不要加任何操作系统。
keil-mdk5 配置时要注意 选项 options for target -> Target -> Floating Point Hardware 请勾选 Not Use.

## 工程结构
实际上只有三个文件。 osKernel.c 负责实现操作系统， osKernel.s 里面定义了 Systic 中断 ，并在中断中实现了上下文切换。
osKernel.h 则是声明。

## 实现思路
本操作系统实现了三个任务调度和切换（我希望可以写完更完备的RTOS但不是这里）。只要SysTic中断发生,即开始切换任务。
对于操作系统而言，**每个任务都要有自己的栈，栈用来存储该任务在被切换前MCU寄存器的数据，通俗来说，栈用来存储该任务停止运行前MCU的状态**，对于MCU来说，**栈就是一片特定的内存区域**，入栈就是把MCU寄存器数据存储到栈之上，出栈就是把栈中数据放置在寄存器之上。
因此每个任务的栈都要在MCU中开辟一片特定的内存,**代码中，栈空间用数组表示**
```
struct tcb
{
    int32_t *stackPt;//指向当前任务的栈顶
    struct tcb *nextPt;//指向下一个任务的tcb control block
};

typedef struct tcb tcbType; 
tcbType tcbs[NUM_OF_THREADS];// 在内存中开辟空间来存储任务控制块
tcbType *currentPt; // 当前任务控制块
int32_t TCB_STACK[NUM_OF_THREADS][STACKSIZE];//内存中各个任务的堆栈`

```
tcb （task block control）这个数据结构有两个作用，第一是指向当前任务的栈顶地址，第二个就是指向下一个任务在哪里。
currentPt 是指向当前的任务控制块

好了我们先不要管初始化代码，就看任务是如何切换的。
```
CONTEXT_SWITCH ; 任务切换
  CPSID I           ;禁止中断
  PUSH{R4 - R11}    ;将当前任务中的 R4-R11 寄存器内容存储到栈中，R0-R3 LR PC PSR 是被自动存储到栈上的
  LDR R0,=currentPt ; currentPtr的地址被放置到R0寄存器中。
  LDR R1,[R0]       ;R1 = currentPtr->StackPtr
  STR SP,[R1]       ;SP = currentPtr->StackPtr
  LDR R1,[R1,#4]    ;R1=currentPtr->nextPtr
  STR R1,[R0]       ;currentPtr=currentPtr->nextPtr 
  LDR SP,[R1]       ;SP=currentPtr->StackPtr 
  POP {R4 - R11}    ;R4-R11 = currentPtr->StackPtr contains
  CPSIE I           ;开启中断
  BX LR             ;函数返回 LR所指向的地址中(中断中实际上并不是)但这条命令的确可以结束中断调用。
```
如上，实际上，上下文切换就做了两步，第一步是把当前任务需要手动存储的寄存器值放到当前任务栈之上，第二是找到下一个任务，把任务栈中的寄存器内容放置到MCU通用寄存器之上，随后MCU就会执行，新任务的代码，切换即完成。

如此简单吧！

那么每次任务切换，栈是什么样子呢根据我DEBUG观测，如下。

内存区域（我们称之为栈）

### 出栈
```
..................................旧栈顶
R4                              |
R5                              |
R6                              |
R7                              |       手动出栈
R8                              |
R9                              |
R11                             |
.......................................
R0                              |
R1                              |
R2                              |
R3                              |      MCU自动帮你出栈
R12                             |
LR                              |
PC                              |
PSR                             |
...................................新栈顶
```

### 入栈
```
..................................新栈顶
R4                              |
R5                              |
R6                              |
R7                              |       手动入栈
R8                              |
R9                              |
R11                             |
.......................................
R0                              |
R1                              |
R2                              |
R3                              |      MCU自动帮你入栈
R12                             |
LR                              |
PC                              |
PSR                             |
...................................旧栈顶
```

实际上该RTOS就是每1ms产生了一个中断，在中断中我们把当前任务寄存器状态保留在任务栈上，然后切换到新任务栈之上，把新任务栈中的数值，放置在MCU寄存器中，退出当前中断后，任务切换完成，MCU跳转到新任务中去了。

#### 信号量
pls read the fuck code

