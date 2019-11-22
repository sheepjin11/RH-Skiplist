//for debugging, traverse() can be used
#include <iostream>
#include <cstdlib>
#include <list>
#include <assert.h>
#include <random>
#include <chrono>
#include "skiplist.h"
#include <iostream>
#include <iomanip>

using namespace std;
using namespace kuku;



index_node::index_node(int min_val, struct leaf_node* leafnode)
	:min(min_val),
	leaf(leafnode){		

	}

leaf_node::leaf_node(int min_val, KuKutable *targetHT)
	:min(min_val),
	leaf_HT(targetHT){	
	}

unique_ptr<index_node> make_indexNode(int min_val, struct leaf_node *leafnode)
{
	return unique_ptr<indeX_node>(new index_node(min_val, leafnode))
}

unique_ptr<leaf_node> make_leafNode(int min_val)
{
	//kuku hash default
	int log_table_size = 8;
    size_t stash_size = 2;
    size_t loc_func_count = 4;
    item_type loc_func_seed = make_random_item();
    uint64_t max_probe = 100;
    item_type empty_item = make_item(0, 0);
	KuKutable* newHT = new KuKutable(log_table_size,stash_size, loc_func_count, loc_func_seed,	max_probe, empty_item);
	leaf_node* leafnode = new leaf_node(min_val, newHT);
	bloom_filter* leafBF = new bloom_filter;
	leafnode->BF = leafBF();
	return leafnode;
}
SkipList::SkipList(uint8_t max_level)
	:_max_level(max_level),
	 _level(1) {
	// key,value for headnode is meanless
	_head = std::move(SkipList::makeNode(-1, null)); // head points NULL leaf node and its min value is -1
	for (size_t i = 0; i <= max_level; i++) {
		_head->forward[i] = SkipList::makeNode(std::numeric_limits<uint64_t>::max(), null);
	}
}
//do not modify anything
uint8_t SkipList::randomLevel() const {
	static thread_local std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(0,1);
	uint8_t lvl = 1;
	while(distribution(generator) && lvl < _max_level) {
		++lvl;
	}
	return lvl;
}


bool SkipList::findNode(uint64_t key, std::vector<std::shared_ptr<index_node>>* preds,
		std::vector<std::shared_ptr<index_node>>* succs, uint8_t* layer) {
	std::shared_ptr<index_node> prev = _head;
	bool found = false;
	assert(preds->size() >= _max_level+1);
	assert(succs->size() >= _max_level+1);
	for (size_t i = _max_level; i >= 1; --i) {
		std::shared_ptr<index_node> curr = prev->forward[i];
		shared_ptr<index_node> next = curr->forward[i];
		while (next->min < key) {
			prev = curr;
			curr = curr->forward[i];
		}
		if (!found && curr->key <= key && next->min > key) {
			//search in leaf node
			leaf_node* curr_leaf = curr->leaf;
			if(curr_leaf->BF.contains(key))
			{
				if(curr_leaf->target_HT.search(key))  // yang needs to modify
				{
					found=true;
					*layer = i;
				}
			}
		}
		(*preds)[i] = prev;
		(*succs)[i] = curr;
	}
	return found;
}

//leaf Get implementation

void SkipList::insert(uint64_t key, const std::string& value) {
	std::vector<index_node*> update(_max_level+1);
	index_node *x = _head.get();
	for (size_t i = _level; i >= 1; --i) {
		shared_ptr<index_node> next = x->forward[i];
		while (next->forward[i]->min < key) {
			x = next.get();
		}
		update[i] = x;
	}
	//x = x->forward[1].get();
	if (x->leaf->Get(key) == true) { // need for hash search 
		//x->value = value;
		// implementation value update;
	} else { //not existing item
		uint8_t lvl = randomLevel();
		if (lvl > _level) {
			for (size_t i = _level+1; i <= lvl; i++) {
				update[i] = _head.get();
			}
			_level = lvl;
		}
		if(checkSplitNode(x->leaf)==true)
		{
			//node split;
			//받아야 할 인자 : hash table
			leaf* before = x->leaf;
			leaf* next = x->forward[0]->leaf; // level 0 일 때의 leaf
			int new_min = (x->min+x->forward[i]->min)/2;
			leaf_node* new_leaf = make_leafNode(new_min);
			auto p = make_indexnode(new_min, new_leaf);	
			std::shared_ptr<index_node> sp = std::move(p);
			for (size_t i = 1; i <= lvl; ++i) {
				sp->forward[i] = update[i]->forward[i];
				update[i]->forward[i] = sp;
			} // for index node
			before->leaf_forward = p->leaf;
			p->leaf->leaf_forward = next;
			
		}
		else
		{
			insertLeaf(x->leaf, key);
			//implement insert to hash 
		}
		
	}
}

bool SkipList::erase(uint64_t key) {
	std::vector<index_node*> update(_max_level+1);
	index_node *x = _head.get();
	for(size_t i = _level; i >= 1; --i) {
		shared_ptr<index_node> next = x->forward[i];
		while (next->min < key && next->forward[i]->min > key) {
			x = next.get();
		}
		update[i] = x;
	}
	if(deleteLeaf(x->leaf, key)==false)
	{
		cout << "this key is not existing" << endl;
	}
	
	return true;
}

