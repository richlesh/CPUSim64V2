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

package cloud.lesh.CPUSim64;

import org.junit.jupiter.api.Test;

import java.util.Arrays;
import java.util.Comparator;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class ThreadLibTest extends BaseTest {
	@Test
	void testTemplate() {
		String src = """
			START:
			#include <system/debug.def>
			#include <system/debug.asm>

			STOP
			STOP
			FINIS:
			""";
		String expected = """
""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(3, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testSpinLock() {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
			#include <system/thread.asm>
			
			call	main
			move	r1, 0
			int		iEXIT

			#def_macro put_dec(i)
				move R2, ${i}
				move R1, 1
				int iPUT_DEC
				int	iPUT_NL
			#end_macro
			
			#global	PIDS: .dca	3
			#def_func	MAIN()
				#CALL	initializeSpinLock(SPINLOCK)
				#var	pid, i
				#macro	create_thread(run, 1)
				store	r0, PIDS[1]
				#macro	create_thread(run, 2)
				store	r0, PIDS[2]
				#macro	create_thread(run, 3)
				store	r0, PIDS[3]
				load	r1, PIDS[1]
				int		iJOIN_THREAD
				load	r1, PIDS[2]
				int		iJOIN_THREAD
				load	r1, PIDS[3]
				int		iJOIN_THREAD
			#end_func
			
			#macro DEFINE_SPINLOCK(SPINLOCK)
			#global		COUNTER:	.dci 0
			#def_func run(data)
				#var	c, d, i
				load	d, data
				#for	0, i < 100, 1
					#call	acquireSpinLock(SPINLOCK)
					load	c, COUNTER
					add		c, 1
					store	c, COUNTER
					#macro	put_dec(c)
					#call	releaseSpinLock(SPINLOCK)
				#end_for
			#end_func
				stop
				stop
			""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		String[] lines = output.split("\n");
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(3, diff.size());
		for (int i = 0; i < 300; ++i) {
			assertEquals(Integer.toString(i + 1), lines[i]);
		}
	}

	@Test
	void testRecursiveSpinLock() {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
			#include <system/thread.asm>

			call	main
			move	r1, 0
			int		iEXIT

			#def_macro put_dec(val)
				push ${val}
				#call	acquireRecursiveSpinLock(SPINLOCK)
				move R3, 4
				pop R2
				move R1, STDOUT
				int iPUT_DEC
				move R1, STDOUT
				int	iPUT_NL
				#call	releaseRecursiveSpinLock(SPINLOCK)
			#end_macro

			#def_macro put_dec2(pid, val)
				push ${val}
				push ${pid}
				#call	acquireRecursiveSpinLock(SPINLOCK)
				move R3, 4
				pop R2
				move R1, STDOUT
				int iPUT_DEC
				move R2, ":"
				move R1, STDOUT
				int iPUTS
				move R3, 4
				pop R2
				move R1, STDOUT
				int iPUT_DEC
				move R1, STDOUT
				int	iPUT_NL
				#call	releaseRecursiveSpinLock(SPINLOCK)
			#end_macro

			#global	PIDS: .dca	3
			#def_func	MAIN()
				#CALL	initializeRecursiveSpinLock(SPINLOCK)
				#var	pid, i
				#macro	create_thread(run, 1)
				store	r0, PIDS[1]
//				#macro	put_dec2(0, r0)
				#macro	create_thread(run, 2)
				store	r0, PIDS[2]
//				#macro	put_dec2(0, r0)
				#macro	create_thread(run, 3)
				store	r0, PIDS[3]
//				#macro	put_dec2(0, r0)
				load	r1, PIDS[1]
				int		iJOIN_THREAD
				load	r1, PIDS[2]
				int		iJOIN_THREAD
				load	r1, PIDS[3]
				int		iJOIN_THREAD
			#end_func

			#macro DEFINE_RECURSIVE_SPINLOCK(SPINLOCK)
			#global		COUNTER:	.dci 0
			#def_func run(data)
				#var	c, d, i, start, stop
				load	d, data
				int		iGET_PID
//				#macro	put_dec2(r0, d)
				#if_cond	d == 0
					#return 0
				#end_cond
				mult	start, 100, d
				add		stop, 100, start
				#for	start, i <= stop, 1
					#if_cond	i == start
						#call	acquireRecursiveSpinLock(SPINLOCK)
						sub		d, 1
						#call	run(d)
						#call	releaseRecursiveSpinLock(SPINLOCK)
					#end_cond
					#call	acquireRecursiveSpinLock(SPINLOCK)
					load	c, COUNTER
					add		c, 1
					store	c, COUNTER
					#call	releaseRecursiveSpinLock(SPINLOCK)
					#macro	put_dec(c)
				#end_for
			#end_func
				stop
				stop
			""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		String[] lines = output.split("\n");
		Arrays.sort(lines, Comparator.comparingInt(Integer::parseInt));
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(3, diff.size());
		for (int i = 0; i < 600; ++i) {
			assertEquals(Integer.toString(i + 1), lines[i]);
		}
	}

	@Test
	void testMutex() {
		String src = """
			#include <system/system.asm>
			#include <system/io.def>
			#include <system/thread.asm>

			call	main
			move	r1, 0
			int		iEXIT

			#def_macro put_dec(i)
				move R2, ${i}
				move R1, 1
				int iPUT_DEC
				int	iPUT_NL
			#end_macro

			#global	PIDS: .dca	8
			#def_func	MAIN()
				#var	pid, i
				#call	initializeMutex(MUTEX)
				#macro	create_thread(run, 1)
				store	r0, PIDS[1]
				#macro	create_thread(run, 2)
				store	r0, PIDS[2]
				#macro	create_thread(run, 3)
				store	r0, PIDS[3]
				#macro	create_thread(run, 4)
				store	r0, PIDS[4]
				#macro	create_thread(run, 5)
				store	r0, PIDS[5]
				#macro	create_thread(run, 6)
				store	r0, PIDS[6]
				#macro	create_thread(run, 7)
				store	r0, PIDS[7]
				#macro	create_thread(run, 8)
				store	r0, PIDS[8]
				#for 	1, i <= 8, 1
					load	r1, PIDS[i]
					int		iJOIN_THREAD
				#end_for
			#end_func

			#macro DEFINE_MUTEX(MUTEX)
			#global		COUNTER:	.dci 0
			#def_func run(data)
				#var	c, d, i
				load	d, data
				#if_cond	d == 0
					#return 0
				#end_cond
				#for	0, i < 100, 1
					#call	acquireMutex(MUTEX)
					#if_cond	i == 0
						sub		d, 1
						#call	run(d)
					#end_cond
					load	c, COUNTER
					add		c, 1
					store	c, COUNTER
					#macro	put_dec(c)
					#call	releaseMutex(MUTEX)
					#call	sleep(2)
				#end_for
			#end_func
				stop
				stop
			""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		String[] lines = output.split("\n");
		Arrays.sort(lines, Comparator.comparingInt(Integer::parseInt));
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(3, diff.size());
		for (int i = 0; i < 3600; ++i) {
			assertEquals(Integer.toString(i + 1), lines[i]);
		}
	}

	@Test
	void testSync() {
		String src = """
			#include <system/system.asm>
			#include <system/io.def>
			#include <system/thread.asm>

			call	main
			move	r1, 0
			int		iEXIT

			#def_macro put_dec(i)
				move R2, ${i}
				move R1, 1
				int iPUT_DEC
				int	iPUT_NL
			#end_macro

			#global	PIDS: .dca	8
			#def_func	MAIN()
				#var	pid, i
				#CALL	initializeMutex(MUTEX)
				#macro	create_thread(run, 1)
				store	r0, PIDS[1]
				#macro	create_thread(run, 2)
				store	r0, PIDS[2]
				#macro	create_thread(run, 3)
				store	r0, PIDS[3]
				#macro	create_thread(run, 4)
				store	r0, PIDS[4]
				#macro	create_thread(run, 5)
				store	r0, PIDS[5]
				#macro	create_thread(run, 6)
				store	r0, PIDS[6]
				#macro	create_thread(run, 7)
				store	r0, PIDS[7]
				#macro	create_thread(run, 8)
				store	r0, PIDS[8]
				#for 	1, i <= 8, 1
					load	r1, PIDS[i]
					int		iJOIN_THREAD
				#end_for
			#end_func

			#macro DEFINE_MUTEX(MUTEX)
			#global		COUNTER:	.dci 0
			#def_func run(data)
				#var	c, d, i
				load	d, data
				#if_cond	d == 0
					#return 0
				#end_cond
				#for	0, i < 100, 1
					#sync(MUTEX)
						#if_cond	i == 0
							sub		d, 1
							#call	run(d)
						#end_cond
						load	c, COUNTER
						add		c, 1
						store	c, COUNTER
						#macro	put_dec(c)
					#end_sync
					#call	sleep(2)
				#end_for
			#end_func
				stop
				stop
			""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		String[] lines = output.split("\n");
		Arrays.sort(lines, Comparator.comparingInt(Integer::parseInt));
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(3, diff.size());
		for (int i = 0; i < 3600; ++i) {
			assertEquals(Integer.toString(i + 1), lines[i]);
		}
	}

	@Test
	void testAtomic() {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
			#include <system/thread.asm>
			
			call	main
			move	r1, 0
			int		iEXIT

			#def_macro put_dec(i)
				move R2, ${i}
				move R1, 1
				int iPUT_DEC
				int	iPUT_NL
			#end_macro
			
			#call	main()
			move	r1, 0
			int		iEXIT
			
			#global	PIDS: .dca	3
			#def_func	MAIN()
				#var	pid, i
				#macro	create_thread(run, 1)
				store	r0, PIDS[1]
				#macro	create_thread(run, 2)
				store	r0, PIDS[2]
				#macro	create_thread(run, 3)
				store	r0, PIDS[3]
				load	r1, PIDS[1]
				int		iJOIN_THREAD
				load	r1, PIDS[2]
				int		iJOIN_THREAD
				load	r1, PIDS[3]
				int		iJOIN_THREAD
				load	r0, ATOMIC
				#macro	put_dec(r0)
				load	r0, ATOMIC
			#end_func
			
			#global		ATOMIC:		.dci 0
			#def_func run(data)
				#var	i
				#for	0, i < 100, 1
					#call	get_and_increment(ATOMIC)
				#end_for
				#for	0, i < 100, 1
					#call	get_and_decrement(ATOMIC)
				#end_for
			#end_func
			stop
			stop
				""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src);
		String output = capturer.stop();
		String[] lines = output.split("\n");
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(2, diff.size());
		diff.assertDiff(0, 0);
	}
}