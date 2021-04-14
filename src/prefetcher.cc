/*PC/DC*/
/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */


#include <queue>
#include <deque>
#include <vector>
#include <algorithm>
#include "interface.hh"
#include "parameters.hh"

#ifndef pref_cc
    #define pref_cc
    struct GHB_ENTRY{
        Addr mem_addr;
        GHB_ENTRY* prev_addr; //NULL if the last one in the FIFO
        GHB_ENTRY* next_addr; //Makes it a double linked list
    };

    struct IT_ENTRY{
        Addr PC;
        GHB_ENTRY* last_ghb_entry;
    };

    // #define IT_MAX_SIZE 50
    // #define GHB_MAX_SIZE 50
    // #define PCDC_degree 10
    //Define max size, play around with this parameter
    
    std::deque<GHB_ENTRY> GHB_fifo;

    std::deque<IT_ENTRY> IT_table;

    //Degree of prefetching, how many deltas we consider
   

    /* Addresses that should be prefetched */
    std::vector<Addr> candidates;

    /* Addresses that are being prefetched */
    std::deque<Addr> prefetching_in_progress;
#endif

IT_ENTRY* search_for_it_entry(Addr PC)
{
    if(IT_table.size() < IT_MAX_SIZE){
        return NULL;
    }else{
        std::deque<IT_ENTRY>::iterator it = IT_table.begin();
        for(; it != IT_table.end(); it++){
            if(it->PC == PC){
                /* Move the newly addressed entry to front of table */
                IT_ENTRY temp = *it;
                IT_table.erase(it);
                IT_table.push_front(temp);

                return &IT_table.front();
            }
            
        }
        // /*The element at the end is always the oldest*/
        IT_table.pop_back();
        return NULL;
    }
}

void find_prefetches(IT_ENTRY curr_it_entry)
{
    candidates.clear();

    /* get delta queue */
    std::deque<int64_t> delta;
    GHB_ENTRY* curr;
    GHB_ENTRY* prev;
    prev = curr_it_entry.last_ghb_entry;
    if(prev->prev_addr){
        curr = prev;
        prev = prev->prev_addr;
    }else{
        return;
    }
    while(prev){
        if(prev->mem_addr == 0) break;
        if(delta.size() > 100) break;
        if(prev->mem_addr>MAX_PHYS_MEM_ADDR || curr->mem_addr > MAX_PHYS_MEM_ADDR)
            continue;
        delta.push_back(curr->mem_addr - prev->mem_addr);
        curr = prev;
        prev = prev->prev_addr;
    }
    if(delta.size()<4)
        return;
   /* Search for patterns */
    std::deque<int64_t>::iterator it = delta.end();
    it--;
    int64_t delta1 = *it;
    it--;
    int64_t delta2 = *it;

    it = delta.begin();
    while(it != delta.end())
    {
        int64_t u = *it;
        it++;
        int64_t v = *it;
        if(u == delta1 && v == delta2)
        {
            /* Pattern found, get candidates*/
            it++;
            Addr last_addr = curr_it_entry.last_ghb_entry->mem_addr;
            int delta_size = 0;
            while(it != delta.end()){
                last_addr+=*it;
                if(last_addr >= MAX_PHYS_MEM_ADDR)
                    return;
                candidates.push_back(last_addr);
                it++;
                if(++delta_size >= PCDC_degree){
                    return;
                }
            }
            return;
        }
    }
    

}

void prefetch_init(void)
{
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */

    //DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
}

void prefetch_access(AccessStat stat)
{
    if(stat.miss == true && stat.mem_addr < MAX_PHYS_MEM_ADDR && stat.mem_addr != 0){
        GHB_ENTRY new_ghb_entry;
        new_ghb_entry.mem_addr = stat.mem_addr;
        new_ghb_entry.prev_addr = NULL;
        new_ghb_entry.next_addr = NULL;

        if(GHB_fifo.size() >= GHB_MAX_SIZE){
            /*Go to last entry. See if it is connected to a newer entry. If so, go to that entry and delete the reference to this last one.*/
            GHB_ENTRY temp = GHB_fifo.back();
            if(temp.next_addr)
                temp.next_addr->prev_addr = NULL;

            GHB_fifo.pop_back();
        }

        /*Check if there is an entry in the IT_table*/
        IT_ENTRY* curr_it_entry = search_for_it_entry(stat.pc);
        if(curr_it_entry == NULL){
            /*No entry in the IT_table*/
            GHB_fifo.push_front(new_ghb_entry);

            IT_ENTRY new_it_entry;
            new_it_entry.PC = stat.pc;
            new_it_entry.last_ghb_entry = &GHB_fifo.front();
            IT_table.push_front(new_it_entry);
            // curr_it_entry = &IT_table.front();

        }else{
            /*Entry in IT_table exists, pointing to it using curr_it_entry*/
            //Update prev addr to point to the last GHB entry for this PC
            new_ghb_entry.prev_addr = curr_it_entry->last_ghb_entry;
            //Push new GHB entry to GHB FIFO
            GHB_fifo.push_front(new_ghb_entry);
            //Change the next pointer of the prev GHB entry to point to the new one
            new_ghb_entry.prev_addr->next_addr = &GHB_fifo.front();
            //The it_table entry also points to this address
            curr_it_entry->last_ghb_entry = &GHB_fifo.front();
        

            find_prefetches(*curr_it_entry);
            
            for(int i = 0; i < PCDC_degree; i++){
                if(i > ((int)candidates.size())-1) {
                    return;
                }
                
                bool is_in_mshr_queue = in_mshr_queue(candidates[i]) == 1;
                bool is_in_cache = in_cache(candidates[i]) == 1;
                bool is_being_prefetched = std::find(prefetching_in_progress.begin(), prefetching_in_progress.end(), candidates[i]) != prefetching_in_progress.end();
                if(!is_in_mshr_queue && !is_in_cache && !is_being_prefetched && prefetching_in_progress.size() < MAX_QUEUE_SIZE){
                    issue_prefetch(candidates[i]);
                    prefetching_in_progress.push_front(candidates[i]);
                }
            }        
        }
    }
}

void prefetch_complete(Addr addr) {
    /* Called when a block requested by the prefetcher has been loaded. */
    std::deque<Addr>::iterator addr_it = std::find(prefetching_in_progress.begin(), prefetching_in_progress.end(), addr);
    if(addr_it != prefetching_in_progress.end())
        prefetching_in_progress.erase(addr_it);
}
