# RH-Skiplist : Range-Hash Skiplist for DRAM-PM Memory Systems

RH-Skiplist is new indexing sturcture with skiplist and hash for hybrid memory system.  
'original_skiplist' contains baseline which we referred.
'skiplist_pmem' contains porting version of skiplist for Intel Optane persistent memory.  
To port, we used PMDK(https://pmem.io/pmdk/) which version is 1.7.
'range_skiplist' contains essential files of RH-skiplist.

Detailed design, implementation and experiment result is shown in report().  
 
If you have questions, please contact via heejin5178@gmail.com or sheepjin11@gmail.com.
