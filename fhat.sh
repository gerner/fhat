#!/bin/bash
set -e

catprog=cat
if type -P pv &> /dev/null; then catprog=pv; fi

heapdump=$1

echo "parsing dump..."
$catprog $heapdump | ~/workspace/fhat/src/parse - classes instances references names

echo;echo "sorting instances..."
~/workspace/fhat/src/sort instances.binary instances.sorted id

echo;echo "sorting references..."
~/workspace/fhat/src/sort references.binary references.sorted link

echo;echo "packing references..."
$catprog references.sorted | ~/workspace/fhat/src/pack instances.sorted > references.packed

#this is a somewhat expensive sanity check
#echo;echo "checking work..."
#~/workspace/fhat/src/sort references.packed /tmp/references.packed.sorted link
#cmp references.packed /tmp/references.packed.sorted
#rm /tmp/references.packed.sorted

echo;echo "finding reachable objects..."
~/workspace/fhat/src/dfs references.packed semi vertex parent $((`stat -c %s instances.binary`/8)) pred

echo;echo "sorting pred..."
~/workspace/fhat/src/sort pred pred.sorted link

echo;echo "compute dominator tree..."
~/workspace/fhat/src/dominate pred.sorted semi vertex parent dominator

echo;echo "done."
