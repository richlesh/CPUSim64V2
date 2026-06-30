///////////////////////////////////////////////////////////////////////////////
// XORCipher.asm
//
// A simple repeating-key XOR stream cipher for arbitrary binary files.
//
// Each byte of the input is combined with a byte of the keyword using the
// bitwise XOR (exclusive-or) operation. The keyword repeats cyclically for
// the length of the input. Because XOR is its own inverse
// (a XOR b XOR b == a), the SAME command both encrypts and decrypts: running
// the program a second time with the same key on the encrypted output
// reproduces the original input.
//
// The files are opened in RAW mode so that every byte is processed exactly
// as-is, with no newline translation. This makes the cipher safe for binary
// data (images, executables, etc.) as well as text.
//
// SECURITY NOTE: A repeating-key XOR cipher is NOT cryptographically secure.
// It is easily broken (e.g. via frequency analysis) and is intended here as
// an educational example, not for protecting real secrets.
//
// Usage:
//   XORCipher keyword input_file output_file
//
//   keyword      The cipher key. Each byte contributes to the XOR mask; the
//                key repeats over the length of the input.
//   input_file   Path to the file to read (treated as raw bytes).
//   output_file  Path to the file to write the result to (raw bytes).
//
// Exit codes:
//   0  Success (or usage/error message printed)
//
// Author: Richard Lesh
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.asm>
#include <system/system.def>          // READ_MODE, WRITE_MODE

    // Program entry point: run main() then exit with its return code.
    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Parses command line arguments, opens the input and output files in raw
// (binary) mode, and dispatches the XOR transformation to XORCipher().
// Prints a usage message if too few arguments are supplied, or an error
// message if a file fails to open.
//
// Command line layout (argc must be >= 4):
//   args(0) = program name
//   args(1) = keyword
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
        #call   puts("Syntax: XORCipher keyword input_file output_file\n")
    #else_cond
        // ---- Argument 1: the keyword ----
        move    r1, 1
        int     iARGS
        move    key, r0

        // ---- Argument 2: the input filename ----
        move    r1, 2
        int     iARGS
        move    filename, r0

        // Open the input file for raw (binary, no newline translation) reading.
        #call   openRawFile(filename, READ_MODE)
        move    inport, r0

        // A return value of -1 indicates the open failed.
        #if_cond    inport, ne, -1
            // ---- Argument 3: the output filename ----
            move    r1, 3
            int     iARGS
            move    outfilename, r0

            // Open (create/truncate) the output file for raw writing.
            #call   openRawFile(outfilename, WRITE_MODE)
            move    outport, r0

            // A return value of -1 indicates the open failed.
            #if_cond    outport, ne, -1
                // Both files are open: perform the XOR transformation.
                #call   XORCipher(key, inport, outport)

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
// XORCipher(key, inport, outport)
//
// Reads bytes one at a time from the input port, XORs each with the next byte
// of the repeating key, and writes the result to the output port. Continues
// until end-of-file (-1) is read.
//
// Every byte is transformed (unlike a printable-only cipher), which is why
// raw file ports are used. Since XOR is symmetric, this single routine serves
// as both the encryptor and the decryptor.
//
// Parameters:
//   key      Address of the keyword string used to generate the XOR mask.
//   inport   Open raw port to read input bytes from.
//   outport  Open raw port to write the transformed bytes to.
//
// Returns: nothing meaningful (r0 undefined)
///////////////////////////////////////////////////////////////////////////////

#def_func    XORCipher(key, inport, outport)
    #var    byteRead, k, p, po, index, keylen, mask
    // Load function arguments into working registers.
    load    k, key                  // k  = address of keyword string
    load    p, inport               // p  = input port
    load    po, outport             // po = output port

    // Determine the length of the keyword (number of mask bytes).
    #macro  GET_CODEPOINTS(k)
    move    k, r0
    load    keylen, k[0]
    add     k, 1

    // index walks through the key, wrapping back to 0 after keylen bytes.
    clear   index

    // Read the first byte to prime the loop.
    #macro  IN1(byteRead, p)

    // Process input until end-of-file (-1).
    #while  byteRead, ne, -1
        // Fetch the current key byte and XOR it into the input byte.
        load    mask, k[index]
        xor     byteRead, mask

        // Advance the key index, wrapping back to the start of the key.
        add     index, 1
        #if_cond    index, ge, keylen
            clear   index
        #end_cond

        // Write the transformed byte, then read the next input byte.
        #macro  OUT1(byteRead, po)
        #macro  IN1(byteRead, p)
    #end_while
#end_func

    stop
    stop