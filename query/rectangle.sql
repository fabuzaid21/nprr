select count(*)
from graph as A, graph as B, graph as C, graph as D
where A.v1 = B.v2 and B.v1 = D.v2 and D.v1 = C.v2 and C.v1 = A.v2;