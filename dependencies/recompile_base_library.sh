#!/bin/sh

cwd=$(pwd)
bl=$cwd/base_library/

cd $bl/build/premake
./premake5 gmake
cd $bl/build/linux
make -j4 config=release
make -j4 config=debug
cd $cwd
cd ../build/premake
ln -s $bl/build/premake/premake5
$bl/build/stage/linux/release/bin/base_library_test

