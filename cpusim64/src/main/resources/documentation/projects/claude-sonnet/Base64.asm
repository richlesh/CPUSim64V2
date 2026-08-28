///////////////////////////////////////////////////////////////////////////////
// Base64.asm
// Base64 encoder/decoder for Base64 file algorithm.
//
// Usage: Base64 [e|d] input_file output_file
//
// Author:   Richard Lesh
// Original: 2025/11/17
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.asm>
#include <system/system.asm>

    #call   main()
    #call   exit(r0)

#def_func   main()
    #var    filename, outfilename, mode, inport, outport, codepoints, arg1_codepoints
    #var    alphabet_codepoints, buffer_size
    // if (argc < 4)
    int     iARGC
    #if_cond    r0, lt, 4
        #call   puts("Syntax: Base64 [e|d] input_file output_file\n")
    #else_cond
        // Get first command line argument and put it in arg1_codepoints
        #call   args(1)
        move    arg1_codepoints, r0
        
        // Extract first character as the mode
        load    r1, arg1_codepoints[1]  // First byte of string (UTF-8)
        move    mode, r1
        #call   free(arg1_codepoints)

        // Get second command line argument and put it in filename.
        #call   args(2)
        move    filename, r0
        
        // Open file in read mode.
        #if_cond    mode, eq, 'e'
            #call   openRawFile(filename, READ_MODE)
        #else_cond
            #call   openTextFile(filename, READ_MODE)
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
                #call   puts("Output file creation failed!")
            #end_cond
        #else_cond
            #call   puts("Input file open failed!")
        #end_cond
    #end_cond

    #return 0
#end_func

ALPHABET: .dcs "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="
DECODE_TABLE: .dcw 96, \  // Size of decode table (96 entries)
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,\
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,64,-1,-1,\
    -1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,\
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1

#def_func   Base64(mode, inport, outport)
    #var    inputValue,outputValue,m,p,po,index,buffer,which,alphabet_codepoints

    // Load parameters into local registers
    load    m, mode
    load    p, inport
    load    po, outport
    
    // Get codepoints from alphabet string
    #call   GET_CODEPOINTS(ALPHABET)
    move    alphabet_codepoints, r0
    
    clear   index
    clear   buffer
// Read a character
    #if_cond    m, eq, 'e'
        IN1(inputValue, p)      // Read raw byte for encoding
    #else_cond
        IN0(inputValue, p)      // Read Unicode codepoint for decoding
    #end_cond
// If it is -1 we are at EOF.
    #while  inputValue, ne, -1
        #if_cond    m, eq, 'e'
            div r0, which, index, 3   // which = index % 3
            #if_cond    which, eq, 0
                move    buffer, inputValue  // buffer = inputValue << 16 (high 8 bits)
                lshift  buffer, 16          // Shift to high byte
                or      buffer, inputValue  // Combine with inputValue in middle byte
                lshift  buffer, 8           // Shift left
                or      buffer, inputValue  // Add third byte in low position
                rshift  outputValue, buffer, 18  // Extract first 6 bits
                add     outputValue, 1
                load    r0, alphabet_codepoints[outputValue]
                OUT0(r0, po)
            #else_if_cond   which, eq, 1
                lshift  buffer, 8
                or      buffer, inputValue  // buffer = (buffer << 8) | inputValue
                rshift  outputValue, buffer, 10  // Extract second 6 bits
                add     outputValue, 1
                load    r0, alphabet_codepoints[outputValue]
                OUT0(r0, po)
            #else_cond
                lshift  buffer, 8
                or      buffer, inputValue  // buffer = (buffer << 8) | inputValue
                rshift  outputValue, buffer, 4   // Extract third 6 bits
                add     outputValue, 1
                load    r0, alphabet_codepoints[outputValue]
                OUT0(r0, po)
                and     buffer, 0x3F        // Extract last 6 bits for fourth output
                add     outputValue, 1
                load    r0, alphabet_codepoints[outputValue]
                OUT0(r0, po)
            #end_cond
            add     index, 1
            div     r0, which, index, 4   // which = index % 4
            #if_cond    which, eq, 0
                #call   fput_nl(po)
            #end_cond
        #else_cond
            // Decode logic
            sub     inputValue, 43          // Convert ASCII to table index (43 = '-')
            #if_cond    inputValue, lt, 0
                #continue
            #end_cond
            load    inputValue, DECODE_TABLE[inputValue]
            #if_cond    inputValue, eq, 64
                #break
            #end_cond
            #if_cond    inputValue, ne, -1
                div     r0, which, index, 4   // which = index % 4
                #if_cond    which, eq, 0
                    move    buffer, inputValue
                #else_if_cond   which, eq, 1
                    lshift  buffer, 6        // buffer = inputValue1 << 6
                    or      buffer, inputValue // buffer = (inputValue1 << 6) | inputValue2
                    rshift  outputValue, buffer, 8 // Extract first output byte
                    #call   putc(outputValue)
                #else_if_cond   which, eq, 2
                    lshift  buffer, 6
                    or      buffer, inputValue
                    rshift  outputValue, buffer, 4
                    and     buffer, 0xF
                    #call   putc(outputValue)
                #else_cond
                    lshift  buffer, 6
                    or      buffer, inputValue
                    #call   putc(buffer)
                #end_cond
                add     index, 1
            #end_cond
        #end_cond

        // Read next input character
        #if_cond    m, eq, 'e'
            IN1(inputValue, p)
        #else_cond
            IN0(inputValue, p)
        #end_cond
    #end_while

// Add padding if needed
    #if_cond    m, eq, 'e'
        div     r0, which, index, 3   // which = index % 3
        #if_cond    which, eq, 1
            rshift  outputValue, buffer, 16  // buffer is in high 8 bits
            add     outputValue, 1
            load    r0, alphabet_codepoints[outputValue]
            OUT0(r0, po)
            load    r0, alphabet_codepoints[64]  // '=' character
            OUT0(r0, po)
            OUT0(r0, po)  // Add second padding
        #else_if_cond   which, eq, 2
            rshift  outputValue, buffer, 8  // buffer is in high 16 bits
            add     outputValue, 1
            load    r0, alphabet_codepoints[outputValue]
            OUT0(r0, po)
            load    r0, alphabet_codepoints[64]  // '=' character
            OUT0(r0, po)
        #end_cond
        OUT0('\n', po)
    #end_cond
    #call   free(alphabet_codepoints)
#end_func
    stop
    stop
