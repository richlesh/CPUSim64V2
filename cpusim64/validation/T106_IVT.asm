///////////////////////////////////////////////////////////////////////////////
// IVT (Run in Debug Mode)
//
// Tests vectored interrupts: the vector table in the heap, STCR/LDCR of
// control register 0, INT n through a handler ending in IRET, INT -n for a
// built-in service, and the exit hook in vector 0.
//
//	STCR, LDCR, IRET, INT n, INT -n
//
// Expect r5 = table base, r6 = 6 (handler result), r7 = 1 (pid), r8 = 42
// (exit code seen by the hook) and Result: 42.
//
// Author: Richard Lesh
///////////////////////////////////////////////////////////////////////////////

#include <system/system.def>
START:
        move    r1, 256
        int     -20                  // iALLOC: the vector table lives in the heap
        move    r10, r0
        move    r11, 0
FILL:   store   r11, r10[r11]        // zero 256 entries (a fresh block is not cleared)
        add     r11, 1
        cmp     r11, 256
        jump    nz, FILL
        move    r2, HANDLER21
        store   r2, r10[0x21]
        move    r2, EXITHOOK
        store   r2, r10[0]
        stcr    0, r10
        ldcr    r5, 0
        move    r1, 1
        move    r2, 5
        int     0x21                 // vectored: r0 = r1 + r2 via the handler
        move    r6, r0
        int     -34                  // built-in iGET_PID
        move    r7, r0
        move    r1, 42
        int     -32                  // iEXIT: the exit hook runs instead
        stop
        stop
HANDLER21:
        add     r1, r2
        move    r0, r1
        iret
EXITHOOK:
        move    r8, r0
        move    r9, 0
        store   r9, r10[0]           // disarm so the next exit terminates
        move    r1, r8
        int     -32
        stop
