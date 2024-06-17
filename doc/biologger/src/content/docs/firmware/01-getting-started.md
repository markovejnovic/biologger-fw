---
title: Getting Started
description: A short guide on how to start developing for FLoggy.
---

FLoggy is developed with mostly
[C](https://en.wikipedia.org/wiki/C_(programming_language)). Although C can be
learned while developing this project, the best way to work with C is to be
comfortable with it on, at least, the platform you write code on.

On top of that, FLoggy is built on top of [Zephyr
RTOS](https://www.zephyrproject.org/), a mature, well-supported, very fast
[real-time operating
system](https://en.wikipedia.org/wiki/Real-time_operating_system).

## Requirements

### Hardware

* You will need some sort of UART-to-USB converter. Inexpensive ones are
  available on Amazon.
![Example UART-to-USB
Converter](https://m.media-amazon.com/images/I/51HiW4H1L2L._AC_SX466_.jpg)
* You will need a [JTAG debugger
  probe](https://www.jtag.com/jtag-hw-debugger/). These are usually extremely
  expensive, but cheap and reliable open-source solutions based on the [Black
  Magic Debug](https://black-magic.org/index.html) firmware are available. I
  ahve used the [Jeff Probe](https://flirc.tv/products/flirc-jeffprobe) and can
  vouch for it working. Ensure its firmware is updated to the latest version.

![Jeff Probe](https://uploads-ssl.webflow.com/6199c18aaf38fe891a464bb1/623b97d314752bd2512f2268_Jeff%20Probe.png)

### Software

Before getting started with FLoggy, you must have the following system
requirements met:

* [Ubuntu](https://ubuntu.com/), macOS or Windows. Other operating systems may
  work (I can vouch for Fedora) but are not officially supported.
* [Zephyr
  RTOS](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
  * Ensure that you can install Zephyr up until and including the "Build the
    Blinky sample" section.
* [git](https://www.git-scm.com/) will be used to manage the source code and
  you will need it to gain access to the repository.

## Reading this Documentation

:::note
While working with *FLoggy* the following assumptions will be made:

* Unless explicitly specified, the working directory is the location of the
  `zephyrproject` directory, ie. `~/zephyrproject`.
* You **must** always be working in the Zephyr virtual environment [as
  described in the getting started
  guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#get-zephyr-and-install-python-dependencies):

  ```sh
  source ~/zephyrproject/.venv/bin/activate
  ```
:::

## Downloading the firmware

The firmware is open-source, licensed under [Apache
2.0](https://apache.org/licenses/LICENSE-2.0), and available on
[GitHub](https://github.com/markovejnovic/biologger-fw). You **must** have a
GitHub account to contribute to this project but can use it for free without an
account.

To get started, you must first clone the code into the appropriate directory.
The location of the firmware **must** be `~/zephyrproject/app`:

```sh
cd ~/zephyrproject
git clone git@github.com:markovejnovic/biologger-fw.git app
```

## Editing the source code

You can edit the source code in any editor you are comfortable with. I
personally use [`vim`](https://www.vim.org/) but you can edit code in
[VsCode](https://code.visualstudio.com/) (and that's a great start!).

## Building the firmware

After you have edited the code, in `~/zephyrproject/app` you can use
[west](https://docs.zephyrproject.org/latest/develop/west/index.html) to build
(convert source code to machine zeroes and ones) your firmware and provide you
with a binary file (think `.exe`) that you can run on FLoggy.

```sh
cd ~/zephyrproject/app
west build -v -p always -b biologger -- -DBOARD_ROOT=.
```

Let's go through these flags:

| Flag           | Req? | Description                                     |
|----------------|----------|-------------------------------------------------|
| `-v`           |      [ ] | Be **Verbose**, print all the compile commands. |
| `-p always`    |      [x] | Always force the system to rebuild all files.   |
| `-b biologger` |      [x] | Build for the `biologger`[1] board.             |
| `-- -DBOARD_ROOT=.` | [x] | The HW description of the board is in the current directory. |

When the build finishes, you will see something like:

```
[224/224] Linking C executable zephyr/zephyr.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:      194912 B         1 MB     18.59%
             RAM:       94416 B       256 KB     36.02%
        IDT_LIST:          0 GB        32 KB      0.00%
Generating files from ~/zephyrproject/app/build/zephyr/zephyr.elf for board: biologger
```

## Flashing the firmware

To flash the firmware you can use your debugger. If you are using a
black-magic-debug-based probe, you can flash the firmware as follows:

:::note
Ensure FLoggy is powered and your debugger probe is attached to it.
:::

1. First spawn a new `arm-zephyr-eabi-gdb`[2] session:

  ```sh
  cd ~/zephyrproject
  ~/zephyr-sdk-0.16.5-1/arm-zephyr-eabi/bin/arm-zephyr-eabi-gdb
  ```

  You should see a popup like:

  ```
  GNU gdb (Zephyr SDK 0.16.5-1) 12.1
  Copyright (C) 2022 Free Software Foundation, Inc.
  License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
  This is free software: you are free to change and redistribute it.
  There is NO WARRANTY, to the extent permitted by law.
  Type "show copying" and "show warranty" for details.
  This GDB was configured as "--host=x86_64-build_pc-linux-gnu --target=arm-zephyr-eabi".
  Type "show configuration" for configuration details.
  For bug reporting instructions, please see:
  <https://github.com/zephyrproject-rtos/sdk-ng/issues>.
  Find the GDB manual and other documentation resources online at:
      <http://www.gnu.org/software/gdb/documentation/>.
  For help, type "help".
  Type "apropos word" to search for commands related to "word".
  (gdb)
  ```

2. Select your debug probe:

  ```
  (gdb) target extended-remote /dev/ttyACM0
  ```

  The exact location of `/dev/ttyACM0` will vary between operating systems (my
  example is from Linux), but on `macOS` you can find what device this is by
  running (in another terminal):
  ```sh
  ls /dev/tty.*
  ```

  Consult the documentation of your debugger.

3. Scan for devices connected to the probe:
  ```
  (gdb) monitor swdp_scan
  ```

  You should see something like:
  ```
  Target voltage: 3.2V
  Available Targets:
  No. Att Driver
   1      STM32F40x M4
  ```

4. Connect to the target:

  ```
  (gdb) attach 1
  ```

5. Load your built application into `gdb`:
  ```
  (gdb) file app/build/zephyr/zephyr.elf
  ```

  This will load the application into `gdb` but **not** onto the chip.

6. Load your application onto the microcontroller:
  ```
  (gdb) load
  ```

  You will see a printout:
  ```
  Loading section rom_start, size 0x1c4 lma 0x8000000
  Loading section text, size 0x28764 lma 0x8000200
  Loading section .ARM.exidx, size 0x8 lma 0x8028964
  Loading section initlevel, size 0x110 lma 0x802896c
  Loading section device_area, size 0x1cc lma 0x8028a7c
  Loading section sw_isr_table, size 0x308 lma 0x8028c48
  Loading section log_const_area, size 0x120 lma 0x8028f50
  Loading section log_backend_area, size 0x10 lma 0x8029070
  Loading section shell_area, size 0x30 lma 0x8029080
  Loading section shell_root_cmds_area, size 0x30 lma 0x80290b0
  Loading section shell_subcmds_area, size 0x14 lma 0x80290e0
  Loading section gnss_data_callback_area, size 0x8 lma 0x80290f4
  Loading section rodata, size 0x5388 lma 0x8029100
  Loading section datas, size 0x1228 lma 0x802e488
  Loading section device_states, size 0x2e lma 0x802f6b0
  Loading section log_mpsc_pbuf_area, size 0x40 lma 0x802f6e0
  Loading section log_msg_ptr_area, size 0x4 lma 0x802f720
  Loading section log_dynamic_area, size 0x90 lma 0x802f724
  Loading section k_mem_slab_area, size 0x38 lma 0x802f7b4
  Loading section k_heap_area, size 0x14 lma 0x802f7ec
  Loading section k_mutex_area, size 0x78 lma 0x802f800
  Loading section k_sem_area, size 0x48 lma 0x802f878
  Loading section usb_descriptor, size 0x78 lma 0x802f8c0
  Loading section usb_cfg_data_area, size 0x24 lma 0x802f938
  Loading section .last_section, size 0x4 lma 0x802f95c
  Start address 0x0800c6fc, load size 194846
  Transfer rate: 31 KB/sec, 873 bytes/write.
  ```

7. Run your firmware
  ```
  (gdb) run
  ```

# Footnotes

- [1] The name *Biologger* is an alternate term I used for *Floggy*.
- [2] `arm` means that we are developing for an
  [ARM](https://en.wikipedia.org/wiki/ARM_architecture_family)-based CPU.
  `zephyr-eabi` means that the layout of the binary code (how the code is laid
  out) will be designed for Zephyr. `gdb` is the name of the program -- [`GNU
  debugger`](https://sourceware.org/gdb/).
