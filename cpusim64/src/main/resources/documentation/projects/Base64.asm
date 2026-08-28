///////////////////////////////////////////////////////////////////////////////
// Base64.asm
// Base64 encoder/decoder for Base64 file algorithm.
//
// Usage: Base64 <e|d> <input_file> <output_file>
//
// Author:   Richard Lesh
// Original: 2025/11/17
// Modified: 2026/08/26
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.asm>
#include <system/system.asm>

    #call   main()
    #call   exit(r0)

#def_func   main()
    #var    filename, outfilename, mode, inport, outport, codepoints
    // if (argc < 4)
    int     iARGC
    #if_cond    r0, lt, 4
        #call   putline("Syntax: Base64 <e|d> <input_file> <output_file>")
        #return 1
    #else_cond
        // Get first command line argument and put it in mode.
        #call   args(1)
        #macro  GET_CODEPOINTS(r0)
        move    codepoints, r0
        load    r0, codepoints[0]
        #if_cond r0
            load    mode, codepoints[1]
            #macro  TO_LOWER(mode)
            move    mode, r0
        #else_cond
            move    mode, ' '
        #end_cond
        #call   free(codepoints)

        // Get second command line argument and put it in filename.
        #call   args(2)
        move    filename, r0
        // Open file in read mode.
        #if_cond    mode, eq, 'e'
            #call   openRawFile(filename, READ_MODE)
        #else_if_cond   mode, eq, 'd'
            #call   openTextFile(filename, READ_MODE)
        #else_cond
            #call   putline("Mode can only be E or D!")
            #return 1
        #end_cond
        move    inport, r0
        // If the port returned is -1 we failed.
        #if_cond    inport, ne, -1
            // Get third command line argument and put it in outfilename.
            #call   args(3)
            move    outfilename, r0
            // Open text file in write mode.
            #if_cond    mode, eq, 'e'
                #call   openTextFile(outfilename, WRITE_MODE)
            #else_cond
                #call   openRawFile(outfilename, WRITE_MODE)
            #end_cond
            move    outport, r0
            // If the port returned is -1 we failed.
            #if_cond    outport, ne, -1
                // Process the input stream
                #call   Base64(mode, inport, outport)
                // Close the files
                #call   closeFile(outport)
                #call   closeFile(inport)
            #else_cond
                #call   closeFile(inport)
                #call   putline("Output file creation failed!")
                #return 3
            #end_cond
        #else_cond
            #call   putline("Input file open failed!")
            #return 2
        #end_cond
    #end_cond

    #return 0
#end_func

#define PAD_CHAR '='
ALPHABET: .dcs "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
DECODE_TABLE: .dcw \
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,\
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,64,-1,-1,\
    -1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,\
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1

///////////////////////////////////////////////////////////////////////////////
// Base64(mode, inport, outport)
//
// Converts a file on the inport to the outport based on mode ('e' or 'd')
// using Base64 coding.
///////////////////////////////////////////////////////////////////////////////

#def_func   Base64(mode, inport, outport)
    #var    inputValue,outputValue,m,p,po,index,buffer,which,line_remainder,alphabet_codepoints,max_decode_value

    load    m, mode
    load    p, inport
    load    po, outport
    
    #macro  GET_CODEPOINTS(ALPHABET)
    move    alphabet_codepoints, r0
    load    max_decode_value, DECODE_TABLE[0]

    clear   index
    clear   buffer
// Read a character
    #if_cond    m, eq, 'e'
        #macro  IN1(inputValue,p)
    #else_cond
        #macro  IN0(inputValue,p)
    #end_cond
// If it is -1 we are at EOF.
    #while  inputValue, ne, -1
        #if_cond    m, eq, 'e'
            div r0, which, index, 3
            #if_cond    which, eq, 0
                move    buffer, inputValue
                rshift  outputValue, buffer, 2
                lshift  buffer, 8
                and     buffer, 0x300
                add     outputValue, 1
                load    r0, alphabet_codepoints[outputValue]
                #macro  OUT0(r0,po)
            #else_if_cond   which, eq, 1
                or      buffer, inputValue
                rshift  outputValue, buffer, 4
                lshift  buffer, 8
                and     buffer, 0xF00
                add     outputValue, 1
                load    r0, alphabet_codepoints[outputValue]
                #macro  OUT0(r0,po)
            #else_cond
                or      buffer, inputValue
                rshift  outputValue, buffer, 6
                add     outputValue, 1
                load    r0, alphabet_codepoints[outputValue]
                #macro  OUT0(r0,po)
                and     outputValue, buffer, 0x3F
                add     outputValue, 1
                load    r0, alphabet_codepoints[outputValue]
                #macro  OUT0(r0,po)
            #end_cond
            add     index, 1
            div     r0, line_remainder, index, 57
            #if_cond    line_remainder, eq, 0
                #call   fput_nl(po)
            #end_cond
        #else_cond
            add     inputValue, 1
            #macro  COMPARE_RANGE(1, le, inputValue, le, max_decode_value)
            #if_cond    r0
                load    inputValue, DECODE_TABLE[inputValue]
            #else_cond
                move    inputValue, -1
            #end_cond

            #if_cond    inputValue, eq, 64
                #break
            #end_cond
            #if_cond    inputValue, ne, -1
                div r0, which, index, 4
                #if_cond    which, eq, 0
                    move    buffer, inputValue
                #else_if_cond   which, eq, 1
                    lshift  buffer, 6
                    or      buffer, inputValue
                    rshift  outputValue, buffer, 4
                    and     buffer, 0xF
                    #macro  OUT1(outputValue,po)
                #else_if_cond   which, eq, 2
                    lshift  buffer, 6
                    or      buffer, inputValue
                    rshift  outputValue, buffer, 2
                    and     buffer, 0x3
                    #macro  OUT1(outputValue,po)
                #else_cond
                    lshift  buffer, 6
                    or      buffer, inputValue
                    #macro  OUT1(buffer,po)
                #end_cond
                add     index, 1
            #end_cond
        #end_cond

        #if_cond    m, eq, 'e'
            #macro  IN1(inputValue,p)
        #else_cond
            #macro  IN0(inputValue,p)
        #end_cond
    #end_while

// Add padding if needed
    #if_cond    m, eq, 'e'
        div r0, which, index, 3
        #if_cond    which, eq, 1
            rshift  outputValue, buffer, 4
            add     outputValue, 1
            load    r0, alphabet_codepoints[outputValue]
            #macro  OUT0(r0,po)
            #macro  OUT0(PAD_CHAR,po)
            #macro  OUT0(PAD_CHAR,po)
        #else_if_cond   which, eq, 2
            rshift  outputValue, buffer, 6
            add     outputValue, 1
            load    r0, alphabet_codepoints[outputValue]
            #macro  OUT0(r0,po)
            #macro  OUT0(PAD_CHAR,po)
        #end_cond
        #if_cond    line_remainder, ne, 0
            #call   fput_nl(po)
        #end_cond
    #end_cond
    #call   free(alphabet_codepoints)
#end_func
    stop
    stop
