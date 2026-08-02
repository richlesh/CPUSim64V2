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

#ifndef _THREAD_ASM
#define _THREAD_ASM
#include <system/system.asm>
#include <system/thread.def>
#include <system/io.asm>

THREAD_ASM_START:
    jump    THREAD_ASM_END

#def_func	initializeSpinLock(mutexAddr)
	load	r0, mutexAddr
	store	false, r0
#end_func

#def_func	acquireSpinLock(mutexAddr)
	load	r0, mutexAddr
$loop:
	cas		false, true, r0
	jump	no, $loop
#end_func

#def_func	releaseSpinLock(mutexAddr)
	load	r0, mutexAddr
	store	false, r0
#end_func

#def_func	initializeRecursiveSpinLock(mutexAddr)
    #var    addr
	load	addr, mutexAddr
	store	0, addr
	store	0, addr[1]
#end_func

#def_func	acquireRecursiveSpinLock(mutexAddr)
	#var	pid, lockPID, addr
	load	addr, mutexAddr
	int		iGET_PID
	move	pid, r0
	load	lockPID, addr
	#if_cond	lockPID == pid
		load	r0, addr[1]
		add		r0, 1
		store	r0, addr[1]
	#else_cond
$loop:
		cas		0, pid, addr
		jump	no, $loop
		store	1, addr[1]
	#end_cond
#end_func

#def_func	releaseRecursiveSpinLock(mutexAddr)
	#var	addr, pid, lockPID
	load	addr, mutexAddr
	int		iGET_PID
	move	pid, r0
	load	lockPID, addr
	#if_cond	lockPID == pid
		load	r0, addr[1]
		sub		r0, 1
		store	r0, addr[1]
		#if_cond	r0 == 0
			store	0, addr
		#end_cond
	#end_cond
#end_func

// Mutex structure is:
// 0: PID that owns mutex or 0
// 1: Re-entry count, decrement to 0 releases lock
// 2: Heap allocated array of waiting PIDs
// 3: SpinLock to protect array of waiting PIDs
// TODO: This won't work for mutex allocated in shared mem.
#def_func	initializeMutex(mutexAddr)
    #var    addr
	load	addr, mutexAddr
	store	0, addr[0]
	store	0, addr[1]
	#call	alloc(10)
	store   r0, addr[2]
	store	0, r0
	move	r0, addr[3]
	#call	initializeSpinLock(r0)
#end_func

#def_func	freeMutex(mutexAddr)
	#var	addr
	load	addr, mutexAddr
	load	r0, addr[2]
    #call	free(r0)
#end_func

#def_func	acquireMutex(mutexAddr)
	#var	pid, lockPID, addr, queue, queueSize, queueAllocSize, i
	load	addr, mutexAddr
	int		iGET_PID										// Get our PID
	move	pid, r0
	load	lockPID, addr[0]
	#if_cond	lockPID != pid								// if re-entrant, we already have the lock
        #while TRUE
            cas		0, pid, addr[0]							// set PID atomically to get lock
            #if_cond_sr o									// We got the lock so break
          		#break
			#end_cond
			move	r0, addr[3]
			#call	acquireSpinLock(r0)						// Use a spinlock to control access to queue
			load    queue, addr[2]
			load	queueSize, queue[0]
			#for	1, i <= queueSize, 1					// Loop looking for our PID in the queue
				load	r0, queue[i]
				#if_cond r0 == pid							// If our PID is found, break
					#break
				#end_cond
			#end_for
			#if_cond	i > queueSize						// If we didn't find it, add it
				add		queueSize, 1						// Compute new queue size to hold our PID
				load	queueAllocSize, queue[-1]			// Compute allocation size
				sub		queueAllocSize, HEAP_BLOCK_HEADER_SIZE
				#if_cond	queueSize == queueAllocSize		// If sizes are the same we need to realloc
					add		r0, queueSize, 1				// New alloc size is new queue size + leading count
					#call	REALLOC(queue, r0)
					move	queue, r0
					store	queue, addr[2]					// Store the realloc block
				#end_cond
				store	queueSize, queue[0]					// Store the new queue size
				store	pid, queue[queueSize]				// Add our PID to the end of the queue
			#end_cond
			move	r0, addr[3]
			#call	releaseSpinLock(r0)
			#call   sleep(10000)							// Sleep until awoken or 10 sec.
       #end_while
	#end_cond
	load	r0, addr[1]										// Increase the re-entry count
	add		r0, 1
	store	r0, addr[1]
#end_func

#def_func	releaseMutex(mutexAddr)
	#var	addr, pid, lockPID, queue, queueSize, i
	load	addr, mutexAddr
	int		iGET_PID										// Get our PID
	move	pid, r0
	load	lockPID, addr[0]
	#if_cond	lockPID == pid								// if we have the lock let's release it
		load	r0, addr[1]									// Decrease the re-entry count
		sub		r0, 1
		store	r0, addr[1]
		#if_cond	r0 == 0									// Only release if re-entry count is 0
			store	0, addr[0]								// Release the lock PID
			move	r0, addr[3]
			#call	acquireSpinLock(r0)
			load    queue, addr[2]
			load	queueSize, queue[0]
			#if_cond	queueSize							// If the queue is not empty pop a PID to wake
				load	pid, queue[1]						// Get first PID in the queue
				move	r1, queue[1]						// Dest of memmove
				move	r2, queue[2]						// Source of memmove
				sub		queueSize, 1						// New queue size is one less
				move	r3, queueSize						// Size of memmove
				store	queueSize, queue[0]					// Store new queue size
				int		iMEMMOVE							// Move the back portion of queue up one.
				#macro  wake_thread(pid)					// Wake waiting thread
			#end_cond
			move	r0, addr[3]
			#call	releaseSpinLock(r0)
		#end_cond
	#end_cond
#end_func

/******************************************************************************
* get_and_increment(addr)
* Atomically increments the value at addr.  Returns the new value.
******************************************************************************/
#def_func get_and_increment(addr)
	#var	oldValue, newValue, atomic
	load	atomic, addr
$_TRY_GET_AND_INCREMENT:
	load	oldValue, atomic
	add		newValue, oldValue, 1
	cas		oldValue, newValue, atomic
	jump	no, $_TRY_GET_AND_INCREMENT
	#return	newValue
#end_func

/******************************************************************************
* get_and_decrement(addr)
* Atomically decrements the value at addr.  Returns the new value.
******************************************************************************/
#def_func get_and_decrement(addr)
	#var	oldValue, newValue, atomic
	load	atomic, addr
$_TRY_GET_AND_DECREMENT:
	load	oldValue, atomic
	sub		newValue, oldValue, 1
	cas		oldValue, newValue, atomic
	jump	no, $_TRY_GET_AND_DECREMENT
	#return	newValue
#end_func


THREAD_ASM_END:	nop
#endif
