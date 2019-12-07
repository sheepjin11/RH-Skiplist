//for debugging, traverse() can be used
#include <iostream>
#include <cstdlib>
#include <list>
#include <assert.h>
#include <random>
#include <chrono>
#include "skiplist.h"
#include <iomanip>
#include <cstddef>
#include <string>
#include <vector>
#include <limits.h>

#define MAX_INT 2147483640

typedef struct leaf_node leaf_node;

index_node::index_node(int lvl, int min_val, TOID(leaf_node) leafnode)
	:min(min_val),
  	leaf(leafnode),
	level(lvl){		
	}

leaf_node::leaf_node(int min_val, TOID(KukuTable) HT)
	:min(min_val),
	leaf_HT(HT){	
	}

index_node* SkipList::make_indexNode(int lvl, int min_val, TOID(leaf_node) leafnode)
{ 
  index_node* new_node =  new index_node(lvl, min_val, leafnode);
  new_node->forward = *(new std::vector<index_node*>(lvl+1));
  for(int i=0;i<lvl+1;i++)
    new_node->forward[i] = NULL;
	return new_node; 
}

TOID(leaf_node) SkipList::make_leafNode(int min_val)
{
	//kuku hash default
	int log_table_size = 8;
	int stash_size = 0;
	int loc_func_count = 4;
	item_type loc_func_seed = make_random_item();
	int max_probe = 100;
	item_type empty_item = make_item(0, 0);
	//KukuTable* newHT = new KukuTable(log_table_size,stash_size, loc_func_count, loc_func_seed,	max_probe, empty_item);
  //pmem
  TOID(KukuTable) newHT;
  /*
  D_RW(newHT)->log_table_size = log_table_size;
  D_RW(newHT)->stash_size = stash_size;
  D_RW(newHT)->log_func_count = log_func_count;
  D_RW(newHT)->log_func_seed = log_func_seed;
  D_RW(newHT)->max_probe = max_probe;
  D_RW(newHT)->emtpy_item = empty_item;
  */
  D_RW(newHT)->setParameters(log_table_size, stash_size, loc_func_seed, max_probe, empty_item);
  POBJ_ALLOC(pop, &newHT, KukuTable, sizeof(KukuTable), NULL, NULL);
  TOID(bloom_filter) newBF;
  POBJ_ALLOC(pop, &newBF, bloom_filter, sizeof(bloom_filter), NULL, NULL);

  TOID(leaf_node) leafnode;
  POBJ_ALLOC(pop, &leafnode, leaf_node, sizeof(leaf_node), NULL, NULL);
  D_RW(leafnode)->min = min_val;
  D_RW(leafnode)->leaf_HT = newHT;
  D_RW(leafnode)->BF = newBF;

	return leafnode;
}

bool SkipList::insertLeaf(TOID(leaf_node) leaf, int key, const std::string& value)
{
	//int val_addr = 0; // need to modify
	uint64_t val_addr = 2; // need to modify
	if (D_RW(D_RW(leaf)->leaf_HT)->insert(make_item(key,val_addr))) // if insert fails, return false. need to split.
  {
		return false;
  }
	D_RW(D_RW(leaf)->BF)->insert(std::to_string(key));
	return true; // insert success.

}

bool SkipList::deleteLeaf(TOID(leaf_node) leaf, int key)
{
  //hash delete

	if ( !D_RW(D_RW(leaf)->leaf_HT)->Delete(key)) // key does not exist
		return false;
	else
		return true;
}

SkipList::SkipList(int max_level)
	:_max_level(max_level),
	 _level(1) {
	char* path = "skiplist_file";
	pop = pmemobj_create(path, POBJ_LAYOUT_NAME(skiplist), PMEMOBJ_MIN_POOL, 0666);
	if(pop==NULL)
	{
		perror("failed to created pool!\n");
		exit(0);	
	}
	// key,value for headnode is meanless
	leaf_head = SkipList::make_leafNode(-1); // head points NULL leaf node and its min value is -1
	index_head = SkipList::make_indexNode(max_level,-1,leaf_head); // head points NULL leaf node and its min value is -1
	leaf_tail = SkipList::make_leafNode(-1); // head points NULL leaf node and its min value is -1
	index_tail = SkipList::make_indexNode(max_level,MAX_INT,leaf_tail); // head points NULL leaf node and its min value is -1
  for (int i = 0; i < max_level; i++) {   
		index_head->forward[i] = index_tail; 
	} 
    	POBJ_ALLOC(pop, &leaf_head, leaf_node, sizeof(leaf_node), NULL, NULL);
    	POBJ_ALLOC(pop, &leaf_tail, leaf_node, sizeof(leaf_node), NULL, NULL); 
	D_RW(leaf_head)->leaf_forward = leaf_tail; 
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
int SkipList::findNode(int key) { // return value address
	index_node* prev = index_head;
	bool found = false;
	int val_addr;
  	index_node* curr;
	for (int i = _max_level-1; i >= 0; i--) {
		curr = prev;
		while (curr->min!=MAX_INT && curr->forward[i]!=NULL) {
      			if(curr->forward[i]->min==MAX_INT || curr->forward[i]->min > key)
					break;
			else
			{
				prev = curr;
				curr = curr->forward[i];
			}
		}
	}
		if (!found && curr->min <= key) {
			//search in leaf node
      			TOID(leaf_node) curr_leaf;
			curr_leaf = curr->leaf;
			if(D_RW(D_RW(curr_leaf)->BF)->contains(key))
			{
				
				if(D_RW(D_RW(curr_leaf)->leaf_HT)->query(make_item(key,0)))  
				{
					found=true;
					val_addr = D_RW(D_RW(curr_leaf)->leaf_HT)->get(key);
					//*layer = i;
				}
			}
		}
	if (!found)
		return 0;
	return val_addr;
}

//leaf Get implementation

void SkipList::insert(int key, const std::string& value) {

  bool _head = false;
  int lvl = randomLevel();  
  index_node* update[lvl];
  for (int i = lvl; i >-1; i--) {
	  index_node* x = this->index_head;
      	  while(x->min < key && x->forward[i] != NULL)
      {
        if(x->forward[i]->min==MAX_INT)
          break;
        else
          x = x->forward[i];
      }
      update[i] = x;
	}  
  if (_head || !insertLeaf(update[0]->leaf,key,value)) 
  {   
    index_node* x = update[0];
	int new_min = (x->min+x->forward[0]->min)/2; // comment : x가 아니라 before->min 아닌가?!
    TOID(leaf_node) new_leaf;
    new_leaf = make_leafNode(new_min);
    index_node* new_index = make_indexNode(lvl, new_min, new_leaf);
    for(int i=0;i<=lvl;i++)
    {
      new_index->forward[i] = update[i]->forward[i];
      update[i]->forward[i] = new_index;
    }
    D_RW(new_leaf)->leaf_forward = D_RW(x->leaf)->leaf_forward;
    D_RW(x->leaf)->leaf_forward = new_leaf; 

    int col_count = 8;
    TOID(leaf_node) before;
    before = x->leaf; 
    for (int row = 0; row <  D_RW(D_RW(before)->leaf_HT)->table_size() / col_count; row++) 
		{
			for (int col = 0; col < col_count; col++)
			{
				int index = row * col_count + col;
				item_type pair = D_RW(D_RW(before)->leaf_HT)->table(index);
				if (pair[0] || pair[1]) // if key exists
				{
					if (pair[0] >= D_RW(new_leaf)->min) // have to migrate 
					{
						D_RW(D_RW(new_leaf)->leaf_HT)->insert(pair);
						D_RW(D_RW(before)->leaf_HT)->Delete(pair[0]);
					}
						
				}
			}
		}
  	POBJ_ALLOC(pop, &new_leaf, leaf_node, sizeof(leaf_node), NULL, NULL);
	}
  else
  {
  }
}
//heejin must re-implement leaf node
bool SkipList::erase(int key) {
	std::vector<index_node*> update(_max_level+1);
	index_node* x= index_head;
	for(int i = _max_level; i >= 1; --i) {
		index_node* next = x->forward[i];
		while (next->min < key && next->forward[i]->min > key) {
			x = next;
		}
		update[i] = x;
	}
	if(deleteLeaf(x->leaf, key)==false)
	{
		std::cout << "this key is not existing" << std::endl;
	}
	
	return true;
}
void SkipList::makeNode(int node_num)
{
  for(int node_iter = 0; node_iter<node_num; node_iter++)
  {
    int lvl = randomLevel();
    int new_min = MAX_INT/(node_num)*(node_iter);
    TOID(leaf_node) new_leaf;
    new_leaf = make_leafNode(new_min);
    index_node* new_index = make_indexNode(lvl, new_min, new_leaf);
    index_node* update[this->_max_level];

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
    for(int i=0;i<lvl+1;i++)
    {
      new_index->forward[i] = update[i]->forward[i];
      update[i]->forward[i] = new_index;
    }
    D_RW(new_leaf)->leaf_forward = D_RW(update[0]->leaf)->leaf_forward;
    D_RW(update[0]->leaf)->leaf_forward = new_leaf;
    
  }
}
void SkipList::traverse()
{
  index_node* iter_node = this->index_head;
  int num =0;
  while(1)
  {
    std::cout << num << "th node min value is " << iter_node->min << std::endl;
    num++;
    iter_node = iter_node->forward[0];
    if(iter_node==this->index_tail)
      break; 
  }
  std::cout << "number of node is " << num << std::endl;
}

int main()
{
  SkipList* _skiplist = new SkipList(8);
  _skiplist->makeNode(10);

  _skiplist->traverse(); 
 for(int i=1;i<200000000;i++)
  {
    _skiplist->insert(i,"a");
  }
/*  for(int i=1;i<200;i++)
  {
    _skiplist->findNode(i);
  }
*/
  std::cout << "inserted " << std::endl;
//	pmemobj_close(_skiplist->pop);
}
