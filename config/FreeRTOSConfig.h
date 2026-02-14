#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>
#include <stddef.h>

// -------------------- Precise Scheduler Definitions ---------------------------
#define configUSE_PRECISE_SCHEDULER              1
#define SUPERVISOR_PRIORITY                      (configMAX_PRIORITIES - 1)
#define HRT_PRIORITY                             (tskIDLE_PRIORITY + 4)
#define LOGGER_PRIORITY                          (tskIDLE_PRIORITY + 3)
#define SRT_PRIORITY                             (tskIDLE_PRIORITY + 2)

#define MAJOR_FRAME_DURATION_MS                  100
#define MINOR_FRAME_DURATION_MS                  10

// --------------------------------- END ----------------------------------------


#define configGENERATE_RUN_TIME_STATS            0

#define configUSE_PREEMPTION                     1
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0

#define configCPU_CLOCK_HZ                       ( ( unsigned long ) 25000000 )
#define configTICK_RATE_HZ                       ( ( TickType_t ) 1000 )
#define configMINIMAL_STACK_SIZE                 ( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE                    ( ( size_t ) ( 100 * 1024 ) )
#define configMAX_TASK_NAME_LEN                  ( 12 )

#define configUSE_TRACE_FACILITY                 0

#define configUSE_16_BIT_TICKS                   0
#define configIDLE_SHOULD_YIELD                  0
#define configUSE_CO_ROUTINES                    0
#define configUSE_MUTEXES                        1
#define configUSE_RECURSIVE_MUTEXES              1

#define configCHECK_FOR_STACK_OVERFLOW           0
#define configUSE_MALLOC_FAILED_HOOK             0

#define configUSE_QUEUE_SETS                     1
#define configUSE_COUNTING_SEMAPHORES            1

#define configMAX_PRIORITIES                     ( 9UL )
#define configMAX_CO_ROUTINE_PRIORITIES          ( 2 )
#define configQUEUE_REGISTRY_SIZE                10

#define configSUPPORT_STATIC_ALLOCATION          0

#define configUSE_TIMERS                         0
#define configTIMER_TASK_PRIORITY                ( configMAX_PRIORITIES - 4 )
#define configTIMER_QUEUE_LENGTH                 20
#define configTIMER_TASK_STACK_DEPTH             ( configMINIMAL_STACK_SIZE * 2 )

#define configUSE_TASK_NOTIFICATIONS             1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES    3

#define INCLUDE_vTaskPrioritySet                  1
#define INCLUDE_uxTaskPriorityGet                 1
#define INCLUDE_vTaskDelete                       1
#define INCLUDE_vTaskCleanUpResources             0
#define INCLUDE_vTaskSuspend                      1
#define INCLUDE_vTaskDelayUntil                   1
#define INCLUDE_vTaskDelay                        1
#define INCLUDE_uxTaskGetStackHighWaterMark       1
#define INCLUDE_xTaskGetSchedulerState            1
#define INCLUDE_xTimerGetTimerDaemonTaskHandle    0
#define INCLUDE_xTaskGetIdleTaskHandle            1
#define INCLUDE_xSemaphoreGetMutexHolder          1
#define INCLUDE_eTaskGetState                     1
#define INCLUDE_xTimerPendFunctionCall            0
#define INCLUDE_xTaskAbortDelay                   1
#define INCLUDE_xTaskGetHandle                    1
#define INCLUDE_xTaskResumeFromISR                1

#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS       __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS       4
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY          0xf
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY     5

#define configUSE_STATS_FORMATTING_FUNCTIONS            0

#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

#define configUSE_PORT_OPTIMISED_TASK_SELECTION          1

#ifndef __IASMARM__
    #undef  configASSERT
    #define configASSERT( x ) do { if( ( x ) == 0 ) { for( ;; ) {} } } while( 0 )
#endif

#define bktPRIMARY_PRIORITY      ( configMAX_PRIORITIES - 3 )
#define bktSECONDARY_PRIORITY    ( configMAX_PRIORITIES - 4 )

#define configENABLE_BACKWARD_COMPATIBILITY 0

#endif /* FREERTOS_CONFIG_H */
