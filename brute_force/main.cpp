#include<stdlib.h>
#include<stdio.h>

#define MAX_LEN 1000

int graph[MAX_LEN][MAX_LEN];

int max(int a, int b) {
  return a < b ? b : a;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: ./%s <infile>", argv[0]);
    return 0;
  }

  FILE *fin = fopen(argv[1], "r");

  for (int i = 0; i < MAX_LEN; i++) {
    for (int j = 0; j < MAX_LEN; j++) {
      graph[i][j] = 0;
    }
  }

  int nVerts = 0;
  
  while (true) {
    int a,b;
    int nRet = fscanf(fin, "%d %d", &a, &b);
    if (nRet != 2) {
      break;
    }

    graph[a][b] = 1;

    nVerts = max(nVerts, a);
    nVerts = max(nVerts, b);
  }
  
  fclose(fin);

  if (nVerts >= MAX_LEN) {
    printf("Graph too large");
    return 0;
  }
  
  int nTriangles = 0;

  for (int i = 0; i <= nVerts ; i++) {
    for (int j = 0; j <= nVerts; j++) {
      if (!graph[i][j]) {
        continue;
      }
      
      for (int k = 0; k <= nVerts; k++) {
        if (graph[j][k] && graph[i][k]) {
          //printf("Found %d %d %d\n", i,j,k);
          nTriangles++;
        }
      }
    }
  }

  printf("%d\n", nTriangles);
}
