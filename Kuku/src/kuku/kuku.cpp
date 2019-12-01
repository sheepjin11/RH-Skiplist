// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include "kuku/kuku.h"
#include "iostream"
#include <assert.h>
using namespace std;

namespace kuku
{
    QueryResult KukuTable::query(item_type item) const
    {
		item_type index_item = make_item(item[0], 0);
        if (is_empty_item(item))
        {
            throw invalid_argument("cannot query the empty item");
        }

        // Search the hash table
        auto lfc = loc_func_count();
        for (size_t i = 0; i < lfc; i++)
        {
            auto loc = location(index_item, i);
			//std::cout << "index_item : (" << index_item[0] << ", " << index_item[1] << "), item : (" << item[0] << ", " << item[1] << ")"<< std::endl;
            if (are_equal_item(table_[loc], item))
            {
                return { loc, i };
            }
        }

        // Search the stash
        for (location_type loc = 0; loc < stash_.size(); loc++)
        {
            if (are_equal_item(stash_[loc], item))
            {
                return { loc, ~size_t(0) };
            }
        }

        // Not found
        return { 0, max_loc_func_count };
    }

    KukuTable::KukuTable(
        int log_table_size, table_size_type stash_size,
        size_t loc_func_count, item_type loc_func_seed,
        uint64_t max_probe, item_type empty_item) :
        log_table_size_(log_table_size),
        stash_size_(stash_size),
        loc_func_seed_(loc_func_seed),
        max_probe_(max_probe),
        empty_item_(empty_item),
        last_insert_fail_item_(empty_item_)
    {
        if (!loc_func_count || loc_func_count > max_loc_func_count)
        {
            throw invalid_argument("invalid loc_func_count");
        }
        if (log_table_size_ < 1 || log_table_size_ > max_log_table_size)
        {
            throw invalid_argument("invalid log_table_size");
        }
        if (!max_probe)
        {
            throw invalid_argument("max_probe cannot be zero");
        }

        // Allocate the hash table
        table_.resize(table_size(), empty_item_);

        // Create the location (hash) functions
        generate_loc_funcs(loc_func_count, loc_func_seed_);
    }

    set<location_type> KukuTable::all_locations(item_type item) const
    {
        set<location_type> result;
        for (auto lf : loc_funcs_)
        {
            result.emplace(lf(item));
        }
        return result;
    }

    void KukuTable::clear_table()
    {
        size_t sz = table_.size();
        table_.resize(0);
        table_.resize(sz, empty_item_);
        stash_.clear();
        last_insert_fail_item_ = empty_item_;
    }

    void KukuTable::generate_loc_funcs(size_t loc_func_count, item_type seed)
    {
        loc_funcs_.clear();
        while (loc_func_count--)
        {
            loc_funcs_.emplace_back(log_table_size_, seed);
            increment_item(seed);
        }
    }

    bool KukuTable::insert(item_type item, uint64_t level)
    {
		item_type index_item = make_item(item[0], 0);
		//std::cout << "index_item : (" << index_item[0] << ", " << index_item[1] << "), item : (" << item[0] << ", " << item[1] << ")"<< std::endl;
        if (is_empty_item(item))
        {
            throw invalid_argument("cannot insert the null item");
        }
        if (level >= max_probe_)
        {
            if (stash_.size() < stash_size_)
            {
                stash_.push_back(item);
                inserted_items_++;
                return true;
            }
            else
            {
                last_insert_fail_item_ = item;
                return false;
            }
        }

        // Choose random location index
        size_t loc_index = size_t(rd_()) % loc_funcs_.size();
        location_type loc = loc_funcs_[loc_index](index_item);
        auto old_item = swap(item, loc); // how to know whether old item has same key as index_item ? need to check 

        if (is_empty_item(old_item))
        {
            inserted_items_++;
            return true;
        }
		else if (are_equal_item(old_item, index_item))
		{
			inserted_items_++;
            return true;
		}
        else
        {
            return insert(old_item, level + 1);
        }
    }
	
	uint64_t KukuTable::get(uint64_t key)
	{
		item_type index_item = make_item(key, 0);
		// Search the hash table
        auto lfc = loc_func_count();
        for (size_t i = 0; i < lfc; i++)
        {
            auto loc = location(index_item, i);
			//std::cout << "index_item : (" << index_item[0] << ", " << index_item[1] << "), item : (" << item[0] << ", " << item[1] << ")"<< std::endl;
            if (are_equal_item(table_[loc], index_item))
            {
                return table_[loc][1];
            }
        }

        // Search the stash
        for (location_type loc = 0; loc < stash_.size(); loc++)
        {
            if (are_equal_item(stash_[loc], index_item))
            {
                return stash_[loc][1];
            }
        }
		
		
	}
}
