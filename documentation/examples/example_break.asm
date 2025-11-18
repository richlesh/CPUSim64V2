#include <system/io.asm>

LICENSE_PLATE: DCS "xxxx"
MAIN:
	#for r1, '1', lt, '4', 1
		store	r1, LICENSE_PLATE[0]
		#for	r2, '1', lt, '4', 1		
			store	r2, LICENSE_PLATE[1]
			#for	r3, '1', lt, '4', 1	
				#if_cond	r3, eq, '2'
					#continue
				#end_cond
				store	r3, LICENSE_PLATE[2]
				#for	r4, '1', lt, '4', 1		
					#if_cond	r4, eq, '2'
						#break
					#end_cond
					store	r4, LICENSE_PLATE[3]
					#call	puts(LICENSE_PLATE)
					#call	put_nl()
				#end_for
			#end_for
		#end_for
	#end_for
	stop
	stop
