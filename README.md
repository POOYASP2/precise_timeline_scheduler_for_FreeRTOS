# Precise Scheduler for FreeRTOS (ARM MPS2-AN386)

A deterministic, timeline-driven scheduler replacing the default FreeRTOS priority-based model. This project implements a time-triggered architecture (TTA) on top of the FreeRTOS Kernel, designed for the **ARM Cortex-M4F** (MPS2-AN386).

## Installation and Simulation

### Prerequisites

To build and run this project, you need the following tools installed on your Linux/WSL environment:

- ARM GCC Toolchain: (arm-none-eabi-gcc)

- Newlib Library: (libnewlib-arm-none-eabi or arm-none-eabi-newlib)

- Build Tools: (make, git)

- Simulation: (qemu-system-arm)

- Debugger: (gdb-multiarch or arm-none-eabi-gdb)

Installation on Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential git qemu-system-arm gdb-multiarch
```

Installation on Arch Linux:

```bash
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib qemu-system-arm-headless arm-none-eabi-gdb
```

### Cloning


```bash
git clone https://baltig.polito.it/eos25/group5.git
cd group5
```

### Building

We use a standard Makefile.

```bash
make
```
- Clean build: make clean

### Simulation

To run the scheduler in the QEMU emulator:
```bash
make qemu
```
- Note: Press Ctrl + A then x to exit QEMU.

### Debugging

**Terminal 1 (Start QEMU in Freeze Mode):**
```bash
make qemu_debug
```

**Terminal 2 (Connect GDB):**
```bash
make gdb
```

## Architecture Details

- **Target Board:** ARM MPS2 (Motherboard Protection System 2)

- **FPGA Image:** AN386 (Cortex-M4)

- **Processor:** Cortex-M4F (Hardware FPU enabled)

- Memory Map:

    - Flash: 0x00000000

    - RAM: 0x20000000

### Implementation Notes

- **FPU Enabled:** The startup_gcc.c explicitly enables CP10 and CP11 to allow hardware floating-point operations. The FreeRTOS port ARM_CM4F is used to save FPU context during task switching.

- **Static Allocation:** FreeRTOSConfig.h is configured to support static memory allocation (configSUPPORT_STATIC_ALLOCATION), requiring vApplicationGetIdleTaskMemory hooks in main.c.

- **Interrupt Priorities:** NVIC priorities are correctly left-shifted to match the Cortex-M4 hardware requirements.

## License

This project follows the FreeRTOS license schema (MIT-style Open Source).

## Repository Structure
```text
.
├── FreeRTOS-Kernel/    	# Official FreeRTOS Kernel (Git Submodule)
├── config/             	# FreeRTOS Configuration (FreeRTOSConfig.h)
├── device/             	# Board Support Package (Startup & Linker scripts)
├── main.c              	# Application entry point and test tasks
├── timeline_scheduler.c	# A file in which the main scheduler is implemented
├── timeline_scheduler.h	# A file in which the daat structures and prototypes are defined
├── Makefile            	# Build system for ARM GCC
└── README.md           	# This documentation
```

## Authors
- [Muhammed Emir Akinci]()
- [Kuzey Kara](https://github.com/kuzeykara)
- [Giacomo Pessolano]()
- [Pooya Sharifi]()
- [Mohammad Tohidnia](https://github.com/mohammadTohidnia) 

