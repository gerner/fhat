BEGIN;
CREATE INDEX cid ON classes (id);
CREATE INDEX iid ON instances (id);
CREATE INDEX icid ON instances (classId);
CREATE INDEX riid ON refs (id);
CREATE INDEX rrid ON refs (refId);
COMMIT;

