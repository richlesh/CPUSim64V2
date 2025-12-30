#include <system/io.asm>
#include <system/math.def>
#include <system/system.def>
#include <adt/vector.def>

	jump	VECTOR_ASM_END
///////////////////////////////////////////////////////////////////////////////
// Vector
// Implements a growable list of integers.  Is thread safe.
// Vector Data is a heap allocated block with the following structure:
//-1:	int		total capacity + 1 + HEAP_BLOCK_HEADER_SIZE (number of elements allocated)
// 0:	int		current length (number of elements used)
// 1..n:	int		elements
///////////////////////////////////////////////////////////////////////////////

_VECTOR_SIZE_FACTOR: 	.dcf	1.2		// Ratio to increase data block size.

///////////////////////////////////////////////////////////////////////////////
// newVector(size)
// Creates a new vector object and returns its address.  Vectors are contiguous
// lists of integers that can grow when needed.
// initialCapacity  capacity to start the vector.
///////////////////////////////////////////////////////////////////////////////
#def_func newVector(initialCapacity)
	#var	s, capacity, addr, data
	load	capacity, initialCapacity
	#macro	ALLOC(_VECTOR_END)
	move	addr, r0
	store	0, addr[_VECTOR_MUTEX]			// To do: Initialize mutex
	load	f0, _VECTOR_SIZE_FACTOR
	mult	f0, capacity
	move	s, f0
	add		s, 1
	#macro	ALLOC(s)						// Allocate storage array
	move	data, r0
	#macro	TO_NOT_BOOLEAN(r0)
	#call	cond_fatal(r0, STDOUT, "Can\'t allocate new vector size %d!\n", capacity)
	store	data, addr[_VECTOR_DATA]
	store	0, data[0]						// Leading count to 0
	#return	addr
#end_func

///////////////////////////////////////////////////////////////////////////////
// resizeVector(vector, size)
// Resizes a vector object to a new size.  Size can be larger or smaller.
// vector	vector to operate on.
// size		new number of elements.
///////////////////////////////////////////////////////////////////////////////
#def_func resizeVector(vector, size)
	#var	v, data, newSize, capacity
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		load	capacity, size
		load	f0, _VECTOR_SIZE_FACTOR
		mult	f0, capacity
		move	newSize, f0
		add		newSize, 1
		#macro	REALLOC(data, newSize)
		move	data, r0
		store	data, v[_VECTOR_DATA]
		#macro	TO_NOT_BOOLEAN(r0)
		#call	cond_fatal(r0, STDOUT, "Can\'t allocate new vector size %d!\n", capacity)
		load	newSize, size
		load	r0, data[0]
		#if_cond	newSize < r0
			store	newSize, data[0]
		#end_cond
//	#endsync
#end_func

///////////////////////////////////////////////////////////////////////////////
// freeVector(vector)
// Deallocates the vector.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func freeVector(vector)
	#var	v, data
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		#macro	FREE(data)
		store	0, v[_VECTOR_DATA]
		#macro	FREE(v)
//	#endsync
#end_func

///////////////////////////////////////////////////////////////////////////////
// trimVector(vector)
// Trims vector capacity down to the vector length.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func trimVector(vector)
	#var	v, data, newSize
	load	v, vector
	load	data, v[_VECTOR_DATA]
	load	newSize, data[0]
	#call	resizeVector(v, newSize)
#end_func

///////////////////////////////////////////////////////////////////////////////
// maximizeVector(vector)
// Increases the length of the vector to the maximum capacity.  New elements
// are set to 0.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func maximizeVector(vector)
	#var	i, v, data, oldSize, newSize
	load	v, vector
	load	data, v[_VECTOR_DATA]
	load	oldSize, data[0]
	load	newSize, data[-1]
	sub		newSize, 1
	sub		newSize, HEAP_BLOCK_HEADER_SIZE
	add		oldSize, 1
	#for	oldSize, i <= newSize, 1
		store	0, data[i]
	#end_for
	store	newSize, data[0]
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorSize(vector)
// Size of the vector. Corresponds to the highest index used to access the 
// vector.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorSize(vector)
	#var	v, data, len
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		load	len, data[0]
//	#endsync
	#return	len
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorIsEmpty(vector)
// Returns TRUE if the vector is empty, i.e. no elements have been added.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorIsEmpty(vector)
	#var	v
	load	v, vector
	#call	vectorSize(v)
	#macro	TO_NOT_BOOLEAN(r0)
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
//	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		load	size, data[-1]
		sub		size, 1
		sub		size, HEAP_BLOCK_HEADER_SIZE
//	#endsync
	#return	size
#end_func

///////////////////////////////////////////////////////////////////////////////
// getVectorAt(vector, index)
// Returns the integer element at index.  Returns 0 if index is out of bounds.
// vector	vector to operate on.
// index	element index into the vector. [0, len - 1]
///////////////////////////////////////////////////////////////////////////////
#def_func getVectorAt(vector, index)
	#var	v, i, data, len, value
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		load	i, index
		add		i, 1
		load	data, v[_VECTOR_DATA]
		load	len, data[0]
		move	value, 0
		#macro	COMPARE_RANGE(1, le, i, le, len)
		#if_cond_sr	nz
			load	value, data[i]
		#end_cond_sr
//	#endsync
	#return	value
#end_func

///////////////////////////////////////////////////////////////////////////////
// setVectorAt(vector, index, value)
// Sets the integer element at index.  Does nothing if index is out of bounds.
// vector	vector to operate on.
// index	element index into the vector. [0, len - 1]
// value	new value to store.
///////////////////////////////////////////////////////////////////////////////
#def_func setVectorAt(vector, index, value)
	#var	v, i, data, len, val
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		load	val, value
		load	i, index
		add		i, 1
		load	data, v[_VECTOR_DATA]
		load	len, data[0]
		#macro	COMPARE_RANGE(1, le, i, le, len)
		#if_cond_sr	nz
			store	val, data[i]
		#end_cond
//	#endsync
#end_func

///////////////////////////////////////////////////////////////////////////////
// PEEK_BACK(vector)
// Returns the last integer value in the vector.  Returns 0 if vector is empty.
// Convenience macro for when using a vector as a stack.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_macro	PEEK_BACK(vector)
	#call	vectorLast(${vector})
#end_macro

///////////////////////////////////////////////////////////////////////////////
// PUSH_BACK(vector, value)
// Pushes the integer value onto the end of the vector.
// Convenience macro for when using a vector as a stack.
// vector	vector to operate on.
// value	new value to store.
///////////////////////////////////////////////////////////////////////////////
#def_macro	PUSH_BACK(vector, value)
	#call	vectorAdd(${vector}, ${value})
#end_macro

///////////////////////////////////////////////////////////////////////////////
// POP_BACK(vector)
// Removes the last integer value in the vector.  Returns 0 if vector is empty.
// Convenience macro for when using a vector as a stack.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_macro	POP_BACK(vector)
	#call	vectorRemoveAtEnd(${vector})
#end_macro

///////////////////////////////////////////////////////////////////////////////
// PEEK_FRONT(vector)
// Returns the first integer value in the vector.  Returns 0 if vector is empty.
// Convenience macro for when using a vector as a queue.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_macro	PEEK_FRONT(vector)
	#call	vectorFirst(${vector})
#end_macro

///////////////////////////////////////////////////////////////////////////////
// POP_FRONT(vector)
// Removes the integer value from the front of the vector.
// Convenience macro for when using a vector as a queue.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_macro	POP_FRONT(vector)
	#call	vectorRemoveAt(${vector}, 0)
#end_macro

///////////////////////////////////////////////////////////////////////////////
// PUSH_FRONT(vector, value)
// Shifts the integer value onto the front of the vector.
// Convenience macro for when using a vector as a queue.
// vector	vector to operate on.
// value	new value to store.
///////////////////////////////////////////////////////////////////////////////
#def_macro	PUSH_FRONT(vector, value)
	#call	vectorAddAt(${vector}, 0, ${value})
#end_macro

///////////////////////////////////////////////////////////////////////////////
// vectorFirst(vector)
// Returns the first integer value in the vector.  Returns 0 if vector is empty.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorFirst(vector)
	#var	v, data, len, value
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		load	len, data[0]
		move	value, 0
		#if_cond	len != 0
			load	value, data[1]
		#end_cond
//	#endsync
	#return	value
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorLast(vector)
// Returns the last integer value in the vector.  Returns 0 if vector is empty.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorLast(vector)
	#var	v, data, len, value
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		load	len, data[0]
		move	value, 0
		#if_cond	len != 0
			load	value, data[len]
		#end_cond
//	#endsync
	#return	value
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorAdd(vector, value)
// Adds the integer value onto the end of the vector.
// vector	vector to operate on.
// value	new value to store.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorAdd(vector, value)
	#var	v, data, len, size
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		load	len, data[0]
		add		len, 1
		load	size, data[-1]
		sub		size, 1
		sub		size, HEAP_BLOCK_HEADER_SIZE
		#if_cond	len > size
			#call	resizeVector(v, len)
		#end_cond
		load	data, v[_VECTOR_DATA]
		load	r0, value
		store	r0, data[len]
		store	len, data[0]
//	#endsync
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorAddAt(vector, index, value)
// Adds the integer value into the vector at index.
// vector	vector to operate on.
// index	element index into the vector. [0, len - 1]
// value	new value to store.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorAddAt(vector, index, value)
	#var	v, i, val, len, data, size, newSize, src, dest, moveLen
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		load	val, value
		load	i, index
		add		i, 1
		load	data, v[_VECTOR_DATA]
		load	len, data[0]
		add		len, 1
		load	size, data[-1]
		sub		size, 1
		sub		size, HEAP_BLOCK_HEADER_SIZE
		#macro	COMPARE_RANGE(1, le, i, le, len)
		#if_cond_sr	nz
			#if_cond	len > size
				#call	resizeVector(v, len)
				load	data, v[_VECTOR_DATA]
			#end_cond_sr
			// Move the data at i one to the right
			add		src, data, i
			add		dest, src, 1
			sub		moveLen, len, i
			#macro	MEMMOVE(dest, src, moveLen)
			store	val, src
			store	len, data[0]
		#end_cond_sr
//	#endsync
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorRemoveAtEnd(vector)
// Removes the last integer value in the vector and returns it in R0.
// Returns 0 if vector is empty.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func vectorRemoveAtEnd(vector)
	#var	v, size, len, data, value
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		load	len, data[0]
		move	value, 0
		#if_cond	len != 0
			load	value, data[len]
			sub		len, 1
			store	len, data[0]
		#end_cond
//	#endsync
	#return	value
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorRemoveAt(vector, index)
// Removes the integer value from the vector at index.  Returns the value
// removed in r0.
// vector	vector to operate on.
// index	element index into the vector. [0, len - 1]
///////////////////////////////////////////////////////////////////////////////
#def_func vectorRemoveAt(vector, index)
	#var	v, i, len, data, value, src, dest, moveLen
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		load	i, index
		add		i, 1
		load	data, v[_VECTOR_DATA]
		load	len, data[0]
		move	value, 0
		#macro	COMPARE_RANGE(1, le, i, le, len)
		#if_cond_sr	nz
			load	value, data[i]
			// Move the data at i one to the left
			add		dest, data, i
			add		src, dest, 1
			sub		moveLen, len, i
			#macro	MEMMOVE(dest, src, len)
			sub		len, 1
			store	len, data[0]
		#end_cond_sr
//	#endsync
	#return	value
#end_func

///////////////////////////////////////////////////////////////////////////////
// clearVector(vector)
// Sets the vector length to 0.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func clearVector(vector)
	#var	v, data
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		#call	resizeVector(v, 1)
		load	data, v[_VECTOR_DATA]
		store	0, data[0]
//	#endsync
#end_func

///////////////////////////////////////////////////////////////////////////////
// vectorIndexOf(vector, value, afterIndex)
// Returns the index of the first occurrence of value in the vector at or
// after the afterIndex. Returns -1 if not found.
// vector	vector to operate on.
// value	value to find.
// afterIndex	element index into the vector to start search. [0, size - 1]
///////////////////////////////////////////////////////////////////////////////
#def_func vectorIndexOf(vector, value, afterIndex)
	#var	v, i, j, k, val, len, data
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		load	i, afterIndex
		add		i, 1
		load	val, value
		load	data, v[_VECTOR_DATA]
		load	len, data[0]
		#for	i, j <= len, 1
			load	k, data[j]
			#if_cond	val == k
				#break
			#end_cond
		#end_for
//	#endsync
	#if_cond	j > len
		#return	-1
	#else_cond
		sub		j, 1
		#return	j
	#end_cond
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
//	#sync	v[_VECTOR_MUTEX]
		load	i, index
		load	val, value
		load	data, v[_VECTOR_DATA]
		load	len, data[0]
		#if_cond	i < 0
			move	i, len
		#else_cond
			add		i, 1
		#end_cond
		#for	i, j >= 1, -1
			load	k, data[j]
			#if_cond	val == k
				#break
			#end_cond
		#end_for
//	#endsync
	#if_cond	j == 0
		#return	-1
	#else_cond
		sub		j, 1
		#return	j
	#end_cond
#end_func

///////////////////////////////////////////////////////////////////////////////
// printVector(vector)
// Prints the vector.
// vector	vector to operate on.
///////////////////////////////////////////////////////////////////////////////
#def_func printVector(vector)
	#var	v, i, j, len, data
	load	v, vector
//	#sync	v[_VECTOR_MUTEX]
		load	data, v[_VECTOR_DATA]
		load	len, data[0]
		#for	1, i <= len, 1
			#if_cond	i != 1
				#macro	out1(',', STDOUT)
			#end_cond
			load	j, data[i]
			#macro	put_dec(j)
		#end_for
		#macro	put_nl()
//	#endsync
#end_func

#def_func debugVector(vector)
	#var	v, data, mutex, size
	load	v, vector
	load	data, v[_VECTOR_DATA]
	move	size, -1
	load	mutex, v[_VECTOR_MUTEX]
	#call	fprintf(STDOUT,"vec[%x, size: %d, %x]\n", data, size, mutex)
#end_func

VECTOR_ASM_END: nop
