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
  vector<index_node*> forward;
  int level;
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

	SkipList(uint8_t max_height);

	// thread-unsafe
	void insert(uint64_t key, const std::string& value);

	// thread-unsafe
	bool erase(uint64_t key);

	// thread-unsafe
	bool contains(uint64_t key); 

	// thread-safe
	int randomLevel() const;

	// for debug
	void traverse();

  uint64_t findNode(uint64_t key);
	index_node* make_indexNode(int lvl, int min_val, leaf_node *leafnode);
	leaf_node* make_leafNode(int min);
	bool insertLeaf(leaf_node* leaf, uint64_t key, const std::string& value);
	bool deleteLeaf(leaf_node* leaf, uint64_t key);
//concurrent operation will be implemented later

private:
	const uint8_t _max_level;

	// current level, not used in concurrent version
	uint8_t _level;

	index_node* index_head;
  leaf_node* leaf_head;

	index_node* index_tail;
  leaf_node* leaf_tail;
};

#endif // __SKIPLIST_H__

