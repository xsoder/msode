# MSODE 
The next music player application for linux.

To build the project follow the instructions below.

Install wget if does not exist.

1. Arch install
```bash
sudo pacman -S curl libarchive libzip
```

2. Debian install
```bash
sudo pacman -S libcurl4-openssl-dev libarchive-dev libzip-dev
```

## BUILD INSTRUCTION
Build Nob first it is responsible for compilation.
```bash
cc -o nob nob.c
```
Then just run the nob executable it does not check if the libraries exist.

```bash
./nob
```
The autogen with grab all the necessary dependencies.
