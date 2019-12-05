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
#include <cstddef>
#include <string>
#include <vector>
using namespace std;
using namespace kuku;



index_node::index_node(int lvl, int min_val, struct leaf_node* leafnode)
	:min(min_val),
	leaf(leafnode),
	level(lvl){		

	}

leaf_node::leaf_node(int min_val, KukuTable *HT)
	:min(min_val),
	leaf_HT(HT){	
	}

index_node* SkipList::make_indexNode(int lvl, int min_val,leaf_node *leafnode)
{ 
  index_node* new_node =  new index_node(lvl, min_val, leafnode);
  new_node->forward = *(new vector<index_node*>(lvl));
	return new_node; 
}

leaf_node* SkipList::make_leafNode(int min_val)
{
	//kuku hash default
	int log_table_size = 8;
	size_t stash_size = 0;
	size_t loc_func_count = 4;
	item_type loc_func_seed = make_random_item();
	uint64_t max_probe = 100;
	item_type empty_item = make_item(0, 0);
	KukuTable* newHT = new KukuTable(log_table_size,stash_size, loc_func_count, loc_func_seed,	max_probe, empty_item);
	leaf_node* leafnode = new leaf_node(min_val, newHT);
//	bloom_filter* leafBF = new bloom_filter;
	leafnode->BF = new bloom_filter();// size can be modified 

	return leafnode;
}
bool SkipList::insertLeaf(leaf_node* leaf, uint64_t key, const std::string& value)
{
	//uint64_t val_addr = 0; // need to modify
	uint64_t val_addr = 2; // need to modify
	if (!leaf->leaf_HT->insert(make_item(key,val_addr))) // if insert fails, return false. need to split.
  {
		return false;
  }
	leaf->BF->insert(to_string(key));
	return true; // insert success.

}

bool SkipList::deleteLeaf(leaf_node* leaf, uint64_t key)
{
  //hash delete

	// item_type pair = leaf->leaf_HT->table(index);
	// if (pair[0] || pair[1]) // if key exists
	// {
	// 	if (pair[0] >= new_leaf->min) // have to migrate 
	// 	{
	// 		new_leaf->leaf_HT->insert(pair);
	// 		before->leaf_HT->table(index) = make_item(0,0);
	// 	}
						
	// }
  //BF cannot delete element 
  return true;
}

SkipList::SkipList(uint8_t max_level)
	:_max_level(max_level),
	 _level(1) {
	// key,value for headnode is meanless
	leaf_head = SkipList::make_leafNode(-1); // head points NULL leaf node and its min value is -1
	index_head = SkipList::make_indexNode(max_level,-1,leaf_head); // head points NULL leaf node and its min value is -1
	leaf_tail = SkipList::make_leafNode(-1); // head points NULL leaf node and its min value is -1
	index_tail = SkipList::make_indexNode(max_level,-1,leaf_tail); // head points NULL leaf node and its min value is -1
  for (int i = 0; i < max_level; i++) {   
    vector<index_node*>::iterator it;
		index_head->forward[i] = index_tail;   
		leaf_head->leaf_forward = leaf_tail; 
	}
}
//do not modify anything
int SkipList::randomLevel() const {
	static thread_local std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(0,1);
	uint8_t lvl = 1;
	while(distribution(generator) && lvl < _max_level) {
		++lvl;
	}
	return lvl;
}

//in LEVELDB, FindGreaterOrEqual
uint64_t SkipList::findNode(uint64_t key, std::vector<index_node>* preds,
	std::vector<index_node>* succs, uint8_t* layer) { // return value address
	index_node* prev = index_head;
	bool found = false;
	uint64_t val_addr;
	assert(preds->size() >= _max_level+1);
	assert(succs->size() >= _max_level+1);
	for (size_t i = _max_level; i >= 1; --i) {
		index_node* curr = prev->forward[i];
		index_node* next = curr->forward[i];
		while (next->min < key) {
			prev = curr;
			curr = curr->forward[i];
		}
		if (!found && curr->min <= key && next->min > key) {
			//search in leaf node
			leaf_node* curr_leaf = curr->leaf;
			if(curr_leaf->BF->contains(key))
			{
				
				if(curr_leaf->leaf_HT->query(make_item(key,0)))  
				{
					found=true;
					val_addr = curr_leaf->leaf_HT->get(key);
					*layer = i;
          break;
				}
			}
		}
/*
		(*preds)[i] = prev;
		(*succs)[i] = curr;
*/
	}
	if (!found)
		return 0;
	return val_addr;
}

//leaf Get implementation

void SkipList::insert(uint64_t key, const std::string& value) {
	std::vector<index_node*> update(_max_level);
  for(size_t i=0;i<_max_level;i++)
  {
    update[i] = this->index_head;
  }
	index_node* x = index_head;
  bool _head = false;
	for (size_t i = _max_level-1; i >= 0; --i) {
		index_node* next = x->forward[i];
    if(next->min == -1) // 1st access or reach to tail
    {
      _head=true;
      break;
    }
		while (next->forward[i]->min < key) {
			x = next;
		}
		update[i] = x;
	}

		uint8_t lvl = randomLevel();
		if (lvl > _level) {
			for (size_t i = _level+1; i <= lvl; i++) {
				update[i] = index_head;//maybe should be modified
		}
			_level = lvl;
		}
    else{}

  if(_head && update[0]->forward[0]->min == -1)
  {
    uint8_t lvl =randomLevel();
    leaf_node* x_leaf = make_leafNode(key);
    x=make_indexNode(lvl, key, x_leaf);
    for(int i=0;i<=lvl;i++)
    {
      x->forward[i] = update[i]->forward[i];
      update[i]->forward[i] = x;
    }
  }

//	if (!insertLeaf(x->leaf,key,value)) 
	else if (!insertLeaf(update[0]->forward[0]->leaf,key,value)) 
  { // if insert is fail, need to split leaf node
		//받아야 할 인자 : hash table
		leaf_node* before = x->leaf;
		leaf_node* next_leaf = x->forward[0]->leaf; // level 0 일 때의 leaf
		int new_min = (x->min+next_leaf->min)/2; // comment : x가 아니라 before->min 아닌가?!
		leaf_node* new_leaf = make_leafNode(new_min);
		//기존 hash table에서 new hash table로 key,value 이동 필요 

		size_t col_count = 8; // log_table_size
		for (size_t row = 0; row < 8; row++) // log_table_size
		{
			for (size_t col = 0; col < col_count; col++)
			{
				size_t index = row * col_count + col;
				item_type pair = before->leaf_HT->table(index);
				if (pair[0] || pair[1]) // if key exists
				{
					if (pair[0] >= new_leaf->min) // have to migrate 
					{
						new_leaf->leaf_HT->insert(pair);
						before->leaf_HT->table(index) = make_item(0,0);
					}
						
				}
			}

		}

		// for (size_t i = 0; i < table.stash().size(); i++)
		// {
		// 	cout << i << ": " << table.stash(i) << endl;
		// }

		
    	leaf_node* new_leaf_pointer = new_leaf;
		auto p = make_indexNode(randomLevel(), new_min, new_leaf_pointer);	
		index_node* sp = p;
		for (size_t i = 1; i <= lvl; ++i) {
			sp->forward[i] = update[i]->forward[i];
			update[i]->forward[i] = sp;
		} // for index node 
  		leaf_node* before_leaf_forward(p->leaf);
    	leaf_node* next_leaf_forward(next_leaf);
		before->leaf_forward = before_leaf_forward;
		p->leaf->leaf_forward = next_leaf_forward;
	}
  else
  {
  }
}
//heejin must implement leaf node
bool SkipList::erase(uint64_t key) {
	std::vector<index_node*> update(_max_level+1);
	index_node* x= index_head;
	for(size_t i = _level; i >= 1; --i) {
		index_node* next = x->forward[i];
		while (next->min < key && next->forward[i]->min > key) {
			x = next;
		}
		update[i] = x;
	}
	if(deleteLeaf(x->leaf, key)==false)
	{
		cout << "this key is not existing" << endl;
	}
	
	return true;
}

void SkipList::traverse()
{
  index_node* iter_node = this->index_head;
  int num =0;
  while(1)
  {
    cout << num << "th node min value is " << iter_node->min << endl;
    num++;
    iter_node = iter_node->forward[0];
    if(iter_node->min==-1)
      break; 
  }
  cout << "number of node is " << num << endl;
}

int main()
{
	cout << "skiplist !! " << endl;
  SkipList* _skiplist = new SkipList(8);
  for(uint64_t i=0;i<200;i++)
  {
    _skiplist->insert(i,"a");
  }

  _skiplist->traverse();  
  cout << "inserted " << endl;
}
