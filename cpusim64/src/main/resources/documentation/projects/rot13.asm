///////////////////////////////////////////////////////////////////////////////
// rot13.asm
//
// Applies the classic ROT13 substitution cipher to a text file.
//
// ROT13 ("rotate by 13 places") replaces each ASCII letter with the letter
// 13 positions later in the alphabet, wrapping around from Z back to A. Because
// the alphabet has 26 letters, applying ROT13 twice restores the original text,
// so the same program both encrypts and decrypts. Non-letter characters are
// passed through unchanged.
//
// Usage:
//      rot13 <input_file> <output_file>
//
//      <input_file>    Path to the text file to read or STDIN
//      <output_file>   Path to the text file to create/overwrite or STDOUT
//
// Example:
//      > run.sh rot13 plain.txt cipher.txt
//      > run.sh rot13 cipher.txt roundtrip.txt   // roundtrip.txt == plain.txt
//
// Author:   Richard Lesh
// Modified: 2026/06/28
// Original: 2022/11/28
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.asm>
#include <system/system.def>

    #call   main()
    #call   exit(r0)       // Exit to OS with main's return value

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Parses the command line, opens the input and output files, and drives the
// ROT13 transformation. Prints an error message and aborts gracefully if the
// arguments are missing or a file cannot be opened.
//
// Arguments: none (reads from the command line via iARGC / iARGS)
// Returns:   0 (always; errors are reported but still return 0)
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    argc, filename, outfilename, inport, outport

    // Require both an input and an output filename (argv[0] is the program name).
    int     iARGC
    move    argc, r0
    #if_cond    argc >= 2
        // Get the first command line argument (input filename).
        #call   args(1)
        move    filename, r0

        // Open the input file for reading.
        #call   openTextFile(filename, READ_MODE)
        move    inport, r0

        // openTextFile returns -1 on failure.
        #if_cond    inport == -1
            #call   printf("Can't open %s for reading.\n", filename)
            #return 1
        #end_cond
    #else_cond
        move    inport, STDIN
    #end_cond

    #if_cond    argc >= 3
        // Get the second command line argument (output filename).
        #call   args(2)
        move    outfilename, r0

        // Open the input file for reading.
        #call   openTextFile(outfilename, WRITE_MODE)
        move    outport, r0

        // openTextFile returns -1 on failure.
        #if_cond    outport == -1
            #call   printf("Can't open %s for writing.\n", outfilename)
            #return 1
        #end_cond
    #else_cond
        move    outport, STDOUT
    #end_cond

    // Both files are open: transform the stream.
    #call   rot13(inport, outport)
    // Close both files.
    #call   closeFile(outport)
    #call   closeFile(inport)

    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// Lookup tables for the letter substitution.
//
// Each table is the alphabet rotated by 13. To translate a letter, subtract the
// alphabet's base ('A' or 'a') to get a 0-based index, then read the rotated
// letter at that index. The leading length word of a .DCS string is skipped by
// the load offset, so UPPER[0] yields 'N', UPPER[1] yields 'O', etc.
///////////////////////////////////////////////////////////////////////////////
UPPER: .dcw  "NOPQRSTUVWXYZABCDEFGHIJKLM"
LOWER: .dcw  "nopqrstuvwxyzabcdefghijklm"

///////////////////////////////////////////////////////////////////////////////
// rot13(inport, outport)
//
// Reads characters one at a time from inport, applies the ROT13 substitution to
// alphabetic characters, and writes the result to outport. Reading stops at
// end-of-file (when a read returns -1).
//
// Arguments:
//      inport   An open input port (file handle) to read from.
//      outport  An open output port (file handle) to write to.
// Returns:   nothing
///////////////////////////////////////////////////////////////////////////////
#def_func   rot13(inport, outport)
    #var    charRead, p, po, index     // charRead: current char, p/po: ports
    push    r1                         // Preserve r1 (used as scratch below)
    load    p, inport
    load    po, outport
LOOP1:
    // Read a single Unicode codepoint from the input port.
    #macro  IN0(charRead, p)

    // A returned -1 indicates end-of-file.
    cmp     charRead, -1
    jump    eq, LOOP_END1

    // Is the character an uppercase letter 'A'..'Z'?
    #macro  COMPARE(charRead, ge, 'A')
    move    r1, r0                     // r1 <- (charRead >= 'A')
    #macro  COMPARE(charRead, le, 'Z')
    and     r0, r1                     // r0 <- (charRead >= 'A') AND (charRead <= 'Z')
    #if_cond    r0, eq, TRUE
        // Uppercase: index into the rotated uppercase table.
        sub     r0, charRead, 'A'
        add     r0, 1
        load    charRead, UPPER[r0]
    #else_cond
        // Is the character a lowercase letter 'a'..'z'?
        #macro  COMPARE(charRead, ge, 'a')
        move    r1, r0                 // r1 <- (charRead >= 'a')
        #macro  COMPARE(charRead, le, 'z')
        and     r0, r1                 // r0 <- (charRead >= 'a') AND (charRead <= 'z')
        #if_cond    r0, eq, TRUE
            // Lowercase: index into the rotated lowercase table.
            sub     r0, charRead, 'a'
            add     r0, 1
            load    charRead, LOWER[r0]
        #end_cond
        // Otherwise the character is non-alphabetic and passes through unchanged.
    #end_cond

    // Write the (possibly transformed) character to the output port.
    #call   fputc(po, charRead)
    jump    LOOP1
LOOP_END1:
    pop     r1                         // Restore r1
#end_func

    stop
    stop