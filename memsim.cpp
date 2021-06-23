#include "memsim.h"
#include <iostream>
#include <cassert>
#include <list>
#include <set>
#include <unordered_map>

struct Partition {
  int tag;
  int64_t size, addr;
  };
//The ordering scmp struct is from Pavol Federl
typedef std::list<Partition>::iterator PartitionRef;
struct scmp {
  bool operator()(const PartitionRef & c1, const PartitionRef & c2) const 
    {
      if (c1->size == c2->size) return c1->addr < c2->addr;//if same size compare address
      else return c1->size > c2->size;// else pick the larger one. Favouring the first
    
    }
  };
struct Simulator {
  int64_t pageSize;// memory has to be a multiple of this
  MemSimResult result;// making result a part of the simulator struct
  // all partitions, in a linked list
  std::list<Partition> all_blocks;// all the partitions
  // sorted partitions by size/address
  std::set<PartitionRef,scmp> free_blocks;// all freed blocks sorted to have largest partition at the top
  // quick access to all tagged partitions
  std::unordered_map<long, std::vector<PartitionRef>> tagged_blocks;// contains a vector of iterators that point into all_blocks towards all the partitions
  //which that particular tag (the long value) has allocated memory to.

  Simulator(int64_t page_size) 
  {// constructor
    
    pageSize = page_size;// setting page_size

  }
  
  void allocate(int tag, int size)
  {
    PartitionRef p;// partition Iterator to point at next value or the last value
    Partition par;//partition to be added or compared
        if(!free_blocks.empty())
        { 
          p = *free_blocks.begin(); // get the largest free space
          
          if(int64_t(size) > p->size){// if the new memory is greater than oour partition size
            auto res = free_blocks.insert(std::prev(all_blocks.end()));//for checking our end
        
            if(!res.second)// checking if the last value issa free space
            {// the res is false if its in the free_blocks set so it would not be added if the last value is a free space
              p = std::prev(all_blocks.end());//set it to the free space
            
            }
            else
            {// it is not a free space
              auto tam = free_blocks.erase(std::prev(all_blocks.end())); assert(tam > 0);// the value we added is not actually free so we should remove it
              Partition pp;// declare a partition
              pp.size = 0;
              pp.addr = std::prev(all_blocks.end())->size + std::prev(all_blocks.end()) -> addr;
              pp.tag = -1;
              all_blocks.push_back(pp);// push back the placeholder free space
              p = std::prev(all_blocks.end());// assign our iterator to that value
              free_blocks.insert(p);//insert our fake free space

            }
          }
        }else{// theres no free space
          par.size = 0;
          par.addr = std::prev(all_blocks.end())->addr + std::prev(all_blocks.end())->size;
          par.tag =-1;
          all_blocks.push_back(par);// push back the placeholder free space
          p = std::prev(all_blocks.end());// assign our iterator to that value
          free_blocks.insert(p);//insert into free blocks

        }
        
    if(all_blocks.size() == 0 && free_blocks.empty() && tagged_blocks.empty())//CHANGE THIS TO FOR ALL NONE FOUND ONES p = all_blocks.begin();
    {
      if(int64_t(size)%pageSize == 0){// the size can perfectly fit into a mutiple of pageSize
        par.addr = 0; 
        par.size = int64_t(size);
        par.tag = tag;
        all_blocks.insert(p,par);//Making and adding the partition
        tagged_blocks[tag].emplace_back(std::prev(p));// adding the iterator which points to this element. This element was emplaced at the back
        result.n_pages_requested += size/pageSize; // size is a multiple of pagesize

      }else if (int64_t(size) > pageSize){// The partiton is larger than a page size
        par.addr = 0; 
        par.size = int64_t(size); 
        par.tag = tag;
        result.n_pages_requested += (int64_t(size) + pageSize-(int64_t(size)%pageSize)) / pageSize;// since the size is larger than a page.
        //the pages requested is dividing the total size of the free space and the size of the partition as thats how many pages are needed
        all_blocks.insert(p,par);
        tagged_blocks[tag].emplace_back(std::prev(p));
        (*p).addr = std::prev(p)->addr +std::prev(p)->size;// Insert adds the partition 
        (*p).size = pageSize-(int64_t(size)%pageSize);
        (*p).tag = -1;// -1 for free space
        free_blocks.insert(p);// insert free partition
        
        
      }else{// its less than the page size
        par.addr = 0; 
        par.size = int64_t(size); 
        par.tag = tag;
        result.n_pages_requested ++;//Only need to increment by one page b/c the partition is smaller than a page
        all_blocks.insert(p,par);
        tagged_blocks[tag].emplace_back(std::prev(p));
        (*p).addr = std::prev(p)->addr +std::prev(p)->size; 
        (*p).size = pageSize-int64_t(size);
        (*p).tag = -1;// -1 for free space
        free_blocks.insert(p);
      }
    }
    else{//ELSE we have a partition in our list
      // These if statements are to make sure that p and par have addresses
      if(p != all_blocks.begin())
       {
          free_blocks.erase(p);//AVOID THE CORRUPTION
          (*p).addr = std::prev(p)->addr +std::prev(p)->size;
          free_blocks.insert(p);
       }
      else
       { 
         free_blocks.erase(p);//AVOID THE CORRUPTION
         (*p).addr = 0;
         free_blocks.insert(p);
       }
      if( p == all_blocks.begin())
        par.addr = 0;
      else
        par.addr = std::prev(p)->addr +std::prev(p)->size; 
      
      if(int64_t(size) == p->size){// the size can perfectly fit into our free partition
        par.size = int64_t(size); 
        par.tag = tag;
        all_blocks.insert(p,par);
        free_blocks.erase(p);//AVOID THE CORRUPTION
        tagged_blocks[tag].emplace_back(std::prev(p));// adding the iterator which points to this element. This element was emplaced at the back
        (*p).addr = std::prev(p)->addr +std::prev(p)->size; 
        (*p).size = 0;
        (*p).tag = -1;// -1 for free space
        free_blocks.insert(p);

      }else if (int64_t(size) < p->size){// the size is less than our free partition

        par.size = int64_t(size); 
        par.tag = tag;
        all_blocks.insert(p,par);
        tagged_blocks[tag].emplace_back(std::prev(p));
        free_blocks.erase(p);//AVOID THE CORRUPTION
        (*p).addr = std::prev(p)->addr +std::prev(p)->size; 
        (*p).size = (*p).size-int64_t(size);// the free space
        (*p).tag = -1;// -1 for free space
        free_blocks.insert(p);
        
         
      }else if(int64_t(size) > p->size){// not enough pages append pages to the end
      
        par.size = int64_t(size); 
        par.tag = tag;
        all_blocks.insert(p,par);
        free_blocks.erase(p);//AVOID THE CORRUPTION
        tagged_blocks[tag].emplace_back(std::prev(p));
        (*p).addr = std::prev(p)->addr +std::prev(p)->size; 
        result.n_pages_requested += (-((*p).size-int64_t(size)) + pageSize-(-((*p).size-int64_t(size))%pageSize)) / pageSize;// calculate added pages b4 changiing (*p).size
        (*p).size = pageSize-(-((*p).size-int64_t(size))%pageSize);//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ is the size of our free space
        (*p).tag = -1;// -1 for free space
        free_blocks.insert(p);
        
      }
      
      if(p != std::prev(all_blocks.end())){// makes sure all the addresses are right from the current manipulated p
        if(std::next(p)->addr != (p)->addr +(p)->size)
          {
            p = std::next(p);
            while(p != all_blocks.end()){
              if(p->tag == -1)
                free_blocks.erase(p); //AVOID THE CORRUPTION
              p->addr =std::prev(p)->addr +std::prev(p)->size;
              if(p->tag == -1)
                free_blocks.insert(p); 
              p = std::next(p);
            }
          }
      }
    }
    
  }
  //Deallocates memory
  void deallocate(int tag) {
    PartitionRef p;
    if(tagged_blocks.find(tag) == tagged_blocks.end()) return;// the tag was not found, return
    if(tagged_blocks[tag].empty())return;// the tag has no partitions, return
    if(!tagged_blocks[tag].empty())
    {
      for(auto & pa: tagged_blocks[tag])
      {// for each partition for this tag
        pa->tag = -1;//make it free
        auto p = free_blocks.insert(pa); assert(p.second);// make sure its added to free and isn't already there (should not be there)
        PartitionRef currentNode = pa;// point our current partition to the node we just freed
      if(currentNode != all_blocks.begin())
                {// if we can go back go back
                  if(std::prev(currentNode)->tag == -1)
                  {
                    currentNode = std::prev(currentNode);// sets our iterator one back
                  }
                }
                // The free space merger except we check to our left and right and we start at or before the partition we just freed(pa)
        while(next(currentNode)->tag == -1 && next(currentNode) !=all_blocks.end())//this runs until we reach the end or a filled partition
        {
                if(currentNode != all_blocks.begin())
                {
                  if(std::prev(currentNode)->tag == -1)// going to the furthest node left which is free
                  {
                    currentNode = std::prev(currentNode);
                    continue;//check while condition and making sure there are not free spaces before
                  }
                }
                auto nextNode = std::next(currentNode);  // need to save this iterator *before* we erase currentNode 
                if (currentNode->tag == -1 && nextNode-> tag == -1 ) {// if both are negative
                        free_blocks.erase(currentNode);//AVOID THE CORRUPTION
                        free_blocks.erase(nextNode);//AVOID THE CORRUPTION still haunts me
                        nextNode ->size += currentNode -> size;// add the size of current node to the next node
                        nextNode ->addr = currentNode -> addr;//the next address will just be the current address
                        all_blocks.erase(currentNode);// remove current node from all_blocks
                        free_blocks.insert(nextNode);// Insert our singular node
                }
                currentNode = nextNode;// set our current node to the next
        }
    }
    tagged_blocks[tag].clear();// clear the tag of its nodes
    }
  }
  /*
   * CONSISTANCY CHECK IS COMMENTED OUT FOR EFFICIENCY
   */
  // void check_consistency(int tag, int size) {
   
    
  //   //fprintf(stderr,"\nEMP: %d",all_blocks.empty());
  //   if(!all_blocks.empty())
  //   {
  //      if(!free_blocks.empty())
  //   {
  //     for(auto o = free_blocks.begin(); o != free_blocks.end(); o++){
  //       //fprintf(stderr,"\ntag: %ld size: %d ",(*o)->tag,(*o)->size);
  //       assert((*o)->tag ==-1);
      
  //       assert((*o) != all_blocks.end());
  //     }

  //   }
  //   //fprintf(stderr,"\nTag Added: %d ISize: %d",tag,size );
  //   int64_t dave = 0;
  //   for(auto i = all_blocks.begin(); i != all_blocks.end(); i++)
  //   {
  //     dave += i->size;
  //     if(i->tag == -1)
  //       {
  //         auto poo = free_blocks.insert(i);
  //         assert(!poo.second);
  //         if(poo.second)
  //           free_blocks.erase(i);
  //       }
  //     else
  //     {
  //       auto iiiii = tagged_blocks[i->tag];
  //       // for(const auto & ooo: iiiii){
  //       //   fprintf(stderr,"\n%d ISize: %ld",ooo->tag,ooo->size );
  //       // }
  //     }
  //     if(i == all_blocks.begin())
  //       {
  //         //fprintf(stderr,"\niADD: %ld whatItShouldBE: %d tag: %d ISize: %ld",i->addr, 0, i->tag, i->size);
  //         assert(i->addr == 0);
  //       }
  //     else
  //       {
  //         //fprintf(stderr,"\niADD: %ld whatItShouldBE: %ld tag: %d ISize: %ld\n",i->addr, std::prev(i)->addr + std::prev(i)->size, i->tag, i->size);
  //         assert(i->addr == std::prev(i)->addr + std::prev(i)->size);
  //       }
  //   }

  //    //fprintf(stderr,"\n%ld %ld %ld %ld",dave,pageSize*result.n_pages_requested, (*free_blocks.begin())->addr, result.n_pages_requested );
  //   assert(dave == pageSize * result.n_pages_requested);
  //   }
    
    
  //   int64_t count = 0;
  //   for(const auto & u : tagged_blocks)
  //   {
  //     count += u.second.size();
  //   }
  //   //fprintf(stderr,"\n%ld %ld %ld \n",all_blocks.size(),free_blocks.size() + count,count);
  //   assert(all_blocks.size() == free_blocks.size() + count);
    
  // }
};


// re-implement the following function
// ===================================
// parameters:
//    page_size: integer in range [1..1,000,000]
//    requests: array of requests
// return:
//    some statistics at the end of simulation
MemSimResult mem_sim(int64_t page_size, const std::vector<Request> & requests)
{
  // The tag of a freed space will be -1
  
  Simulator sim(page_size);//construct 
  
  //initializing the results
  sim.result.n_pages_requested = 0; 
  sim.result.max_free_partition_size = 0; 
  sim.result.max_free_partition_address = 0; 
  for (const auto & req : requests) {
    if (req.tag < 0) {
      
      sim.deallocate(-req.tag);// the negative in a request tag only denotes that its a deallocation so no tag will actually be negative
      
    } else {
      sim.allocate(req.tag, req.size);
    }
    //sim.check_consistency(req.tag, req.size);
  }
  
  if(!sim.free_blocks.empty())
 {
   // the largest free partition will be the first value from the free_blocks as long as its not corrupted
  sim.result.max_free_partition_size = (*sim.free_blocks.begin())->size; 
  sim.result.max_free_partition_address = (*sim.free_blocks.begin())->addr; 
 }
  return sim.result;
}
