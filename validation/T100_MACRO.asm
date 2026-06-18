///////////////////////////////////////////////////////////////////////////////
// Macro Functionality Test
//
// Test the preprocessor macro capability.
//
// Author: Richard Lesh
// Modified: 2022/10/30
///////////////////////////////////////////////////////////////////////////////

BEGIN:
// Test to load sequence into R or F registers 0-12
#def_macro	setup(r)
	move	${r}0,0
	move	${r}1,1
	move	${r}2,2
	move	${r}3,3
	move	${r}4,4
	move	${r}5,5
	move	${r}6,6
	move	${r}7,7
	move	${r}8,8
	move	${r}9,9
	move	${r}10,10
	move	${r}11,11
	move	${r}12,12
	move	${r}13,13
	move	${r}14,14
	move	${r}15,15
	move	${r}16,16
	move	${r}17,17
	move	${r}18,18
	move	${r}19,19
	move	${r}20,20
	move	${r}21,21
	move	${r}22,22
	move	${r}23,23
	move	${r}24,24
	move	${r}25,25
	move	${r}26,26
	move	${r}27,27
	move	${r}28,28
#end_macro

// Test to load sequence into highest three FP registers
#def_macro	setupF()
	move	F29,29
	move	F30,30
	move	F31,31
#end_macro

MAIN:	
		#macro setup(r)
		#macro setup(f)
		#macro setupF()
END:	stop
		stop
