///////////////////////////////////////////////////////////////////////////////
// Caesar.asm
// Caesar cipher encrypt/decrypt.
//
// Usage: Caesar <key> <input_file> <output_file>
//
//    key can be in the range [1,94] to encrypt.
//    key can be in the range [-94,-1] to decrypt.
//
// Author:   Richard Lesh
// Original: 2025/11/04
// Modified: 2026/08/26
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/math.asm>
#include <system/string.def>
#include <system/string.asm>
#include <system/system.def>
#include <system/system.asm>

    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Parses command line arguments (key, input file, output file), opens the
// files, and invokes Caesar() to perform the encryption/decryption. Prints
// a usage message and returns non-zero if arguments are missing or a file
// fails to open.
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    filename, outfilename, key, inport, outport
    // if (argc < 4)
    int     iARGC
    #if_cond    r0, lt, 4
        #call   putline("Syntax: Caesar <key> <input_file> <output_file>")
        #call   putline("    key can be in the range [1,94] to encrypt.")
        #call   putline("    key can be in the range [-94,-1] to decrypt.")
        #call   putline("    key == 0 is a no-op")
        #return 1
    #else_cond
        // Get first command line argument and put it in key.
        #call   args(1)
        #macro  PARSE_INT(r0)
        // Normalize key into [-94,94] regardless of sign/magnitude of input
        #call   mod(r0, 95)
        move    key, r0
        // Get second command line argument and put it in filename.
        #call   args(2)
        move    filename, r0
        // Open text file in read mode.
        #call   openTextFile(filename, READ_MODE)
        move    inport, r0
        // If the port returned is -1 we failed.
        #if_cond    inport, ne, -1
            // Get third command line argument and put it in outfilename.
            #call   args(3)
            move    outfilename, r0
            // Open text file in write mode.
            #call   openTextFile(outfilename, WRITE_MODE)
            move    outport, r0
            // If the port returned is -1 we failed.
            #if_cond    outport, ne, -1
                // Process the input stream
                #call   Caesar(key, inport, outport)
                // Close the files
                #call   closeFile(outport)
                #call   closeFile(inport)
            #else_cond
                #call   closeFile(inport)
                #call   putline("Output file creation failed!")
                #return 2
            #end_cond
        #else_cond
            #call   putline("Input file open failed!")
            #return 3
        #end_cond
    #end_cond

    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// Caesar(key, inport, outport)
//
// Reads UTF-8 text from inport one codepoint at a time, applies a Caesar
// shift of `key` positions to printable ASCII characters (' ' through '~'),
// and writes the result to outport. Non-printable characters (including
// newlines) are passed through unchanged.
//
//    key can be in the range [1,94] to encrypt.
//    key can be in the range [-94,-1] to decrypt.
//    key == 0 is a no-op
///////////////////////////////////////////////////////////////////////////////

#def_func   Caesar(key, inport, outport)
    #var    charRead, k, p, po
    load    k, key
    load    p, inport
    load    po, outport
    #macro  IN0(charRead,p)
    #while  charRead, ne, -1
        #macro  COMPARE_RANGE(' ', le, charRead, le, '~')
        #if_cond    r0
            sub     charRead, ' '
            add     charRead, k
            #while  charRead, lt, 0
                add charRead, 95
            #end_while
            #while  charRead, ge, 95
                sub charRead, 95
            #end_while
            add     charRead, ' '
        #end_cond
    
    // Output the character
        #macro  OUT0(charRead,po)
        #macro  IN0(charRead,p)
    #end_while
#end_func
    stop
    stop
