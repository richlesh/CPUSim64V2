#include <system/io.asm>
#include <system/system.asm>
#include <adt/heap.asm>

	jump	@LINKEDLIST_ASM_END

#define	_LINKED_LIST_LEN	0
#define	_LINKED_LIST_HEAD	1
#define	_LINKED_LIST_TAIL	2
#define _LINKED_LIST_NODES	3		// Heap of nodes
#define	_LINKED_LIST_MUTEX	4
#define	_LINKED_LIST_END	5

#define	_LINKED_LIST_NODE_PREV	0
#define	_LINKED_LIST_NODE_NEXT	1
#define	_LINKED_LIST_NODE_DATA	2
#define _LINKED_LIST_NODE_END	3

#def_func newLinkedList()
	#var	addr, nodes
	ALLOC(_LINKED_LIST_END)
	move	addr, r0
	store	0, addr[_LINKED_LIST_LEN]
	store	0, addr[_LINKED_LIST_MUTEX]
	#call	newHeap(10, _LINKED_LIST_NODE_END, 0, 0)
	move	nodes, r0
	store	nodes, addr[_LINKED_LIST_NODES]
	TO_NOT_BOOLEAN(nodes)
	#call	cond_fatal(r0, STDOUT, "Can\'t allocate new linked list nodes!\n")
	store	0, addr[_LINKED_LIST_HEAD]
	store	0, addr[_LINKED_LIST_TAIL]
	#return	addr
#end_func

#def_func _linkedListLinkNodes(firstNode, secondNode)
	#var	a, b
	load	a, firstNode
	load	b, secondNode
	#cond	a, eq, 0
		store	0, b[_LINKED_LIST_NODE_PREV]
	#elseifcond b, eq, 0
		store	0, a[_LINKED_LIST_NODE_NEXT]
	#elsecond
		store	b, a[_LINKED_LIST_NODE_NEXT]
		store	a, b[_LINKED_LIST_NODE_PREV]
	#endcond
	#endcond
#end_func

#def_func _linkedListInsertNode(afterThisNode, node)
	#var	n, a, b
	load	n, node
	load	a, afterThisNode
	load	b, a[_LINKED_LIST_NODE_NEXT]
	#call	_linkedListLinkNodes(a, n)
	#call	_linkedListLinkNodes(n, b)
#end_func

#def_func _linkedListRemoveNode(node)
	#var	n, a, b
	load	n, node
	load	a, n[_LINKED_LIST_NODE_PREV]
	load	b, n[_LINKED_LIST_NODE_NEXT]
	#call	_linkedListLinkNodes(ll, a, b)
	#return	n
#end_func

#def_func linkedListFree(list)
	#var	ll, nodes
	load	ll, list
	load	nodes, ll[_LINKED_LIST_NODES]
	#call	heapFree(nodes)
	store	0, ll[_LINKED_LIST_NODES]
	FREE(list)
#end_func

#def_func linkedListSize(list)
	#var	ll, len
	load	ll, list
	#sync	ll[_LINKED_LIST_MUTEX]
		load	len, ll[_LINKED_LIST_LEN]
	#endsync
	#return	len
#end_func

#def_func linkedListIsEmpty(list)
	#var	ll
	load	ll, list
	#call	linkedListSize(ll)
	TO_NOT_BOOLEAN(r0)
#end_func

#def_func linkedListNodeAt(list, index)
	#var	ll, i, j, p
	load	ll, list
	#sync	ll[_LINKED_LIST_MUTEX]
		load	i, index
		load	len, ll[_LINKED_LIST_LEN]
		load	p, ll[_LINKED_LIST_HEAD]
		COMPARE_RANGE(0, le, i, lt, len)
		#cond_sr	nz
			#for	j, 0, lt, i, 1
				load	p, p[_LINKED_LIST_NODE_NEXT]
			#endfor
		#end_cond_sr
	#endsync
	#return	p
#end_func

#def_func linkedListAt(list, index)
	#var	ll, i, p, value
	load	ll, list
	load	i, index
	move	value, 0
	#sync	ll[_LINKED_LIST_MUTEX]
		#call	linkedListNodeAt(ll, i)
		move	p, r0
		#cond_sr	nz
			load	value, p[_LINKED_LIST_NODE_DATA]
		#endcond
	#endsync
	#return	value
#end_func

#def_func linkedListSetAt(list, value, index)
	#var	ll, i, p, v
	load	ll, list
	load	i, index
	load	v, value
	#sync	ll[_LINKED_LIST_MUTEX]
		#call	linkedListNodeAt(ll, i)
		move	p, r0
		#cond_sr	nz
			store	v, p[_LINKED_LIST_NODE_DATA]
		#endcond
	#endsync
#end_func

#def_func linkedListHeadNode(list)
	#var	ll, p
	load	ll, list
	load	r0, ll[_LINKED_LIST_HEAD]
#end_func

#def_func linkedListTailNode(list)
	#var	ll, p
	load	ll, list
	load	r0, ll[_LINKED_LIST_TAIL]
#end_func

#def_func linkedListAddToHead(list, value)
	#var	ll, v, head, nodes, newNode, len
	load	ll, list
	load	v, value
	load	head, ll[_LINKED_LIST_HEAD]
	load	nodes, ll[_LINKED_LIST_NODES]
	#call	heapGetNextElement(nodes)
	move	newNode, r0
	store	v, newNode[_LINKED_LIST_NODE_DATA]
	store	newNode, ll[_LINKED_LIST_HEAD]
	#call	_linkedListLinkNodes(newNode, head)
	load	len, ll[_LINKED_LIST_LEN]
	add		len, 1
	store	len, ll[_LINKED_LIST_LEN]
	#cond	len, eq, 1
		store	newNode, ll[_LINKED_LIST_TAIL]
	#endcond
#end_func

#def_func linkedListAddToTail(list, value)
	#var	ll, v, tail, nodes, newNode
	load	ll, list
	load	v, value
	load	tail, ll[_LINKED_LIST_TAIL]
	load	nodes, ll[_LINKED_LIST_NODES]
	#call	heapGetNextElement(nodes)
	move	newNode, r0
	store	v, newNode[_LINKED_LIST_NODE_DATA]
	store	newNode, ll[_LINKED_LIST_TAIL]
	#call	_linkedListLinkNodes(tail, newNode)
	load	len, ll[_LINKED_LIST_LEN]
	add		len, 1
	store	len, ll[_LINKED_LIST_LEN]
	#cond	len, eq, 1
		store	newNode, ll[_LINKED_LIST_HEAD]
	#endcond
#end_func

#def_func linkedListRemoveNodeFromHead(list)
	#var	ll, p, head, len
	load	ll, list
	load	len, ll[_LINKED_LIST_LEN]
	#cond	len, eq, 0
		move	r0, 0
	#elsecond
		load	head, ll[_LINKED_LIST_HEAD]
		load	p, head[_LINKED_LIST_NODE_NEXT]
		store	p, ll[_LINKED_LIST_HEAD]
		sub		len, 1
		store	len, ll[_LINKED_LIST_LEN]
		#cond	len, le, 1
			store	p, ll[_LINKED_LIST_TAIL]
		#endcond
		#return	head
	#endcond
#end_func

#def_func linkedListRemoveNodeFromTail(list)
	#var	ll, p, v, tail, len
	load	ll, list
	load	len, ll[_LINKED_LIST_LEN]
	#cond	len, eq, 0
		move	r0, 0
	#elsecond
		load	tail, ll[_LINKED_LIST_TAIL]
		load	p, tail[_LINKED_LIST_NODE_PREV]
		store	p, ll[_LINKED_LIST_TAIL]
		sub		len, 1
		store	len, ll[_LINKED_LIST_LEN]
		#cond	len, le, 1
			store	p, ll[_LINKED_LIST_HEAD]
		#endcond
		#return	tail
	#endcond
#end_func

#def_func linkedListAddAfter(list, value, index)
	#var	ll, i, p, v, len, next, newNode, nodes
	load	ll, list
	load	i, index
	load	v, value
	load	len, ll[_LINKED_LIST_LEN]
	sub		len, 1
	#sync	v[_VECTOR_MUTEX]
		#cond	i, le, -1
			linkedListAddToHead(ll, v)
		#elsecond	i, ge, len
			linkedListAddToTail(ll, v)
		#elsecond
			linkedListNodeAt(ll, i)
			move	p, r0
			load	nodes, ll[_LINKED_LIST_NODES]
			#call	heapGetNextElement(nodes)
			move	newNode, r0
			store	v, newNode[_LINKED_LIST_NODE_DATA]
			_linkedListInsertNode(p, newNode)
			load	len, ll[_LINKED_LIST_LEN]
			add		len, 1
			store	len, ll[_LINKED_LIST_LEN]
		#endcond
		#endcond
	#endsync
#end_func

#def_func linkedListRemoveNodeAt(list, index)
	#var	ll, i, p, len
	load	ll, list
	load	i, index
	load	len, ll[_LINKED_LIST_LEN]
	sub		len, 1
	#sync	v[_VECTOR_MUTEX]
		#cond	i, le, 0
			linkedListRemoveNodeFromHead(ll)
			move	p, r0
		#elsecond	i, ge, len
			linkedListRemoveNodeFromTail(ll)
			move	p, r0
		#elsecond
			_linkedListRemoveNode(ll, i)
			move	p, r0
			load	len, ll[_LINKED_LIST_LEN]
			sub		len, 1
			store	len, ll[_LINKED_LIST_LEN]
		#endcond
		#endcond
	#endsync
	#return	p
#end_func

#def_func linkedListClear(list)
	#var	ll
	load	ll, list
	#sync	ll[_LINKED_LIST_MUTEX]
		store	0, ll[_LINKED_LIST_LEN]
		store	0, ll[_LINKED_LIST_HEAD]
		store	0, ll[_LINKED_LIST_TAIL]
	#endsync
#end_func

#def_func linkedListIndexOf(list, value, index)
	#var	v, i, j, p, len, data
	load	ll, list
	load	v, value
	load	i, index
	#sync	ll[_LINKED_LIST_MUTEX]
		// TODO check for empty list
		load	len, ll[_LINKED_LIST_LEN]
		#call	linkedListNodeAt(ll, i)
		move	p, r0
		#for	j, i, lt, len, 1
			load	data, p[_LINKED_LIST_NODE_DATA]
			#cond	data, eq, v
				#break
			#endcond
			load	p, p[_LINKED_LIST_NODE_NEXT]
		#endfor
	#endsync
	#cond	j, eq, len
		#return	-1
	#elsecond
		#return	j
	#endcond
#end_func

#def_func linkedListLastIndexOf(vector, value, index)
	#var	v, i, j, p, len, data
	load	ll, list
	load	v, value
	load	i, index
	#sync	ll[_LINKED_LIST_MUTEX]
		// TODO check for empty list
		load	len, ll[_LINKED_LIST_LEN]
		#call	linkedListNodeAt(ll, i)
		move	p, r0
		#for	j, i, ge, 0, -1
			load	data, p[_LINKED_LIST_NODE_DATA]
			#cond	data, eq, v
				#break
			#endcond
			load	p, p[_LINKED_LIST_NODE_PREV]
		#endfor
	#endsync
	#return	j
#end_func

#def_func linkedListPrint(vector)
	#var	v, i, p, len, data
	load	ll, list
	load	v, value
	load	i, index
	#sync	ll[_LINKED_LIST_MUTEX]
		// TODO check for empty list
		load	len, ll[_LINKED_LIST_LEN]
		load	p, ll[_LINKED_LIST_HEAD]
		#for	i, 0, lt, len, 1
			load	data, p[_LINKED_LIST_NODE_DATA]
			#cond	i, ne, 0
				#call	putc(',')
			#endcond
			load	data, p[_LINKED_LIST_NODE_DATA]
			#call	put_dec(data)
			load	p, p[_LINKED_LIST_NODE_NEXT]
		#endfor
		#call	put_nl()
	#endsync
#end_func
LINKEDLIST_ASM_END: nop
