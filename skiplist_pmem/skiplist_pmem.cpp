//for debugging, traverse() can be used
#include <iostream>
#include <cstdlib>
#include <list>
#include <assert.h>
#include <random>
#include <chrono>
#include <iomanip>
#include <cstddef>
#include <string>
#include <vector>
#include <limits.h>
#include <libpmemobj.h>
#include <unistd.h>
#include "../pmdk-1.7/src/examples/libpmemobj/hashmap/hashmap_rp.h"

#define MAX_INT 21474836
#define THRESHOLD 1000 

#define POOL_SIZE 85899345920
#include "skiplist_pmem.h"
 index_node::index_node(int lvl, int min_val, leaf_node* leafnode)
	:min(min_val),
  	leafnode(leafnode),
	level(lvl){		
	}
/*
leaf_node::leaf_node(int min_val)
	:min(min_val){	
	}
*/
index_node* SkipList::make_indexNode(int lvl, int min_val, leaf_node* leafnode)
{ 
  index_node* new_node =  new index_node(lvl, min_val, leafnode);
  new_node->leafnode = leafnode;
  new_node->forward = *(new std::vector<index_node*>(lvl+1));
  for(int i=0;i<lvl+1;i++)
    new_node->forward[i] = NULL;
	return new_node; 
}

leaf_node* SkipList::make_leafNode(int min_val)
{	
	struct leaf_node* leafnode = new leaf_node;	
	leafnode->min = min_val;
	leafnode->cnt=0;
	leafnode->leaf_forward=NULL;
	TOID(struct hashmap_rp) map;
	hm_rp_create(this->pop, &map, NULL);
	hm_rp_init(this->pop, map);
	leafnode->leaf_HT = map;
	pmemobj_persist(this->pop, leafnode, sizeof(leafnode));

	return leafnode;
}

bool SkipList::insertLeaf(leaf_node* leaf, int key, const std::string& value)
{
	uint64_t val_addr = 2; // need to modify
	/*
	 * - 0 if successful,
	 * - 1 if value already existed
	 * - -1 on error
	 */
	if (hm_rp_insert(this->pop,(leaf->leaf_HT), key, val_addr)==-1) // if insert fails, return false. need to split.
	{
		return false;
	}
//	D_RW(leaf)->cnt++;
	return true; // insert success.

}
/*
bool SkipList::deleteLeaf(TOID(leaf_node) leaf, int key)
{
  //hash delete

	D_RW(leaf)->cnt--;
	if ( !D_RW(D_RW(leaf)->leaf_HT)->Delete(key)) // key does not exist
		return false;
	else
		return true;
}
*/
SkipList::SkipList(int max_level)
	:_max_level(max_level),
	 _level(1) {
	char* path = "/mnt/pmem/skiplist_file";
	if (access( path, F_OK) == -1) {
		if ((this->pop = pmemobj_create(path, POBJ_LAYOUT_NAME(skiplist),
			POOL_SIZE, 0666)) == NULL) {
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

			if(hm_rp_get(this->pop, curr_leaf->leaf_HT, key)==0)
			{
				printf("there is no value for key : %d\n", key); 
				return 0;
			}
			else
				return hm_rp_get(this->pop, curr_leaf->leaf_HT, key);
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
  
  if (_head || !insertLeaf(update[0]->leafnode,key,value)) 
  {

    index_node* x = update[0];
	int new_min = (x->min+x->forward[0]->min)/2; // comment : x가 아니라 before->min 아닌가?!
 
	leaf_node* new_leaf;
    new_leaf = make_leafNode(new_min);
    
	index_node* new_index = make_indexNode(lvl, new_min, new_leaf);
    for(int i=0;i<=lvl;i++)
   	{
      	new_index->forward[i] = update[i]->forward[i];
      	update[i]->forward[i] = new_index;
   	}
	new_leaf->min = new_min;
    new_leaf->leaf_forward = x->leafnode->leaf_forward;
    x->leafnode->leaf_forward = new_leaf; 
	leaf_node* before;
    before = x->leafnode;
	split_map(this->pop,before->leaf_HT, new_leaf->leaf_HT, new_min);
	if(key>=new_min)
		insertLeaf(new_leaf, key, value);
	else
		insertLeaf(before, key, value);
	}
  else
  {
  }
}
/*
std::vector< std::pair<int, uint64_t> > SkipList::Query(std::vector<int> key_vector) {
  std::vector<int> min_vector;
  TOID(leaf_node) iter_node = this->leaf_head;
	std::cout << "query in 1" << std::endl;
  while(1)
  {
    iter_node = D_RW(iter_node)->leaf_forward;
    if(D_RW(iter_node)->min==MAX_INT)
      break;
    else  
    {
      min_vector.push_back(D_RW(iter_node)->min);
    }   
	p
  }
	std::cout << "query in 2" << std::endl;
  std::vector< std::vector<int> > result(key_vector.size());
  for(int i=0;i<key_vector.size();i++)
  {
    result[i].resize(min_vector.size());
  }
  for(int i=0;i<key_vector.size();i++)
  {
    for(int j=0;j<min_vector.size();j++)
    {
        if(key_vector[i] <= min_vector[j])
        {
          result[j].push_back(key_vector[i]);
          break;
        }
    }
  }	std::cout << "query in 3" << std::endl;
  std::vector< std::pair<int,uint64_t> > val_addr_vector(key_vector.size());

  for(int i=0;i<min_vector.size();i++)
  {
    TOID(leaf_node) curr_leaf = this->leaf_head;
    for(int z=0;z<i;z++)
    {
      curr_leaf = D_RW(curr_leaf)->leaf_forward;
    }
    for(int j=0;j<result[i].size();j++)
    {
      int key = result[i][j];
      if(key!=0)
      {
				if(D_RW(D_RW(curr_leaf)->leaf_HT)->query(make_item(key,0)))  
				{
					uint64_t val_addr = D_RW(D_RW(curr_leaf)->leaf_HT)->get(key);
					std::pair<int, uint64_t> return_value = std::make_pair(key, val_addr);
					val_addr_vector.push_back(return_value);
				}
      }
    }
  }
	return val_addr_vector;

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
*/
void SkipList::makeNode(int node_num)
{
  for(int node_iter = 0; node_iter<node_num; node_iter++)
  {
    int lvl = randomLevel();
    int new_min = MAX_INT/(node_num)*(node_iter);
    //TOID(leaf_node) new_leaf;
	leaf_node* new_leaf;
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
    new_leaf->leaf_forward = update[0]->leafnode->leaf_forward;
    update[0]->leafnode->leaf_forward = new_leaf;
  }
}
void SkipList::traverse()
{
//  index_node* iter_node = this->index_head;
  leaf_node* iter_node = this->leaf_head;
  int num =0;
  while(1)
  {
    num++;
	iter_node = iter_node->leaf_forward;
    if(iter_node->min==MAX_INT)
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
 for(int i=1;i<300000;i++)
  {
    _skiplist->insert(i,"a");
  }
	std::cout << "after traverse!!" << std::endl;
  _skiplist->traverse(); 

  for(int i=1;i<200;i++)
  {
	std::cout << "find:: " << i << std::endl;
    _skiplist->findNode(i);
  }
  /* 
  std::vector<int> query_ ;
  for(int i=1;i<200;i++)
  {
    query_.push_back(i);
  }
  std::vector< std::pair<int,uint64_t> > result = _skiplist->Query(query_);
  std::cout << "query finished" <<  std::endl;
  for(int i=0;i<result.size();i++)
  {
	  std::cout << result[i].first << "result is " << result[i].second << std::endl;
  }*/
	pmemobj_close(_skiplist->pop);
}


