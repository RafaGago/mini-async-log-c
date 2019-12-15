#!/bin/sh

# This script builds and runs the tests for all the configuration variants.

errcho() {
  echo "$@" 1>&2;
}

DSTDIR="$1"
EXTRAOPT=$2

if [ -z "$DSTDIR" ]; then
    errcho "DSTDIR is needed as the first positional argument."
    exit 1
fi

if [ -d "$DSTDIR" ]; then
    read -p "DSTDIR exists. Do you want to clear \"$DSTDIR\"? [y/n]" yn
    if [ "$yn" != "y" ]; then
        exit 0
    fi
fi

rm -rf "$DSTDIR" && mkdir -p "$DSTDIR" || exit 1

VARIANTS=$(cat << END
dbg           -Db_sanitize=address --buildtype=debug   -Dcompressed_builtins=false "$EXTRAOPT"
dbg-cbuiltins -Db_sanitize=address --buildtype=debug   -Dcompressed_builtins=true  "$EXTRAOPT"
rel           --buildtype=release -Dcompressed_builtins=false "$EXTRAOPT"
rel-cbuiltins --buildtype=release -Dcompressed_builtins=true  "$EXTRAOPT"
END
)

meson setup "$DSTDIR" || exit 1
echo "$VARIANTS" | while read VARIANT; do
    NAME=$(echo "$VARIANT" | cut -d ' ' -f 1)
    FLAGS=$(echo "$VARIANT" | cut -d ' ' -f 2- | xargs)
    meson configure $FLAGS "$DSTDIR"
    if [ "$?" != "0" ]; then
        errcho "error when configuring variant: $NAME, with flags: $FLAGS"
        exit 1
    fi
    ninja -C "$DSTDIR"
    if [ "$?" != "0" ]; then
        errcho "error when building variant: $NAME, with flags: $FLAGS"
        exit 1
    fi
    ninja -C "$DSTDIR" test
    if [ "$?" != "0" ]; then
        errcho "error when testing variant: $NAME, with flags: $FLAGS"
        exit 1
    fi
done
