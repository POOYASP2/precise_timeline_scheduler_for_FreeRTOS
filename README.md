# Precise Timeline Scheduler for FreeRTOS

This project implements a deterministic, timeline-driven scheduler for FreeRTOS, replacing the standard priority-based preemption model with a Time-Triggered Architecture (TTA). Designed for time-critical embedded applications, the system governs task execution through a static schedule defined at compile time. This ensures that operations occur within strict time windows (Sub-frames) inside a globally repeating cycle (Major Frame), providing predictable and repeatable behavior.

The architecture supports a hybrid task model: Hard Real-Time (HRT) tasks execute based on absolute start and end times, while Soft Real-Time (SRT) tasks run sequentially during the idle periods of a sub-frame. To ensure system stability, a Supervisor task monitors execution to detect and reset tasks that miss their deadlines. The project ecosystem includes a JSON-based configuration tool for auto-generating schedules, custom FreeRTOS kernel hooks for precise time-keeping, and a lightweight tracing system for validating performance via UART.

For a comprehensive guide on the architecture, kernel modifications, and API reference, please refer to the full documentation: Precise_Timeline_Scheduler_for_FreeRTOS.pdf.

## Authors

* Giacomo Pessolano (s352714)
* Kuzey Kara (s362739)
* Mohammad Tohidnia (s355479)
* Muhammed Emir Akinci (s360728)
* Pooya Sharifi (s355130)