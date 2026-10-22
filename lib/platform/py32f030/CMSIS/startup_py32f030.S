/* GCC startup file for Puya PY32F030 (Arm Cortex-M0+)
 
   Ported from Keil's startup_py32f030.s to GNU assembler / GNU ld.
 
   Responsibilities of Reset_Handler:
     1. Copy .data initializers from flash (LMA) to RAM (VMA)
     2. Zero-fill .bss
     3. Call SystemInit() (weak - provided by system_py32f030.c)
     4. Call __libc_init_array() (C++ ctors / newlib init)
     5. Call main()
  
   The linker script (py32f030.ld) must define the symbols used below:
     _estack, _sidata, _sdata, _edata, _sbss, _ebss
  
 */

    .syntax unified
    .cpu cortex-m0plus
    .fpu softvfp
    .thumb

    .global g_pfnVectors
    .global Default_Handler

/* Reset_Handler 
 */
    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function

Reset_Handler:
    /* Set stack pointer (also already loaded by hardware from vector[0], this line is defensive / matches common CMSIS templates) */
    ldr   r0, =_estack
    mov   sp, r0

/* Copy .data section from flash to RAM 
 */
    ldr   r0, =_sdata
    ldr   r1, =_edata
    ldr   r2, =_sidata
    movs  r3, #0
    b     LoopCopyDataInit

CopyDataInit:
    ldr   r4, [r2, r3]
    str   r4, [r0, r3]
    adds  r3, r3, #4

LoopCopyDataInit:
    adds  r4, r0, r3
    cmp   r4, r1
    bcc   CopyDataInit

/* Zero-fill .bss section 
 */
    ldr   r2, =_sbss
    ldr   r4, =_ebss
    movs  r3, #0
    b     LoopFillZerobss

FillZerobss:
    str   r3, [r2]
    adds  r2, r2, #4

LoopFillZerobss:
    cmp   r2, r4
    bcc   FillZerobss

/* Call system/library init 
 */
    bl    SystemInit

    bl    __libc_init_array

    bl    main

/* main() should never return on an embedded target, but just in case: 
 */
LoopForever:
    b     LoopForever

    .size Reset_Handler, .-Reset_Handler

/* Default_Handler (IRQ)
 */
    .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
    b     Infinite_Loop
    .size Default_Handler, .-Default_Handler

/* SystemInit (weak default) 

   Reset_Handler always calls SystemInit. If linked against Puya's own system_py32f030.c (which defines a real SystemInit, e.g. clock setup), that strong definition overrides this empty stub. Without it, keep this so the link succeeds and the core runs on its default clock.              
 */
    .section .text.SystemInit,"ax",%progbits
    .weak SystemInit
    .type SystemInit, %function
SystemInit:
    bx    lr
    .size SystemInit, .-SystemInit

/* _init / _fini (weak defaults) 
   
   Normally supplied by newlib's crti.o/crtn.o, which -nostartfiles omits. newlib's __libc_init_array() (called above) calls _init() before running. init_array (C++ static constructors); Cortex-M never uses _start/_fini itself, so empty stubs are all that's needed here.                       
 */
    .section .text._init,"ax",%progbits
    .weak _init
    .type _init, %function
_init:
    bx    lr
    .size _init, .-_init

    .section .text._fini,"ax",%progbits
    .weak _fini
    .type _fini, %function
_fini:
    bx    lr
    .size _fini, .-_fini

/* Vector Table

   From the Keil version shipped with the CMSIS pack, verified with the Puya PY32F030 Reference Manual.
 */
    .section .isr_vector,"a",%progbits
    .type g_pfnVectors, %object

g_pfnVectors:
    .word _estack                      /* Initial stack pointer            */
    .word Reset_Handler                /* Reset                            */
    .word NMI_Handler                  /* NMI                              */
    .word HardFault_Handler            /* Hard fault                       */
    .word 0                            /* Reserved (no MemManage on M0+)   */
    .word 0                            /* Reserved (no BusFault on M0+)    */
    .word 0                            /* Reserved (no UsageFault on M0+)  */
    .word 0                            /* Reserved                         */
    .word 0                            /* Reserved                         */
    .word 0                            /* Reserved                         */
    .word 0                            /* Reserved                         */
    .word SVC_Handler                  /* SVCall                           */
    .word 0                            /* Reserved (no DebugMon on M0+)    */
    .word 0                            /* Reserved                         */
    .word PendSV_Handler               /* PendSV                           */
    .word SysTick_Handler              /* SysTick                          */

    /* External Interrupts
     */

    .word WWDG_IRQHandler                /* 0:  Window Watchdog            */
    .word PVD_IRQHandler                 /* 1:  PVD through EXTI line 16   */
    .word RTC_IRQHandler                 /* 2:  RTC through EXTI line 19   */
    .word FLASH_IRQHandler               /* 3:  FLASH global               */
    .word RCC_IRQHandler                 /* 4:  RCC global                 */
    .word EXTI0_1_IRQHandler             /* 5:  EXTI Line 0 and 1          */
    .word EXTI2_3_IRQHandler             /* 6:  EXTI Line 2 and 3          */
    .word EXTI4_15_IRQHandler            /* 7:  EXTI Line 4 to 15          */
    .word 0                              /* 8:  Reserved                   */
    .word DMA1_Channel1_IRQHandler       /* 9:  DMA1 Channel 1             */
    .word DMA1_Channel2_3_IRQHandler     /* 10: DMA1 Channel 2 and 3       */
    .word 0                              /* 11: Reserved                   */
    .word ADC_COMP_IRQHandler            /* 12: ADC & COMP through EXTI    */
    .word TIM1_BRK_UP_TRG_COM_IRQHandler /* 13: TIM1 Break/Update/Trig/Com */
    .word TIM1_CC_IRQHandler             /* 14: TIM1 Capture Compare       */
    .word 0                              /* 15: Reserved                   */
    .word TIM3_IRQHandler                /* 16: TIM3 global                */
    .word LPTIM1_IRQHandler              /* 17: LPTIM1                     */
    .word 0                              /* 18: Reserved                   */
    .word TIM14_IRQHandler               /* 19: TIM14 global               */
    .word 0                              /* 20: Reserved                   */
    .word TIM16_IRQHandler               /* 21: TIM16 global               */
    .word TIM17_IRQHandler               /* 22: TIM17 global               */
    .word I2C1_IRQHandler                /* 23: I2C1 (combined w/ EXTI23)  */
    .word 0                              /* 24: Reserved                   */
    .word SPI1_IRQHandler                /* 25: SPI1                       */
    .word SPI2_IRQHandler                /* 26: SPI2                       */
    .word USART1_IRQHandler              /* 27: USART1                     */
    .word USART2_IRQHandler              /* 28: USART2                     */
    .word 0                              /* 29: Reserved                   */
    .word LED_IRQHandler                 /* 30: LED                        */
    .word 0                              /* 31: Reserved                   */

    .size g_pfnVectors, .-g_pfnVectors

/* Weak aliases for each handler 
   
   Override any of these in the C code simply by defining a function with the same name. Also useful for just knowing their names:)

   NOTE that those are C names, so when overriding from C++ they *must* be declared as `extern "C"` otherwise the default handler gets called instead. A quick way to verify is inspect the elf file for the handler, T before the name means it is the expected thumb function provided by the user, W means its weak symbol, which is the default implementation, run variant of this:

       arm-none-eabi-nm blinky.elf  | grep RTC_IRQHandler
 */
    .weak      NMI_Handler
    .thumb_set NMI_Handler,Default_Handler

    .weak      HardFault_Handler
    .thumb_set HardFault_Handler,Default_Handler

    .weak      SVC_Handler
    .thumb_set SVC_Handler,Default_Handler

    .weak      PendSV_Handler
    .thumb_set PendSV_Handler,Default_Handler

    .weak      SysTick_Handler
    .thumb_set SysTick_Handler,Default_Handler

    .weak      WWDG_IRQHandler
    .thumb_set WWDG_IRQHandler,Default_Handler

    .weak      PVD_IRQHandler
    .thumb_set PVD_IRQHandler,Default_Handler

    .weak      RTC_IRQHandler
    .thumb_set RTC_IRQHandler,Default_Handler

    .weak      FLASH_IRQHandler
    .thumb_set FLASH_IRQHandler,Default_Handler

    .weak      RCC_IRQHandler
    .thumb_set RCC_IRQHandler,Default_Handler

    .weak      EXTI0_1_IRQHandler
    .thumb_set EXTI0_1_IRQHandler,Default_Handler

    .weak      EXTI2_3_IRQHandler
    .thumb_set EXTI2_3_IRQHandler,Default_Handler

    .weak      EXTI4_15_IRQHandler
    .thumb_set EXTI4_15_IRQHandler,Default_Handler

    .weak      DMA1_Channel1_IRQHandler
    .thumb_set DMA1_Channel1_IRQHandler,Default_Handler

    .weak      DMA1_Channel2_3_IRQHandler
    .thumb_set DMA1_Channel2_3_IRQHandler,Default_Handler

    .weak      ADC_COMP_IRQHandler
    .thumb_set ADC_COMP_IRQHandler,Default_Handler

    .weak      TIM1_BRK_UP_TRG_COM_IRQHandler
    .thumb_set TIM1_BRK_UP_TRG_COM_IRQHandler,Default_Handler

    .weak      TIM1_CC_IRQHandler
    .thumb_set TIM1_CC_IRQHandler,Default_Handler

    .weak      TIM3_IRQHandler
    .thumb_set TIM3_IRQHandler,Default_Handler

    .weak      LPTIM1_IRQHandler
    .thumb_set LPTIM1_IRQHandler,Default_Handler

    .weak      TIM14_IRQHandler
    .thumb_set TIM14_IRQHandler,Default_Handler

    .weak      TIM16_IRQHandler
    .thumb_set TIM16_IRQHandler,Default_Handler

    .weak      TIM17_IRQHandler
    .thumb_set TIM17_IRQHandler,Default_Handler

    .weak      I2C1_IRQHandler
    .thumb_set I2C1_IRQHandler,Default_Handler

    .weak      SPI1_IRQHandler
    .thumb_set SPI1_IRQHandler,Default_Handler

    .weak      SPI2_IRQHandler
    .thumb_set SPI2_IRQHandler,Default_Handler

    .weak      USART1_IRQHandler
    .thumb_set USART1_IRQHandler,Default_Handler

    .weak      USART2_IRQHandler
    .thumb_set USART2_IRQHandler,Default_Handler

    .weak      LED_IRQHandler
    .thumb_set LED_IRQHandler,Default_Handler
