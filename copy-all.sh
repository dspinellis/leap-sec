#!/bin/bash

add_label()
{
  LABEL="$1"
  SUFFIX="${2:-m}"
  awk 'BEGIN {OFS = "\t"} {$3 = "'$LABEL'"; print "'$SUFFIX'\t" $0 }'
}

get()
{
  SUFFIX=$1
  HOSTS="$2"
  {
    for i in parrot swa papers freefall ; do
      ssh $i cat /tmp/leap-sec-$i$SUFFIX.txt |
      add_label $i $SUFFIX
    done
  } |
  sort -k2n
}

{
  echo "type	abs	unix	system	fdate	ftime"
  get -b
  get | grep -v $'\t-'
  get -e
} >all.txt
