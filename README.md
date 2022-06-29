# game-of-life

Simple command-line adaptation of the infamous [Matrix digital rain](https://en.wikipedia.org/wiki/Matrix_digital_rain).

Compilation
===========

> This version uses some UNIX system headers.  
> It thus won't compile on Windows, but will probably get changed in the future.

Using CMake : 
```bash
mkdir build && cd build && cmake .. && make
```

Or just compile the source with C++11 and linking your OS' thread library.