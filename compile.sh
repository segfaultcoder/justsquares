#!/usr/bin/sh

rm build/justsquares
rm build/justsquares.exe
echo LINUX COMPILATION
g++ justsquares.cpp libs/PerlinNoise.cpp libs/enet/*.c -std=c++14 -lX11 -lGL -lpthread -lpng -lstdc++fs -o build/justsquares -lasound
echo WINDOWS COMPILATION
i686-w64-mingw32-g++ justsquares.cpp libs/PerlinNoise.cpp libs/enet/*.c -posix -static-libgcc -static-libstdc++ -std=c++14 -lX11 -lopengl32 -lpthread -lgdi32 -lgdiplus -lstdc++fs -lwinmm -o build/justsquares.exe -lwsock32 -lws2_32 -posix -static
echo LINUX EXECUTION
cd build
./justsquares
cd ..