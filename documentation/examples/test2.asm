#call test2(1, \
    2, \            // This is a comment 2
    3,          \ /* This is a comment 3 */
    4) // This is a comment 4
stop
stop

Data: .dcw 1, 2, 3, \   
4, 5, 6,  \ // This is a comment
7, 8, 9, 10,    \ /* this is a comment */     
11, 12, 13, 14, 15, 16, 17, 18, 19, 20

/* This is a function
arguments a,b,c
*/
#def_func test2(a, b, c, d)
#var	s, \
			addr, \					// this is a test
			blockElementSize, \
			blockListSize, \		/* This is a test */
			blockNumElem, \
			destroy, \				/* a
										b
										c */
			init
	load	s, a             // This is a comment
    load    addr, b          /* This is a comment*/  

    // This is also a comment
Label:    load    blockElementSize, c
    move    init,  \ 
            100
    move    blockListSize, /* This is a comment */ 200  
    move    blockNumElem, 300
#end_func
