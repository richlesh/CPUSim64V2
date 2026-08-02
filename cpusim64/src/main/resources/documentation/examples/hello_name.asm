#include <system/io.asm>
#include <system/system.def>

    #call   main()
    int     iEXIT

#def_func   main()
    #var    first, last
    // Prompt the user for their name
    #call   puts("What is your first name? ")
    // Read a line of input from STDIN.
    // Passing 0 as the buffer lets the system allocate one from the heap.
    #call   fgetline(STDIN, 0)
    move    first, r0

    // Prompt the user for their name
    #call   puts("What is your last name? ")
    #call   fgetline(STDIN, 0)
    move    last, r0

    // Greet the user using their name
    #call   printf("Hello, %s %s!\n", first, last)

    // Free the heap-allocated buffer when done
    #call   free(first)
    #call   free(last)
    #return 0
#end_func
    stop
    stop