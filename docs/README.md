# nox_os

A minimal educational x86 OS that boots with a custom bootloader, enters protected mode, and runs a tiny kernel with VGA text output.

## Build

Requirements:
- nasm
- i686-elf-gcc, i686-elf-ld, i686-elf-objcopy
- qemu-system-x86_64

```sh
make
make run
```

The build produces `build/nox_os.bin` (flat raw image). QEMU command used:

```
qemu-system-x86_64 -drive format=raw,file=build/nox_os.bin -serial stdio -no-reboot -monitor none -d guest_errors
```

## Next Steps
- Add GDT/IDT/ISRs in kernel
- PIC remap and PIT timer driver
- Keyboard driver
- Memory management (frames, paging, heap)
- Scheduler and simple tasks
- RAM disk and FS
- Shell
