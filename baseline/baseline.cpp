#include <stdio.h>
#include <stdlib.h>

#include<vector>
#include<map>

using namespace std;

vector<pair<int, int> > rel1;
map<int, vector<int> > rel2;
map<int, vector<int> > rel3;

int main(int argc, char **argv) {
  FILE *fin = fopen(argv[1], "r");

  while (true) {
    int a,b;
    int nRet = fscanf(fin, "%d %d", &a, &b);
    if (nRet != 2) {
      break;
    }


    rel1.push_back(make_pair(a,b));

    if (!rel2.count(a)) {
      vector<int> v;
      rel2[a] = v;
    }

    rel2[a].push_back(b);

    
    if (!rel3.count(a)) {
      vector<int> v;
      rel3[a] = v;
    }

    rel3[a].push_back(b);
  }

  int nTriangles = 0;

  for (auto x : rel1) {
    int a = x.first;
    int b = x.second;

    if (rel2.count(b)) {
      for (int c : rel2[b]) {
        if (rel3.count(a)) {
          for (int cp : rel3[a]) {
            if (c == cp) {
              nTriangles++;
            }
          }
        }
      }
    }
  }

  printf("%d\n", nTriangles);

  return 0;
}
