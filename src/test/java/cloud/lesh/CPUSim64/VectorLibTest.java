package cloud.lesh.CPUSim64;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class VectorLibTest extends BaseTest {
	@Test
	void testCreateVector() {
		String src = """
			START:
			#include <adt/vector.asm>
			#call	newVector(1)
			move	r28, r0
			#call	newVector(10)
			move	r27, r0
			int		iWalk_Heap
			STOP
			STOP
			FINIS:
			""";
		String expected = """
Heap Blocks:
00001000: 8
00001008: 8
00001010: 8
00001018: 21
0000102d: -4051
""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
//		assertEquals(6, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testFreeVector() {
		String src = """
			START:
			#include <adt/vector.asm>
			#call	newVector(1)
			move	r28, r0
			#call	newVector(10)
			move	r27, r0
			int		iWalk_Heap
			#call	freeVector(r28)
			#call	freeVector(r27)
			int		iWalk_Heap
			STOP
			STOP
			FINIS:
			""";
		String expected = """
Heap Blocks:
00001000: 8
00001008: 8
00001010: 8
00001018: 21
0000102d: -4051
Heap Blocks:
00001000: -4096
""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(6, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testResizeVector() {
		String src = """
			START:
			#include <adt/vector.asm>
			#call	newVector(1)
			move	r28, r0
			#call	resizeVector(r28,4)		// 3 + 1 + 4 * 1.2 <= 8
			int		iWalk_Heap
			load	r0, r28[_VECTOR_DATA]
			store	3, r0
			store	1, r0[1]
			store	2, r0[2]
			store	3, r0[3]
			#call	resizeVector(r28,5)		// 3 + 1 + 5 * 1.2 <= 13
			int		iWalk_Heap
			#call	resizeVector(r28,8)		// 3 + 1 + 8 * 1.2 <= 13
			int		iWalk_Heap
			#call	resizeVector(r28,9)		// 3 + 1 + 9 * 1.2 <= 21
			int		iWalk_Heap
			#call	resizeVector(r28,14)	// 3 + 1 + 14 * 1.2 <= 21
			int		iWalk_Heap
			#call	resizeVector(r28,15)	// 3 + 1 + 15 * 1.2 <= 34
			int		iWalk_Heap
			load	r0, r28[_VECTOR_DATA]
			load	r1, r0[0]
			load	r2, r0[1]
			load	r3, r0[2]
			load	r4, r0[3]
			#call	freeVector(r28)
			int		iWalk_Heap
			STOP
			STOP
			FINIS:
			""";
		String expected = """
Heap Blocks:
00001000: 8
00001008: 8
00001010: -4080
Heap Blocks:
00001000: 8
00001008: -8
00001010: 13
0000101d: -4067
Heap Blocks:
00001000: 8
00001008: -8
00001010: 13
0000101d: -4067
Heap Blocks:
00001000: 8
00001008: -21
0000101d: 21
00001032: -4046
Heap Blocks:
00001000: 8
00001008: -21
0000101d: 21
00001032: -4046
Heap Blocks:
00001000: 8
00001008: -42
00001032: 34
00001054: -4012
Heap Blocks:
00001000: -4096
""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(9, diff.size());
		assertEquals(expected, output);
		diff.assertDiff(1, 3);
		diff.assertDiff(2, 1);
		diff.assertDiff(3, 2);
		diff.assertDiff(4, 3);
	}

	@Test
	void testTrimVector() {
		String src = """
			START:
			#include <adt/vector.asm>
			#call	newVector(100)
			move	r28, r0
			int		iWalk_Heap
			load	r0, r28[_VECTOR_DATA]
			store	10, r0
			store	1, r0[1]
			store	10, r0[10]
			#call	trimVector(r28)
			int		iWalk_Heap
			load	r0, r28[_VECTOR_DATA]
			load	r27, r0[1]
			load	r26, r0[10]
			STOP
			STOP
			FINIS:
			""";
		String expected = """
Heap Blocks:
00001000: 8
00001008: 144
00001098: -3944
Heap Blocks:
00001000: 8
00001008: -144
00001098: 21
000010ad: -3923
""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(7, diff.size());
		assertEquals(expected, output);
		diff.assertDiff(27, 1);
		diff.assertDiff(26, 10);
	}

	@Test
	void testMaximizeVector() {
		String src = """
			START:
			#include <adt/vector.asm>
			#call	newVector(100)
			move	r28, r0
			int		iWalk_Heap
			load	r0, r28[_VECTOR_DATA]
			store	10, r0
			store	1, r0[1]
			store	10, r0[10]
			store	11, r0[11]
			store	100, r0[100]
			#call	maximizeVector(r28)
			int		iWalk_Heap
			load	r0, r28[_VECTOR_DATA]
			load	r27, r0[1]
			load	r26, r0[10]
			load	r25, r0[11]
			load	r24, r0[100]
			load	r23, r0[0]
			STOP
			STOP
			FINIS:
			""";
		String expected = """
Heap Blocks:
00001000: 8
00001008: 144
00001098: -3944
Heap Blocks:
00001000: 8
00001008: 144
00001098: -3944
""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(8, diff.size());
		assertEquals(expected, output);
		diff.assertDiff(27, 1);
		diff.assertDiff(26, 10);
		diff.assertDiff(25, 0);
		diff.assertDiff(24, 0);
		diff.assertDiff(23, 140);
	}

	@Test
	void testVectorSize() {
		String src = """
			START:
			#include <adt/vector.asm>
			#call	newVector(100)
			move	r28, r0
			load	r0, r28[_VECTOR_DATA]
			store	10, r0
			#call	vectorSize(r28)
			STOP
			STOP
			FINIS:
			""";
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(5, diff.size());
		diff.assertDiff(0, 10);
	}

	@Test
	void testVectorIsEmpty() {
		String src = """
			START:
			#include <adt/vector.asm>
			#call	newVector(100)
			move	r28, r0
			#call	vectorIsEmpty(r28)
			move	r27, r0
			load	r0, r28[_VECTOR_DATA]
			store	10, r0
			#call	vectorIsEmpty(r28)
			move	r26, r0
			STOP
			STOP
			FINIS:
			""";
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(5, diff.size());
		diff.assertDiff(27, -1);
		diff.assertDiff(26, 0);
	}

	@Test
	void testVectorCapacity() {
		String src = """
			START:
			#include <adt/vector.asm>
			#call	newVector(100)
			move	r28, r0
			#call	vectorCapacity(r28)
			STOP
			STOP
			FINIS:
			""";
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(5, diff.size());
		diff.assertDiff(0, 140);
	}

	@Test
	void testVectorAt() {
		String src = """
			START:
			#include <adt/vector.asm>
			#call	newVector(100)
			move	r28, r0
			load	r0, r28[_VECTOR_DATA]
			store	10, r0
			#call	setVectorAt(r28, 0, 1)
			#call	setVectorAt(r28, 1, 2)
			#call	setVectorAt(r28, 9, 10)
			#call	setVectorAt(r28, 10, 11)
			#call	setVectorAt(r28, -1, 11)
			#call	getVectorAt(r28, -1)
			move	r27, r0
			#call	getVectorAt(r28, 0)
			move	r26, r0
			#call	getVectorAt(r28, 1)
			move	r25, r0
			#call	getVectorAt(r28, 9)
			move	r24, r0
			#call	getVectorAt(r28, 10)
			move	r23, r0
			STOP
			STOP
			FINIS:
			""";
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
		assertEquals(7, diff.size());
		diff.assertDiff(27, 0);
		diff.assertDiff(26, 1);
		diff.assertDiff(25, 2);
		diff.assertDiff(24, 10);
		diff.assertDiff(23, 0);
	}

	@Test
	void testStack() {
		String src = """
			START:
			#include <adt/vector.asm>
			#var	v, i
			#call	newVector(1)
			move	v, r0
			#for	0, i < 10, 1
				#macro	PUSH_BACK(v, i)
			#end_for
			#macro	PEEK_BACK(v)
			#macro	PUT_DEC(r0)
			#macro	PUT_NL()
			#for	0, i < 10, 1
				#macro	POP_BACK(v)
				#macro	PUT_DEC(r0)
				#macro	PUT_NL()
			#end_for
			int		iWALK_HEAP
			STOP
			STOP
			FINIS:
			""";
		String expected = """
9
9
8
7
6
5
4
3
2
1
0
Heap Blocks:
00001000: 8
00001008: -21
0000101d: 21
00001032: -4046
			""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
//		assertEquals(7, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testDequeue() {
		String src = """
			START:
			#include <adt/vector.asm>
			#var	v, i
			#call	newVector(1)
			move	v, r0
			#for	0, i < 10, 1
				#macro	PUSH_FRONT(v, i)
			#end_for
			#macro	PEEK_FRONT(v)
			#macro	PUT_DEC(r0)
			#macro	PUT_NL()
			#for	0, i < 10, 1
				#macro	POP_FRONT(v)
				#macro	PUT_DEC(r0)
				#macro	PUT_NL()
			#end_for
			int		iWALK_HEAP
			STOP
			STOP
			FINIS:
			""";
		String expected = """
9
9
8
7
6
5
4
3
2
1
0
Heap Blocks:
00001000: 8
00001008: -21
0000101d: 21
00001032: -4046
			""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
//		assertEquals(7, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testClear() {
		String src = """
			START:
			#include <adt/vector.asm>
			#var	v, i
			#call	newVector(100)
			move	v, r0
			#for	1, i <= 100, 1
				#macro	PUSH_BACK(v, i)
			#end_for
			#call	clearVector(v)
			int		iWALK_HEAP
			STOP
			STOP
			FINIS:
			""";
		String expected = """
Heap Blocks:
00001000: 8
00001008: -144
00001098: 8
000010a0: -3936
			""";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
//		assertEquals(7, diff.size());
		assertEquals(expected, output);
	}

	@Test
	void testIndexOf() {
		String src = """
			START:
			#include <adt/vector.asm>
			#var	v, i, i1, i2, i3, i4, i5, i6
			#call	newVector(1)
			move	v, r0
			#for	0, i < 10, 1
				#macro	PUSH_BACK(v, i)
			#end_for
			#for	0, i < 10, 1
				#macro	PUSH_BACK(v, i)
			#end_for
			#call	vectorIndexOf(v, 4, 0)
			move	i1, r0
			#call	vectorIndexOf(v, 4, 10)
			move	i2, r0
			#call	vectorIndexOf(v, 20, 0)
			move	i3, r0
			#call	vectorLastIndexOf(v, 4, -1)
			move	i4, r0
			#call	vectorLastIndexOf(v, 4, 10)
			move	i5, r0
			#call	vectorLastIndexOf(v, 20, -1)
			move	i6, r0
			STOP
			STOP
			FINIS:
			""";
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
//		assertEquals(7, diff.size());
		diff.assertDiff(26, 4);
		diff.assertDiff(25, 14);
		diff.assertDiff(24, -1);
		diff.assertDiff(23, 14);
		diff.assertDiff(22, 4);
		diff.assertDiff(21, -1);
	}

	@Test
	void testPrint() {
		String src = """
			START:
			#include <adt/vector.asm>
			#var	v, i
			#call	newVector(1)
			move	v, r0
			#for	0, i < 10, 1
				#macro	PUSH_BACK(v, i)
			#end_for
			#call	printVector(v)
			STOP
			STOP
			FINIS:
			""";
		String expected = "0,1,2,3,4,5,6,7,8,9\n";
		ConsoleOutputCapturer capturer = new ConsoleOutputCapturer();
		capturer.start(ConsoleOutputCapturer.StdStream.STDOUT);
		var tuple = runProgram(src, new String[] {"--DEBUG"});
		String output = capturer.stop();
		var result = tuple.getLeft();
		var sim = tuple.getMiddle();
		var diff = tuple.getRight();
//		assertEquals(7, diff.size());
		assertEquals(expected, output);
	}
}