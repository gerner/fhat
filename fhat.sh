#!/bin/bash
set -e

catprog=cat
if type -P pv &> /dev/null; then catprog=pv; fi

binarypath="/home/nick/workspace/fhat/src"

heapdump=$1

if [ ! -e $heapdump ]
then
	echo "can't open $heapdump" 1>&2
	exit 0
fi

echo "parsing dump..."
$catprog $heapdump | $binarypath/parse - classes instances references names

echo;echo "sorting names..."
$binarypath/sort names.binary names.sorted u64
$binarypath/sortcolumns u64 names.binary u64 names.offset.binary names.sorted names.offset.sorted
date +%s

echo;echo "sorting classes..."
$binarypath/sort classes.binary classes.sorted u64
$binarypath/sortcolumns u64 classes.binary u64 classes.name.binary classes.sorted classes.name.sorted
date +%s

echo;echo "translating class names..."
$catprog classes.name.sorted | $binarypath/translate names.sorted - - 1 > classes.name.packed

echo;echo "sorting instances..."
$binarypath/sort instances.binary instances.sorted u64
date +%s

echo;echo "sorting instance columns..."
$binarypath/sortcolumns u64 instances.binary u64 instances.class.binary instances.sorted instances.class.sorted
$binarypath/sortcolumns u64 instances.binary u64 instances.size.binary instances.sorted instances.size.sorted
$binarypath/sortcolumns u64 instances.binary u64 instances.offset.binary instances.sorted instances.offset.sorted
date +%s

echo;echo "sorting references..."
$binarypath/sort references.binary references.sorted link
date +%s

echo;echo "translating references..."
$catprog references.sorted | $binarypath/translate instances.sorted - - 1 > references.packed

#the only reason we need to do this is because translating may result in some bad refs which we don't suppress
# if this is an issue the translate prog needs to know about the structure of links in the data and drop bad refs
$binarypath/sort references.packed references.packed.sorted link
date +%s

echo;echo "finding reachable objects..."
$binarypath/dfs references.packed.sorted semi vertex parent $((`stat -c %s instances.binary`/8)) pred
date +%s

echo;echo "sorting pred..."
$binarypath/sort pred pred.sorted link
date +%s

echo;echo "compute dominator tree..."
$binarypath/dominate pred.sorted semi vertex parent dominator
date +%s

echo;echo "translating dominators..."
$catprog dominator | $binarypath/unpack instances.sorted - - > dominator.translated 

echo;echo "compute retained memory for each node..."
$binarypath/retain vertex dominator instances.size.sorted instances.retained.sorted instances.retaincount.sorted
date +%s

echo;echo "sort by retained size..."
$binarypath/zipfiles zip instances.retained.sorted instances.size.sorted instances.retaincount.sorted dominator.translated instances.sorted instances.class.sorted instances.offset.sorted | pv | $binarypath/sort - - ru64_7 | $binarypath/zipfiles unzip i.r i.s i.rc i.d i i.c i.o | head -n 1000
date +%s

#for sqlite: (id, retained, retCount, size, dominator, class)
echo;echo "preparing for sqlite..."

#do joins to get names
$catprog i.c | $binarypath/translate classes.sorted - - 1 | $binarypath/unpack classes.name.sorted - - | $binarypath/translate names.sorted - - 0 | $binarypath/unpack names.offset.sorted - - | $binarypath/unpack names.name.binary - - string | tr '\0' '\n' > i.n

$binarypath/zipfiles zip i i.r i.rc i.s i.d i.o | pv - | od -t d8 -w$((6*8)) -An -v | sed 's/[ ][ ]*/\t/g' | sed 's/^\t//' | paste - i.n | head -n 1000 > instances.data.full
date +%s

#$catprog instances.class.sorted | $binarypath/translate classes.sorted - - 1 | $binarypath/unpack classes.name.sorted - - | $binarypath/translate names.sorted - - 0 | $binarypath/unpack names.offset.sorted - - | $binarypath/unpack names.name.binary - - string | tr '\0' '\n' > instances.data.names

#this pastes the class names to the right of all the other instance columns made readable
#$binarypath/zipfiles zip instances.sorted instances.retained.sorted instances.retaincount.sorted instances.size.sorted dominator.translated instances.offset.sorted | pv - | od -t d8 -w$((6*8)) -An -v | sed 's/[ ][ ]*/\t/g' | sed 's/^\t//' | paste - instances.data.names > instances.data.full

#echo;echo "getting top 1000 instances by retained size..." 
#sort -k2 -r -n instances.data.full | head -n 1000 > instances.top1k

echo;echo "done."
