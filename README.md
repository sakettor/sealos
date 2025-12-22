# SealOS
SealOS is an operating system (apparently) that runs in 64 bits. It is very early in development, current functions of SealOS are:

Outputting text;

Getting input;

Has a command prompt with useful commands;

Loading an IDT;

Fat32 support;

Has a bitmap memory allocator;

Cool seal logo

# Building & Running
To build, just run

``` make ```

To run, use this command:

``` qemu-system-x86_64 -M pc -m 2G -drive file=fat32.img,format=raw,index=0,media=disk -drive file=template-x86_64.iso,format=raw,index=1,media=cdrom -boot order=d ```

I am happy for all support and critique!
