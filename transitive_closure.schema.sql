CREATE TEMP TABLE closure (id INTEGER CONSTRAINT iid_unique UNIQUE ON CONFLICT IGNORE);
CREATE INDEX cid ON closure (id);

