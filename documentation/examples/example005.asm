#include <system/system.def>

	move	r0,' '
	debug	r0
	move	r1,'A'
	move	r2,'\u263A'
	debug	r1,r2
	move	r3,-5
	move	r4,326
	move	r5,0x123456789abcd
	debug	r3,r4,r5
	int		iPrintCPUState
	move	f0,-10
	move	f1,-1
	move	f2,3.1415926
	move	f3,1.e9
	debug	f0,f1,f2,f3
	debug	SP,SF,PC
	stop
	stop
