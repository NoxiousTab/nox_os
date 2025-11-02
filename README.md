# ⚙️ nox_os — Educational Command-Line Operating System

**nox_os** is a minimal, educational operating system built entirely from scratch to demonstrate the core principles of OS design — from bootloading and memory management to multitasking and file systems — all through a clean **command-line interface**.

Unlike Linux-based kernels, **nox_os** is an **independent OS** written in **C, x86 Assembly**, designed to run directly on hardware via **QEMU or Bochs**.
It aims to be **readable, hackable, and deeply instructive**, making it ideal for anyone learning how operating systems actually work.

---

## Features

* **Custom Bootloader** (no GRUB dependency)
* ⚙️ **Protected Mode Kernel** written in C/C++
* 🧠 **Paging and Heap Memory Management**
* 🔄 **Round-Robin Task Scheduler**
* 💾 **Simple FAT12-like Filesystem**
* ⌨️ **Keyboard and VGA Text Drivers**
* 🔌 **System Call Interface**
* 💻 **Command-Line Shell** with built-in commands:

  ```
  help, ls, cat, echo, meminfo, ps, reboot
  ```
* 💽 **Bootable QEMU Image** included for easy testing

---

## 🎯 Project Goals

* Understand the **low-level boot process** and transition from real to protected mode.
* Learn how a kernel manages **memory, interrupts, processes, and drivers**.
* Provide a **minimal, modular OS** that’s easy to explore and modify.
* Serve as a **hands-on educational tool** for OS developers and enthusiasts.

---

---

## 🛠️ Building nox_os

### Prerequisites

You’ll need:

* `i686-elf-gcc` or `x86_64-elf-gcc` cross-compiler
* `nasm` for assembling bootloader code
* `qemu-system-x86_64` for running the OS
* `make` for automated builds

> 💡 Tip: If you don’t have a cross-compiler, the [OSDev Wiki](https://wiki.osdev.org/GCC_Cross-Compiler) has setup instructions.

---

### Build Commands

```bash
# 1. Build the OS
make all

# 2. Run it in QEMU
make run

# 3. Clean build files
make clean
```

Once built, the kernel image will appear in:

```
build/nox_os.bin
```

Run it directly:

```bash
qemu-system-x86_64 -drive format=raw,file=build/nox_os.bin
```

---

## 💻 CLI Commands

| Command   | Description                         |
| --------- | ----------------------------------- |
| `help`    | Show available commands             |
| `ls`      | List files in the current directory |
| `cat`     | Display file contents               |
| `echo`    | Print text to screen                |
| `meminfo` | Show memory usage info              |
| `ps`      | Display running processes           |
| `reboot`  | Restart the system                  |

---

## 🧩 System Overview

| Component          | Description                                                          |
| ------------------ | -------------------------------------------------------------------- |
| **Bootloader**     | Written in x86 assembly; sets up protected mode and loads the kernel |
| **Kernel Core**    | Initializes system structures and manages memory, tasks, and I/O     |
| **Memory Manager** | Frame allocator + virtual memory paging                              |
| **Scheduler**      | Simple round-robin process scheduler                                 |
| **Filesystem**     | Minimal FAT12 or custom layout with basic file I/O                   |
| **CLI Shell**      | Interactive command-line interface for system interaction            |
| **System Calls**   | Interface layer between user commands and kernel routines            |
