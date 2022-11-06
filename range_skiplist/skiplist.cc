//for debugging, traverse() can be used
#include <iostream>
#include <cstdlib>
#include <list>
#include <sstream>

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

#include "hashmap_internal.h"
#include <inttypes.h>
#define SEED 10
#define MAX_INT 21474836
#define THRESHOLD 1000 

// code ref: https://github.com/pmem/pmdk/blob/master/src/examples/libpmemobj/hashmap/hashmap_atomic.c

/* layout definition */
TOID_DECLARE(struct buckets, HASHMAP_ATOMIC_TYPE_OFFSET + 1);
TOID_DECLARE(struct entry, HASHMAP_ATOMIC_TYPE_OFFSET + 2);

struct entry {
	uint64_t key;
	PMEMoid value;

	/* list pointer */
	POBJ_LIST_ENTRY(struct entry) list;
};

struct entry_args {
	uint64_t key;
	PMEMoid value;
};

POBJ_LIST_HEAD(entries_head, struct entry);
struct buckets {
	/* number of buckets */
	size_t nbuckets;
	/* array of lists */
	struct entries_head bucket[];
};

struct hashmap_atomic {
	/* random number generator seed */
	uint32_t seed;

	/* hash function coefficients */
	uint32_t hash_fun_a;
	uint32_t hash_fun_b;
	uint64_t hash_fun_p;

	/* number of values inserted */
	uint64_t count;
	/* whether "count" should be updated */
	uint32_t count_dirty;

	/* buckets */
	TOID(struct buckets) buckets;
	/* buckets, used during rehashing, null otherwise */
	TOID(struct buckets) buckets_tmp;
};

/*
 * create_entry -- entry initializer
 */
static int
create_entry(PMEMobjpool *pop, void *ptr, void *arg)
{
	struct entry *e = (struct entry *)ptr;
	struct entry_args *args = (struct entry_args *)arg;

	e->key = args->key;
	e->value = args->value;

	memset(&e->list, 0, sizeof(e->list));

	pmemobj_persist(pop, e, sizeof(*e));

	return 0;
}

/*
 * create_buckets -- buckets initializer
 */
static int
create_buckets(PMEMobjpool *pop, void *ptr, void *arg)
{
	struct buckets *b = (struct buckets *)ptr;

	b->nbuckets = *((size_t *)arg);
	pmemobj_memset_persist(pop, &b->bucket, 0,
			b->nbuckets * sizeof(b->bucket[0]));
	pmemobj_persist(pop, &b->nbuckets, sizeof(b->nbuckets));

	return 0;
}

/*
 * create_hashmap -- hashmap initializer
 */
static void
create_hashmap(PMEMobjpool *pop, TOID(struct hashmap_atomic) hashmap,
		uint32_t seed)
{
	D_RW(hashmap)->seed = seed;
	do {
		D_RW(hashmap)->hash_fun_a = (uint32_t)rand();
	} while (D_RW(hashmap)->hash_fun_a == 0);
	D_RW(hashmap)->hash_fun_b = (uint32_t)rand();
	D_RW(hashmap)->hash_fun_p = HASH_FUNC_COEFF_P;

	size_t len = INIT_BUCKETS_NUM;
	size_t sz = sizeof(struct buckets) +
			len * sizeof(struct entries_head);

	pmemobj_persist(pop, D_RW(hashmap), sizeof(*D_RW(hashmap)));

	if (POBJ_ALLOC(pop, &D_RW(hashmap)->buckets, struct buckets, sz,
			create_buckets, &len)) {
		fprintf(stderr, "root alloc failed: %s\n", pmemobj_errormsg());
		abort();
	}
}

/*
 * hash -- the simplest hashing function,
 * see https://en.wikipedia.org/wiki/Universal_hashing#Hashing_integers
 */
static uint64_t
hash(const TOID(struct hashmap_atomic) *hashmap,
		const TOID(struct buckets) *buckets,
	uint64_t value)
{
	uint32_t a = D_RO(*hashmap)->hash_fun_a;
	uint32_t b = D_RO(*hashmap)->hash_fun_b;
	uint64_t p = D_RO(*hashmap)->hash_fun_p;
	size_t len = D_RO(*buckets)->nbuckets;

	return ((a * value + b) % p) % len;
}

index_node::index_node(int lvl, int min_val, TOID(leaf_node) leafnode)
	:min(min_val),
  	leaf(leafnode),
	level(lvl) {};

leaf_node::leaf_node(int min_val, TOID(struct hashmap_atomic) HT)
	:min(min_val),
	leaf_HT(HT) {};

value_node::value_node()
	:real_value("") {};

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
	TOID(leaf_node) leafnode;
	int ret = POBJ_ALLOC(this->pop, &leafnode, leaf_node, sizeof(leaf_node), NULL, NULL);
	D_RW(leafnode)->min = min_val;
	D_RW(leafnode)->cnt=0;
	ret = POBJ_ALLOC(this->pop, &(D_RW(leafnode)->leaf_HT), struct hashmap_atomic, sizeof(struct hashmap_atomic), NULL, NULL);
	create_hashmap(this->pop, D_RW(leafnode)->leaf_HT, SEED);	
	return leafnode;
}

bool SkipList::insertLeaf(TOID(leaf_node) leaf, int key, char* value)
{
	if(D_RW(leaf)->cnt > THRESHOLD)
	{
		return false;
	}
	uint64_t cast_key = static_cast<uint64_t>(key);
	
	TOID(value_node) persist_value;
	POBJ_ALLOC(this->pop, &persist_value, value_node, sizeof(value_node), NULL, NULL);
	D_RW(persist_value)->real_value = value;
	if (OID_EQUALS(persist_value.oid, OID_NULL)) {
		fprintf(stdout, "value is null\n");
	}
	int result = hm_atomic_insert(this->pop, D_RW(leaf)->leaf_HT, cast_key, persist_value.oid);
	if (result == 1) { // new insert
		D_RW(leaf)->cnt++;
		return true; 
	}
	else if (result == 0) { // update
		return true;
	}
	else { // fail
		fprintf(stdout, "insert fail\n");
		return false;
	}
}

SkipList::SkipList(int max_level)
	:_max_level(max_level),
	 _level(1) {
	char* path = "/mnt/pmem/skiplist_file";
	size_t pool_size = 5<<30;
	if (access(path, F_OK) == -1) {
		if ((this->pop = pmemobj_create(path, "skiplist",
			pool_size, 0666)) == NULL) {
			perror("failed to create pool\n");
			exit(0);
		}
		fprintf(stdout, "create pool %lu\n", pool_size);
	} else {
		fprintf(stdout, "This version requests to remove existing file\n");
		exit(1);
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

int SkipList::randomLevel() const {
	static thread_local std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(0,1);
	int lvl = 1;
	while(distribution(generator) && lvl < _max_level) {
		++lvl;
	}
	return lvl;
}

char* SkipList::findNode(int key) { // return value address
	index_node* prev = index_head;
	bool found = false;
	int val_addr;
	index_node* curr;
	for (int i = _max_level-1; i >= 0; i--) {
		curr = prev;
		while (curr->min != MAX_INT && curr->forward[i] != NULL) {
			if(curr->forward[i]->min == MAX_INT || curr->forward[i]->min > key)
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
		TOID(leaf_node) curr_leaf = curr->leaf;
		uint64_t cast_key = static_cast<uint64_t>(key);
		PMEMoid result = hm_atomic_get(this->pop, D_RW(curr_leaf)->leaf_HT, cast_key);	
		if (OID_EQUALS(result, OID_NULL)) {
			TOID(struct entry) var;
			TOID(struct buckets) buckets = D_RO(D_RO(curr_leaf)->leaf_HT)->buckets;
			uint64_t h = hash(&D_RO(curr_leaf)->leaf_HT, &buckets, key);
			POBJ_LIST_FOREACH(var, &D_RO(buckets)->bucket[h], list)
				if (D_RO(var)->key == key)
					fprintf(stdout, "func is malfunc\n");
			for (size_t i = 0; i < D_RO(buckets)->nbuckets; ++i)
				POBJ_LIST_FOREACH(var, &D_RO(buckets)->bucket[i], list) {
					if (D_RO(var)->key == key) {
						fprintf(stdout, "actually, this hashmap has the key.. %d, %d\n", i, h);
					}
				}
			return NULL; // fail to search
		}
		else {
			return static_cast<char*>(pmemobj_direct(result));
		}
	}
}

// void SkipList::MigrateNewNode(TOID(leaf_node) before_node, TOID(leaf_node) new_node) {
void SkipList::MigrateNewNode(void* before_node_ptr, void* new_node_ptr) {
	TOID(leaf_node) before_node = pmemobj_oid(before_node_ptr);
	TOID(leaf_node) new_node = pmemobj_oid(new_node_ptr);
	TOID(struct buckets) buckets = D_RO(D_RO(before_node)->leaf_HT)->buckets;
	TOID(struct entry) var;
	int new_min = D_RO(new_node)->min;
	int ret = 0;
	for (size_t i = 0; i < D_RO(buckets)->nbuckets; ++i)
		POBJ_LIST_FOREACH(var, &D_RO(buckets)->bucket[i], list) {
			if (D_RO(var)->key > new_min) { // migration needed
				// insert new node
				int result = hm_atomic_insert(this->pop, D_RW(new_node)->leaf_HT, D_RO(var)->key, D_RO(var)->value);
				if (result == -1) {
					fprintf(stderr, "during migration, insert failed\n");
					exit(1);
				}
				// remove item from original node
				PMEMoid remove_result = hm_atomic_remove(this->pop, D_RO(before_node)->leaf_HT, D_RO(var)->key);
				if (OID_EQUALS(remove_result, OID_NULL)) {
					fprintf(stdout, "during migration, remove failed\n");
					exit(1);
				}
				// update cnt	
				D_RW(before_node)->cnt--;
				D_RW(new_node)->cnt++;
			}
		}
}

void SkipList::insert(int key, char* value) {
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
  
	if (_head || !insertLeaf(update[0]->leaf,key,const_cast<char*>(value))) 
  { 
    index_node* x = update[0];
		int new_min = (x->min+x->forward[0]->min)/2;
	 
		std::cout << "split_new key: " << D_RW(x->forward[0]->leaf)->min << std::endl;
		std::cout << "split_before key: " << D_RW(x->leaf)->min << std::endl;
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

		TOID(leaf_node) before;
		before = x->leaf; 
		MigrateNewNode(pmemobj_direct(before.oid), pmemobj_direct(new_leaf.oid));
	}
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

int main()
{
	SkipList* _skiplist = new SkipList(8);
  _skiplist->makeNode(10);

  _skiplist->traverse(); 
 for(int i=1;i<200;i++)
  {
    _skiplist->insert(i, "a");
  }
	size_t search_num = 200;
	size_t search_success = 1;
  for(int i=1;i<search_num;i++)
  {
		char* ret = _skiplist->findNode(i);
		if (ret == NULL) {
			fprintf(stdout, "find failed\n");
		}
		else {
			search_success++;
		}
  }
	if (search_num == search_success) {
		fprintf(stdout, "Every key is searched\n");
	}
	pmemobj_close(_skiplist->pop);
}
