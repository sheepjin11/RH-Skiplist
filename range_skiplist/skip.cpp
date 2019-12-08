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
#include <libpmemobj.h>
#include <unistd.h>

#define MAX_INT 21474836
#define THRESHOLD 1000 
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
  new_node->leaf = leafnode;
  new_node->forward = *(new std::vector<index_node*>(lvl+1));
  for(int i=0;i<lvl+1;i++)
    new_node->forward[i] = NULL;
	return new_node; 
}

TOID(leaf_node) SkipList::make_leafNode(int min_val)
{
	int log_table_size = 8;
	table_size_type stash_size = 0;
	size_t loc_func_count = 4;
	item_type loc_func_seed = make_random_item();
	uint64_t max_probe = 100;
	item_type empty_item = make_item(0, 0);
	
	TOID(KukuTable) newHT;
	POBJ_ALLOC(this->pop, &newHT, KukuTable, sizeof(KukuTable), NULL, NULL);

	D_RW(newHT)->log_table_size_ = log_table_size;
	D_RW(newHT)->stash_size_ = stash_size;
	D_RW(newHT)->loc_func_seed_ = loc_func_seed;
	D_RW(newHT)->max_probe_ = max_probe;
	D_RW(newHT)->empty_item_ = empty_item;
	D_RW(newHT)->generate_loc_funcs(loc_func_count, loc_func_seed);
	
	TOID(leaf_node) leafnode;
	POBJ_ALLOC(this->pop, &leafnode, leaf_node, sizeof(leaf_node), NULL, NULL);
	D_RW(leafnode)->min = min_val;
	D_RW(leafnode)->leaf_HT = newHT;
	D_RW(leafnode)->cnt=0;

	return leafnode;
}

bool SkipList::insertLeaf(TOID(leaf_node) leaf, int key, const std::string& value)
{
	uint64_t val_addr = 2; // need to modify
	//if (!D_RW(D_RW(leaf)->leaf_HT)->insert(make_item((uint64_t)key,val_addr))) // if insert fails, return false. need to split.
	
	if(D_RW(leaf)->cnt > THRESHOLD)
	{
		return false;
	}
	D_RW(D_RW(leaf)->leaf_HT)->insert(make_item((uint64_t)key,val_addr));
	D_RW(leaf)->cnt++;
	return true; // insert success.

}

bool SkipList::deleteLeaf(TOID(leaf_node) leaf, int key)
{
  //hash delete

	D_RW(leaf)->cnt--;
	if ( !D_RW(D_RW(leaf)->leaf_HT)->Delete(key)) // key does not exist
		return false;
	else
		return true;
}

SkipList::SkipList(int max_level)
	:_max_level(max_level),
	 _level(1) {
	char* path = "/mnt/pmem/skiplist_file";
	if (access( path, F_OK) == -1) {
		if ((this->pop = pmemobj_create(path, POBJ_LAYOUT_NAME(skiplist),
			1024*1024*1024, 0666)) == NULL) {
			perror("failed to create pool\n");
			exit(0);
		}
	} else {
		if ((this->pop = pmemobj_open(path,
				POBJ_LAYOUT_NAME(skiplist))) == NULL) {
			perror("failed to open pool\n");
			exit(0);
		}
	}

	// key,value for headnode is meanless
	leaf_head = SkipList::make_leafNode(-1); // head points NULL leaf node and its min value is -1
	index_head = SkipList::make_indexNode(max_level,-1,leaf_head); // head points NULL leaf node and its min value is -1
	leaf_tail = SkipList::make_leafNode(MAX_INT); // head points NULL leaf node and its min value is -1
	index_tail = SkipList::make_indexNode(max_level,MAX_INT,leaf_tail); // head points NULL leaf node and its min value is -1
  for (int i = 0; i < max_level; i++) {   
		index_head->forward[i] = index_tail; 
	} 
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
//			if(D_RW(D_RW(curr_leaf)->BF)->contains(key))
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
        if(x->forward[i]->min==MAX_INT || x->forward[i]->min > key)
          break;
        else
          x = x->forward[i];
      }
      update[i] = x;
	} 
		std::cout << "min is  " << update[0]->min << "and key is " << key << std::endl;
  
  if (_head || !insertLeaf(update[0]->leaf,key,value)) 
  { 
    index_node* x = update[0];
	int new_min = (x->min+x->forward[0]->min)/2; // comment : x가 아니라 before->min 아닌가?!
 
	std::cout << "split_new key: " << D_RW(x->forward[0]->leaf)->min << std::endl;
	std::cout << "split_before key: " << D_RW(x->leaf)->min << std::endl;
//	if(D_RW(x->forward[0]->leaf)->min == D_RW(x->leaf)->min)
//		new_min += key;

	TOID(leaf_node) new_leaf;
    new_leaf = make_leafNode(new_min);
	D_RW(new_leaf)->cnt++; //may be modified
    index_node* new_index = make_indexNode(lvl, new_min, new_leaf);
    for(int i=0;i<=lvl;i++)
    {
      new_index->forward[i] = update[i]->forward[i];
      update[i]->forward[i] = new_index;
    }
	D_RW(new_leaf)->min = new_min;
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
						std::cout << key << key << " row: " << row << std::endl;
						//if (!D_RW(D_RW(new_leaf)->leaf_HT)->insert(pair))
						if (D_RW(new_leaf)->cnt > THRESHOLD)
						{
							cout << "insert fail during split, key : " << pair[0] << endl; 
							item_type re_insert =D_RW(D_RW(new_leaf)->leaf_HT)->last_insert_fail_item();
							insert(re_insert[0], to_string(re_insert[1])); // value & value address must be separated
						}
						D_RW(D_RW(before)->leaf_HT)->Delete(pair[0]);
						D_RW(new_leaf)->cnt++;
						D_RW(before)->cnt--;
					}
						
				}
			}
		}
	  
	  	item_type re_insert =D_RW(D_RW(before)->leaf_HT)->last_insert_fail_item();
		insert(re_insert[0], to_string(re_insert[1])); // value & value address must be separated
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
//  index_node* iter_node = this->index_head;
  TOID(leaf_node) iter_node = this->leaf_head;
  int num =0;
  while(1)
  {
    std::cout << num << "th node min value is " << D_RW(iter_node)->min << std::endl;
    num++;
	iter_node = D_RW(iter_node)->leaf_forward;
    if(D_RW(iter_node)->min==MAX_INT)
      break; 
  }
  std::cout << "number of node is " << num << std::endl;
}
void fillseq()
{

}

void fillrandom()
{

}

void findseq()
{

}

void findrandom()
{

}


int main()
{
	SkipList* _skiplist = new SkipList(8);
  _skiplist->makeNode(10);

  _skiplist->traverse(); 
 for(int i=1;i<20000000;i++)
  {
	std::cout << "insert: " << i << std::endl;
    _skiplist->insert(i,"a");
  }
  for(int i=1;i<200;i++)
  {
	std::cout << "find:: " << i << std::endl;
    _skiplist->findNode(i);
  }
	_skiplist->traverse();
	pmemobj_close(_skiplist->pop);
}
