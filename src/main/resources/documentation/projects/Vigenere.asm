///////////////////////////////////////////////////////////////////////////////
// Vigenere.asm
//
// A Vigenère cipher encryption/decryption utility.
//
// The Vigenère cipher is a polyalphabetic substitution cipher that uses a
// repeating keyword to shift each printable character of the input. Each
// character of the keyword determines the shift amount applied to the
// corresponding character of the plaintext. The keyword repeats cyclically
// for the length of the input.
//
// This implementation operates over the 94 printable ASCII characters in the
// range '!' (0x21) through '~' (0x7E). Whitespace and control characters
// (including spaces and newlines) pass through unchanged. Shifts wrap around
// within the 94-character printable range.
//
// Usage:
//   Vigenere keyword  input_file output_file   (encrypt)
//   Vigenere -keyword input_file output_file   (decrypt)
//
//   keyword      The encryption key. Prefix with '-' to DECRYPT instead of
//                encrypt. Only printable characters in the key contribute to
//                the shift.
//   input_file   Path to the text file to read.
//   output_file  Path to the text file to write the result to.
//
// Exit codes:
//   0  Success (or usage/error message printed)
//
// Author:   Richard Lesh
// Modified: 2026/06/29
// Original: 2025/11/04
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.asm>
#include <system/system.def>          // TRUE, FALSE, READ_MODE, WRITE_MODE

    // Program entry point: run main() then exit with its return code.
    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Parses command line arguments, opens the input and output files, and
// dispatches the encryption/decryption work to Vigenere(). Prints a usage
// message if too few arguments are supplied, or an error message if a file
// fails to open.
//
// Command line layout (argc must be >= 4):
//   args(0) = program name
//   args(1) = keyword (optionally prefixed with '-' for decrypt)
//   args(2) = input filename
//   args(3) = output filename
//
// Returns: 0 in r0
///////////////////////////////////////////////////////////////////////////////

#def_func    main()
    #var    filename, outfilename, key, inport, outport

    // Require at least 4 command line arguments (program + 3 args).
    int     iARGC
    #if_cond    r0, lt, 4
        #call   puts("Syntax: Vigenere [-]keyword input_file output_file")
    #else_cond
        // ---- Argument 1: the keyword ----
        move    r1, 1
        int     iARGS
        move    key, r0

        // ---- Argument 2: the input filename ----
        move    r1, 2
        int     iARGS
        move    filename, r0

        // Open the input file for reading.
        #call   openTextFile(filename, READ_MODE)
        move    inport, r0

        // A return value of -1 indicates the open failed.
        #if_cond    inport, ne, -1
            // ---- Argument 3: the output filename ----
            move    r1, 3
            int     iARGS
            move    outfilename, r0

            // Open (create/truncate) the output file for writing.
            #call   openTextFile(outfilename, WRITE_MODE)
            move    outport, r0

            // A return value of -1 indicates the open failed.
            #if_cond    outport, ne, -1
                // Both files are open: perform the cipher transformation.
                #call   Vigenere(key, inport, outport)

                // Release file resources.
                #call   closeFile(outport)
                #call   closeFile(inport)
            #else_cond
                // Output failed to open; clean up the input we already opened.
                #call   closeFile(inport)
                #call   puts("Output file creation failed!")
            #end_cond
        #else_cond
            #call   puts("Input file open failed!")
        #end_cond
    #end_cond

    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// Vigenere(key, inport, outport)
//
// Reads characters one at a time from the input port, applies the Vigenère
// shift (using the repeating key), and writes the transformed characters to
// the output port. Continues until end-of-file (-1) is read.
//
// Only printable characters in the range '!'..'~' (the 94-character window)
// are transformed; all other characters (spaces, tabs, newlines, etc.) are
// written through unchanged and do NOT advance the key index.
//
// Encryption shifts forward by (keyChar - '!'); decryption shifts backward by
// the same amount. Results wrap within the 94-character printable window.
//
// Parameters:
//   key      Address of the keyword string. A leading '-' selects decrypt
//            mode and is skipped before processing.
//   inport   Open port to read plaintext/ciphertext from.
//   outport  Open port to write the result to.
//
// Returns: nothing meaningful (r0 undefined)
///////////////////////////////////////////////////////////////////////////////

#def_func    Vigenere(key, inport, outport)
    #var    charRead, k, p, po, index, decrypt, keylen, offset
    // Load function arguments into working registers.
    load    k, key                  // k  = address of keyword string
    #macro  GET_CODEPOINTS(k)
    move    k, r0
    load    p, inport               // p  = input port
    load    po, outport             // po = output port

    // Check for a leading '-' which selects DECRYPT mode.
    // (k[0] is the first character byte of the string.)
    load    keylen, k[0]
    move    decrypt, FALSE
    load    charRead, k[1]
    #if_cond    charRead, eq, '-'
        move    decrypt, TRUE
        add     k, 2                // Skip past the '-' so the real key begins.
        sub     keylen, 1
    #else_cond
        add     k, 1
    #end_cond

    // index walks through the key, wrapping back to 0 after keylen characters.
    clear   index

    // Read the first character to prime the loop.
    #macro  IN0(charRead, p)

    // Process input until end-of-file (-1).
    #while  charRead, ne, -1
        // Determine whether charRead is a printable character: '!' <= c <= '~'.
        #macro  COMPARE(charRead, gt, ' ')      // c > ' '  -> r0
        move    r1, r0
        #macro  COMPARE(charRead, le, '~')      // c <= '~' -> r0
        and     r0, r1                          // r0 = both conditions true?

        #if_cond    r0, eq, TRUE
            // Map character into the 0..93 window.
            sub     charRead, '!'

            // Compute the shift amount from the current key character.
            load    offset, k[index]
            sub     offset, '!'

            // For decryption, the shift is reversed.
            #if_cond    decrypt, eq, TRUE
                neg     offset
            #end_cond

            // Apply the shift.
            add     charRead, offset

            // Wrap the result back into the valid 0..93 window.
            #while  charRead, lt, 0
                add charRead, 94
            #end_while
            #while  charRead, ge, 94
                sub charRead, 94
            #end_while

            // Map back from the window into printable ASCII.
            add     charRead, '!'

            // Advance the key index (only for transformed characters),
            // wrapping back to the start of the key when needed.
            add     index, 1
            #if_cond    index >= keylen
                clear   index
            #end_cond
        #end_cond

        // Write the (possibly transformed) character to the output.
        #call   fputc(po, charRead)

        // Read the next character.
        #macro  IN0(charRead, p)
    #end_while
#end_func

    stop
    stop