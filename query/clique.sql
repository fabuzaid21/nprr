select count(*)
from graph as A, graph as B, graph as C, graph as D, graph as E, graph as F
where A.v1 = B.v2 and B.v1 = D.v2 and D.v1 = C.v2 and C.v1 = A.v2 and A.v1 = F.v1 and C.v2 = F.v2 and B.v1 = E.v1 and C.v1 = E.v2;
