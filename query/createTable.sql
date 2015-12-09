CREATE TABLE facebook (v1 int, v2 int); 
COPY facebook FROM '/Users/ylu/nprr/graph/facebook.txt' DELIMITER ' ';
CREATE TABLE gnutella (v1 int, v2 int); 
COPY gnutella FROM '/Users/ylu/nprr/graph/gnutella.txt' DELIMITER ' ';
CREATE TABLE wikivote (v1 int, v2 int); 
COPY wikivote FROM '/Users/ylu/nprr/graph/wikivote.txt' DELIMITER ' ';
CREATE TABLE condmat (v1 int, v2 int); 
COPY wikivote FROM '/Users/ylu/nprr/graph/condmat.txt' DELIMITER ' ';