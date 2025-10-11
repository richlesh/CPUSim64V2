.LINE «test.asm», 1
START:
.LINE_BEGIN «test.asm», 11
.block block_label_{}
move	r0, 10
$LABEL:
sub		r0, 1
cmp		r0, 4
jump	z, $LABEL
.block_end
.LINE_END
.LINE_BEGIN «test.asm», 12
.block block_label_{}
move	r0, 10
$LABEL:
sub		r0, 1
cmp		r0, 8
jump	z, $LABEL
.block_end
.LINE_END
.LINE_BEGIN «test.asm», 13
.block block_label_{}
move	r0, 10
$LABEL:
sub		r0, 1
cmp		r0, 2
jump	z, $LABEL
.block_end
.LINE_END
.LINE «test.asm», 14
FINIS:

