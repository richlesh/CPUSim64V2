  #include <system/io.asm>
  #include "util.asm"            // each module this program uses
  #include "data.asm"
  #extern add3, gCalls           // labels defined in those modules (optional: not needed here because of prior #include)

  __MAIN__:
      #call   add3(10)
      load    r1, gCalls
      #call   printf("add3(10) = %d after %d call\n", r0, r1)
      move    r0, 0              // exit code
      return
