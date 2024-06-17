---
title: Debugging
description: Helpful information when debugging
---

Debugging is an important and frequent task. We will mostly use `gdb` to debug.

## Console Interaction with FLoggy

The first, and usually most helpful, thing to do is to use the console and
`printk` (function similar to `printf`) to read the status of the firmware.

1. Connect the USB-to-UART convert to FLoggy's 3rd from the left UART port.
   Ensure that the `TX` of the converter is connected to `RX` of Floggy and the
   converse for `RX-TX`.
2. Connect the USB-to-UART converter to your PC.
3. Use a tool like [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/)
   configured at `9600` baud to communicate with the serial device. You should
   be able to see FLoggy's printout.
