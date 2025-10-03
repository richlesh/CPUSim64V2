#include <adt/heap.asm>
#include <system/io.asm>
#include <system/string.asm>
#include <system/system.def>

	#call	main()
	int		iEXIT

SIZE: dci	10
#define	STRUCT_A	0
#define STRUCT_B	1
#define STRUCT_C	2
#define STRUCT_END	3

#def_func structToString(addr)
	#var 	sa, a, b, c
	load	sa, addr
	load	a, sa[STRUCT_A]
	load	b, sa[STRUCT_B]
	load	c, sa[STRUCT_C]
	#call	sprintf("[%d,%d,%d]", a, b, c)
#end_func

#def_func structInit(addr)
	#var 	sa
	load	sa, addr
//#call	debug(STDOUT, "Init %d...\n", sa)
	store	1, sa[STRUCT_A]
	store	2, sa[STRUCT_B]
	store	3, sa[STRUCT_C]
#end_func

#def_func structDestroy(addr)
	#var	sa
	load	sa, addr
//#call	debug(STDOUT, "Destroying %d...\n", sa)
#end_func

#def_func	main()
	#var	i, j, k, h, s, struct
	
	store	2, _HEAP_DEFAULT_BLOCK_LIST_SIZE
	store	12, _HEAP_DEFAULT_BLOCK_NUM_ELEMENTS

	load	s, SIZE
	#call	fprintf(STDOUT, "Allocating heap size %d\n", s)
	#call	newHeap(s, STRUCT_END, structInit, structDestroy)
	move	h, r0
	#call	heapStats(h)
	#call	heapPrint(h, structToString)

	move	s, 1000
	#call	fprintf(STDOUT, "Resizing heap size %d\n", s)
	#call	heapResize(h, s)
	#call	heapStats(h)
	#call	heapPrint(h, structToString)

	move	s, 2000
	#call	fprintf(STDOUT, "Resizing heap size %d\n", s)
	#call	heapResize(h, s)
	#call	heapStats(h)
	#call	heapPrint(h, structToString)

	#call	puts("Trimming heap to 500\n")
	#call	heapResize(h,500)
	#call	heapStats(h)
	#call	heapPrint(h, structToString)

	ALLOC(STRUCT_END)
	move	struct, r0
	#call	puts("Add 20...\n")
	#for	i, 0, lt, 20, 1
		store	i, struct[STRUCT_A]
		store	i, struct[STRUCT_B]
		store	i, struct[STRUCT_C]
		#call	heapSetElementAt(h, struct, i)
	#endfor
	#call	heapStats(h)
	#call	heapPrint(h, structToString)

	#call	puts("Add 100...\n")
	#for	i, 20, lt, 120, 1
		#call	heapGetNextIndex(h)
		move	k, r0
		store	i, struct[STRUCT_A]
		store	i, struct[STRUCT_B]
		store	i, struct[STRUCT_C]
		#call	heapSetElementAt(h, struct, k)
	#endfor
	#call	heapStats(h)
	#call	heapPrint(h, structToString)

	#call	puts("Add @181...\n")
	store	181, struct[STRUCT_A]
	store	181, struct[STRUCT_B]
	store	181, struct[STRUCT_C]
	#call	heapSetElementAt(h, struct, 181)
	#call	heapStats(h)
	#call	heapPrint(h, structToString)

	#call	puts("Trim 0...\n")
	#call	heapResize(h, 0)
	#call	heapStats(h)

	#call	puts("Adding 10000...\n")
	int		iCLOCK
	move	j, r0
	#for	i, 0, lt, 10000, 1
		#call	heapGetNextElement(h)
		store	i, r0[STRUCT_A]
		store	i, r0[STRUCT_B]
		store	i, r0[STRUCT_C]
	#endfor
	int		iCLOCK
	sub		j, r0, j
	#call	fprintf(STDOUT, "Time: %d\n", j)
	#call	heapStats(h)

	#call	puts("Freeing...\n")
	#call	heapFree(h)
	#call	heapStats(h)

	#return	0
#end_func

#def_func heapStats(heap)
	#var	h, len, size, blockList, blockNumElements, isEmpty
	load	h, heap
	#call	heapIsEmpty(h)
	move	isEmpty, r0
	#call	heapSize(h)
	move	len, r0
	#call	heapCapacity(h)
	move	size, r0
	load	blockList, h[_HEAP_BLOCK_LIST]
	load	blockNumElements, h[_HEAP_BLOCK_NUM_ELEMENTS]
	#call	fprintf(STDOUT, "Heap(%d): %x %d (%d) [%d]\n", isEmpty, blockList, len, size, blockNumElements)
#end_func

	stop
	stop
