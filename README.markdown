# fhat

A fast java heap (HPROF binary format) analysis tool written in c++

If you like you can pronounce it "fat" (as in phat).  But I don't.

## Building

you should just be able to clone the repo, and run:

	aclocal
	automake --add-missing
	autoconf
	./configure
	make

now you have the fhat executable in the src directory

## Using

fhat takes a dump file in the HPROF binary format and produces several text files representing that dump.  The command line is as follows:

	fhat dumpfile instances_file classes_file references_file names_file

where 
* dumpfile is the HPROF binary dump
* instances_file is the output file for information about instances
* classes_file is the output file for information about classes
* references_file is the output file for information about references held by instances
* names_file is the output file for all identifiers (in particular field names)

## Analyzing

the output of fhat is meant to be analyzed by other simple tools such as:
* grep
* awk
* sort
* uniq
* sqlite

### Quick Report

The script fhat.sh will (after building and with some small tweaks to binary paths) quickly (at roughly 4MB/sec) generate a usable report in instances.data.full.  

Specifically there are rows of data sorted by retained heap size.  The columns are id, retained heap, retained count, object size, dominator, class id, class name.

This will quickly identify the objects which are retaining large amounts of memory.  That is, these are the objects who, if garbage collected, would make a very large amount of memory in the heap collectable.  

### Getting Data into Sqlite

First, I highly recommend you run sqlite in nosync mode.  This will improve the time it import data and do any inserts you might do (see transitive closure below).

	PRAGMA synchronous = OFF;

In my experience, on a laptop I can get 9 or 10 MB/sec on a laptop with sync on (synchronous = NORMAL).  But I get 15-20 MB/sec with nosync (synchronous OFF).

to that end here's a simple schema for sqlite3:

	CREATE TABLE classes (code TEXT, id INTEGER, superId INTEGER, name TEXT, size INTEGER);
	CREATE TABLE instances (code TEXT, id INTEGER, classId INTEGER, size INTEGER);
	CREATE TABLE refs (code TEXT, id INTEGER, classId INTEGER, refId INTEGER, fieldId INTEGER);
	CREATE TABLE names (code TEXT, id INTEGER, name TEXT);

to import into these tables you'll need to run commands similar to the following:

	.import /path/to/classes classes
	.import /path/to/instances instances
	.import /path/to/references refs
	.import /path/to/names refs

I suggest the following indexes once you have imported all of your data:

	CREATE INDEX cid ON classes (id);
	CREATE INDEX iid ON instances (id);
	CREATE INDEX icid ON instances (classId);
	CREATE INDEX riid ON refs (id);
	CREATE INDEX rcid ON refs (classId);
	CREATE INDEX rrid ON refs (refId);

Note that creating all of these indexes will take many minutes for large dumps and will more than double the size of the resulting database. This should not be a problem since you won't be inserting into these tables and the indexes are critical for analysis.

All of this can be somewhat automated by sql scripts included in this distribution:

	PRAGMA synchronous = OFF;
	.separator '	'
	.read schema.sql
	.import /tmp/heapdump/names names
	.import /tmp/heapdump/classes classes
	.import /tmp/heapdump/instances instance
	.import /tmp/heapdump/references refs
	.read indexes.sql

### Finding Simple Info About Instances

find the top 50 classes whose instances account for the most memory:

	SELECT classes.id, classes.name, COUNT(instances.id), SUM(instances.size) FROM classes JOIN instances ON classes.id = instances.classId GROUP BY classes.id, classes.name ORDER BY SUM(instances.size) DESC LIMIT 50

find the number of Strings:

	SELECT COUNT(instances.id), SUM(instances.size) FROM instances JOIN classes ON instances.classId = classes.id WHERE classes.name = "java/lang/String";

find the number of objects that have direct references to strings:

	CREATE TEMP TABLE string_instances (id INTEGER);
	INSERT INTO string_instances SELECT instances.id FROM instances JOIN classes ON instances.classId = classes.id WHERE classes.name = "java/lang/String";
	SELECT COUNT(*) FROM instances JOIN references ON instances.id = references.id JOIN string_instances ON references.refId = string_instances.id;

find the classes holding on to references of a particular type:

	SELECT refs.classid, COUNT(refs.id), SUM(instances.size) FROM instances JOIN refs ON instances.id = refs.refid WHERE instances.classid=46912581001104 GROUP BY refs.classid ORDER BY sum(instances.size) DESC LIMIT 20;

### Cumulative Memory for Objects: Transitive Closure

To find the memory actually used by an object or set of objects is somewhat tricky because the reference graph for a Java heap can be dense.  That is, lots of objects are referenced from many other objects and it's not always easy to say that just because an object is reachable from another means that the other object is "responsible" for that memory usage.  However, finding the amount of memory reachable from an object (the transitive closure) is still valuable.

transitive_closure.schema.sql contains the schema and indexes for a temporary table to facilitate finding an object or set of object's reachable memory transitive closure.

transitive_closure.sql contains two queries that should be run iteratively to gather all reachable objects from some seed set.

To start, first create the closure table according to transitive_closure.schema.sql (or empty if you've already created it.) For example:

	INSERT INTO closure SELECT id FROM instances WHERE classId=12345;

The above query will find add all instances of class whose id is 12345 to the closure table and will act as the seed set for our transitive closure.

Once you've got a seed set in the closure table, we'll iteratively find objects reachable from that set.  The two queries in transitive_closure.sql are exactly what you need:

	INSERT INTO closure SELECT refs.refId FROM closure JOIN refs ON closure.id = refs.id;
	SELECT COUNT(*) FROM closure;

The first query inserts reachable objects (the closure table should have a UNIQUE constraint with an IGNORE conflict resolution to drop duplicates.) The second gives you a count of the current closure.  Continue running this pair of queries until the count remains the same from one iteration to the next.  Each iteration can take several minutes for large (1 or more GB).  There are already appropriate indexes on closure and refs.

Once you've materialized the transitive closure, you can do some analysis on the bundled group of objects.  For instance, what is the total size of memory reachable from your original seed set:

	SELECT COUNT(closure.id), SUM(size) FROM closure JOIN instances ON closure.id = instances.id;

What is the size used by each type of object reachable from your original seed set:

	SELECT classId, COUNT(closure.id), SUM(size) FROM closure JOIN instances ON closure.id = instances.id GROUP BY classId ORDER BY SUM(size) DESC LIMIT 20;

It's valuable to compare this to the total memory used by all objects (including those not reachable from your seed set) of those types:

	SELECT SUM(size) FROM instances WHERE classId=1234

Where 1234 appears high up in the list of object types above.

## Domination!

parse the dump
	~/workspace/fhat/src/parse heap.dump classes instances references names

sort the instances
	~/workspace/fhat/src/sort instances.binary instances.sorted id

sort the references
	~/workspace/fhat/src/sort references.binary references.sorted link

pack the references
	cat references.sorted | ~/workspace/fhat/src/pack instances.sorted > references.packed

assert it's still sorted
	~/workspace/fhat/src/sort references.packed /tmp/references.packed.sorted link
	cmp references.packed /tmp/references.packed.sorted
	rm /tmp/references.packed.sorted

get the spanning tree by dfs
	~/workspace/fhat/src/dfs references.packed semi vertex parent $((`stat -c %s instances.binary`/8))
