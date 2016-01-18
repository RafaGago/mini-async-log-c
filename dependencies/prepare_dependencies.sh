#!/bin/sh

cwd=$(pwd)
bl=$cwd/base_library/

cd $bl/dependencies
./prepare_dependencies.sh
cd $cwd

./recompile_base_library.sh
