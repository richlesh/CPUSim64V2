#include <system/system.def>

jump	@THREAD_ASM_END

// Returns thread ID of the new thread in R0
#def_func	threadCreate(entryPoint, data)
	load	r0, entryPoint
	load	r1, data
	int		iTHREAD_CREATE
#end_func

#def_func	threadJoin(threadId)
	load	r0, threadId
	int		iTHREAD_JOIN
#end_func

#def_func	createLatch(maxCount)
	int		iTHREAD_EXIT

THREAD_ASM_END:	nop
