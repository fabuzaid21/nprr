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
    vector<tuple<int, int> > hashKeys = computeHashKeysPerRelation(rel,
        totalOrder);
    buildHashIndices(hashKeys, rel, totalOrder);
    for (auto const & keyValuePair : rel.htIndexes) {
      tuple<int, int> kAndA = keyValuePair.first;
      int k, a;
      std::tie(k, a) = kAndA;
      std::cout << "K: " << k << ", ";
      std::cout << "A: " << a <<
        std::endl;

      int loc =
        keyValuePair.second;
      set<vector<int> >
        tuplesInK =
        rel.ht1[loc];
      for (auto
          const &
          tup :
          tuplesInK)
      {
        printVector("ht1",
            tup);
      }
      map<TUPLE,
        vector<TUPLE>
          >
          kATuplesMap
          =
          rel.ht2[loc];
      for
        (auto
         const
         &
         keyValuePair
         :
         kATuplesMap)
        {
          TUPLE
            t
            =
            keyValuePair.first;
          printVector("t",
              t);
          vector<TUPLE>
            tuples
            =
            keyValuePair.second;
          std::cout
            <<
            "maps
            to:
            "
            <<
            std::endl;
          for
            (const
             auto
             &
             tup
             :
             tuples)
            {
              printVector("ht2",
                  tup);
            }
        }
    }
  }
}
