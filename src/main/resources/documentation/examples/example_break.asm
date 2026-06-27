#include <system/io.asm>

#global LICENSE_PLATE: .DCW 'x','x','x','x'
MAIN:
	#for '1', r1, lt, '4', 1
		store	r1, LICENSE_PLATE[1]
		#for	'1', r2, lt, '4', 1		
			store	r2, LICENSE_PLATE[2]
			#for	'1', r3, lt, '4', 1	
				#if_cond	r3, eq, '2'
					#continue
				#end_cond
				store	r3, LICENSE_PLATE[3]
				#for	'1', r4, lt, '4', 1		
					#if_cond	r4, eq, '2'
						#break
					#end_cond
					store	r4, LICENSE_PLATE[4]
					#macro	FROM_CODEPOINTS(LICENSE_PLATE)
					move	r5, r0
					#call	puts(r5)
					#call	put_nl()
					#call	free(r5)
				#end_for
			#end_for
		#end_for
	#end_for
	stop
	stop
