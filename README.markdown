a fast java heap (HPROF binary format) analysis tool written in c++

--------
BUILDING
--------

you should just be able to clone the repo, and run:

aclocal
automake --add-missing
autoconf
./configure
make

--------
ANALYSIS
--------

the output of fhat is meant to be analyzed by other simple tools such as:
 * grep
 * awk
 * sort
 * uniq
 * sqlite

to that end here's a simple schema for sqlite3:

CREATE TABLE classes (code TEXT, id INTEGER, superId INTEGER, name TEXT, size INTEGER);
CREATE TABLE instances (code TEXT, id INTEGER, classId INTEGER, size INTEGER);
CREATE TABLE refs (code TEXT, id INTEGER, classId INTEGER, refId INTEGER, fieldName TEXT);

to import into these tables you'll need to run commands similar to the following:

.separator ' '
BEGIN
.import /path/to/classes classes
COMMIT
BEGIN
.import /path/to/instances instances
COMMIT
BEGIN
.import /path/to/references refs
COMMIT

I suggest the following indexes once you have imported all of your data:

CREATE INDEX cid ON classes (id);
CREATE INDEX iid ON instances (id);
CREATE INDEX icid ON instances (classId);
CREATE INDEX riid ON refs (id);
CREATE INDEX rcid ON refs (classId);
CREATE INDEX rrid ON refs (refId);

now some useful queries:

find the number of Strings:
SELECT COUNT(*) FROM instances JOIN classes ON instances.classId = classes.id WHERE classes.name = "java/lang/String";

find the number of objects that have direct references to strings:
CREATE TEMP TABLE string_instances (id INTEGER);
INSERT INTO string_instances SELECT instances.id FROM instances JOIN classes ON instances.classId = classes.id WHERE classes.name = "java/lang/String";
SELECT COUNT(*) FROM instances JOIN references ON instances.id = references.id JOIN string_instances ON references.refId = string_instances.id;

find the different classes that are holding references to HasTableEntry objects, and how many references they hold

CREATE TEMP TABLE entry_instances (id INTEGER);
INSERT INTO entry_instances SELECT instances.id FROM instances JOIN classes ON instances.classId = classes.id WHERE classes.name = "java/util/HashTableEntry";
SELECT refs.classId, COUNT(refs.refId) FROM refs JOIN map_instances ON refs.refId = map_instances.id GROUP BY refs.classId ORDER BY COUNT(refs.refId) DESC LIMIT 20;

now you'll need to look up the class name for the items that ocurr high up on the list (this is faster than joining against classes)

find the classes of instances that are holding on to the most memory:
select classId, count(id), sum(size) from instances group by classId order by sum(size) desc limit 20;

find the classes holding on to references of a particular type:
select refs.classid, count(refs.id), sum(instances.size) from instances join refs on instances.id = refs.refid where instances.classid=46912581001104 group by refs.classid order by sum(instances.size) desc limit 20;

