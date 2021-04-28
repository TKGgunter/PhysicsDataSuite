#!/bin/sh


#makes SDF lib
g++ -c SDF.cc -std=c++11 -fPIC  &&  g++ -shared -o SDF.dll SDF.o



#turn bmp into obj file
#this might need to change for different arch
objcopy --input binary --output elf64-x86-64 --binary-architecture i386 sdfviewer.bmp sdfviewer.o


#makes sdfviewer
g++ sdfviewer.o sdfviewer.c -o sdfviewer -lX11 -std=c++11
