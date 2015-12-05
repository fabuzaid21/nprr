#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <glpk.h>
#include <cmath>


using std::map;
using std::max;
using std::min;
using std::multimap;
using std::multiset;
using std::set;
using std::string;
using std::vector;
using std::tuple;

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
};


struct relation {
    set<int> attrs;
    vector<vector<int> > tuples;
    map< tuple<int,int>,int  > location;
    vector< set<vector<int> > > ht1;
    vector< map< vector<int>, vector<vector<int>> > > ht2; // a bunch of hashtables, each one maps a key (a tuple, vector of int) to a vector of tuples (vector of vectors of int)
};

vector<double> fractionalEdgeCover(const vector<relation> &hyperedges){

    set<int> V;
    for(const auto& e: hyperedges){
        for(int attr : e.attrs){
            V.insert(attr);
        }
    }

    int rowNum = V.size();
    int colNum = hyperedges.size();


    glp_prob *lp = glp_create_prob();
    glp_set_prob_name(lp, "edgecover");
    glp_set_obj_dir(lp, GLP_MIN);
    glp_add_rows(lp, rowNum);

    for(int i = 1; i <= rowNum; i ++){
        glp_set_row_bnds(lp, i, GLP_LO, 1.0, 0.0);
    }

    glp_add_cols(lp, colNum);


    for(int i = 1 ; i <= colNum; i++){
        glp_set_col_bnds(lp, i, GLP_LO, 0.0, 0.0);
    }


    std::vector<int> ia(1), ja(1);
    std::vector<double> cof(1);

    for(int i = 1 ;i <= colNum; i ++){
        for(int j : hyperedges[i - 1].attrs){
            ia.push_back(i); // row
            ja.push_back(j); // col
            cof.push_back(1.0); // 1.0 * X_e_j
        }
    }

    for(int i = 1; i<= colNum; i ++){
        glp_set_obj_coef(lp, i, log(hyperedges[i - 1].tuples.size() * 1.0));
    }

    glp_load_matrix(lp, ia.size() - 1, &ia[0], &ja[0], &cof[0]);

    glp_smcp param;
    glp_init_smcp(&param);
    param.msg_lev = GLP_MSG_OFF;

    glp_simplex(lp, &param);


    vector<double> edgecover;

    for(int i = 1 ;i <= colNum; i ++){
        double x = glp_get_col_prim(lp, i);
        edgecover.push_back(x);

    }

    double opt = glp_get_obj_val(lp);

    glp_delete_prob(lp);

    return edgecover;

}

vector<relation> readRelations(char *infile);

node *buildTree(set<int> joinAttributes, const vector<relation> &hyperedges, int k) {
    bool empty = true;
    int i = 0;
    for (const auto &r: hyperedges) {
        if (i == k) {
            break;
        }
        std::set<int> s_intersection;
        std::set_intersection(r.attrs.begin(), r.attrs.end(),
                              joinAttributes.begin(), joinAttributes.end(),
                              std::inserter(s_intersection, s_intersection.begin()));
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
                std::set<int> s_1 = joinAttributes;
                std::set<int> s_2;

                for (auto attr: e_k) {
                    if (s_1.count(attr)) {
                        s_1.erase(s_1.find(attr));
                    }
                }
                std::set_intersection(e_k.begin(), e_k.end(),
                                      joinAttributes.begin(), joinAttributes.end(),
                                      std::inserter(s_2, s_2.begin()));
                currNode->leftChild = buildTree(s_1, hyperedges, k - 1);
                currNode->rightChild = buildTree(s_2, hyperedges, k - 1);
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
    for (const auto &attr: currNode->universe) {
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
    for (const auto &elem:totalOrder) {
        if (r.attrs.count(elem)) {
            orderedAttrs.push_back(elem);
        }
    }
    // orderedAttrs = {4, 2, 6}
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


vector< vector<int> > getProject(relation& rel, int projectionAttrs, const vector<int>& totalOrder){
    vector< vector<int> > ret;
    const vector<vector<int> >& tuples = rel.tuples;
    vector<int> loc(totalOrder.size() + 1, 0);
    for(int i = 0, j = 0 ;i < totalOrder.size(); i ++){
        if(rel.attrs.count(totalOrder[i])){
            loc[ totalOrder[i] ] = j ++;
        }
    }

    for(int i = 0; i< tuples.size(); i ++){
        auto tuple = tuples[i];
        vector<int> t;
        for(int j = 0; j < totalOrder.size(); j ++){
            int attr = totalOrder[j];
            int bit = 1 << attr;
            if( (bit &  projectionAttrs) == bit){
                t.push_back( tuple[ loc[attr] ]);
            }
        }
        ret.push_back(t);
    }
    return ret;
}

int countbit(int k){
    int ret = 0;
    while(k){
       k &= (k-1);
       ret ++;
    }
    return ret;
}

void buildHashIndices(vector<tuple<int, int> > &hashKeys, relation& rel,  const vector<int>& totalOrder){

    map< tuple<int,int>,int  >& location = rel.location;
    const vector<vector<int> >& tuples = rel.tuples;

    for(const auto& key: hashKeys){
        int size = location.size();
        location[ key ] = size;

        int K = std::get<0>(key);
        int A = std::get<1>(key);

        vector<vector<int> > projectionOnKA = getProject(rel, K | A, totalOrder);
        vector<vector<int> > projectionOnK = getProject(rel, K, totalOrder);

        set<vector<int> > ht1;
        map< vector<int>, vector<vector<int> > > ht2;

        for(const auto& t: projectionOnK){
            ht1.insert(t);
        }

        for(const auto& u: projectionOnKA){
            vector<int> t = vector<int>(u.begin(), u.begin() + countbit(K));
            ht2[t].push_back(u);
        }

        rel.ht1.push_back(ht1);
        rel.ht2.push_back(ht2);
    }
}

void testBuildTree(char *infile) {
    // setup
    set<int> joinAttributes = {1, 2, 3, 4, 5, 6};
    vector<relation> hyperedges = readRelations(infile);
    const int k = 5;

    // test buildTree
    node *root = buildTree(joinAttributes, hyperedges, k);
    printQueryPlanTree(root);

    // test computeTotalOrder
    vector<int> totalOrder = computeTotalOrder(root);
    std::cout << "Total Order: ";
    for (const auto &elem:totalOrder) {
        std::cout << elem << " ";
    }
    std::cout << std::endl;

    // test computeHashKeysPerRelation
    vector<tuple<int, int> > hashKeys = computeHashKeysPerRelation(hyperedges[3], totalOrder);
    for (const auto &elem:hashKeys) {
        int first, second;
        std::tie(first, second) = elem;
        std::cout << "first: " << first << ", ";
        std::cout << "second: " << second << std::endl;
    }

    for (auto &rel:hyperedges) {
        vector<tuple<int, int> > hashKeys = computeHashKeysPerRelation(rel, totalOrder);
        buildHashIndices(hashKeys, rel, totalOrder);
    }
}

// vector<int> GenericJoin() {
// }

int countTriangles(char **argv) {
    //(set<int> joinAttributes, const vector<relation> & hyperedges, int k)
    // read file(s) from disk into memory
    // build indexes (i.e. hashmaps on each relation)
    // compute generic join
    // count results
    return 1;
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

int main(int argc, char **argv) {
    testBuildTree(argv[1]);
    int count = countTriangles(argv);
    std::cout << "number of triangles: " << count << std::endl;
    return 0;
}
