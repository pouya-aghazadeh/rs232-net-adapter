## RS232 protocol's data buffer adapter to IPv4 Request-Response server
>  Compile The src.cpp File With Below Shell Command By Mingw-w64 For Windows Or GCC For GNU/Linux

```sh
g++ -std=c++11 src.cpp -o bin.exe rs232.c -lws2_32 -static -static-libgcc -static-libstdc++ -municode
```
