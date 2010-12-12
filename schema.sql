CREATE TABLE classes (code TEXT, id INTEGER, superId INTEGER, name TEXT, size INTEGER);
CREATE TABLE instances (code TEXT, id INTEGER, classId INTEGER, size INTEGER);
CREATE TABLE refs (code TEXT, id INTEGER, refId INTEGER, fieldId INTEGER);
CREATE TABLE names (code TEXT, id INTEGER, name TEXT);

