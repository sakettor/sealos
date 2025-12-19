# SealOS
SealOS is an operating system (apparently) that runs in 64 bits. It is very early in development, current functions of SealOS are:

Outputting text;

Getting input;

Has a kernel shell;

Loading an IDT;

Not crashing;

Runs on every 64bit PC;

Takes up 0 mb of space

# Building & Running
To build, just run

``` make ```

To run, use this command:

``` qemu-system-x86_64 -M q35 -cdrom template-x86_64.iso -m 2G ```

I am happy for all support and critique!
