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
		while (next->min < key && next->forward[i]->min > key) {
			x = next.get();
		}
		update[i] = x;
	}
	x = x->forward[1].get();
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
			int new_min = (x->min+x->forward[i]->min)/2;
			leaf_node* new_leaf = new leaf_node(new_min, NewHT);
			auto p = SkipList::make_indexnode(new_min, new_leaf);	
			std::shared_ptr<index_node> sp = std::move(p);
			for (size_t i = 1; i <= lvl; ++i) {
				sp->forward[i] = update[i]->forward[i];
				update[i]->forward[i] = sp;
			}
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

