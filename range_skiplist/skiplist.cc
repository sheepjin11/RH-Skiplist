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
#include <limits.h>

using namespace std;
using namespace kuku;

#define MAX_INT 2147483640

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
	int stash_size = 0;
	int loc_func_count = 4;
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

	if ( !leaf->leaf_HT->Delete(key)) // key does not exist
		return false;

  //BF cannot delete element 
  return true;
}

SkipList::SkipList(int max_level)
	:_max_level(max_level),
	 _level(1) {
	// key,value for headnode is meanless
	leaf_head = SkipList::make_leafNode(-1); // head points NULL leaf node and its min value is -1
	index_head = SkipList::make_indexNode(max_level,-1,leaf_head); // head points NULL leaf node and its min value is -1
	leaf_tail = SkipList::make_leafNode(-1); // head points NULL leaf node and its min value is -1
	index_tail = SkipList::make_indexNode(max_level,MAX_INT,leaf_tail); // head points NULL leaf node and its min value is -1
  for (int i = 0; i < max_level; i++) {   
		index_head->forward[i] = index_tail; 
	}  
		leaf_head->leaf_forward = leaf_tail; 
}
//do not modify anything
int SkipList::randomLevel() const {
	static thread_local std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(0,1);
	int lvl = 1;
	while(distribution(generator) && lvl < _max_level) {
		++lvl;
	}
	return lvl;
}

//in LEVELDB, FindGreaterOrEqual
uint64_t SkipList::findNode(uint64_t key) { // return value address
	index_node* prev = index_head;
	bool found = false;
	uint64_t val_addr;
  index_node* curr;
	for (int i = _max_level-1; i >= 0; i--) {
		curr = prev->forward[i];
		while (curr->min!=-1 && curr->min < key) {
      prev = curr;
			curr = curr->forward[i];
		}
	}
		if (!found && curr->min <= key) {
			//search in leaf node
			leaf_node* curr_leaf = curr->leaf;
			if(curr_leaf->BF->contains(key))
			{
				
				if(curr_leaf->leaf_HT->query(make_item(key,0)))  
				{
					found=true;
					val_addr = curr_leaf->leaf_HT->get(key);
					//*layer = i;
				}
			}
		}
	if (!found)
		return 0;
	return val_addr;
}

//leaf Get implementation

void SkipList::insert(uint64_t key, const std::string& value) {

  bool _head = false;
	int lvl = randomLevel();  
  index_node* update[lvl];
	for (int i = lvl; i >-1; i--) {
	  index_node* x = this->index_head;
/*
    if(x->forward[i]->min == MAX_INT)
    {
      update[i]=x;
    }
    else    
    {
      while(x->min < key)
      {
        if(x->forward[i]->min==MAX_INT)
          break;
        else
          x = x->forward[i];
      }
      update[i] = x;
    }
*/
      while(x->min < key)
      {
        if(x->forward[i]->min==MAX_INT)
          break;
        else
          x = x->forward[i];
      }
      update[i] = x;

	}  
  if (_head || !insertLeaf(update[0]->forward[0]->leaf,key,value)) 
  {   
    cout << "hit : " << key << endl;
    index_node* x = update[0]->forward[0];
		int new_min = (x->min+x->forward[0]->min)/2; // comment : x가 아니라 before->min 아닌가?!
    leaf_node* new_leaf = make_leafNode(new_min);
    index_node* new_index = make_indexNode(lvl, new_min, new_leaf);
    for(int i=0;i<=lvl;i++)
    {
      new_index->forward[i] = update[i]->forward[i];
      update[i]->forward[i] = new_index;
    }
    new_leaf->leaf_forward = x->leaf->leaf_forward;
    x->leaf->leaf_forward = new_leaf; 


		int col_count = 8;
    leaf_node* before = x->leaf; 
		for (int row = 0; row <  before->leaf_HT->table_size() / col_count; row++) 
		{
			for (int col = 0; col < col_count; col++)
			{
				int index = row * col_count + col;
				item_type pair = before->leaf_HT->table(index);
				if (pair[0] || pair[1]) // if key exists
				{
					if (pair[0] >= new_leaf->min) // have to migrate 
					{
						new_leaf->leaf_HT->insert(pair);
						before->leaf_HT->Delete(pair[0]);
					}
						
				}
			}
		}
	}
  else
  {
  }
}
//heejin must implement leaf node
bool SkipList::erase(uint64_t key) {
	std::vector<index_node*> update(_max_level+1);
	index_node* x= index_head;
	for(int i = _level; i >= 1; --i) {
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
void SkipList::makeNode(int node_num)
{
  for(int node_iter = 0; node_iter<node_num; node_iter++)
  {
    int lvl = randomLevel();
    int new_min = MAX_INT/(node_num)*(node_iter);
    leaf_node* new_leaf = make_leafNode(new_min);
    index_node* new_index = make_indexNode(lvl, new_min, new_leaf);
    index_node* update[this->_max_level];
    //for debugging
/*    for(int i=0;i<this->_max_level;i++)
    {
      cout << i << " of head is " << this->index_head->forward[i]->min << endl;
    }
*/
    for(int i=lvl;i>-1;i--)
    {
      index_node* index_iter = this->index_head;
      if(index_iter->forward[i]->min == MAX_INT)
      {
        update[i]=index_iter;
      }
      else
      {
        while(index_iter->min < new_min)
        {
          if(index_iter->forward[i]->min==MAX_INT)
            break;
          else
            index_iter = index_iter->forward[i];
        }
        update[i] = index_iter;
      } 
    }
    for(int i=0;i<lvl;i++)
    {
      new_index->forward[i] = update[i]->forward[i];
      update[i]->forward[i] = new_index;
    }
    new_leaf->leaf_forward = update[0]->leaf->leaf_forward;
    update[0]->leaf->leaf_forward = new_leaf;
    
  }
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
    if(iter_node==this->index_tail)
      break; 
  }
  cout << "number of node is " << num << endl;
}

int main()
{
  SkipList* _skiplist = new SkipList(8);
  _skiplist->makeNode(10);

  _skiplist->traverse(); 
 for(uint64_t i=1;i<20000;i++)
  {
    _skiplist->insert(i,"a");
  }
/*  for(uint64_t i=1;i<200;i++)
  {
    _skiplist->findNode(i);
  }
*/
  cout << "inserted " << endl;
}
