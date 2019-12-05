// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <kuku/kuku.h>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace kuku;

ostream &operator <<(ostream &stream, item_type item)
{
    stream << item[1] << " " << item[0];
    return stream;
}

void print_table(const KukuTable &table)
{
    size_t col_count = 8;
    for (size_t row = 0; row < table.table_size() / col_count; row++)
    {
        for (size_t col = 0; col < col_count; col++)
        {
            size_t index = row * col_count + col;
			item_type pair = table.table(index);
			uint64_t a = table.getIndex(0);
			uint64_t b = table.get(pair[0]);
            cout << setw(5) << table.getIndex(pair[0]) << ": " << setw(5) << pair[0] << " " << b<< "\t";

        }
        cout << endl;

		
    }

    cout << endl << "Stash: " << endl;
    for (size_t i = 0; i < table.stash().size(); i++)
    {
        cout << i << ": " << table.stash(i) << endl;
    }
    cout << endl;
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        cout << "Usage: ./example log_table_size stash_size loc_func_count max_probe" << endl;
        cout << "E.g., ./example 8 2 4 100" << endl;

        return 0;
    }

    int log_table_size = atoi(argv[1]);
    size_t stash_size = static_cast<size_t>(atoi(argv[2]));
    size_t loc_func_count = static_cast<size_t>(atoi(argv[3]));
    item_type loc_func_seed = make_random_item();
    uint64_t max_probe = static_cast<uint64_t>(atoi(argv[4]));
    item_type empty_item = make_item(0, 0);

    KukuTable table(
        log_table_size,
        stash_size,
        loc_func_count,
        loc_func_seed,
        max_probe,
        empty_item
    );

    uint64_t round_counter = 0;
    while (true)
    {
        cout << "Inserted " << round_counter * 20 << " items" << endl;
        cout << "Fill rate: " << table.fill_rate() << endl;

        char c;
        cin.get(c);

        for (uint64_t i = 0; i < 100; i++)
        {
			item_type item = make_item(i + 1, round_counter+1);
            if (!table.insert(item))
            {
                cout << "Insertion failed~~: round_counter = "
                    << round_counter << ", i = " << i << endl;
                cout << "Inserted successfully " << round_counter * 20 + i << " items" << endl;
                cout << "Fill rate: " << table.fill_rate() << endl;
                cout << "Leftover item: " << table.last_insert_fail_item() << endl << endl;
                break;
            }
			cout << "------------------------------------" <<endl;
			cout << "round_counter+1 = "
                    << round_counter+1 << ", i+1 = " << i+1 << endl;
			cout << "("<< i+1 << ", " << round_counter+1  <<") item : " << table.query(item) << endl;
			cout << "("<< i+1 << ", " << 0 <<") item : " << table.query(make_item(i+1,0)) <<endl;
        }

        print_table(table);

        if (!table.is_empty_item(table.last_insert_fail_item()))
        {
            break;
        }

        round_counter++;
    }

    while (true)
    {
        cout << "Query item: ";
        char hw[64];
        char lw[64];
        cin.getline(hw, 10, ' ');
        cin.getline(lw, 10, '\n');
        item_type item = make_item(
            static_cast<uint64_t>(atoi(lw)), static_cast<uint64_t>(atoi(hw)));
        QueryResult res = table.query(item);
        cout << "Found: " << boolalpha << !!res << endl;
        cout << "Location: " << res.location() << endl;
        cout << "In stash: " << boolalpha << res.in_stash() << endl;
        cout << "Hash function index: " << res.loc_func_index() << endl << endl;
    }

    return 0;
}
