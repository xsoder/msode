# MSODE 

## Change Log
> [!NOTE]
> Windows Support Added Through Mingw
> Yay


> [!WARNING]
> The project is not fully completed. 

The next music player application for linux.

## BUILD INSTRUCTION
Build Nob first it is responsible for compilation.
```bash
cc -I -o nob nob.c
```

For windows it is going to be 
```bash
x86_64-w64-mingw32-gcc.exe -I -o nob nob.c
```

Then just run the nob executable it does not check if the libraries exist.

```bash
./nob or nob.exe
```
The autogen with grab all the necessary dependencies.

To run the program simply execute
```bash
./build/msode or build\msode.exe
```
