#!/bin/bash

for cnt in 1000000 2000000 4000000 8000000
do
	echo $cnt > ~/hashmap_log.txt
	sudo ./hashmap_rp_cmp $cnt > ~/hashmap_log.txt
	echo "====================" > ~/hashmap_log.txt
done


