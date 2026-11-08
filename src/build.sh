#!/bin/sh
set -e
cd "$(dirname "$0")"
mkdir -p ../build
llvm-dlltool -m i386:x86-64 -d kernel32.def -l ../build/kernel32.lib
llvm-dlltool -m i386:x86-64 -d user32.def -l ../build/user32.lib
clang --target=x86_64-pc-windows-msvc -O2 -c sekiro_tweaks.cpp -o ../build/sekiro_tweaks.obj \
  -ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -mno-stack-arg-probe -Wall -Wno-unused-function
lld-link /DLL /NOLOGO /NODEFAULTLIB /ENTRY:DllMain /MACHINE:X64 /SUBSYSTEM:WINDOWS /OPT:REF \
  /DEF:dinput8.def /OUT:../build/dinput8.dll ../build/sekiro_tweaks.obj ../build/kernel32.lib ../build/user32.lib
