#include <adt/vector.asm>
#include <system/io.asm>
#include <system/system.def>

	#call	main()
	int		iEXIT

SIZE: dci	5

#def_func	main()
	#var	i, j, v, s
	
	load	s, SIZE
	#call	fprintf(STDOUT, "Allocating vector size %d\n", s)
	#call	newVector(s)
	move	v, r0
	#call	vectorStats(v)

	#call	puts("Add 20...\n")
	#for	i, 0, lt, 20, 1
		#call	vectorAdd(v, i)
	#endfor
	#call	vectorStats(v)
	#call	vectorPrint(v)

	#call	puts("Trim...\n")
	#call	vectorTrim(v)
	#call	vectorStats(v)

	#call	puts("Add 80...\n")
	#for	i, 0, lt, 80, 1
		#call	vectorAdd(v, i)
	#endfor
	#call	vectorStats(v)
	#call	vectorPrint(v)

	#call	puts("Delete 80...\n")
	#for	i, 0, lt, 80, 1
		#call	vectorRemoveAtEnd(v)
	#endfor
	#call	vectorStats(v)
	#call	vectorPrint(v)

	#call	puts("Trim...\n")
	#call	vectorTrim(v)
	#call	vectorStats(v)

	#call	puts("Delete even...\n")
	#for	i, 20, ge, 0, -2
		#call	vectorRemoveAt(v, i)
	#endfor
	#call	vectorStats(v)
	#call	vectorPrint(v)

	#call	puts("Add even...\n")
	#for	i, 0, le, 20, 2
		#call	vectorAddAt(v, i, i)
	#endfor
	#call	vectorStats(v)
	#call	vectorPrint(v)
	
	#call	puts("Set/Get...\n")
	#call	vectorAt(v,0)
	#call	fprintf(STDOUT, "v[%d] = %d\n", 0, r0)
	#call	vectorAt(v,10)
	#call	fprintf(STDOUT, "v[%d] = %d\n", 10, r0)
	#call	vectorAt(v,20)
	#call	fprintf(STDOUT, "v[%d] = %d\n", 20, r0)
	#call	vectorLast(v)
	#call	fprintf(STDOUT, "v[last] = %d\n", r0)
	#call	vectorSetAt(v,-1,0)
	#call	vectorSetAt(v,-2,10)
	#call	vectorSetAt(v,-3,20)
	#call	vectorStats(v)
	#call	vectorPrint(v)

	#call	puts("IndexOf...\n")
	#call	vectorIndexOf(v, -1, 0)
	#call	fprintf(STDOUT, "indexOf(%d, %d) = %d\n", -1, 0, r0)
	#call	vectorIndexOf(v, -2, 0)
	#call	fprintf(STDOUT, "indexOf(%d, %d) = %d\n", -2, 0, r0)
	#call	vectorIndexOf(v, -3, 0)
	#call	fprintf(STDOUT, "indexOf(%d, %d) = %d\n", -3, 0, r0)
	#call	vectorIndexOf(v, 30, 0)
	#call	fprintf(STDOUT, "indexOf(%d, %d) = %d\n", 30, 0, r0)
	#call	vectorIndexOf(v, -1, 10)
	#call	fprintf(STDOUT, "indexOf(%d, %d) = %d\n", -1, 10, r0)
	#call	vectorIndexOf(v, -2, 10)
	#call	fprintf(STDOUT, "indexOf(%d, %d) = %d\n", -2, 10, r0)
	#call	vectorIndexOf(v, -3, 10)
	#call	fprintf(STDOUT, "indexOf(%d, %d) = %d\n", -3, 10, r0)
	#call	vectorIndexOf(v, 30, 10)
	#call	fprintf(STDOUT, "indexOf(%d, %d) = %d\n", 30, 10, r0)
	
	#call	puts("LastIndexOf...\n")
	#call	vectorLastIndexOf(v, -1, -1)
	#call	fprintf(STDOUT, "LastIndexOf(%d, %d) = %d\n", -1, -1, r0)
	#call	vectorLastIndexOf(v, -2, -1)
	#call	fprintf(STDOUT, "LastIndexOf(%d, %d) = %d\n", -2, -1, r0)
	#call	vectorLastIndexOf(v, -3, -1)
	#call	fprintf(STDOUT, "LastIndexOf(%d, %d) = %d\n", -3, -1, r0)
	#call	vectorLastIndexOf(v, 30, -1)
	#call	fprintf(STDOUT, "LastIndexOf(%d, %d) = %d\n", 30, -1, r0)
	#call	vectorLastIndexOf(v, -1, 10)
	#call	fprintf(STDOUT, "LastIndexOf(%d, %d) = %d\n", -1, 10, r0)
	#call	vectorLastIndexOf(v, -2, 10)
	#call	fprintf(STDOUT, "LastIndexOf(%d, %d) = %d\n", -2, 10, r0)
	#call	vectorLastIndexOf(v, -3, 10)
	#call	fprintf(STDOUT, "LastIndexOf(%d, %d) = %d\n", -3, 10, r0)
	#call	vectorLastIndexOf(v, 30, 10)
	#call	fprintf(STDOUT, "LastIndexOf(%d, %d) = %d\n", 30, 10, r0)
	
	#call	puts("Clear...\n")
	#call	vectorClear(v)
	#call	vectorStats(v)
	#call	vectorPrint(v)

	#return	0
#end_func

#def_func vectorStats(vector)
	#var	v, len, size, data, isEmpty
	load	v, vector
	#call	vectorIsEmpty(v)
	move	isEmpty, r0
	#call	vectorSize(v)
	move	len, r0
	#call	vectorCapacity(v)
	move	size, r0
	load	data, v[_VECTOR_DATA]
	#call	fprintf(STDOUT, "Vector(%d): %x %d (%d)\n", isEmpty, data, len, size)
#end_func

	stop
	stop
