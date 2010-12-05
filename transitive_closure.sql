BEGIN;
INSERT INTO closure SELECT refs.refId FROM closure JOIN refs ON closure.id = refs.id;
SELECT COUNT(*) FROM closure;
COMMIT;
