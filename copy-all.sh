#!/bin/sh
{
  for i in parrot swa papers freefall eleana ; do
    ssh $i cat /tmp/leap-sec-$i.txt |
    awk 'BEGIN {OFS = "\t"} {$3 = "'$i'"; print}'
  done
  dos2unix <win.txt |
  awk 'BEGIN {OFS = "\t"} {$3 = "win"; print}'
} |
{
  echo "abs	unix	system	fdate	ftime"
  sort -n
} >all.txt
