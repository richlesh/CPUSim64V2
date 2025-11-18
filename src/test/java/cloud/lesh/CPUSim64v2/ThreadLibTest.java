package cloud.lesh.CPUSim64v2;

import org.junit.jupiter.api.Test;

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
		assertEquals(2, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testSpinLock() {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
			#include <system/thread.asm>
			
			call	main
			move	r0, 0
			int		iEXIT

			#def_macro put_dec(i)
				move R1, ${i}
				move R0, 1
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
				load	r0, PIDS[1]
				int		iJOIN_THREAD
				load	r0, PIDS[2]
				int		iJOIN_THREAD
				load	r0, PIDS[3]
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
			move	r0, 0
			int		iEXIT

			#def_macro put_dec(i)
				move R1, ${i}
				move R0, 1
				int iPUT_DEC
				int	iPUT_NL
			#end_macro

			#global	PIDS: .dca	3
			#def_func	MAIN()
				#CALL	initializeRecursiveSpinLock(SPINLOCK)
				#var	pid, i
				#macro	create_thread(run, 1)
				store	r0, PIDS[1]
				#macro	create_thread(run, 2)
				store	r0, PIDS[2]
				#macro	create_thread(run, 3)
				store	r0, PIDS[3]
				load	r0, PIDS[1]
				int		iJOIN_THREAD
				load	r0, PIDS[2]
				int		iJOIN_THREAD
				load	r0, PIDS[3]
				int		iJOIN_THREAD
			#end_func

			#macro DEFINE_RECURSIVE_SPINLOCK(SPINLOCK)
			#global		COUNTER:	.dci 0
			#def_func run(data)
				#var	c, d, i
				load	d, data
				jump	z, $_RETURN
				#for	0, i < 100, 1
					#call	acquireRecursiveSpinLock(SPINLOCK)
					#if_cond	i == 0
						sub		d, 1
						#call	run(d)
					#end_cond
					load	c, COUNTER
					add		c, 1
					store	c, COUNTER
					#macro	put_dec(c)
					#call	releaseRecursiveSpinLock(SPINLOCK)
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
		for (int i = 0; i < 600; ++i) {
			assertEquals(Integer.toString(i + 1), lines[i]);
		}
	}

	@Test
	void testMutex() {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
			#include <system/thread.asm>

			call	main
			move	r0, 0
			int		iEXIT

			#def_macro put_dec(i)
				move R1, ${i}
				move R0, 1
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
					load	r0, PIDS[i]
					int		iJOIN_THREAD
				#end_for
			#end_func

			#macro DEFINE_MUTEX(MUTEX)
			#global		COUNTER:	.dci 0
			#def_func run(data)
				#var	c, d, i
				load	d, data
				jump	z, $_RETURN
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
					#macro	sleep(2)
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
		for (int i = 0; i < 3600; ++i) {
			assertEquals(Integer.toString(i + 1), lines[i]);
		}
	}

	@Test
	void testSync() {
		String src = """
			#include <system/system.def>
			#include <system/io.def>
			#include <system/thread.asm>

			call	main
			move	r0, 0
			int		iEXIT

			#def_macro put_dec(i)
				move R1, ${i}
				move R0, 1
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
					load	r0, PIDS[i]
					int		iJOIN_THREAD
				#end_for
			#end_func

			#macro DEFINE_MUTEX(MUTEX)
			#global		COUNTER:	.dci 0
			#def_func run(data)
				#var	c, d, i
				load	d, data
				jump	z, $_RETURN
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
					#macro	sleep(2)
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
			move	r0, 0
			int		iEXIT

			#def_macro put_dec(i)
				move R1, ${i}
				move R0, 1
				int iPUT_DEC
				int	iPUT_NL
			#end_macro
			
			#call	main()
			move	r0, 0
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
				load	r0, PIDS[1]
				int		iJOIN_THREAD
				load	r0, PIDS[2]
				int		iJOIN_THREAD
				load	r0, PIDS[3]
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