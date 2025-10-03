#include <system/io.asm>

LICENSE_PLATE: DCS "xxxx"
MAIN:
	#for r1, '1', lt, '4', 1
		store	r1, LICENSE_PLATE[0]
		#for	r2, '1', lt, '4', 1		
			store	r2, LICENSE_PLATE[1]
			#for	r3, '1', lt, '4', 1	
				#cond	r3, eq, '2'
					#continue
				#endcond	
				store	r3, LICENSE_PLATE[2]
				#for	r4, '1', lt, '4', 1		
					#cond	r4, eq, '2'
						#break
					#endcond
					store	r4, LICENSE_PLATE[3]
					#call	puts(LICENSE_PLATE)
					#call	put_nl()
				#endfor
			#endfor
		#endfor
	#endfor
	stop
	stop
