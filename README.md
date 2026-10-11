# ZIP

A small C program that creates ZIP archives using the store method (no compression).

## Build

```sh
gcc src/zip.c -o zip
```

## Usage

```sh
./zip <output.zip> <file1.txt> [<file2.txt> ... <fileN.txt>]
```

Each input file must be no larger than 1 MiB.

## Limitation

Tool can only zip so far. Unzip is not yet supported.
