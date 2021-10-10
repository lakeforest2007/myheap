/*
 * Name: Julie Wang
 * File: explicit.c
 * This program implements a explicit heap manager using
 * in-place reallocation and coalescing. The doubly linked list
 * is made with a last-in-first-out approach.
 */
#include "allocator.h"
#include "debug_break.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define HEADER_SIZE 8 //bytes

typedef struct { //this stores prev and next pointers for freelist
    void *prev;
    void *nxt;
} listnode;

typedef struct {
    unsigned long size; //takes care of casting header size
} header; 


static size_t segment_size;
static void *segment_start;

static listnode *start; //always pts to beg of linked list
static size_t nbytes_inuse; //bytes currently in use
static int num_header;

static size_t nused;
//static int countline; //for debugging purposes

/*
 * This function rounds up the size requested to the given multiple 
 * (must b * e a power of 2) and returns the result (taken from bump.c).
 */
size_t addpad(size_t size, size_t mult) {
    return (size + mult - 1) & ~(mult - 1);
}

/* 
 * Myint is called by a client before making any allocation
 * requests.  The function returns true if initialization was 
 * successful, or false otherwise. The myinit function can be 
 * called to reset the heap to an empty state. When running 
 * against a set of of test scripts, our test harness calls 
 * myinit before starting each new script.
 */
bool myinit(void *heap_start, size_t heap_size) {
    //check if heap_size is at least 24 bytes (header + two pointers)
    if (heap_size < (HEADER_SIZE + sizeof(listnode))) return false;

    segment_size = heap_size;
    segment_start = heap_start;
    
    //initialize header
    header *first = (header *)segment_start;
    nbytes_inuse = 0; //initiate payload bytes in-use to 0
    first->size = segment_size - HEADER_SIZE; //lsb = 0
    num_header = 1;

    nused = HEADER_SIZE + sizeof(listnode); //24 bytes

    //initialize linked list node with first free header
    start = (listnode *)((char *)segment_start + HEADER_SIZE);
    start->prev = NULL;
    start->nxt = NULL;

    //countline = 0; --for debugging purposes
    
    return true;
}

/* This function updates the linked list by adding the 
 * new node to its beginning.
 */
void add_to_beg(header *hdr) {
    listnode *newfree = (listnode *)((char *)hdr + HEADER_SIZE);
    if (start == NULL) { 
        start = newfree;
        start->prev = 0x0;
        start->nxt = 0x0;
    } else {
        newfree->nxt = (char *)start - HEADER_SIZE;
        newfree->prev = NULL;
        start->prev = hdr;
        start = newfree;
    }
}

/* This function helps to remove the passed in node from
 * the freelist.
 */
void remove_node(listnode *ithnode) {
    if (ithnode->prev == NULL && ithnode->nxt == NULL) { //only
        start = NULL;
    } else if (ithnode->prev == NULL) { //start
        start = (listnode *)((char *)ithnode->nxt + HEADER_SIZE);
        start->prev = NULL;
    } else if (ithnode->nxt == NULL) { //end
        listnode *second_last = (listnode *)((char *)ithnode->prev + HEADER_SIZE);
        second_last->nxt = NULL;
    } else { //middle
        listnode *left = (listnode *)((char *)ithnode->prev + HEADER_SIZE);
        listnode *right = (listnode *)((char *)ithnode->nxt + HEADER_SIZE);
        left->nxt = ithnode->nxt;
        right->prev = ithnode->prev;
    }
}

/* This function taken in a requested size and allocates memory
 * on the heap. Because it is an explicit implementation, it only
 * iterates through free nodes, and when the function finishes, a
 * pointer to a memory location is returned. If there are no more free
 * space, a NULL ptr is returned.
 */
void *mymalloc(size_t requested_size) {
    if (requested_size <= 0 || requested_size > MAX_REQUEST_SIZE) return NULL;
    //align requested_size
    size_t needed = addpad(requested_size, ALIGNMENT);

    listnode *ithnode = start; //start from the beginning
    
    while (true) { //check if we reached end of linked list
        header *curhdr = (header *)((char *)ithnode - HEADER_SIZE);
        size_t prev_size = curhdr->size;
        if (needed <= curhdr->size) {
                
            header *check_last = (header *)((char *)ithnode - HEADER_SIZE);
            if ((char *)check_last + check_last->size + HEADER_SIZE == ((char *)segment_start + segment_size)) {
                    
                if (needed <= sizeof(listnode)) {
                    curhdr->size = sizeof(listnode);
                } else curhdr->size = needed;
                nused += curhdr->size;
                nbytes_inuse += curhdr->size;
                
                //attach new header at end
                void *nxthdr = (char *)curhdr + curhdr->size + HEADER_SIZE;
                header *new_hdr = (header *)nxthdr;
                new_hdr->size = prev_size - HEADER_SIZE - curhdr->size;
                num_header += 1;
                nused += HEADER_SIZE;

                //replace the old node (now taken) with a new node (the large free node)
                remove_node(ithnode); 
                add_to_beg(new_hdr); 
                    
            } else {
                nbytes_inuse += curhdr->size; 
                remove_node(ithnode);   
            }          
            curhdr->size |= 0x1;
            break;
        }
        //else update ithnode
        if (ithnode->nxt == NULL) return NULL;
        ithnode = (listnode *)((char *)ithnode->nxt + HEADER_SIZE);
    }
    return (void *)ithnode; 
}

/* This function `coalesces` the two taken in headers.
 * Specifically, in this implementation, we delete the neighboring
 * node from the freelist if it is not adjacent to the header node.
 * If it is, we rewire the links.
 */
void coalesce(header *hdr, header *neighbor) {
    listnode *neinode = (listnode *)((char *)neighbor + HEADER_SIZE);

    //check if hdrnode and neinode are adjacent
    if ((neinode->nxt == NULL && neinode->prev == NULL) ||
        (neinode->prev == hdr) || (neinode->nxt == hdr)) {
 
        if (neinode->nxt != NULL) { 
            void *ptr_temp = neinode->nxt;
            listnode *node_temp = (listnode *)((char *)ptr_temp + HEADER_SIZE);
            node_temp->prev = hdr;
        }
       
        listnode *hdrnode = (listnode *)((char *)hdr + HEADER_SIZE);
        if (neinode == start) { 
            listnode *start_temp = start;
            start = hdrnode;
            hdrnode->prev = NULL;
            hdrnode->nxt = start_temp->nxt;
        } else {
            void *ptr_before = neinode->prev;
            listnode *node_before = (listnode *)((char *)ptr_before + HEADER_SIZE);
            if (node_before == hdrnode) { 
                hdrnode->nxt = neinode->nxt;
            } else { 
                hdrnode->prev = neinode->prev; 
                hdrnode->nxt = neinode->nxt;
                node_before->nxt = (char *)hdrnode - HEADER_SIZE;
            }
        }
        
    } else remove_node(neinode);
}


/*
 *Takes care of freeing block at the pointer address.
 * Freed block should coalesce with its neighbor block to the right
 * if it is also free. If a null pointer is taken in, we simply return.
 */
void myfree(void *ptr) {
    if (!ptr) return;
    
    //update header
    header *hdr = (header *)((char *)ptr - HEADER_SIZE);
    hdr->size ^= 0x1;
    nbytes_inuse -= hdr->size;

    header *neighbor = (header *)((char *)hdr + hdr->size + HEADER_SIZE);
   
    if (!(neighbor->size & 0x1)) { //coalesce
        //update header (same as implicit)
        hdr->size += HEADER_SIZE + neighbor->size;
        num_header--; //we lose a header
        
        add_to_beg(hdr); 
        coalesce(hdr, neighbor);
        
    } else add_to_beg(hdr); //without coalescing
}

//This function attaches a new header to the end of heap.
void attach_header(header *curhdr, unsigned long prev_size) {
    void *nxthdr = (char *)curhdr + curhdr->size + HEADER_SIZE;
    header *new_hdr = (header *)nxthdr;
    new_hdr->size = prev_size - HEADER_SIZE - curhdr->size;
    num_header += 1;
    nused += HEADER_SIZE;
}

//This function saves the first 16 bytes of payload data.
void save_data(void *old_ptr, unsigned long pay1, unsigned long pay2) { 
    *(unsigned long *)old_ptr = pay1;
    char *temp = old_ptr;
    temp += HEADER_SIZE;
    *(unsigned long *)temp = pay2;
}

/* This function reallocates memory given a new size. It does
 * coalescing if neighboring right blocks are free. Then, it does
 * in-place realloc if size is big enough. If not, it calls
 * on mymalloc to move memory elsewhere.
 */
void *myrealloc(void *old_ptr, size_t new_size) {
    if (!old_ptr) {
        return mymalloc(new_size);
    }
    if (new_size == 0) {
        myfree(old_ptr);
        return NULL;
    }
    //if new_size is already smaller than current block, just return old ptr
    header *curhdr = (header *)((char *)old_ptr - HEADER_SIZE);
    if ((curhdr->size - 1) >= new_size) {
        return old_ptr;
    }
    curhdr->size ^= 0x1; //turn lsb off
    header *neighbor = (header *)((char *)curhdr + curhdr->size + HEADER_SIZE);
    unsigned long pay1 = 0; //save first 16 bytes of payload data
    unsigned long pay2 = 0;
    pay1 = *((unsigned long *)(old_ptr));
    pay2 = *((unsigned long *)((char *)old_ptr + HEADER_SIZE));
    
    bool coalesced = false;
    int numit = 0;
    while (true) { //if next node is free
        if ((char *)neighbor == ((char *)segment_start + segment_size)) break;
        if (!(neighbor->size & 0x1)) {
            curhdr->size += HEADER_SIZE + neighbor->size; //update header
            num_header--; //we lose a header
            if (numit == 0) {
                add_to_beg(curhdr);
            }
            coalesce(curhdr, neighbor);
            coalesced = true;
        } else break;
        //update neighbor
        neighbor = (header *)((char *)neighbor + neighbor->size + HEADER_SIZE);
        numit++;
    }
    if (!coalesced) add_to_beg(curhdr);
    curhdr->size ^= 0x1; //turn lsb back on

    if ((curhdr->size - 1) < new_size) { //if new_size still larger, reallocate
        void *moved_ptr = mymalloc(new_size); 
        if (!moved_ptr) return NULL;

        unsigned long old_prev = *(unsigned long *)old_ptr; //save prev and nxt addresses
        unsigned long old_nxt = *(unsigned long *)((char *)old_ptr + HEADER_SIZE);
        
        //put user data back
        save_data(old_ptr, pay1, pay2);
        memcpy(moved_ptr, old_ptr, new_size);

        *(unsigned long *)old_ptr = old_prev; //put prev and nxt addresses back
        old_ptr = (char *)old_ptr + HEADER_SIZE;
        *(unsigned long *)old_ptr = old_nxt;
        
        curhdr->size ^= 0x1; //mark the pointer as freed
        return moved_ptr;
    } else { //in-place realloc and return block of proper size by splitting
        unsigned long align_size = addpad(new_size, ALIGNMENT);
        unsigned long prev_size = curhdr->size - 1;
        if ((prev_size - align_size) <= sizeof(listnode)) {
            //remove node from freelist
            listnode *old_node = (listnode *)((char *)curhdr + HEADER_SIZE);
            remove_node(old_node);
            
            //put data back
            *(unsigned long *)old_ptr = pay1;
            char *temp = old_ptr;
            temp += HEADER_SIZE;
            *(unsigned long *)temp = pay2;
            return old_ptr;
        }
        //if what's left is nonzero, update header and add free block to beg. of list
        curhdr->size = align_size; //lsb currently off
        
        //attach new free header at end
        void *nxthdr = (char *)curhdr + curhdr->size + HEADER_SIZE;
        header *new_hdr = (header *)nxthdr;
        new_hdr->size = prev_size - HEADER_SIZE - curhdr->size;
        num_header += 1;
        nused += HEADER_SIZE;
        curhdr->size ^= 0x1; //turn lsb back on
        
        //move info from first part to second part (the splitted block)
        listnode *oldnode = (listnode *)((char *)curhdr + HEADER_SIZE);
        remove_node(oldnode); //remove the coalesced node from freelist
        add_to_beg(new_hdr); //add splitted node to freelist
        
        //put data back
        *(unsigned long *)old_ptr = pay1;
        char *temp = old_ptr;
        temp += HEADER_SIZE;
        *(unsigned long *)temp = pay2;
        
        return old_ptr;
    }
}

/* 
 * Return true if all is ok, or false otherwise.
 * This function is called periodically by the test
 * harness to check the state of the heap allocator.
 */
bool validate_heap() {
    int num_free_hdr = 0;
    header *cur = (header *)segment_start;
    for (int i = 0; i < num_header; i++) {
        if (!(cur->size & 0x1)) {
            num_free_hdr++;
        }
        void *temp; 
        if (cur->size & 0x1) { //used
            temp = (char *)cur + HEADER_SIZE - 1 + cur->size;
        } else { //if not used
            temp = (char *)cur + HEADER_SIZE + cur->size;
        }
        cur = (header *)temp;
    }

    listnode *node = start;
    int cnt = 0;
    while (node != NULL) {
        cnt++;
        if (node->nxt != NULL) {
            node = (listnode *)((char *)node->nxt + HEADER_SIZE);
        } else break;
    }

    if (cnt != num_free_hdr) {
        printf("Number of free headers does not match number of free list nodes\n");
        breakpoint();
        return false;
    }

    cur = (header *)segment_start;
    size_t segment_bytes = 0;
    for (int i = 0; i < num_header; i++) {
        segment_bytes += HEADER_SIZE;
        void *temp;
        if (cur->size & 0x1) { //used
            temp = (char *)cur + HEADER_SIZE - 1 + cur->size;
            segment_bytes += cur->size - 1;
        } else { //if not used, free
            temp = (char *)cur + HEADER_SIZE + cur->size;
            segment_bytes += cur->size;
        }
        cur = (header *)temp;
    }
    //size - check if all segment_size is accounted for
    if (segment_bytes != segment_size) {
        printf("Oops! Not all of the segment size is accounted for.\n");
        breakpoint();
        return false;
    }
    
    return true;
}

/* Used to dump the heap and its contents. Also 
 * Prints out header information and the freelist.
 */
void dump_heap() {
     printf("Heap segment starts at address %p, ends at %p. %lu bytes currently used.",
            segment_start, (char *)segment_start + segment_size, nused);

     //nused = 0; //custom for testing
     for (int i = 0; i < nused; i++) {
          unsigned char *cur = (unsigned char *)segment_start + i;
          if (i % 32 == 0) {
              printf("\n%p: ", cur);
          }
          printf("%02x ", *cur);
    }
    printf("\n\n");
    header *cur = (header *)segment_start;
    for (int i = 0; i < num_header; i++) {
        printf("Header %d (%p): %lu\n", i, cur, cur->size);
        //printf("Ptrs: (%p)\n", (listnode *)((char *)cur + HEADER_SIZE));
        void *temp; //extract the following into helper function!
        if (cur->size & 0x1) { //used
            temp = (char *)cur + HEADER_SIZE - 1 + cur->size;
        } else { //if not used
            temp = (char *)cur + HEADER_SIZE + cur->size;
        }
        cur = (header *)temp;
    }
    printf("\n");
    listnode *node = start;
    int cnt = 0;
    while (node != NULL) {
        printf("%d Free list node (%p): %p %p\n", cnt, node, node->prev, node->nxt);
        cnt++;
        if (node->nxt != NULL) {
            node = (listnode *)((char *)node->nxt + HEADER_SIZE);
        } else break;
    }
}


