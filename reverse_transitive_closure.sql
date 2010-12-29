CREATE TEMP TABLE cd2 (id INTEGER);
BEGIN;
INSERT INTO cd2 SELECT DISTINCT refs.id FROM closure_delta JOIN refs ON closure_delta.id = refs.refId LEFT OUTER JOIN closure ON refs.id = closure.id WHERE closure.id IS NULL;
DELETE FROM closure_delta;
INSERT INTO closure SELECT id FROM cd2;
INSERT INTO closure_delta SELECT id FROM cd2;
SELECT COUNT(*) FROM closure_delta;
select classes.id, name, COUNT(closure_delta.id), SUM(instances.size) FROM closure_delta join instances on closure_delta.id=instances.id join classes on instances.classId = classes.id GROUP BY classes.id, name ORDER BY COUNT(closure_delta.id) DESC LIMIT 10;
COMMIT;
DROP TABLE cd2;

