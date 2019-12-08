#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

#include <memory>
#include <string>
#include <vector>
#include <atomic>

#include "kuku/kuku.h"
#include "bloom_filter.hpp"
#include <libpmemobj.h>
#define DEBUG 1

typedef struct leaf_node leaf_node;
typedef class KukuTable KukuTable;
typedef class bloom_filter bloom_filter;
typedef struct value_node value_node;

POBJ_LAYOUT_BEGIN(skiplist);
POBJ_LAYOUT_TOID(skiplist, leaf_node);
POBJ_LAYOUT_TOID(skiplist, KukuTable);
POBJ_LAYOUT_TOID(skiplist, bloom_filter);
POBJ_LAYOUT_TOID(skiplist, value_node);
POBJ_LAYOUT_END(skiplist);

struct value_node
{
	value_node();
	char* real_value;
};

struct index_node
{
  index_node(int lvl, int min_val, TOID(leaf_node) leaf);
  ~index_node();
  int min;
  TOID(leaf_node) leaf;
  int level;
  std::vector<index_node*> forward; 
  bloom_filter* BF;
};

struct leaf_node
{
	leaf_node(int min_val, TOID(KukuTable) HT);
	~leaf_node();
	int min;
	int cnt;
	TOID(leaf_node) leaf_forward;
	TOID(KukuTable) leaf_HT;	
};

class SkipList {
public:

	SkipList(int max_level);

	// thread-unsafe
	void insert(int key, std::string value);

	// thread-unsafe
	bool erase(int key);

	// thread-unsafe
	bool contains(int key); 

	// thread-safe
	int randomLevel() const;

	// for debug
	void traverse();

	std::vector< std::pair<int, uint64_t> > Query(std::vector<int> key_vector);

	void makeNode(int node_num);
	int findNode(int key);
	index_node* make_indexNode(int lvl, int min_val, TOID(leaf_node) leafnode);
	TOID(leaf_node) make_leafNode(int min);
	bool insertLeaf(TOID(leaf_node) leaf, int key, std::string value);
	bool deleteLeaf(TOID(leaf_node) leaf, int key);
//concurrent operation will be implemented later

	PMEMobjpool *pop;
private:
	int _max_level;

	// current level, not used in concurrent version
	uint8_t _level;

	index_node* index_head;
	TOID(leaf_node) leaf_head;

	index_node* index_tail;
	TOID(leaf_node) leaf_tail;

};

#endif // __SKIPLIST_H__

