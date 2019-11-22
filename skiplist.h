#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

#include <mutex>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <atomic>

#include <kuku/kuku.h>
#include "bloom_filter.hpp"

#define DEBUG 1

using namespace std;
struct index_node
{
  index_node(int min_val, struct leaf_node *leaf);
  ~index_node();
  int min;
  struct leaf_node *leaf;
  vector<shared_ptr<index_node>> forward;
};

struct leaf_node
{
	leaf_node(int min, KuKutable* leaf_HT);
	~leaf_node();
	int min;
	shared_ptr<leaf_node> leaf_forward;
	bloom_filter* BF;
	//kuku hash default
	int log_table_size = 8;
    size_t stash_size = 2;
    size_t loc_func_count = 4;
    item_type loc_func_seed = make_random_item();
    uint64_t max_probe = 100;
    item_type empty_item = make_item(0, 0);
	
	KukuTable leaf_HT(
		log_table_size,
		stash_size,
		loc_func_count,
		loc_func_seed,
				max_probe,
				empty_item
			);
	
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
	uint8_t randomLevel() const;

	// for debug
	void traverse();

	static std::unique_ptr<index_node> make_indexNode(uint8_t lvl, int min_val, struct leaf_node *leafnode);

	static std::unique_ptr<leaf_Node> make_leafNode(int min);

//concurrent operation will be implemented later

private:
	const uint8_t _max_level;

	// current level, not used in concurrent version
	uint8_t _level;

	std::shared_ptr<index_node> index_head;
  shared_ptr<leaf_node> leaf_head;
};

#endif // __SKIPLIST_H__

