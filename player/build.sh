./build-sdl.sh $1

gcc -O2 -o player *.cpp *.c -lstdc++ -lpthread -lm -ldl -LSDL/build-$1 -ISDL/include -lSDL2
