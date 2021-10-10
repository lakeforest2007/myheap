/*
 * Name: Julie Wang
 * File: implicit.c
 * This program implements a implicit heap manager using
 * in-place reallocation and first-fit search.
 */
#include "allocator.h"
#include "debug_break.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define HEADER_SIZE 8 //bytes

typedef struct {
    unsigned long size; //used for casting header size
} header;

static size_t segment_size;
static size_t nused;
static size_t nbytes_inuse;
static void *segment_start;
static int num_header;

/*
 * This function rounds up the size requested to the given multiple (must b * e a power of 2) and returns the result.
 */
size_t addpad(size_t size, size_t mult) {
    return (size + mult - 1) & ~(mult - 1);
}

/* 
 * This must be called by a client before making any allocation
 * requests.  The function returns true if initialization was 
 * successful, or false otherwise. The myinit function can be 
 * called to reset the heap to an empty state. When running 
 * against a set of of test scripts, our test harness calls 
 * myinit before starting each new script.
 */
bool myinit(void *heap_start, size_t heap_size) {
    //return false if heap_size is less than header size
    if (heap_size < HEADER_SIZE) return false;

    segment_size = heap_size;
    segment_start = heap_start;
    
    //make first header
    header *first = (header *)segment_start;
    nbytes_inuse = 0; //initiate payload bytes in-use to 0
    nused = HEADER_SIZE; //8 bytes used to store header
    first->size = segment_size - HEADER_SIZE; //lsb = 0
    num_header = 1;
    
    return true;
}

/* This function takes in a requested size and allocates memory
 * on the heap. Because it is an implicit implementation, it
 * iterates through every header/block, and when the function finishes, a
 * pointer to a memory location is returned. If there are no more free
 * space, a NULL ptr is returned.
 */
void *mymalloc(size_t requested_size) {
    if (requested_size <= 0 || requested_size > MAX_REQUEST_SIZE) { //exceptions
        return NULL;
    }
    //align requested_size
    size_t needed = addpad(requested_size, ALIGNMENT);
    //iterate through headers and check space available using first-fit
    header *ithdr = (header *)segment_start; //start from the beg

    int cnt;
    bool block = false;
    for (cnt = 0; cnt < num_header; cnt++) {    
        if (needed <= ithdr->size && !(ithdr->size & 0x1)) {
            //if header is the last one, update payload size
            if (cnt == num_header - 1) {
                nused += needed;
                ithdr->size = needed;
                nbytes_inuse += needed;
            } else nbytes_inuse += ithdr->size;
            ithdr->size |= 0x1;
            block = true;
            break;
        } //else increment ptr by header size:
        void *temp; 
        if (ithdr->size & 0x1) { //used
            temp = (char *)ithdr + HEADER_SIZE - 1 + ithdr->size;
        } else { //if not used
            temp = (char *)ithdr + HEADER_SIZE + ithdr->size;
        }
        ithdr = (header *)temp;
    }
    //attach new header at end
    if ((cnt == num_header - 1) && block && (segment_size - nused >= HEADER_SIZE)) {
        void *nxthdr = (char *)ithdr + ithdr->size + HEADER_SIZE - 1; //to acct for set lsb
        header *new_hdr = (header *)nxthdr;
        new_hdr->size = segment_size - nused - HEADER_SIZE; //subtract curr header size
        nused += HEADER_SIZE;
        num_header += 1;
    }
    if (block) return (char *)ithdr + HEADER_SIZE;
    
    //if we reach here, we know that requested_size is too big
    //for any of our available blocks, so just return NULL
    return NULL;
}

/*
 * Takes care of freeing block at the pointer address.
 * If a null pointer is taken in, we simply return.
 */
void myfree(void *ptr) {
    if (ptr) { //exception: check that ptr is non-null
        //update the header only (size shouldn't change)
        header *hdr = (header *)((char *)ptr - HEADER_SIZE);
        hdr->size ^= 0x1; //set bit to 0
        nbytes_inuse -= hdr->size;
    }
}

/* This function reallocates memory given a new size. It does
 * in-place realloc if size is big enough. If not, it calls
 * on mymalloc to move memory elsewhere.
 */
void *myrealloc(void *old_ptr, size_t new_size) {
    if (!old_ptr) { 
        void *new_ptr = mymalloc(new_size);
        return new_ptr;
    }
    if (new_size == 0) {
        myfree(old_ptr);
        return NULL;
    }
    //check if current block is large enough
    header *curhdr = (header *)((char *)old_ptr - HEADER_SIZE);
    if ((curhdr->size - 1) >= new_size) {
        return old_ptr;
    }
    //if not, use memcpy to move the data to a block (already aligned)
    void *new_ptr = mymalloc(new_size); 
    if (!new_ptr) return NULL;

    memcpy(new_ptr, old_ptr, new_size);
    myfree(old_ptr);
    
    return new_ptr;
}

/* 
 * Return true if all is ok, or false otherwise.
 * This function is called periodically by the test
 * harness to check the state of the heap allocator.
 * You can also use the breakpoint() function to stop
 * in the debugger - e.g. if (something_is_wrong) breakpoint();
 */
bool validate_heap() {
    if (nused > segment_size) {
        printf("Oops! Have used more heap than total available?!\n");
        breakpoint();
        return false;
    }
    //check location, size, free status for each block
    header *cur = (header *)segment_start;
    size_t segment_bytes = 0;
    size_t validate_payload = 0;
    for (int i = 0; i < num_header; i++) {
        //location - must be within address boundaries
        if ((char *)cur > (char *)segment_start + segment_size) {
            printf("Oops! Address is not within heap bounds.\n");
            breakpoint();
            return false;
        }
        segment_bytes += HEADER_SIZE;
        void *temp;
        if (cur->size & 0x1) { //used
            temp = (char *)cur + HEADER_SIZE - 1 + cur->size;
            segment_bytes += cur->size - 1;
            validate_payload += cur->size - 1;
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
    //free status - does total bytes in-use match sum of in-use block sizes? (nbytes_inuse)
    if (validate_payload != nbytes_inuse) { 
        printf("Oops! Total payload bytes currently in use does not match sum of in-use block sizes.\n");
        breakpoint();
        return false;
    }
    return true;
}

/* Used to print out heap contents as well as each
 * header information.
 */
void dump_heap() {
    printf("Heap segment starts at address %p, ends at %p. %lu bytes currently used.", 
        segment_start, (char *)segment_start + segment_size, nused);
    for (int i = 0; i < nused; i++) {
        unsigned char *cur = (unsigned char *)segment_start + i;
        if (i % 32 == 0) {
            printf("\n%p: ", cur);
        }
        printf("%02x ", *cur);
    }
    printf("\n");
    header *cur = (header *)segment_start;
    for (int i = 0; i < num_header; i++) {
        printf("Header %d (%p): %lu\n", i, cur, cur->size);
        void *temp; //extract the following into helper function!
        if (cur->size & 0x1) { //used
            temp = (char *)cur + HEADER_SIZE - 1 + cur->size;
        } else { //if not used
            temp = (char *)cur + HEADER_SIZE + cur->size;
        }
        cur = (header *)temp;
    }
}
