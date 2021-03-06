#!/bin/bash

# A very unefficient demo script that converts the dates on the log files from
# monotonic to calendar time. This works on files like e.g. the one generated
# by example/overview.c

errcho() {
  echo "$@" 2>&1
}

FILE="$1"
if [[ ! -f "$FILE" ]]; then
  errcho "File does't exist: $FILE"
  exit 1;
fi

CALENDAR=$(basename "$FILE" | cut -d _ -f 2 | tr a-z A-Z)
MONOTONIC=$(basename "$FILE" | cut -d _ -f 3 | cut -d . -f 1 | tr a-z A-Z)

CALENDAR=$(echo "obase=10; ibase=16; $CALENDAR" | bc)
MONOTONIC=$(echo "obase=10; ibase=16; $MONOTONIC" | bc)
DIFF=$(echo "($CALENDAR - $MONOTONIC) / 1000000000" | bc)

while IFS= read -r line; do
  DATE="$(echo "$line" | cut -d [ -f 1)"
  SEC="$(echo "$DATE" | cut -d . -f 1)"
  SEC="$(echo "$SEC + $DIFF" | bc)"
  CAL="$(date "+%z %Y-%m-%d %H:%M:%S" --date="@$SEC")"
  DECIMAL="$(echo "$DATE" | cut -d . -f 2)"
  LOG="[$(echo "$line" | cut -d [ -f 2-)"
  echo "${CAL}.${DECIMAL}${LOG}"
done < "$FILE"
