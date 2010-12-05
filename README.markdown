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

I suggest the following indexes once you have imported all of your data:
CREATE INDEX cid ON classes (id);
CREATE INDEX iid ON instances (id);
CREATE INDEX icid ON instances (classId);
CREATE INDEX riid ON refs (id);
CREATE INDEX rcid ON refs (cid);
CREATE INDEX rrid ON refs (refId);

