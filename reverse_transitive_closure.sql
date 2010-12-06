BEGIN;
INSERT INTO closure SELECT refs.id FROM closure JOIN refs ON closure.id = refs.refId;
SELECT COUNT(*) FROM closure;
COMMIT;
