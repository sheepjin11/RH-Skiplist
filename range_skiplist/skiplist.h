#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

#include <memory>
#include <string>
#include <vector>
#include <atomic>

#include "kuku/kuku.h"
#include "bloom_filter.hpp"
#define DEBUG 1

using namespace kuku;
using namespace std;
struct index_node
{
  index_node(int lvl, int min_val, struct leaf_node *leaf);
  ~index_node();
  int min;
  struct leaf_node *leaf;
  int level;
  vector<index_node*> forward; 
};

struct leaf_node
{
	leaf_node(int min_val, KukuTable *HT);
	~leaf_node();
	int min;
	leaf_node* leaf_forward;
	bloom_filter* BF;
  KukuTable* leaf_HT;	
};

class SkipList {
public:

	SkipList(int max_level);

	// thread-unsafe
	void insert(int key, const std::string& value);

	// thread-unsafe
	bool erase(int key);

	// thread-unsafe
	bool contains(int key); 

	// thread-safe
	int randomLevel() const;

	// for debug
	void traverse();

  std::vector< pair<int, uint64_t> > Query(std::vector<int> key_vector);
  void makeNode(int node_num);
  int findNode(int key);
	index_node* make_indexNode(int lvl, int min_val, leaf_node *leafnode);
	leaf_node* make_leafNode(int min);
	bool insertLeaf(leaf_node* leaf, int key, const std::string& value);
	bool deleteLeaf(leaf_node* leaf, int key);
//concurrent operation will be implemented later

private:
	int _max_level;

	// current level, not used in concurrent version
	uint8_t _level;

	index_node* index_head;
  leaf_node* leaf_head;

	index_node* index_tail;
  leaf_node* leaf_tail;
};

#endif // __SKIPLIST_H__

