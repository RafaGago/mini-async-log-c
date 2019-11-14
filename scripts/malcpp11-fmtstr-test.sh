#!/bin/bash

BUILDFOLDER="$1"
if [[ ! -d "$BUILDFOLDER" ]]; then
  echo "The build folder doesn't exist" 1>&2
  exit 1
fi

PROJROOT=$(dirname "$(readlink -f "$0")")/..
FILE="$PROJROOT"/meson.build

ERRORS=""
for v in $(grep 'cpp-fmtstr-' "$FILE" | tr -d \' | tr -d ' ' | tr -d ,); do
  target=$(echo "$v" | cut -d : -f 1)
  exp=$(echo "$v" | cut -d : -f 2 | cut -d _ -f 1)
  ninja -C "$BUILDFOLDER" "$target"
  ret=$?
  if [[ $ret = "0" ]] && [[ "$exp" == "fail" ]]; then
    ERRORS="$ERRORS\n$target: passed but was expected to fail"
  fi
  if [[ $ret != "0" ]] && [[ "$exp" == "pass" ]]; then
    ERRORS="$ERRORS\n$target: failed but was expected to pass"
  fi
done

if [[ -z "$ERRORS" ]]; then
  printf "\nDone: All tests succeeeded!\n"
  exit 0
fi

printf "\nErrors were found:${ERRORS}\n"
exit 1
