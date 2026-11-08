///////////////////////////////////////////////////////////////////////////////
// unicode_test.asm
//
// Prints several ranges of Unicode codepoints to STDOUT, 32 characters per
// line. CPUSim64 version of the Java UnicodeTest program.
//
// Ranges printed:
//   0x0020 - 0x007F   ASCII
//   0x00A0 - 0x01FF   Latin-1 Supplement / Latin Extended-A/B
//   0x2200 - 0x22FF   Mathematical Operators
//   0x2500 - 0x25FF   Box Drawing / Block Elements / Geometric Shapes
//   0x2600 - 0x27FF   Miscellaneous Symbols / Dingbats / Arrows
//   0x1F300 - 0x1F4FF Miscellaneous Symbols and Pictographs (Icons)
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/system.asm>

    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Prints each Unicode range in turn.
//
// Returns:
//   r0 = 0
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    // ASCII
    #call   putline("ASCII")
    #call   print_range(0x20, 0x7f)
    // Basic Multilingual
    #call   putline("Basic Multilingual")
    #call   print_range(0xa0, 0x1ff)
    // Math Symbols
    #call   putline("Math Symbols")
    #call   print_range(0x2200, 0x22ff)
    // Drawing
    #call   putline("Drawing")
    #call   print_range(0x2500, 0x25ff)
    // Symbols
    #call   putline("Symbols")
    #call   print_range(0x2600, 0x27ff)
    // Icons
    #call   putline("Icons")
    #call   print_range(0x1f300, 0x1f4ff)
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// print_range(first, last)
//
// Prints every Unicode codepoint from first to last (inclusive) to STDOUT.
// A newline is printed after each codepoint where (c % 32 == 31), giving
// 32 characters per line.
//
// Arguments:
//   first - first codepoint to print (inclusive)
//   last  - last codepoint to print (inclusive)
//
// No return value.
///////////////////////////////////////////////////////////////////////////////

#def_func   print_range(first, last)
    #var    c, start_cp, end_cp
    load    start_cp, first
    load    end_cp, last
    #for    start_cp, c <= end_cp, 1
        #call   putc(c)
        // if (c % 32 == 31) print newline
        and     r0, c, 31
        #if_cond    r0 == 31
            #call   put_nl()
        #end_cond
    #end_for
#end_func

    stop
    stop