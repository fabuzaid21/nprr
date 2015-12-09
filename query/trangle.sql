select count(*) 
from graph as A, graph as B, graph as C
where A.v2 = B.v1 and B.v2 = C.v1 and C.v2 = A.v1;

