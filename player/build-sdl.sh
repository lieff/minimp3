set -e

SDL_VERSION=2.0.8

if [ ! -d "SDL" ]; then
  wget https://www.libsdl.org/release/SDL2-$SDL_VERSION.tar.gz
  tar -xf SDL2-$SDL_VERSION.tar.gz
  mv SDL2-$SDL_VERSION SDL
  rm SDL2-$SDL_VERSION.tar.gz
fi
SDL_DIR=SDL/build-$1
if [ "$1" = "win" ]; then
  TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE=../../windows/mingw64.cmake -DRENDER_D3D=OFF"
fi
if [ ! -d "$SDL_DIR" ]; then
  mkdir $SDL_DIR
  cd $SDL_DIR
  cmake $TOOLCHAIN -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=TRUE -DVIDEO_OPENGLES=0 -DSDL_SHARED=OFF -DPTHREADS_SEM=OFF -DOSS=OFF -DSNDIO=OFF -DDISKAUDIO=OFF -DVIDEO_WAYLAND=OFF -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_C_FLAGS=-m32 ..
  make -j4
  cd ../../
fi
if [ ! -d "SDL/include/SDL2" ]; then
  mkdir SDL/include/SDL2
  cp SDL/include/*.h SDL/include/SDL2
fi
