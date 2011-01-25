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
~/workspace/fhat/src/sortcolumns u64 instances.binary u64 instances.offset.binary instances.sorted instances.offset.sorted

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

echo;echo "translating dominators..."
$catprog dominator | ~/workspace/fhat/src/unpack instances.sorted - - > dominator.translated 

echo;echo "compute retained memory for each node..."
~/workspace/fhat/src/retain vertex dominator instances.size.sorted instances.retained.sorted instances.retaincount.sorted

#echo;echo "sort by retained size..."
#~/workspace/fhat/src/zipfiles zip instances.retained.sorted instances.size.sorted instances.sorted instances.class.sorted | pv | ~/workspace/fhat/src/sort - - ru64_4 | ~/workspace/fhat/src/zipfiles unzip i.r i.s i i.c

#for sqlite: (id, retained, retCount, size, dominator, class)
echo;echo "preparing for sqlite..."

#this gets the class name for each instance
$catprog instances.class.sorted | ~/workspace/fhat/src/translate classes.sorted - - 1 | ~/workspace/fhat/src/unpack classes.name.sorted - - | ~/workspace/fhat/src/translate names.sorted - - 0 | ~/workspace/fhat/src/unpack names.offset.sorted - - | ~/workspace/fhat/src/unpack names.name.binary - - string | tr '\0' '\n' > instances.data.names

#this pastes the class names to the right of all the other instance columns made readable
~/workspace/fhat/src/zipfiles zip instances.sorted instances.retained.sorted instances.retaincount.sorted instances.size.sorted dominator.translated instances.offset.sorted | pv - | od -t d8 -w$((6*8)) -An -v | sed 's/[ ][ ]*/\t/g' | sed 's/^\t//' | paste - instances.data.names > instances.data.full

echo;echo "getting top 1000 instances by retained size..." 
sort -k2 -r -n instances.data.full | head -n 1000 > instances.top1k

echo;echo "done."
