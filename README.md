
# TinyFileManager

TinyFileManager is a miniature in-memory file system simulation written in C, designed for educational purposes. It mimics the structure and behavior of a basic file system, including inodes, directories, file operations, and symbolic and hard links.

## Features

- Inode-based architecture
- Support for:
  - Regular files
  - Directories
  - Hard links
  - Symbolic links
- File operations:
  - Create, delete, read, write, open, close, seek
  - Copy and move files and directories
- Command-line interface with interactive shell
- File permissions (`r`, `w`, `x`)
- Persistent state via binary file (`filesystem.img`)
- Path resolution with support for relative and absolute paths

## File Structure

- `TinyFileManager.c` – The main source file containing all logic for the simulated file system.

## Getting Started

### Compilation

```bash
gcc -o filesystem TinyFileManager.c
```

### Usage

```bash
./filesystem [-i]
```

- `-i`: Force initialization of the file system.

### Interactive Commands

- `ls` — List current directory
- `cd <path>` — Change directory
- `mkdir <dir>` — Create a new directory
- `touch <file>` — Create a new file
- `rm <file>` — Delete a file
- `remdir <dir>` — Recursively delete a directory
- `cp <src> <newname> <dest_path>` — Copy a file or directory
- `mv <src> <dest_path>` — Move a file or directory
- `ln <filename> <linkname> <target_path>` — Create a hard link
- `sym <path> <linkname>` — Create a symbolic link
- `open <file>` / `close <desc>` — Open or close file descriptors
- `wfile <desc> <text> <size>` — Write to a file
- `rfile <desc> <size>` — Read from a file
- `sfile <desc> <offset> <whence>` — Move file read/write head
- `stat <file>` — Show file info
- `list_desc` — List open file descriptors
- `pwd` — Print current directory
- `exit` — Quit the shell

## Notes

- The simulated file system is stored in a binary file named `filesystem.img`.
- Inodes and blocks are managed in-memory and persisted upon saving.
- Ideal for understanding the fundamentals of file system implementation.

## License

MIT License
