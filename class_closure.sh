#!/bin/bash
set -e

#0. prepare schema
sqlite3 $1 ".read transitive_closure.schema.sql"

#1. seed with the instances
sqlite3 $1 "INSERT INTO closure SELECT id FROM instances WHERE classId=$2"
lastCount=`sqlite3 $1 "SELECT COUNT(*) FROM closure"`

#2. loop until we get completion
nextCount=`sqlite3 $1 ".read transitive_closure.sql"`
while [ nextCount -neq lastCount]
do
	lastCount=$nextCount
	nextCount=`sqlite3 $1 ".read transitive_closure.sql"`
done

