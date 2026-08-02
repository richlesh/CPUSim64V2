// Fibonacci.asm
// Calculates and prints the first 20 Fibonacci numbers using recursion
// CPUSim64 Assembly

#include <system/io.asm>
#call	main()
int 	iEXIT

//#define STDOUT 0
#define COUNT 20

// Main function
#def_func main()
	#var i

	// Print header
	#call puts(header)

	// Loop from 0 to COUNT-1
	move i, 0
$loop:
	cmp i, COUNT
	jump ge, $done

	// Calculate fibonacci(i)
	#call fibonacci(i)
	// Result is in r0

	// Print the result
	move r1, STDOUT
	move r2, r0
	int 202				// iPUT_DEC - print integer in decimal

	// Print newline
	move r1, STDOUT
	int 200				// iPUT_NL - print newline

	// Increment counter
	add i, i, 1
	jump $loop

$done:
	#return 0
#end_func

// Fibonacci function using recursion
// Returns the nth Fibonacci number
// fib(0) = 0, fib(1) = 1, fib(n) = fib(n-1) + fib(n-2)
#def_func fibonacci(n0)
	#var result, temp, n
	load n, n0
	// Base case: if n <= 1, return n
	cmp n, 1
	jump gt, $recurse
	#return n

$recurse:
	// Calculate fib(n-1)
	sub temp, n, 1
	#call fibonacci(temp)
	move result, r0

	// Calculate fib(n-2)
	sub temp, n, 2
	#call fibonacci(temp)

	// Return fib(n-1) + fib(n-2)
	add result, result, r0
	#return result
#end_func

// Data section
header:
	.dcs "First 20 Fibonacci Numbers:\n"

	stop
	stop
