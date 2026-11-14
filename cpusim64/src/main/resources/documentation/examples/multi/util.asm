  #export add3                  // labels other modules may use
  #extern gCalls                // Required because there is no #include to declare the symbol
  
  #def_func add3(n)
      load    r0, gCalls
      add     r0, 1
      store   r0, gCalls
      load    r0, n
      add     r0, 3
  #end_func
