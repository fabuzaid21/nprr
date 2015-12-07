#include <stdint.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <vector>
#include <glpk.h>
#include <cmath>
#include <cassert>

using std::map;
using std::max;
using std::min;
using std::multimap;
using std::multiset;
using std::set;
using std::string;
using std::stringstream;
using std::vector;
using std::tuple;

#define ERROR(str) do { printf("Error: %s", (str)); } while (false);

struct node {
  set<int> universe;
  int label;
  node *leftChild;
  node *rightChild;

  node(const set<int> &_universe, int _label) {
    universe = _universe;
    label = _label;
    leftChild = rightChild = NULL;
  }

  bool isLeaf() {
    return leftChild == NULL && rightChild == NULL;
  }
};

typedef vector<int> TUPLE;

struct relation {
  set<int> attrs;
  vector<TUPLE> tuples;
  map<tuple<int, int>, int> htIndexes;
  vector<set<TUPLE> > ht1;
  // a bunch of hashtables, each one maps a key
  // (a tuple, vector of int) to a vector of tuples
  // (vector of vectors of int)
  vector<map<TUPLE, vector<TUPLE> > > ht2;
};

set<int> set_union(const set<int> & left, const set<int> & right) {
  set<int> result;
  std::set_union(left.begin(), left.end(),
      right.begin(), right.end(),
      std::inserter(result, result.begin()));
  return result;
}

set<int> difference(const set<int> & left, const set<int> & right) {
  set<int> result;
  std::set_difference(left.begin(), left.end(),
      right.begin(), right.end(),
      std::inserter(result, result.begin()));
  return result;
}

set<int> intersect(const set<int> & left, const set<int> & right) {
  set<int> result;
  std::set_intersection(left.begin(), left.end(),
      right.begin(), right.end(),
      std::inserter(result, result.begin()));
  return result;
}

vector<double> fractionalEdgeCover(const vector<relation> &hyperedges) {
  set<int> V;
  for (const auto& e: hyperedges) {
    for (int attr : e.attrs) {
      V.insert(attr);
    }
  }

  int rowNum = V.size();
  int colNum = hyperedges.size();

  glp_prob *lp = glp_create_prob();
  glp_set_prob_name(lp, "edgecover");
  glp_set_obj_dir(lp, GLP_MIN);
  glp_add_rows(lp, rowNum);

  for (int i = 1; i <= rowNum; i ++) {
    glp_set_row_bnds(lp, i, GLP_LO, 1.0, 0.0);
  }

  glp_add_cols(lp, colNum);

  for (int i = 1 ; i <= colNum; i++) {
    glp_set_col_bnds(lp, i, GLP_LO, 0.0, 0.0);
  }

  std::vector<int> ia(1), ja(1);
  std::vector<double> cof(1);

  for (int i = 1 ;i <= colNum; i ++) {
    for (int j : hyperedges[i - 1].attrs) {
      ia.push_back(i); // row
      ja.push_back(j); // col
      cof.push_back(1.0); // 1.0 * X_e_j
    }
  }

  for (int i = 1; i<= colNum; i ++) {
    glp_set_obj_coef(lp, i, log(hyperedges[i - 1].tuples.size() * 1.0));
  }

  glp_load_matrix(lp, ia.size() - 1, &ia[0], &ja[0], &cof[0]);

  glp_smcp param;
  glp_init_smcp(&param);
  param.msg_lev = GLP_MSG_OFF;

  glp_simplex(lp, &param);


  vector<double> edgecover;

  for (int i = 1 ;i <= colNum; i ++) {
    double x = glp_get_col_prim(lp, i);
    edgecover.push_back(x);

  }

  glp_delete_prob(lp);
  return edgecover;
}

node * buildTree(const set<int> & joinAttributes, const vector<relation> & hyperedges, int k) {
  bool empty = true;
  int i = 0;
  for (const auto & r : hyperedges) {
    if (i == k) {
      break;
    }
    set<int> s_intersection = intersect(r.attrs, joinAttributes);
    if (s_intersection.size() > 0) {
      empty = false;
      break;
    }
    ++i;
  }
  if (empty) {
    return NULL;
  }

  node *currNode = new node(joinAttributes, k);

  if (k > 1) {
    int i = 0;
    for (const auto &r: hyperedges) {
      if (i == k) {
        break;
      }
      std::set<int> s_subset;
      // U is not a subset of e_i
      if (!std::includes(r.attrs.begin(), r.attrs.end(),
            joinAttributes.begin(), joinAttributes.end())) {
        auto e_k = hyperedges[k - 1].attrs;
        std::set<int> s_1 = difference(joinAttributes, e_k);
        std::set<int> s_2 = intersect(joinAttributes, e_k);
        // std::set<int> s_1 = joinAttributes;

        // for (auto attr: e_k) {
        //   if (s_1.count(attr)) {
        //     s_1.erase(s_1.find(attr));
        //   }
        // }
        currNode->leftChild = buildTree(s_1, hyperedges, k - 1);
        currNode->rightChild = buildTree(s_2, hyperedges, k - 1);
        break;
      }
      ++i;
    }
  }

  return currNode;
}

void computeTotalOrder(vector<int> &totalOrder, node *currNode) {
  if (currNode->leftChild == NULL && currNode->rightChild == NULL) {
    for (const auto &elem: currNode->universe) {
      totalOrder.push_back(elem);
    }
  } else if (currNode->leftChild == NULL) {
    computeTotalOrder(totalOrder, currNode->rightChild);
  } else if (currNode->rightChild == NULL) {
    computeTotalOrder(totalOrder, currNode->leftChild);
    for (const auto &elem: currNode->universe) {
      if (!currNode->leftChild->universe.count(elem)) {
        totalOrder.push_back(elem);
      }
    }
  } else {
    computeTotalOrder(totalOrder, currNode->leftChild);
    computeTotalOrder(totalOrder, currNode->rightChild);
  }
}

vector<int> computeTotalOrder(node *currNode) {
  vector<int> toReturn;
  computeTotalOrder(toReturn, currNode);
  return toReturn;
}

/**
 * recursively prints in-order traversal of the query plan tree
 set<int> universe;
 int label;
 node * leftChild;
 node * rightChild;
 **/
void printQueryPlanTree(node *currNode) {
  if (currNode == NULL) {
    return;
  }
  printQueryPlanTree(currNode->leftChild);
  std::cout << "Label: " << currNode->label << std::endl;
  std::cout << "Universe: ";
  for (const auto & attr: currNode->universe) {
    std::cout << attr << " ";
  }
  std::cout << std::endl;
  printQueryPlanTree(currNode->rightChild);
}

/**
 * Example: r = {2, 4, 6}, totalOrder = {1, 4, 2, 5, 3, 6}
 **/
vector<tuple<int, int> > computeHashKeysPerRelation(relation &r, vector<int> &totalOrder) {
  vector<tuple<int, int> > toReturn;
  vector<int> orderedAttrs;
  for (const auto & elem : totalOrder) {
    if (r.attrs.count(elem)) {
      orderedAttrs.push_back(elem);
    }
  }
  // orderedAttrs = {4, 2, 6}
  // unsigned int first = (1 << orderedAttrs[0]);
  unsigned int first = 0;
  for (int i = 0; i < orderedAttrs.size(); ++i) {
    unsigned int second = 0;
    for (int j = i; j < orderedAttrs.size(); ++j) {
      second |= (1 << orderedAttrs[j]);
      tuple<int, int> key = std::make_tuple(first, second);
      toReturn.push_back(key);
    }
    first |= (1 << orderedAttrs[i]);
  }
  tuple<int, int> key = std::make_tuple(first, 0);
  toReturn.push_back(key);
  return toReturn;
}

int setToBitVector(const set<int> & s) {
  int toReturn = 0;
  for (const auto & attr : s) {
    toReturn |= (1 << attr);
  }
  return toReturn;
}

TUPLE projectTuple(TUPLE tup, int projectionAttrs, const vector<int>& order, const vector<int>& loc) {
  vector<int> t;
  for (int j = 0; j < order.size(); j++) {
    int attr = order[j];
    int bit = (1 << attr);
    if ((bit & projectionAttrs) == bit) {
      t.push_back(tup[loc[attr]]);
    }
  }

  return t;
}

vector<int> orderToLoc(const vector<int>& order, int attrs) {
  vector<int> loc(order.size() + 1, 0);
  for (int i = 0, j = 0; i < order.size(); i++) {
    if (attrs & (1 << order[i])) {
      loc[order[i]] = j++;
    }
  }

  return loc;
}

TUPLE projectTuple(TUPLE tup, int tupleAttrs, int projectionAttrs, const vector<int> & order) {
  vector<int> loc = orderToLoc(order, tupleAttrs);
  return projectTuple(tup, projectionAttrs, order, loc);
}

vector<TUPLE> getOrderedProjection(relation & rel, int projectionAttrs, const vector<int> & order) {
  vector<vector<int> > ret;
  const vector<vector<int> > & tuples = rel.tuples;
  vector<int> loc = orderToLoc(order, setToBitVector(rel.attrs));

  for (int i = 0; i < tuples.size(); i ++) {
    auto tuple = tuples[i];
    ret.push_back(projectTuple(tuple, projectionAttrs, order, loc));
  }
  return ret;
}

TUPLE concatTuples(const TUPLE & t, const TUPLE & u) {
  TUPLE ret(t);
  for (int x : u) {
    ret.push_back(x);
  }
  return ret;
}

int countBit(int k) {
  int ret = 0;
  while(k) {
    k &= (k - 1);
    ret++;
  }
  return ret;
}

void buildHashIndices(vector<tuple<int, int> > & hashKeys, relation & rel,
    const vector<int> & totalOrder) {
  map<tuple<int, int>, int> & location = rel.htIndexes;

  for (const auto & key: hashKeys) {
    int size = location.size();
    location[key] = size;

    int K = std::get<0>(key);
    int A = std::get<1>(key);

    vector<TUPLE> projectionOnKA = getOrderedProjection(rel, K | A, totalOrder);
    vector<TUPLE> projectionOnK = getOrderedProjection(rel, K, totalOrder);

    set<TUPLE> ht1;
    map<vector<int>, vector<TUPLE> > ht2;

    for (const auto & t: projectionOnK) {
      ht1.insert(t);
    }

    for (const auto & u: projectionOnKA) {
      vector<int> t = vector<int>(u.begin(), u.begin() + countBit(K));
      ht2[t].push_back(u);
    }

    rel.ht1.push_back(ht1);
    rel.ht2.push_back(ht2);
  }
}

void printVector(const string & label, const vector<int> & vec) {
  std::cout << label << ": ";
  for (const auto & elem : vec) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;
}

double computeLeftHandSide(const int k, const TUPLE tup, vector<relation> & rels,
    const int sBitVector, const int wBitVector, const int wMinusBitVector, const double y_e_k,
    const vector<double> & fractionalCover, const vector<int> & totalOrder) {
  double sum = 0.0;
  for (int i = 0; i < k - 1; ++i) {
    relation & r = rels[i];
    set<int> eI = r.attrs;
    const int eIBitVector = setToBitVector(eI);
    const int key1 = (sBitVector | wBitVector) & eIBitVector;
    const int key2 = (eIBitVector & wMinusBitVector);
    const tuple<int, int> htIndexKey = std::make_tuple(key1, key2);

    const int ht2Location = r.htIndexes[htIndexKey];
    map<TUPLE, vector<TUPLE> > & tupleMap = r.ht2[ht2Location];
    TUPLE projectedTuple = projectTuple(tup, sBitVector | wBitVector, key1, totalOrder);
    const double numTuples = tupleMap.count(projectedTuple) ? tupleMap[projectedTuple].size() : 0.0;
    sum += (fractionalCover[i] / (1 - y_e_k)) * log(numTuples);
  }
  return sum;
}

double computeRightHandSide(const TUPLE tup, relation & r, const int sBitVector,
    const int wBitVector, const int wMinusBitVector, const vector<int> & totalOrder) {
  set<int> eK = r.attrs;
  const int eKBitVector = setToBitVector(eK);
  const int key1 = (sBitVector & eKBitVector);
  const int key2 = wMinusBitVector;
  const tuple<int, int> htIndexKey = std::make_tuple(key1, key2);

  const int ht2Location = r.htIndexes[htIndexKey];
  map<TUPLE, vector<TUPLE> > & tupleMap = r.ht2[ht2Location];
  TUPLE projectedTuple = projectTuple(tup, sBitVector | wBitVector, key1, totalOrder);
  const double numTuples = tupleMap.count(projectedTuple) ? tupleMap[projectedTuple].size() : 0.0;
  return log(numTuples);
}

vector<TUPLE> recursiveJoin(vector<relation> & rels, node * currNode, vector<double> & fractionalCover,
    const vector<int> & totalOrder, TUPLE parentTuple, set<int> & parentTupleAttrs) {
  // 1: Let U = univ(u), k = label(u)
  set<int> & u = currNode->universe;
  const int k = currNode->label;
  const int sBitVector = setToBitVector(parentTupleAttrs);
  // 2: Ret ← ∅
  // Ret is the returned tuple set
  vector<TUPLE> ret;

  // 3: if u is a leaf node of T then
  // note that U ⊆ ei, ∀i ≤ k
  if (currNode->isLeaf()) {
    // Pseudocode line 4: find smallest relation < k when sectioned on parentTuple
    assert(u.size() == 1);

    const int universeBitVector = setToBitVector(u);

    // find smallest relation
    int smallRelIndex = 0;
    relation &smallest = rels[smallRelIndex];
    const int sAndEiBitVector = setToBitVector(smallest.attrs) & sBitVector;
    TUPLE r0t = projectTuple(parentTuple, sBitVector, sAndEiBitVector, totalOrder);
    tuple<int, int> smallestLocKey = std::make_tuple(sAndEiBitVector, universeBitVector);

    vector<TUPLE> &smallestProjectedTuples = smallest.ht2[smallest.htIndexes[smallestLocKey]][r0t];
    int minSize = smallestProjectedTuples.size();

    for (int j = 1; j < k; j++) {
      relation r = rels[j];
      vector<TUPLE> projectionOnUniverseAttrs = getOrderedProjection(smallest, universeBitVector, totalOrder);
      int rAttrs = setToBitVector(r.attrs) & setToBitVector(parentTupleAttrs);
      tuple<int,int> rLocKey = std::make_tuple(sAndEiBitVector, universeBitVector);
      TUPLE rT = projectTuple(parentTuple, setToBitVector(parentTupleAttrs), rAttrs, totalOrder);

      vector<TUPLE>& projectedTuples = r.ht2[r.htIndexes[rLocKey]][rT];
      const int count = projectedTuples.size();

      if (count < minSize) {
        minSize = count;
        smallRelIndex = j;
        smallest = rels[j];
        smallestProjectedTuples = projectedTuples;
      }
    }

    // Pseudocode lines 6-8
    for (TUPLE tU : smallestProjectedTuples) {
      bool addTuple = true;
      for (int j = 0; j < k; ++j) {
        if (j == smallRelIndex) {
          continue;
        }
        int eJBitVector = setToBitVector(rels[j].attrs);
        tuple<int, int> key = std::make_tuple((sBitVector & eJBitVector) | universeBitVector, 0);
        const int ht1Index = rels[j].htIndexes[key];
        set<TUPLE> & tupleSet = rels[j].ht1[ht1Index];
        TUPLE projectedTuple = projectTuple(tU, sAndEiBitVector | universeBitVector, (sBitVector & eJBitVector) | universeBitVector, totalOrder);
        if (!tupleSet.count(tU)) {
          addTuple = false;
          break;
        }
      }
      if (addTuple) {
        ret.push_back(concatTuples(parentTuple, tU));
      }
    }
    return ret;
  }
  // Pseudocode lines 10-14
  vector<TUPLE> leftChildTuples;
  if (currNode->leftChild == NULL) {
    leftChildTuples.push_back(parentTuple);
  } else {
    vector<double> newFracCover = vector<double>(fractionalCover.begin(), fractionalCover.end() - 1);
    leftChildTuples = recursiveJoin(rels, currNode->leftChild, newFracCover, totalOrder, parentTuple, parentTupleAttrs);
  }

  // Pseudocode lines 15-17
  // const int sBitVector = setToBitVector(parentTupleAttrs);
  set<int> w = difference(u, rels[k - 1].attrs);
  const int wBitVector = setToBitVector(w);
  set<int> wMinus = intersect(u, rels[k - 1].attrs);
  const int wMinusBitVector = setToBitVector(wMinus);
  if (wMinus.size() == 0) {
    return leftChildTuples;
  }

  // Pseudocode lines 18-30
  set<int> eK = rels[k - 1].attrs;
  const int eKBitVector = setToBitVector(eK);
  for (const auto & leftTup : leftChildTuples) {
    const double y_e_k = fractionalCover.back();
    const double lhs = computeLeftHandSide(k, leftTup, rels, sBitVector, wBitVector,
        wMinusBitVector, y_e_k, fractionalCover, totalOrder);

    const double rhs = computeRightHandSide(leftTup, rels[k - 1], sBitVector,
        wBitVector, wMinusBitVector, totalOrder);
    if (y_e_k < 1 && lhs < rhs) {
      vector<double> rightChildFractionalCover;
      for (int i = 0; i < k - 1; ++i) {
        rightChildFractionalCover.push_back(fractionalCover[i] / (1 - y_e_k));
      }
      set<int> sUnionW = set_union(parentTupleAttrs, w);
      vector<TUPLE> rightChildTuples = recursiveJoin(rels, currNode->rightChild, rightChildFractionalCover, totalOrder, leftTup, sUnionW);
      // Pseudocode lines 23-25
      for (const auto & rightTup : rightChildTuples) {
        TUPLE tWMinus = projectTuple(rightTup, sBitVector | wBitVector | wMinusBitVector, wMinusBitVector, totalOrder);
        const int key1 = (sBitVector & eKBitVector) | wMinusBitVector;
        const int key2 = 0;
        const tuple<int, int> htIndexKey = std::make_tuple(key1, key2);
        const int ht1Location = rels[k - 1].htIndexes[htIndexKey];
        set<TUPLE> & tupleSet = rels[k - 1].ht1[ht1Location];
        if (tupleSet.count(tWMinus)) {
          ret.push_back(rightTup);
        }
      }
    } else {
      // Pseudocode lines 27-29
      const int key1 = (sBitVector & eKBitVector);
      const int key2 = wMinusBitVector;
      const tuple<int, int> htIndexKey = std::make_tuple(key1, key2);
      const int ht2Location = rels[k - 1].htIndexes[htIndexKey];
      map<TUPLE, vector<TUPLE> > & tupleMap = rels[k - 1].ht2[ht2Location];
      for (const auto & keyValuePair : tupleMap) {
        TUPLE tup = keyValuePair.first;
        bool addTuple = true;
        for (int i = 0; i < k - 1; ++i) {
          relation & r = rels[i];
          set<int> eI = r.attrs;
          const int eIBitVector = setToBitVector(eI);
          const int eIAndWMinusBitVector = eIBitVector & wMinusBitVector;
          if (eIAndWMinusBitVector == 0) {
            continue;
          }
          const int key1 = eIAndWMinusBitVector | ((sBitVector | wBitVector) & eIBitVector);
          const int key2 = 0;
          const tuple<int, int> htIndexKey = std::make_tuple(key1, key2);
          const int ht1Location = r.htIndexes[htIndexKey];
          set<TUPLE> & tupleSet = r.ht1[ht1Location];
          TUPLE projectedTuple = projectTuple(tup, (sBitVector & eKBitVector) | wMinusBitVector, key1, totalOrder);
          if (!tupleSet.count(projectedTuple)) {
            addTuple = false;
            break;
          }
        }
        if (addTuple) {
          TUPLE t = concatTuples(leftTup, tup);
          ret.push_back(t);
        }
      }
    }
  }
  return ret;
}

void testBuildTree(const set<int> & joinAttributes, vector<relation> & hyperedges) {
  // test buildTree
  node *root = buildTree(joinAttributes, hyperedges, hyperedges.size());
  printQueryPlanTree(root);

  // test computeTotalOrder
  vector<int> totalOrder = computeTotalOrder(root);
  printVector("Total Order", totalOrder);

  // test computeHashKeysPerRelation
  vector<tuple<int, int> > hashKeys = computeHashKeysPerRelation(hyperedges[0], totalOrder);
  for (const auto & elem : hashKeys) {
    int first, second;
    std::tie(first, second) = elem;
    std::cout << "first: " << first << ", ";
    std::cout << "second: " << second << std::endl;
  }

  std::cout << std::endl << "TESTING INDICES" << std::endl << std::endl;
  for (auto & rel : hyperedges) {
    vector<tuple<int, int> > hashKeys = computeHashKeysPerRelation(rel, totalOrder);
    buildHashIndices(hashKeys, rel, totalOrder);
    for (auto const & keyValuePair : rel.htIndexes) {
      tuple<int, int> kAndA = keyValuePair.first;
      int k, a;
      std::tie(k, a) = kAndA;
      std::cout << "K: " << k << ", ";
      std::cout << "A: " << a << std::endl;

      int loc = keyValuePair.second;
      set<vector<int> > tuplesInK = rel.ht1[loc];
      for (auto const & tup : tuplesInK) {
        printVector("ht1", tup);
      }
      map<TUPLE, vector<TUPLE> > kATuplesMap = rel.ht2[loc];
      for (auto const & keyValuePair : kATuplesMap) {
        TUPLE t = keyValuePair.first;
        printVector("t", t);
        vector<TUPLE> tuples = keyValuePair.second;
        std::cout << "maps to: " << std::endl;
        for (const auto & tup : tuples) {
          printVector("ht2", tup);
        }
      }
    }
  }
}

void split(const string & s, char delim, vector<string> & elems) {
  stringstream ss(s);
  string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

vector<string> split(const string & s, char delim) {
  vector<string> elems;
  split(s, delim, elems);
  return elems;
}

int countTriangles(char **argv) {
  set<int> joinAttributes;
  std::ifstream infile(argv[1]);
  vector<string> splits = split(argv[2], ':');
  vector<relation> relations;
  for (const auto & attrString : splits) {
    relation r;
    vector<string> attrs = split(attrString, ',');
    for (const auto & attr : attrs) {
      int attrVal = atoi(attr.c_str());
      joinAttributes.insert(attrVal);
      r.attrs.insert(attrVal);
    }
    relations.push_back(r);
  }

  int a, b;
  while (infile >> a >> b) {
    vector<int> tuple;
    tuple.push_back(a);
    tuple.push_back(b);
    for (auto & r : relations) {
      r.tuples.push_back(tuple);
    }
  }
  for (const auto & r : relations) {
    for (const auto & tuple : r.tuples) {
      for (const auto & val : tuple) {
        std::cout << val << " ";
      }
      std::cout << std::endl;
    }
  }
  testBuildTree(joinAttributes, relations);
  node * const root = buildTree(joinAttributes, relations, relations.size());
  vector<double> fractionalCover = fractionalEdgeCover(relations);
  TUPLE emptyTuple;
  set<int> emptySet;
  vector<int> totalOrder = computeTotalOrder(root);
  vector<TUPLE> results = recursiveJoin(relations, root, fractionalCover,
      totalOrder, emptyTuple, emptySet);
  return results.size();
}

vector<relation> readRelations(char *filnam) {
  FILE *in = fopen(filnam, "r");

  vector<relation> result;

  int nRelations;
  fscanf(in, "%d", &nRelations);

  for (int i = 0; i < nRelations; i++) {
    int nAttrs;
    fscanf(in, "%d", &nAttrs);

    set<int> attrs;
    for (int j = 0; j < nAttrs; j++) {
      int attr;
      fscanf(in, "%d", &attr);
      attrs.insert(attr);
    }

    int nTuples;
    fscanf(in, "%d", &nTuples);

    vector< vector<int> > tuples;

    for (int j = 0; j < nTuples; j++) {
      vector<int> tuple;
      for (int k = 0; k < nAttrs; k++) {
        int v;
        fscanf(in, "%d", &v);
        tuple.push_back(v);
      }
      tuples.push_back(tuple);
    }

    relation rel;
    rel.attrs = attrs;
    rel.tuples = tuples;
    result.push_back(rel);
  }

  return result;
}

// argv[1] = file on disk for the relation
// argv[2] = attributes per relation. See usage for appropriate format
int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Usage ./nprr <infile> <joinAttributes: '1,2:2,3:1,3'>" << std::endl;
    exit(1);
  }
  int count = countTriangles(argv);
  std::cout << "number of triangles: " << count << std::endl;
  return 0;
}
