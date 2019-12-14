#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

#include <memory>
#include <string>
#include <vector>
#include <atomic>

#include <libpmemobj.h>
#include "../pmdk-1.7/src/examples/libpmemobj/hashmap/hashmap_rp.h"
#define DEBUG 1
#ifdef __cplusplus
extern "C" {
#endif

typedef struct leaf_node leaf_node;
typedef struct hashmap_rp hashmap_rp;

POBJ_LAYOUT_BEGIN(skiplist);
POBJ_LAYOUT_END(skiplist);

struct index_node
{
  index_node(int lvl, int min_val, leaf_node* leafnode);
  int min;
  leaf_node* leafnode;
  int level;
  std::vector<index_node*> forward; 
};

struct leaf_node
{
	int min;
	int cnt;
	leaf_node* leaf_forward;
	TOID(struct hashmap_rp) leaf_HT;	
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

	std::vector< std::pair<int, uint64_t> > Query(std::vector<int> key_vector);

	void makeNode(int node_num);
	int findNode(int key);
	index_node* make_indexNode(int lvl, int min_val, leaf_node* leafnode);
	leaf_node* make_leafNode(int min);
	bool insertLeaf(leaf_node* leaf, int key, const std::string& value);
	bool deleteLeaf(leaf_node* leaf, int key);
	PMEMobjpool *pop;

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

#ifdef __cplusplus
}
#endif
