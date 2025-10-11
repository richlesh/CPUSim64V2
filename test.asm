			START:
			#DEF_MACRO block(x)
			.block block_label_{}
				move	r0, 10
			$LABEL:
				sub		r0, 1
				cmp		r0, ${x}
				jump	z, $LABEL
			.block_end
			#END_MACRO
			#MACRO block(4)
			#MACRO block(8)
			#MACRO block(2)
			FINIS:
