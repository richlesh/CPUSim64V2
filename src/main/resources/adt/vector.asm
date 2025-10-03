#include <system/io.asm>
#include <system/math.def>
#include <system/system.asm>

	jump	@VECTOR_ASM_END
///////////////////////////////////////////////////////////////////////////////
// Vector
// Implements a growable list of integers.  Is thread safe.
///////////////////////////////////////////////////////////////////////////////

#define	_VECTOR_LEN		0		// Number of elements used
#define	_VECTOR_DATA	1		// Heap allocated block for the vector data
#define	_VECTOR_MUTEX	2		// Mutex used for concurrent access
#define	_VECTOR_END		3
_VECTOR_SIZE_FACTOR: dcf 1.2	// Ratio to increase data block size.
_VECTOR_MIN_SIZE: dci 8			// Smallest vector size

///////////////////////////////////////////////////////////////////////////////
// newVector(size)
// Creates a new vector object and returns its address.  Vectors are contiguous
// lists of integers that can grow when needed.
// initialCapacity	capacity to start the vector.
///////////////////////////////////////////////////////////////////////////////
#def_func newVector(initialCapacity)
	#var	s, addr
	load	s, initialCapacity
	ALLOC(_VECTOR_END)
	move	addr, r0
	store	0, addr[_VECTOR_LEN]
	store	0, addr[_VECTOR_MUTEX]
	// Allocate some extra
	load	r0, _VECTOR_MIN_SIZE
	MAX(r0, s)
	move	s, r0
	load	f0, _VECTOR_SIZE_FACTOR
	mult	f0, s
	move	s, f0
	ALLOC(s)
	store	r0, addr[_VECTOR_DATA]
	TO_NOT_BOOLEAN(r0)
	#call	cond_fatal(r0, STDOUT, "Can\'t allocate new vector size %d!\n", s)
	#return	addr
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorResize(vector, size)
// Resizes a vector object to a new size.  Size can be larger or smaller.
// vector	vector to operate on.
// size		new number of elements.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorResize(vector, size)
	#var	v, data, newSize, len
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		load	newSize, size
		// Allocate some extra
		load	r0, _VECTOR_MIN_SIZE
		MAX(r0, newSize)
		load	f0, _VECTOR_SIZE_FACTOR
		mult	f0, r0
		move	newSize, f0
		REALLOC(data, newSize)
		store	r0, v[_VECTOR_DATA]
		TO_NOT_BOOLEAN(r0)
		#call	cond_fatal(r0, STDOUT, "Can\'t allocate new vector size %d!\n", newSize)
		load	len, size
		load	r0, v[_VECTOR_LEN]
		#cond	len, lt, r0
			store	len, v[_VECTOR_LEN]
		#endcond
	#endsync
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorTrim(vector)
// Trims vector capacity down to the vector length.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorTrim(vector)
	#var	v, newSize
	load	v, vector
	load	newSize, v[_VECTOR_LEN]
	#call	vectorResize(v, newSize)
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorMaximize(vector)
// Increases the length of the vector to the maximum capacity.  New elements
// are set to 0.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorMaximize(vector)
	#var	i, v, data, oldSize, newSize
	load	v, vector
	load	oldSize, v[_VECTOR_LEN]
	load	data, v[_VECTOR_DATA]
	load	newSize, data[-1]
	#for	i, oldSize, lt, newSize, 1
		store	0, data[i]
	#endfor
	store	newSize, v[_VECTOR_LEN]
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorFree(vector)
// Deallocates the vector.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorFree(vector)
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

///////////////////////////////////////////////////////////////////////////////
// vectorSize(vector)
// Size of the vector. Corresponds to the highest index used to access the 
// vector.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorSize(vector)
	#var	v, len
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	len, v[_VECTOR_LEN]
	#endsync
	#return	len
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorIsEmpty(vector)
// Returns TRUE if the vector is empty, i.e. no elements have been accessed.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorIsEmpty(vector)
	#var	v, len
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	len, v[_VECTOR_LEN]
	#endsync
	TO_NOT_BOOLEAN(len)
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorCapacity(vector)
// Capacity of the vector. Corresponds to the largest index that can be used
// without growing the vector.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorCapacity(vector)
	#var	v, data, size
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		load	size, data[-1]
	#endsync
	#return	size
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorAt(vector, index)
// Returns the integer element at index.  Returns 0 if index is out of bounds.
// vector	vector to operate on.
// index	element index into the vector.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorAt(vector, index)
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

///////////////////////////////////////////////////////////////////////////////
// vectorSetAt(vector, value, index)
// Sets the integer element at index.  Does nothing if index is out of bounds.
// vector	vector to operate on.
// value	new value to store.
// index	element index into the vector.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorSetAt(vector, value, index)
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

///////////////////////////////////////////////////////////////////////////////
// STACK_PEEK(vector)
// Returns the last integer value in the vector.  Returns 0 if vector is empty.
// Convenience macro for when using a vector as a stack.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_macro	STACK_PEEK(vector)
	#call	vectorLast(${vector})
#end_macro

///////////////////////////////////////////////////////////////////////////////
// vectorLast(vector)
// Returns the last integer value in the vector.  Returns 0 if vector is empty.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorLast(vector)
	#var	v, i, value, len, data
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	len, v[_VECTOR_LEN]
		load	data, v[_VECTOR_DATA]
		move	value, 0
		#cond	len, ne, 0
			sub		len, 1
			load	value, data[len]
		#end_cond
	#endsync
	#return	value
#end_func

///////////////////////////////////////////////////////////////////////////////
// STACK_PUSH(vector, value)
// Pushes the integer value onto the end of the vector.
// Convenience macro for when using a vector as a stack.
// vector	vector to operate on.
// value	new value to store.
///////////////////////////////////////////////////////////////////////////////
#def_macro	STACK_PUSH(vector, value)
	#call	vectorAdd(${vector}, ${value})
#end_macro

///////////////////////////////////////////////////////////////////////////////
// vectorAdd(vector, value)
// Adds the integer value onto the end of the vector.
// vector	vector to operate on.
// value	new value to store.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorAdd(vector, value)
	#var	v, i, size, len, data
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	i, value
		load	len, v[_VECTOR_LEN]
		load	data, v[_VECTOR_DATA]
		load	size, data[-1]
		cmp		len, size
		#cond_sr	ge
			#call	vectorResize(v, len)
		#end_cond_sr
		load	data, v[_VECTOR_DATA]
		store	i, data[len]
		add		len, 1
		store	len, v[_VECTOR_LEN]
	#endsync
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorAddAt(vector, value, index)
// Adds the integer value into the vector at index.
// vector	vector to operate on.
// value	new value to store.
// index	element index into the vector.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorAddAt(vector, value, index)
	#var	v, i, val, len, data, size, newSize, src, dest
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	val, value
		load	i, index
		load	len, v[_VECTOR_LEN]
		load	data, v[_VECTOR_DATA]
		load	size, data[-1]
		COMPARE_RANGE(0, le, i, le, len)
		#cond_sr	nz
			cmp		len, size
			#cond_sr	ge
				#call	vectorResize(v, len)
				load	data, v[_VECTOR_DATA]
			#end_cond_sr
			// Move the data at i one to the right
			add		src, data, i
			move	dest, src
			add		dest, 1
			sub		len, i
			MEMMOVE(dest, src, len)
			store	val, src
			load	len, v[_VECTOR_LEN]
			add		len, 1
			store	len, v[_VECTOR_LEN]
		#end_cond_sr
	#endsync
#end_func

///////////////////////////////////////////////////////////////////////////////
// STACK_POP(vector)
// Removes the last integer value in the vector.  Returns 0 if vector is empty.
// Convenience macro for when using a vector as a stack.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_macro	STACK_POP(vector)
	#call	vectorRemoveAtEnd(${vector})
#end_macro

///////////////////////////////////////////////////////////////////////////////
// vectorRemoveAtEnd(vector)
// Removes the last integer value in the vector.  Returns 0 if vector is empty.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorRemoveAtEnd(vector)
	#var	v, size, len, data, value
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	len, v[_VECTOR_LEN]
		load	data, v[_VECTOR_DATA]
		move	value, 0
		#cond	len, ne, 0
			sub		len, 1
			load	value, data[len]
			store	len, v[_VECTOR_LEN]
		#endcond
	#endsync
	#return	value
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorRemoveAt(vector, index)
// Removes the integer value from the vector at index.  Returns the value
// removed in r0.
// vector	vector to operate on.
// index	element index into the vector.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorRemoveAt(vector, index)
	#var	v, i, len, data, value, src, dest
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	i, index
		load	len, v[_VECTOR_LEN]
		load	data, v[_VECTOR_DATA]
		move	value, 0
		COMPARE_RANGE(0, le, i, lt, len)
		#cond_sr	nz
			load	value, data[i]
			// Move the data at i one to the left
			add		dest, data, i
			move	src, dest
			add		src, 1
			sub		len, i
			sub		len, 1
			MEMMOVE(dest, src, len)
			load	len, v[_VECTOR_LEN]
			sub		len, 1
			store	len, v[_VECTOR_LEN]
		#end_cond_sr
	#endsync
	#return	value
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorClear(vector)
// Sets the vector length to 0.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorClear(vector)
	#var	v
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	r0, _VECTOR_MIN_SIZE
		#call	vectorResize(v, r0)
		store	0, v[_VECTOR_LEN]
	#endsync
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorIndexOf(vector, value, afterIndex)
// Returns the index of the first occurrance of value in the vector at or
// after the afterIndex. Returns -1 if not found.
// vector	vector to operate on.
// value	value to find.
// afterIndex	element index into the vector to start search.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorIndexOf(vector, value, index)
	#var	v, i, j, k, val, len, data
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	i, index
		load	val, value
		load	len, v[_VECTOR_LEN]
		load	data, v[_VECTOR_DATA]
		#for	j, i, lt, len, 1
			load	k, data[j]
			#cond	val, eq, k
				#break
			#endcond
		#endfor
	#endsync
	#cond	j, eq, len
		#return	-1
	#elsecond
		#return	j
	#endcond
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorLastIndexOf(vector, value, beforeIndex)
// Returns the index of the last occurrance of value in the vector at or
// before the beforeIndex. Returns -1 if not found.
// vector	vector to operate on.
// value	value to find.
// afterIndex	element index into the vector to start search.  Use -1 for end
//				of the vector.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorLastIndexOf(vector, value, index)
	#var	v, i, j, k, val, len, data
	load	v, vector
	#sync	v[_VECTOR_MUTEX]
		load	i, index
		load	val, value
		load	len, v[_VECTOR_LEN]
		load	data, v[_VECTOR_DATA]
		#cond	i, lt, 0
			move	i, len
			sub		i, 1
		#endcond
		#for	j, i, ge, 0, -1
			load	k, data[j]
			#cond	val, eq, k
				#break
			#endcond
		#endfor
	#endsync
	#return	j
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorPrint(vector)
// Prints the vector.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorPrint(vector)
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
VECTOR_ASM_END: nop
