#!/bin/bash
set -e

catprog=cat
if type -P pv &> /dev/null; then catprog=pv; fi

heapdump=$1

if [ ! -e $heapdump ]
then
	echo "can't open $heapdump" 1>&2
	exit 0
fi

echo "parsing dump..."
$catprog $heapdump | ~/workspace/fhat/src/parse - classes instances references names

echo;echo "sorting names..."
~/workspace/fhat/src/sort names.binary names.sorted u64
~/workspace/fhat/src/sortcolumns u64 names.binary u64 names.offset.binary names.sorted names.offset.sorted

echo;echo "sorting classes..."
~/workspace/fhat/src/sort classes.binary classes.sorted u64
~/workspace/fhat/src/sortcolumns u64 classes.binary u64 classes.name.binary classes.sorted classes.name.sorted

echo;echo "translating class names..."
$catprog classes.name.sorted | ~/workspace/fhat/src/translate names.sorted - - 1 > classes.name.packed

echo;echo "sorting instances..."
~/workspace/fhat/src/sort instances.binary instances.sorted u64
echo;echo "sorting instance columns..."
~/workspace/fhat/src/sortcolumns u64 instances.binary u64 instances.class.binary instances.sorted instances.class.sorted
~/workspace/fhat/src/sortcolumns u64 instances.binary u64 instances.size.binary instances.sorted instances.size.sorted

echo;echo "sorting references..."
~/workspace/fhat/src/sort references.binary references.sorted link

echo;echo "translating references..."
$catprog references.sorted | ~/workspace/fhat/src/translate instances.sorted - - 1 > references.packed

#the only reason we need to do this is because translating may result in some bad refs which we don't suppress
# if this is an issue the translate prog needs to know about the structure of links in the data and drop bad refs
~/workspace/fhat/src/sort references.packed references.packed.sorted link

echo;echo "finding reachable objects..."
~/workspace/fhat/src/dfs references.packed.sorted semi vertex parent $((`stat -c %s instances.binary`/8)) pred

echo;echo "sorting pred..."
~/workspace/fhat/src/sort pred pred.sorted link

echo;echo "compute dominator tree..."
~/workspace/fhat/src/dominate pred.sorted semi vertex parent dominator

echo;echo "compute retained memory for each node..."
~/workspace/fhat/src/retain vertex dominator instances.size.sorted instances.retained.sorted

echo;echo "done."
