// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2001-2026 Richard Lesh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <system/io.asm>
#include <system/math.def>
#include <system/system.asm>
#include <adt/vector.asm>

	jump	HEAP_ASM_END

///////////////////////////////////////////////////////////////////////////////
// Block List / Block Data Structure
//
// Block List (_HEAP_BLOCK_LIST)     Blocks of _HEAP_BLOCK_SIZE
// n is in _HEAP_BLOCK_LIST[-1]      Block 0
//     +-----------------+           +---------------------------------------+
//   0 | 0 or block addr | --------> | Element 0                             |
//     +-----------------+           +---------------------------------------+
//   1 | 0 or block addr | ----+     | Element 1                             |
//     +-----------------+     |     + ...                                   +
//   2 | 0 or block addr |     |     | Element _HEAP_BLOCK_NUM_ELEMENTS - 1  |
//     +-----------------+     |     +---------------------------------------+
// ... | 0 or block addr |     |     Block 1
//     +-----------------+     |     +---------------------------------------+
// n-2 | 0 or block addr |     +---> | Element _HEAP_BLOCK_NUM_ELEMENTS      |
//     +-----------------+           +---------------------------------------+
// n-1 | 0 or block addr |           | Element _HEAP_BLOCK_NUM_ELEMENTS + 1  |
//     +-----------------+           + ...                                   +
//                                   | Elem 2 * _HEAP_BLOCK_NUM_ELEMENTS - 1 |
//                                   +---------------------------------------+
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Heap
// Implements a growable list of structures.  Is thread safe.
///////////////////////////////////////////////////////////////////////////////
#define	_HEAP_LEN					0	// Number of elements used
#define	_HEAP_CAPACITY				1	// Maximum number of elements available
#define	_HEAP_BLOCK_LIST			2	// Vector of allocated blocks
#define	_HEAP_BLOCK_NUM_ELEMENTS	3	// Number of elements in a block
#define	_HEAP_BLOCK_ELEMENT_SIZE	4	// Size of a block element
#define	_HEAP_BLOCK_SIZE			5	// Size in words of a block (computed)
#define	_HEAP_INIT_FUNCTION			6	// Init function for new elements
#define	_HEAP_DESTROY_FUNCTION		7	// Destroy function for elements
#define	_HEAP_MUTEX					8	// Mutex used for concurrent access
#define	_HEAP_END					8 + MUTEX_SIZE

#global _HEAP_DEFAULT_BLOCK_LIST_SIZE: .dci 10		// Smallest size of block list
#global _HEAP_DEFAULT_BLOCK_NUM_ELEMENTS: .dci 100	// Smallest number of elements/block

///////////////////////////////////////////////////////////////////////////////
// newHeap(numElements, elementSize, initFunc, destroyFunc)
// Creates a new heap object and returns its address.
// numElements	Number of elements to start the heap.
// elementSize	Size of elements stored in the heap.
// initFunc		Stack-based function to initialize an element.  Takes 
//				one argument that is the address of the element.
// destroyFunc	Stack-based function to destroy an element.  Takes 
//				one argument that is the address of the element.
///////////////////////////////////////////////////////////////////////////////
#def_func newHeap(numElements, elementSize, initFunc, destroyFunc)
	#var	s, \
			addr, \
			blockElementSize, \
			blockListSize, \
			blockNumElem, \
			destroy, \
			init
	load	s, numElements
	load	blockElementSize, elementSize
	load	blockNumElem, _HEAP_DEFAULT_BLOCK_NUM_ELEMENTS
	// Determine number of blocks to start with s/blockNumElem.
	// Plus 1 block if remainder to account for partial block.
	div		blockListSize, r0, s, blockNumElem
	#if_cond	r0, ne, 0
		add		blockListSize, 1
	#end_cond
	// Make sure blockListSize is not below minimum size.
	load	r0, _HEAP_DEFAULT_BLOCK_LIST_SIZE
	#macro	MAX(r0, blockListSize)
	move	blockListSize, r0
	// Allocate the Heap structure
	#macro	ALLOC(_HEAP_END)
	move	addr, r0
	// Initialze members
	load	r0, initFunc
	store	r0, addr[_HEAP_INIT_FUNCTION]
	load	r0, destroyFunc
	store	r0, addr[_HEAP_DESTROY_FUNCTION]
	store	0, addr[_HEAP_LEN]
	store	0, addr[_HEAP_MUTEX]
	store	blockElementSize, addr[_HEAP_BLOCK_ELEMENT_SIZE]
	store	blockNumElem, addr[_HEAP_BLOCK_NUM_ELEMENTS]
	#call	_heapComputeBlockSize(addr)
	// Allocate the block list and fail on alloc error.
	#call	newVector(blockListSize)
	store	r0, addr[_HEAP_BLOCK_LIST]
	// Expand heap_block_list to maximum size
	#call	maximizeVector(r0)
	// Compute capacity of allocated block list (may be larger than requested)
	#call	_heapComputeCapacity(addr)
	move	r0, addr[_HEAP_MUTEX]
	#call	initializeMutex(r0)
	#return	addr
#end_func

///////////////////////////////////////////////////////////////////////////////
// heapResize(heap, size)
// Resizes the heap to either a smaller or larger capacity, truncating or 
// adding blocks as needed.
///////////////////////////////////////////////////////////////////////////////
#def_func heapResize(heap, size)
	#var	a, c, ff, h, i, \
			blockList, \
			blockListSize, \
			blockNumElem, \
			defaultBlockListSize, \
			newSize, \
			oldBlockListSize
	load	h, heap
	load	newSize, size
//#call debug(STDOUT, "heapResize(%x, %d)\n", h, newSize)
	load	ff, h[_HEAP_DESTROY_FUNCTION]
	#sync	h[_HEAP_MUTEX]
		load	blockList, h[_HEAP_BLOCK_LIST]
		#call	vectorSize(blockList)
		move	oldBlockListSize, r0
//#call debug(STDOUT, "vectorSize(%x)=%d\n", blockList, oldBlockListSize)
		load	blockNumElem, h[_HEAP_BLOCK_NUM_ELEMENTS]
		// Compute the new size of the block list as newSize/blockNumElem.
		// Plus 1 block if remainder to account for partial block.
		div		blockListSize, r0, newSize, blockNumElem
		#if_cond	r0, ne, 0
			add		blockListSize, 1
		#end_cond
		// Make sure blockListSize is not below minimum size.
		load	r0, _HEAP_DEFAULT_BLOCK_LIST_SIZE
		#macro	MAX(r0, blockListSize)
		move	blockListSize, r0
//#call debug(STDOUT, "newBlockListSize=%d\n", blockListSize)
		
		// Free blocks that are now completely unused.
		#if_cond	blockListSize, lt, oldBlockListSize
//#call debug(STDOUT, "Freeing blocks...%d to %d", blockListSize, oldBlockListSize)
			#for	blockListSize, i, lt, oldBlockListSize, 1
				#call	_heapFreeBlock(h, i)
			#end_for
		#end_cond
		// Make the block list new size and fail on alloc error.
//#call debug(STDOUT, "vectorResize(%x, %d)\n", blockList, blockListSize)
		#call	resizeVector(blockList, blockListSize)
		// Expand heap_block_list to maximum size
//#call debug(STDOUT, "vectorMaximize(%x)\n", blockList)
		#call	maximizeVector(blockList)
		// Save the block list and compute new capacity.
		#call	_heapComputeCapacity(h)
	#end_sync
#end_func

///////////////////////////////////////////////////////////////////////////////
// _heapComputeBlockSize(heap)
// Computes the block size of the heap.  Will recompute the block number of 
// elements based on the minimum iALLOC size for the block thus increasing it
// slightly to make use of all available space in the block allocation.
// heap		heap to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func _heapComputeBlockSize(heap)
	#var	h, num, size
	load	h, heap
	load	num, h[_HEAP_BLOCK_NUM_ELEMENTS]
	load	size, h[_HEAP_BLOCK_ELEMENT_SIZE]
	mult	r0, num, size
	#call	miniumAllocSize(r0)
	// Determine number of elements that will fit in the min alloc size.
	div		num, r0, size
	store	num, h[_HEAP_BLOCK_NUM_ELEMENTS]
	// Compute and store the new block size.
	mult	r0, num, size
	store	r0, h[_HEAP_BLOCK_SIZE]
#end_func

///////////////////////////////////////////////////////////////////////////////
// _heapAddBlock(heap, index)
// Allocates a block for index in block list.  Initializes the elements of the
// block using the init. function specified when creating the heap.
// heap		heap to operate on.
// index	index of block to add
///////////////////////////////////////////////////////////////////////////////
#def_func _heapAddBlock(heap, index)
	#var	a, h, i, \
			aLimit, \
			block, \
			blockElementSize, \
			blockList, \
			blockListSize, \
			blockSize, \
			init
	load	h, heap
	load	i, index
	load	init, h[_HEAP_INIT_FUNCTION]
	clear	block
	#sync	h[_HEAP_MUTEX]
		load	blockList, h[_HEAP_BLOCK_LIST]
		#call	vectorSize(blockList)
		move	blockListSize, r0
		// Check to make sure index is in the block list.
		#macro	COMPARE_RANGE(0, le, i, lt, blockListSize)
		#if_cond_sr	nz
			#call	getVectorAt(blockList, i)
			move	block, r0
			#if_cond_sr	z
				// Allocate a new block and fail on alloc error.
				load	blockSize, h[_HEAP_BLOCK_SIZE]
				#macro	ALLOC(blockSize)
				move	block, r0
				#macro	TO_NOT_BOOLEAN(block)
				#call	cond_fatal(r0, STDOUT, "Can\'t allocate new heap block %d!\n", i)
				// Store the block in the block list.
				#call	setVectorAt(blockList, block, i)
				// If the init function is non-null, use it to initialize the
				// elements in the block.
				#if_cond	init, ne, 0
					load	blockElementSize, h[_HEAP_BLOCK_ELEMENT_SIZE]
					load	r0, h[_HEAP_BLOCK_SIZE]
					add		aLimit, block, r0
					#for	block, a, lt, aLimit, blockElementSize
						push	a
						call	init
						add		sp, 1
					#end_for
				#end_cond
			#end_cond
		#else_cond
			#call	cond_fatal(TRUE, STDOUT, "Illegal allocate new heap block %d!\n", i)
		#end_cond
	#end_sync
	#return	block
#end_func

///////////////////////////////////////////////////////////////////////////////
// _heapFreeBlock(heap, index)
// Frees the heap block specified by index.  Calls the destroy function on each
// element in the freed block.  If the specified block is null, does nothing.
// heap		heap to operate on.
// index	index of block to add
///////////////////////////////////////////////////////////////////////////////
#def_func _heapFreeBlock(heap, index)
	#var	a, ff, h, i, \
			aLimit, \
			block, \
			blockElementSize, \
			blockList, \
			blockListSize
	load	h, heap
	load	i, index
	load	ff, h[_HEAP_DESTROY_FUNCTION]
	#sync	h[_HEAP_MUTEX]
		load	blockList, h[_HEAP_BLOCK_LIST]
		#call	vectorSize(blockList)
		move	blockListSize, r0
		load	blockElementSize, h[_HEAP_BLOCK_ELEMENT_SIZE]
		// Check to make sure index is in the block list.
		#macro	COMPARE_RANGE(0, le, i, lt, blockListSize)
		#if_cond_sr	nz
			#call	getVectorAt(blockList, i)
			move	block, r0
			#if_cond	block, ne, 0
				// Clear the block list at index
				#call	setVectorAt(blockList, 0, i)
				// If the destroy function is non-null, use it to destroy
				// the elements in the block.
				#if_cond	ff, ne, 0
					load	r0, h[_HEAP_BLOCK_SIZE]
					add		aLimit, block, r0
					#for	block, a, lt, aLimit, blockElementSize
						push	a
						call	ff
						add		sp, 1
					#end_for
				#end_cond
				// Finally free the block allocation
				#macro	FREE(block)
			#end_cond
		#else_cond
			#call	cond_fatal(TRUE, STDOUT, "Illegal free heap block %d!\n", i)
		#end_cond_sr
	#end_sync
#end_func

///////////////////////////////////////////////////////////////////////////////
// heapFree(heap)
// Deallocates the heap.
// heap		heap to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func heapFree(heap)
	#var	i, h, blockList, blockListSize
	load	h, heap
	#sync	h[_HEAP_MUTEX]
		load	blockList, h[_HEAP_BLOCK_LIST]
		#call	vectorSize(blockList)
		move	blockListSize, r0
		#for	0, i, lt, blockListSize, 1
			#call	_heapFreeBlock(h, i)
		#end_for
		#call	freeVector(blockList)
		store	0, h[_HEAP_LEN]
		store	0, h[_HEAP_BLOCK_LIST]
		#macro	FREE(h)
	#end_sync
#end_func

///////////////////////////////////////////////////////////////////////////////
// heapSize(heap)
// Size of the heap. Corresponds to the highest index used to access the heap.
// heap		heap to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func heapSize(heap)
	#var	h, len
	load	h, heap
	#sync	h[_HEAP_MUTEX]
		load	len, h[_HEAP_LEN]
	#end_sync
	#return	len
#end_func

///////////////////////////////////////////////////////////////////////////////
// heapIsEmpty(heap)
// Returns TRUE if the heap is empty, i.e. no elements have been accessed.
// heap		heap to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func heapIsEmpty(heap)
	#var	h, len
	load	h, heap
	#sync	h[_HEAP_MUTEX]
		load	len, h[_HEAP_LEN]
	#end_sync
	#macro	TO_NOT_BOOLEAN(len)
#end_func

///////////////////////////////////////////////////////////////////////////////
// heapCapacity(heap)
// Capacity of the heap. Corresponds to the largest index that can be used
// without growing the heap.
// heap		heap to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func heapCapacity(heap)
	#var	h, size
	load	h, heap
	#sync	h[_HEAP_MUTEX]
		load	size, h[_HEAP_CAPACITY]
	#end_sync
	#return	size
#end_func

///////////////////////////////////////////////////////////////////////////////
// _heapComputeCapacity(heap)
// Computes the heap capacity based on the actual size allocated for the 
// block list.
// heap		heap to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func _heapComputeCapacity(heap)
	#var	h, size, blockList, blockNumElem
	load	h, heap
	clear	size
	#sync	h[_HEAP_MUTEX]
		load	blockList, h[_HEAP_BLOCK_LIST]
		#if_cond	blockList, ne, 0
			#call	vectorSize(blockList)
			move	size, r0
			load	blockNumElem, h[_HEAP_BLOCK_NUM_ELEMENTS]
			mult	size, blockNumElem
		#end_cond
		store	size, h[_HEAP_CAPACITY]
	#end_sync
	#return	size
#end_func

///////////////////////////////////////////////////////////////////////////////
// _heapIndexToBlockNum(heap, index)
// Computes the block number for a given heap element.
// heap		heap to operate on.
// index	element index into the heap.
///////////////////////////////////////////////////////////////////////////////
#def_func _heapIndexToBlockNum(heap, index)
	#var	h, i, blockNumElem, blockNum
	load	h, heap
	load	i, index
	load	blockNumElem, h[_HEAP_BLOCK_NUM_ELEMENTS]
	div		blockNum, i, blockNumElem
	#return	blockNum
#end_func

///////////////////////////////////////////////////////////////////////////////
// heapGetNextIndex(heap)
// Returns the index of the next available heap element.  Allocates the element
// if necessary.
// heap		heap to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func heapGetNextIndex(heap)
	#var	h, len, result
	load	h, heap
	#sync	h[_HEAP_MUTEX]
		load	len, h[_HEAP_LEN]
		#call	heapElementAt(h, len)
		move	result, len
	#end_sync
	#return	result
#end_func

///////////////////////////////////////////////////////////////////////////////
// heapGetNextElement(heap)
// Returns the address of the next available heap element.  Allocates the
// element if necessary.
// heap		heap to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func heapGetNextElement(heap)
	#var	h, len, result
	load	h, heap
	#sync	h[_HEAP_MUTEX]
		load	len, h[_HEAP_LEN]
		#call	heapElementAt(h, len)
		move	result, r0
	#end_sync
	#return	result
#end_func

///////////////////////////////////////////////////////////////////////////////
// heapElementAt(heap, index)
// Computes the address for a given heap element.  If that heap element is in
// a block that hasn't been allocated, will allocate and initialize the block.
// If the index is greater than the capacity, the capacity will increase.
// Address is returned in r0.
// heap		heap to operate on.
// index	element index into the heap.
///////////////////////////////////////////////////////////////////////////////
#def_func heapElementAt(heap, index)
	#var	s, h, i, len, \
			addr, \
			block, \
			blockElemSize, \
			blockList, \
			blockNum, \
			blockNumElem, \
			blockOffset, \
			capacity
	load	h, heap
	#sync	h[_HEAP_MUTEX]
		load	i, index
		load	len, h[_HEAP_LEN]
		load	capacity, h[_HEAP_CAPACITY]
		load	blockNumElem, h[_HEAP_BLOCK_NUM_ELEMENTS]
		// Resize the heap if index >= capacity.
		#if_cond	i, ge, capacity
			add		s, i, 1
			#call	heapResize(h, s)
			load	capacity, h[_HEAP_CAPACITY]
		#end_cond
		// Check to make sure index is in the block list now.
		#macro	COMPARE_RANGE(0, le, i, lt, capacity)
		#if_cond_sr	nz
			// Compute the block num and the index into the block.
			div		blockNum, blockOffset, i, blockNumElem
			load	blockList, h[_HEAP_BLOCK_LIST]
			// Load the block and add a new one if null
			#call	getVectorAt(blockList, blockNum)
			move	block, r0
			#if_cond_sr	z
				#call	_heapAddBlock(h, blockNum)
				move	block, r0
			#end_cond
			// Compute the offset into the block for the element
			load	blockElemSize, h[_HEAP_BLOCK_ELEMENT_SIZE]
			mult	blockOffset, blockElemSize
			// Compute the actual address of the element.
			move	addr, block[blockOffset]
			// Adjust the lenth of the heap if this index is larger than len.
			add		i, 1
			#macro	MAX(i, len)
			store	r0, h[_HEAP_LEN]
		#else_cond
			#call	cond_fatal(TRUE, STDOUT, "Heap element invalid %d!\n", i)
		#end_cond
	#end_sync
	#return	addr
#end_func

///////////////////////////////////////////////////////////////////////////////
// heapSetElementAt(heap, index)
// Sets the data for a given heap element.  If that heap element is in
// a block that hasn't been allocated, will allocate and initialize the block.
// If the index is greater than the capacity, the capacity will increase.
// Calls the destroy function on the heap element before the copy.
// heap		heap to operate on.
// newElementAddr	Address of struct to copy into the heap at index.
// index	element index into the heap.
///////////////////////////////////////////////////////////////////////////////
#def_func heapSetElementAt(heap, newElementAddr, index)
	#var	h, i, ff, nea, \
			blockElementSize, \
			elementAddr
	load	h, heap
	load	i, index
	load	nea, newElementAddr
	#sync	h[_HEAP_MUTEX]
		#call	heapElementAt(h, i)
		move	elementAddr, r0
		load	blockElementSize, h[_HEAP_BLOCK_ELEMENT_SIZE]
		load	ff, h[_HEAP_DESTROY_FUNCTION]
		#if_cond	ff, ne, 0
			push	elementAddr
			call	ff
			add		sp, 1
		#end_cond
		#macro	MEMMOVE(elementAddr, nea, blockElementSize)
	#end_sync
#end_func

///////////////////////////////////////////////////////////////////////////////
// _heapPrintBlock(heap, index, toStringFunc)
// Prints a block of heap elements using the toStringFunc function.
// heap		heap to operate on.
// index	element index into the heap.
// toStringFunc	Stack-based function that returns a string for the element
//			address passed on the stack.
///////////////////////////////////////////////////////////////////////////////
#def_func _heapPrintBlock(heap, index, toStringFunc)
	#var	s, a, h, i, pf, \
			aLimit, \
			block, \
			blockElementSize, \
			blockList
	load	h, heap
	load	i, index
	load	pf, toStringFunc
	load	blockList, h[_HEAP_BLOCK_LIST]
	load	blockElementSize, h[_HEAP_BLOCK_ELEMENT_SIZE]
	#call	getVectorAt(blockList, i)
	move	block, r0
	#if_cond	block, gt, 0
		load	r0, h[_HEAP_BLOCK_SIZE]
		add		aLimit, block, r0
		#for	block, a, lt, aLimit, blockElementSize
			#if_cond	a, ne, block
				#call	putc(',')
			#end_cond
			#if_cond	pf, ne, 0
				push	a
				call	pf
				add		sp, 1
				move	s, r0
				#call	puts(s)
				#call	free(s)
			#else_cond
				load	i, a
				#call	put_dec(i)
			#end_cond
		#end_for
	#else_cond
		#call	puts("null")
	#end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// heapPrint(heap, toStringFunc)
// Prints a heap using the toStringFunc function.
// heap		heap to operate on.
// toStringFunc	Stack-based function that returns a string for the element
//			address passed on the stack.
///////////////////////////////////////////////////////////////////////////////
#def_func heapPrint(heap, toStringFunc)
	#var	h, i, len, pf
	load	h, heap
	load	pf, toStringFunc
	#sync	h[_HEAP_MUTEX]
		load	len, h[_HEAP_LEN]
		sub		len, 1
		#call	_heapIndexToBlockNum(h, len)
		move	len, r0
		#macro	MAX(len, 0)
		move	len, r0
		#for	0, i, le, len, 1
			#if_cond	i, ne, 0
				#call	putc(';')
			#end_cond
			#call	_heapPrintBlock(h, i, pf)
		#end_for
		#call	put_nl()
	#end_sync
#end_func
HEAP_ASM_END: nop
