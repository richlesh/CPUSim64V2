///////////////////////////////////////////////////////////////////////////////
// Interrupt Test (Run in Debug Mode)
//
// Tests the Interrupt instruction.
//
//	INT		R,C
//
// Author: Richard Lesh
// Modified: 2017/02/08
///////////////////////////////////////////////////////////////////////////////

#define iINT_MIN		1
#define iINT_MAX		2
#define iFLOAT_MIN		3
#define iFLOAT_MAX		4
#define iNEGATIVE_INFINITY	5
#define iPOSITIVE_INFINITY	6
#define iNAN			7

BEGIN:
	interrupt	iINT_MIN
	move	r1,r0
	int		iINT_MAX
	move	r2,r0
	int		iFLOAT_MIN
	move	f1,f0
	int		iFLOAT_MAX
	move	f2,f0
	move	r3,iNEGATIVE_INFINITY
	int		r3
	move	f3,f0
	move	r4,iPOSITIVE_INFINITY
	int		r4
	move	f4,f0
	move	r5,iNAN
	int		r5
	move	f5,f0

END:
	stop
	stop
