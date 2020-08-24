# EXT2 Emulator
An emulator that simulate EXT2 file system.

This emulator merge super block, group descriptor, inode map and block map into a new super block. It occupied 656 Bytes.

The maximum size of a single file is 6KB.
The maximum number of files and directories a single folder can contain is 46.
The whole file system can contain mostly 1024 files and directories.

## build and run

create makefile and other files.

```bash
$ cmake ./
```

build

```bash
$ make
```

create disk file

```bash
$ dd if=/dev/zero of=disk.os bs=4M count=1
```

You can also use "disk.os" file in this repository.

run

```bash
$ ./ext2_emu
```

## How to use

You can also use "help" command in Emulator to get tips below.

```
create:
Usage: create SIZE FILE
	or: create OPTION DIRECTORY
Create the FILE or DIRECTORY, if it does not already exist.
	-d create directory
```

```
delete:
Usage: delete OPTION FILE
Delete the FILE
	-d delete directory and its contents recursively.
	-f delete file
```

```
df:
Usage: df
Show information about the file system.
```

```
ls:
Usage: ls FILE
List information about the FILEs.
```

```
move:
Usage: move SOURCE DESTINATION
move SOURCE to DESTINATION.
```

```
shutdown:
Usage: shutdown
Shut down the file system.
```
