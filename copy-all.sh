#!/bin/sh

add_label()
{
  LABEL="$1"
  SUFFIX="${2:-m}"
  awk 'BEGIN {OFS = "\t"} {$3 = "'$LABEL'"; print "'$SUFFIX'\t" $0 }'
}

get()
{
  SUFFIX=$1
  {
    for i in parrot swa papers freefall eleana ; do
      ssh $i cat /tmp/leap-sec-$i$SUFFIX.txt |
      add_label $i $SUFFIX
    done
    dos2unix <win$SUFFIX.txt |
    add_label win $SUFFIX
  } |
  sort -k2n
}

{
  echo "type	abs	unix	system	fdate	ftime"
  get -b
  get
  get -e
} >all.txt
