# SealOS
SealOS is an MS-DOS like operating system that runs in 64 bits. It is very early in development, has a lot of bugs, but there is already a ton of cool features.

# Command prompt
In SealOS there is a command prompt with some basic commands, some of them being:

``` dir ``` - Shows all the files & folders in the current directory.

``` cd ``` - Changes the current directory to the specified folder.

``` mkdir ``` - Creates a folder with a specified name.

``` edit ``` - Opening a built-in editor of the specified file.

``` isl ``` - Shows all the Interpreted Script Language options. 

# Interpreted Script Language for SealOS
ISL is a simple language that behaves similarly to .sh files on linux.

To initialize a script, use ``` isl init <filename> ```. This create a file called ``` <filename>.isl ``` Important to mention, SealOS doesn't support custom extensions. ```create```/```edit``` for .txt, ```isl init <filename>``` for .isl.

Example script:

```
echo hello world
sleep
sleep
mkdir test
cd test
echo done
cd ..
sleep
sleep
echo bye
sleep
sleep
reboot
```

To run an ISL script, get its cluster by typing ```dir```. After that, run ```isl run <cluster>```

This will run every single line of the script as a command.

# Building & Running
To build it, run ```make```.

To create a hard drive for the operating system, use ```./diskformat.sh```.

To run, use ```qemu-system-x86_64 -M pc -m 2G   -drive file=fat32.img,format=raw,index=0,media=disk   -drive file=template-x86_64.iso,format=raw,index=1,media=cdrom   -boot order=d```

# Screenshots

<img width="1417" height="1595" alt="Снимок экрана 2025-12-23 070154" src="https://github.com/user-attachments/assets/2a79cd14-e242-4b34-a2b8-9cd5eb6e20ab" />

<img width="893" height="197" alt="Снимок экрана 2025-12-23 070913" src="https://github.com/user-attachments/assets/b329740a-46af-4a2c-bdc5-35afa96a8c31" />

<img width="645" height="384" alt="image" src="https://github.com/user-attachments/assets/7b9e3dc4-a26a-4e56-bcd3-2647c62f1678" />











