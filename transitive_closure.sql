BEGIN;
DELETE FROM cd2;
INSERT INTO cd2 SELECT DISTINCT refs.refId FROM closure_delta JOIN refs ON closure_delta.id = refs.id LEFT OUTER JOIN closure ON refs.refId = closure.id WHERE closure.id IS NULL;
DELETE FROM closure_delta;
INSERT INTO closure SELECT id FROM cd2;
INSERT INTO closure_delta SELECT id FROM cd2;
SELECT COUNT(*) FROM closure_delta;
select classes.id, name, COUNT(closure_delta.id) FROM closure_delta join instances on closure_delta.id=instances.id join classes on instances.classId = classes.id GROUP BY classes.id, name ORDER BY COUNT(closure_delta.id) DESC LIMIT 10;
COMMIT;

