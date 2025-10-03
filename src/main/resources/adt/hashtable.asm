#include <system/io.asm>
#include <system/system.asm>

	jump	@HASHTABLE_ASM_END

#define	_HASHTABLE_LEN			0
#define	_HASHTABLE_DATA			1
#define	_HASHTABLE_MUTEX		2
#define	_HASHTABLE_COUNT		3
#define	_HASHTABLE_HASH_FUNC	4
#define	_HASHTABLE_END			5

#def_func newHashTable(size)
	#var	s, addr
	load	s, size
	#cond	s, lt, 10
		move	s, 10
	#endcond
	ALLOC(_VECTOR_END)
	move	addr, r0
	store	0, addr[_VECTOR_LEN]
	store	0, addr[_VECTOR_MUTEX]
	ALLOC(s)
	store	r0, addr[_VECTOR_DATA]
	TO_NOT_BOOLEAN(r0)
	#call	cond_fatal(r0, STDOUT, "Can\'t allocate new vector size %d!\n", s)
	#return	addr
#end_func

#def_func hashTableResize(vector, size)
	#var	v, data, newSize
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		load	newSize, size
		REALLOC(data, newSize)
		store	r0, v[_VECTOR_DATA]
		TO_NOT_BOOLEAN(r0)
		#call	cond_fatal(r0, STDOUT, "Can\'t allocate new vector size %d!\n", newSize)
	#endsync
#end_func

#def_func hashTableFree(vector)
	#var	v, data
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		FREE(data)
		store	0, v[_VECTOR_LEN]
		store	0, v[_VECTOR_DATA]
		FREE(v)
	#endsync
#end_func

#def_func hashTableSize(vector)
	#var	v, len
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	len, v[_VECTOR_LEN]
	#endsync
	#return	len
#end_func

#def_func hashTableIsEmpty(vector)
	#var	v, len
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	len, v[_VECTOR_LEN]
	#endsync
	TO_NOT_BOOLEAN(len)
#end_func

#def_func hashTableCapacity(vector)
	#var	v, data, size
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		load	size, data[-1]
	#endsync
	#return	size
#end_func

#def_func hashTableGet(vector, index)
	#var	v, i, value, len, data
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	i, index
		load	len, v[_VECTOR_LEN]
		load	data, v[_VECTOR_DATA]
		move	value, 0
		COMPARE_RANGE(0, le, i, lt, len)
		#cond_sr	nz
			load	value, data[i]
		#end_cond_sr
	#endsync
	#return	value
#end_func

#def_func hashTablePut(vector, value, index)
	#var	v, i, val, len, data
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	val, value
		load	i, index
		load	len, v[_VECTOR_LEN]
		load	data, v[_VECTOR_DATA]
		COMPARE_RANGE(0, le, i, lt, len)
		#cond_sr	nz
			store	val, data[i]
		#end_cond_sr
	#endsync
#end_func

#def_func hashTableClear(vector)
	#var	v
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		store	0, v[_VECTOR_LEN]
	#endsync
#end_func

#def_func hashTablePrint(vector)
	#var	v, i, j, len, data
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	len, v[_VECTOR_LEN]
		load	data, v[_VECTOR_DATA]
		#for	i, 0, lt, len, 1
			#cond	i, ne, 0
				#call	putc(',')
			#endcond
			load	j, data[i]
			#call	put_dec(j)
		#endfor
		#call	put_nl()
	#endsync
#end_func

#def_func	hashString(str)
#end_func

HASHTABLE_ASM_END: nop
