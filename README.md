# 📘 Project Report: Precise Timeline Scheduler on FreeRTOS

## 1. Overview
This project implements a **Time-Triggered Architecture (TTA)** on top of the standard event-driven FreeRTOS kernel. Unlike standard FreeRTOS (which relies on dynamic priorities), this scheduler is **deterministic**. It guarantees that tasks run exactly when they are scheduled, following a pre-calculated timeline.

### The Core Concept: Cyclic Executive
The system operates on a **Global Time** basis divided into **Major Frames** (the full cycle) and **Sub-Frames** (smaller slots).

* **The Plan:** We define a static table (`my_schedule`) listing every task, its Start Time, End Time (Deadline), and Sub-frame.
* **The Enforcer:** The Scheduler checks the clock every millisecond (Tick) and forces tasks to Start or Stop based on the table.

---

## 2. Key Architectural Decisions (The "Why")

To make this work on FreeRTOS, we had to solve three major problems. Here is why we designed the code this way:

### Problem A: "Dumb" Tasks vs. FreeRTOS Tasks
**The Issue:** FreeRTOS tasks must be infinite loops (`while(1)`). If a function returns, the task is deleted. However, our requirements state that tasks are simple linear functions (e.g., `calculate_control()`) that run once and finish.

**The Solution: The "Wrapper" Task (`vTaskWrapper`)**
We created a generic "Robot" function called the Wrapper.
1.  It takes the user's function as a parameter.
2.  It calls the user's function.
3.  When the function finishes, the Wrapper **marks the task as DONE** and **Suspends itself**.
4.  It waits inside an infinite loop to be "woken up" again in the next cycle.

### Problem B: Killing Tasks inside an Interrupt
**The Issue:** If a task misses its deadline, we must terminate it immediately. We detect this inside the Timer Interrupt (ISR). However, FreeRTOS functions like `vTaskDelete` or `xTaskCreate` are **not safe to call from an ISR**. Doing so causes a crash.

**The Solution: The "Supervisor" Task (`vSupervisorTask`)**
We implemented the **Supervisor Pattern** (also known as a "Hitman" task).
1.  The Supervisor is a high-priority task that sleeps most of the time.
2.  When the ISR detects a deadline miss, it **notifies** the Supervisor.
3.  The Supervisor wakes up immediately (preempting everything), deletes the bad task, re-creates it fresh, and goes back to sleep.

### Problem C: Efficiency
**The Issue:** Scanning a huge table of 50+ tasks every single millisecond is slow.

**The Solution: The Lookup Table (Buckets)**
We organized tasks into "Buckets" based on their Sub-frame.
* If we are in Sub-frame 0, we *only* check the tasks inside Bucket 0. This makes the scheduler extremely fast ($O(1)$ complexity).

---

## 3. How It Works: The Lifecycle (Step-by-Step)

Here is the flow of the `timeline_scheduler.c` logic:

### Phase 1: Initialization (`vStartTimelineScheduler`)
Before the system starts, we prepare the board:
1.  **Bucket Creation:** We scan the user's schedule and organize pointers into `pxSubframeTable`.
2.  **Task Creation:** We create a FreeRTOS task for every entry in the table. Crucially, we pass the `vTaskWrapper` as the code to run, and the config structure as the parameter.
3.  **Immediate Suspension:** As soon as a task is created, we `vTaskSuspend` it. All tasks start in a "sleeping" state.
4.  **Time Zero Handling:** We check if any task starts at `t=0`. If yes, we mark it `READY` immediately so it runs the moment the OS starts.

### Phase 2: The Heartbeat (`xUpdateTimelineScheduler`)
This function runs **every 1ms** inside the FreeRTOS Tick Interrupt (`xTaskIncrementTick`). It acts as the conductor.



**Logic flow every 1ms:**
1.  **Check Start Times:**
    * It looks at the current sub-frame bucket.
    * If `Task_A.Start_Time == Current_Time`:
        * It updates state to `TASK_RUNNING`.
        * It calls `xTaskResumeFromISR`. This wakes up the wrapper.

2.  **Check Deadlines:**
    * If `Task_A.End_Time == Current_Time`:
        * It checks the `state` variable.
        * **Scenario 1 (Success):** State is `TASK_DONE` (The wrapper finished). -> *Do nothing.*
        * **Scenario 2 (Failure):** State is `TASK_RUNNING` (The wrapper is still working). -> *CRITICAL ERROR!*
            * Set state to `TASK_DEADLINE_MISSED`.
            * **Activate Supervisor:** Notify the Supervisor handle to kill this task.

### Phase 3: The Reset (`vResetTimelineMajorFrame`)
When the Major Frame (e.g., 100ms) ends:
1.  The Global Time resets to 0.
2.  This function loops through all tasks and sets their state back to `TASK_NOT_STARTED`.
3.  This prepares everyone for the next cycle.

---

## 4. Visual State Machine
Here is how a single task moves through states during one cycle:

```text
[ Start of Cycle ]  --> State: TASK_NOT_STARTED
       |
       v
[ Start Time Arrives ]
(Scheduler Resumes Task) --> State: TASK_RUNNING
       |
       +-----------------------+
       |                       |
[ User Function Finishes ]   [ Deadline Arrives & Still Running ]
       |                       |
       v                       v
[ Wrapper Updates State ]    [ Scheduler Detects Error ]
State: TASK_DONE             State: TASK_DEADLINE_MISSED
       |                       |
       v                       v
[ Task Suspends Self ]       [ Supervisor Deletes & Recreates Task ]
       |                       |
       +-----------------------+
       |
       v
[ End of Major Frame ] --> Reset to TASK_NOT_STARTED
```

### 5. Kernel Modifications (`task.c`)

We had to modify the core FreeRTOS kernel file `task.c` to give our scheduler a "heartbeat." Standard FreeRTOS is event-driven (it switches tasks only when they block or a time-slice ends). Our scheduler is **time-driven**, meaning it needs to check the schedule every single millisecond.

We injected code into `xTaskIncrementTick()`. This function is called by the hardware timer interrupt (SysTick) every 1ms.

**The Modification Logic:**
1.  **Global Time Tracking:** We added global variables (`ulGlobalTimeInFrame`) to count milliseconds from 0 to 100 (Major Frame).
2.  **The Hook:** At the end of the standard FreeRTOS tick processing, we call `xUpdateTimelineScheduler()`.
3.  **The Override:** If our scheduler returns `pdTRUE` (meaning "I started a high-priority HRT task"), we force `xSwitchRequired = pdTRUE`. This tells the low-level Assembly code to switch context *immediately*, ensuring the HRT task starts with minimal jitter.

**Code Snippet (What we added):**
```c
/* Inside xTaskIncrementTick() */
#if( configUSE_TIMELINE_SCHEDULER == 1 )
    /* 1. Update Time Counters */
    ulGlobalTimeInFrame++; 
    /* ... (Rollover Logic) ... */

    /* 2. Call Our Scheduler */
    extern BaseType_t xUpdateTimelineScheduler(void);
    if( xUpdateTimelineScheduler() == pdTRUE ) {
        xSwitchRequired = pdTRUE; // Force Context Switch
    }
#endif
