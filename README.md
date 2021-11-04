# template
A template for assignments and projects.
## Setup
```
mkdir cmake-build-debug
```

## Source Code Additions
Add your source files to the CMakeLists.txt:

```
set(HEADER_LIST [files])
set(SOURCE_LIST [files])
```

```
cmake -DCMAKE_C_COMPILER="gcc" -DCMAKE_CXX_COMPILER="g++" -S . -B cmake-build-debug & cmake --build cmake-build-debug --clean-first
```
or:

```
cmake -DCMAKE_C_COMPILER="clang" -DCMAKE_CXX_COMPILER="clang++" -S . -B cmake-build-debug
```

## Build 
Examples:
```
cmake --build cmake-build-debug
cmake --build cmake-build-debug --clean-first
cmake --build cmake-build-debug --target docs
cmake --build cmake-build-debug --target format
```


echo "hi" | ./cmake-build-debug/src/ascii2hamming --parity odd --prefix abc
./cmake-build-debug/src/hamming2asii --parity odd --prefix program
./cmake-build-debug/src/hamming2asii 

xxd -b program2.hamming
dcdump < program2.hamming

