#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <new>
#include <math.h>
#include "allocator.h"
using namespace std;

int fd;
BibopHeader *openPages[8];
BibopHeader *fullPages[8];

Allocator::Allocator (void) {
  fd = open("/dev/zero", O_RDWR);
  for( int i = 0; i < 8; i++) {
    openPages[i] = 0;
    fullPages[i] = 0;
  }
}

void * Allocator::malloc(size_t sz) {

  BibopHeader * bibop_ptr;
  void * nextaddr;

  //If the request is oversized
  if(sz > 1024) {
    //allocate new memory (not page-sized) for it with a header
    void * page_ptr = mmap(NULL, sz + sizeof(BibopHeader), PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	bibop_ptr = new (page_ptr) BibopHeader;
	//objectSize is the only relevant field for oversized requests
	bibop_ptr -> _objectSize = sz;
	//return the a pointer to the address immediately following the header
    unsigned int address_int = (unsigned int) page_ptr + sizeof(BibopHeader);
    nextaddr = (void *) address_int;
	return nextaddr;
  }  
  else {
    //Otherwise, find the necessary object size and array slot
    int objectSize;
    int arrayslot;
    for (int i = 3; i <= 10; i++) {
      if (pow(2,i) >= sz) {
        objectSize = (int) pow(2,i);
        arrayslot = i - 3;
        break;
      }
    }

    //if there are no open pages with objects of the correct size
    if(openPages[arrayslot] == 0) {
      //create a new page
      void * page_ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
      bibop_ptr = new (page_ptr) BibopHeader;
      openPages[arrayslot] = bibop_ptr;
      bibop_ptr -> _objectSize = objectSize;
      bibop_ptr -> _prev = 0;
      bibop_ptr -> _next = 0;
      bibop_ptr -> _available = (4096-sizeof(BibopHeader)) / objectSize;
      //set the freelist pointer to the address immediately following the header
      unsigned int address_int = (unsigned int) page_ptr + sizeof(BibopHeader);
      bibop_ptr ->_freeList = (FreeObject*) addressint;
      unsigned int * link;
      //fill in the begining of each object space with the address of the next
      for(int i = 0; i < bibop_ptr -> _available; i++) {
        link = (unsigned int *) address_int;
        address_int += bibop_ptr -> _objectSize;
        *link = address_int;
      }
      *link = 0;
    }
    else {
      //if there at least one open page with objects of the correct size, select the first one
      bibop_ptr = openPages[arrayslot];
    }
    //store the value of the next available memory slot; to be returned at end
    nextaddr = bibop_ptr -> _freeList;
    //cast the freelist pointer to a pointer to an unsigned int
    unsigned int * newFreeAddr = (unsigned int *) bibop_ptr -> _freeList;
    //if it's not a null pointer, make it equal to the address which it's pointing to
    if(bibop_ptr -> _freeList != 0) {
      bibop_ptr -> _freeList = (FreeObject*) *newFreeAddr;
    }
    //decrement the amount available
    bibop_ptr -> _available--;
    //if the page is full...
    if(bibop_ptr -> _available == 0) {
      //remove it from the available list
      openPages[arrayslot] = bibop_ptr -> _next;
	  if(openPages[arrayslot] != 0) {
        openPages[arrayslot] -> _prev = 0;
      }
      bibop_ptr -> _freeList = 0;
      //and place it on top the the full list
      bibop_ptr -> _next = fullPages[arrayslot];
      bibop_ptr -> _prev = 0;
      if(fullPages[arrayslot] != 0) {
        fullPages[arrayslot] -> _prev = bibop_ptr;
      }
      fullPages[arrayslot] = bibop_ptr;
    }
  //return the address originally pointed to by freelist
  return nextaddr;
  }
}

void Allocator::free(void * freeAddr) {
  //mask it to get the header
  //  assumes no malloc calls for values greater than 4096 have been made
  BibopHeader * bibop_ptr = (BibopHeader*) ((unsigned int) freeAddr & 0xFFFFF000);
  int sz = bibop_ptr -> _objectSize;
  
  if(sz > 1024) {
    //if it's oversize, just unmap the page
    munmap(bibop_ptr, bibop_ptr -> _objectSize + sizeof(BibopHeader));
  }
  else {
    int arrayslot = (int) log2(sz) - 3;
    //if it's in a list of closed pages
	if(bibop_ptr -> _available == 0) {
      //remove it from the list it's currently in...
      if(bibop_ptr -> _prev == 0) {
	    fullPages[arrayslot] = bibop_ptr -> _next;
      }
      else {
        bibop_ptr -> _prev -> _next = bibop_ptr -> _next;
      }
      if(bibop_ptr -> _next != 0) {
        bibop_ptr -> _next -> _prev = bibop_ptr -> _prev;
      }	
	  //and put it on top of the corresponding open list
      bibop_ptr -> _next = openPages[arrayslot];
      bibop_ptr -> _prev = 0;
	  if(openPages[arrayslot] != 0) {
        openPages[arrayslot] -> _prev = bibop_ptr;
      }
      openPages[arrayslot] = bibop_ptr;
	
	}
    //increment the number of available slots
    bibop_ptr -> _available++;
    //if available slots is back to it's original value, then the page is empty
    if(bibop_ptr -> _available == (4096 - (int) sizeof(BibopHeader)) / sz) {
	  //remove it from the list it's currently in...
      if(bibop_ptr -> _prev == 0) {
	    openPages[arrayslot] = bibop_ptr -> _next;
      }
      else {
        bibop_ptr -> _prev -> _next = bibop_ptr -> _next;
      }
      if(bibop_ptr -> _next != 0) {
        bibop_ptr -> _next -> _prev = bibop_ptr -> _prev;
      }	
	  //unmap the page
	  munmap(bibop_ptr, 4096);
    }
	else {
	  //otherwise, put the newly freed memory address on top of the freelist
      unsigned int address_int = (unsigned int) bibop_ptr -> _freeList;
      bibop_ptr -> _freeList = (FreeObject*) freeAddr;
      unsigned int * int_ptr = (unsigned int*) freeAddr;
      *int_ptr = address_int;  
  	}
  }
}

size_t Allocator::getSize (void * addr) {
  BibopHeader * bibop_ptr = (BibopHeader*) ((unsigned int) addr & 0xFFFFF000);
  return bibop_ptr -> _objectSize;
}
