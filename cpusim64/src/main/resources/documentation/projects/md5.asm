///////////////////////////////////////////////////////////////////////////////
// md5.asm
//
// Computes the MD5 message digest (RFC 1321) of a raw binary stream.
//
// Usage:
//   md5              — reads from STDIN and prints the MD5 hash
//   md5 <filename>   — reads the named file and prints "<filename>:<hash>"
//
// The 128-bit digest is printed as 32 hex digits in standard little-endian
// byte order, for example:
//   a810f89e9f8e213aebd06b9f8c5157d8 shakespeare.txt
//
// MD5 Algorithm Summary (RFC 1321):
//   1. Append a single 0x80 byte after the message data.
//   2. Append 0x00 bytes until the length ≡ 56 (mod 64) bytes.
//   3. Append the original message length in bits as a 64-bit little-endian
//      integer, making the total a multiple of 512 bits (64 bytes).
//   4. Process each 512-bit block through 64 rounds using four auxiliary
//      boolean functions (F/G/H/I) and 64 sine-derived constants (K table).
//   5. Accumulate each block result into four 32-bit digest words (A,B,C,D).
//
// Author:   Richard Lesh
// Date:     2022/11/26
// Modified: 2026/06/28
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/math.def>
#include <system/string.def>
#include <system/system.def>

// Performance Optimizations
#define USE_PRECOMPUTED_K_ARRAY
#define INLINE_STORE_BYTE_IN_BUFFER

    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Program entry point. Determines the input source based on command line
// arguments:
//   - If no filename is supplied (argc < 2), reads from STDIN and prints
//     "<hash>".
//   - If a filename is supplied, opens it as a raw file and prints
//     "<hash> <filename>".
//
// Returns:
//   r0 = 0 on success, 1 if the file could not be opened.
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    filename, port
    int     iARGC
    // if (argc < 2)
    #if_cond    r0 < 2
        // then we don't have command args use STDIN.
        #call   md5(STDIN)

        #call   printbits(r0, 128)
        #call   put_nl()
        #return 0
    #end_cond
    // Get first command line argument and put it in filename.
    #call   args(1)
    move    filename, r0
    // Open text file in read mode.
    #call   openRawFile(filename, READ_MODE)
    // Save the port returned.
    move    port, r0
    // If the port returned is -1 we failed.
    #if_cond    port == -1
        #call   putline("File open failed!")
        #return 1
    #end_cond
    // Process the input stream
    #call   md5(port)
    #call   printbits(r0, 128)
    #call   putc(' ')
    #call   puts(filename)
    #call   put_nl()
    // Close the file
    #call   closeFile(port)
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// Converts a raw stream into a 128-bit MD5 message digest
// Returns the digest in r0
///////////////////////////////////////////////////////////////////////////////

#global MD5_hashcode: .dca   4

// SHIFT_ARRAY specifies the per-round shift amounts
SHIFT_ARRAY: 
    .dcw 7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22, \
        5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20, \
        4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23, \
        6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21

#ifdef USE_PRECOMPUTED_K_ARRAY
// Use binary integer part of the sines of integers (Radians) as constants:
    #global K_ARRAY: .dcw \
        0xD76AA478,0xE8C7B756,0x242070DB,0xC1BDCEEE,0xF57C0FAF,0x4787C62A,0xA8304613,0xFD469501, \
        0x698098D8,0x8B44F7AF,0xFFFF5BB1,0x895CD7BE,0x6B901122,0xFD987193,0xA679438E,0x49B40821, \
        0xF61E2562,0xC040B340,0x265E5A51,0xE9B6C7AA,0xD62F105D,0x02441453,0xD8A1E681,0xE7D3FBC8, \
        0x21E1CDE6,0xC33707D6,0xF4D50D87,0x455A14ED,0xA9E3E905,0xFCEFA3F8,0x676F02D9,0x8D2A4C8A, \
        0xFFFA3942,0x8771F681,0x6D9D6122,0xFDE5380C,0xA4BEEA44,0x4BDECFA9,0xF6BB4B60,0xBEBFBC70, \
        0x289B7EC6,0xEAA127FA,0xD4EF3085,0x04881D05,0xD9D4D039,0xE6DB99E5,0x1FA27CF8,0xC4AC5665, \
        0xF4292244,0x432AFF97,0xAB9423A7,0xFC93A039,0x655B59C3,0x8F0CCC92,0xFFEFF47D,0x85845DD1, \
        0x6FA87E4F,0xFE2CE6E0,0xA3014314,0x4E0811A1,0xF7537E82,0xBD3AF235,0x2AD7D2BB,0xEB86D391
#else
    #global K_ARRAY: .dca 64

///////////////////////////////////////////////////////////////////////////////
// compute_k_array()
//
// Populates the global K_ARRAY with the 64 MD5 round constants. Each constant
// K[i] is computed as floor(abs(sin(i)) * 2^32) for i = 1..64, matching the
// constant table defined in RFC 1321. Values are stored 1-based to align with
// the array's 1-based indexing convention.
//
// No arguments. No return value (modifies K_ARRAY in place).
///////////////////////////////////////////////////////////////////////////////

#def_func   compute_k_array()
    #var    i
    #for    1, i <= 64, 1
        move    f1, i
        int     iSIN
        move    f1, f0
        int     iABS_FP
        mult    f0, 0x100000000
        move    f1, f0
        int     iFLOOR
        move    r0, f0
        store   r0, K_ARRAY[i]
//        #call   put_hex_size(r0, 8)
//        #call   putc(',')
    #end_for
#end_func
#endif

#global a0init: .dci 0x67452301     // A
#global b0init: .dci 0xefcdab89     // B
#global c0init: .dci 0x98badcfe     // C
#global d0init: .dci 0x10325476     // D

#global a0: .dci 0                  // A
#global b0: .dci 0                  // B
#global c0: .dci 0                  // C
#global d0: .dci 0                  // D

///////////////////////////////////////////////////////////////////////////////
// md5(port)
//
// Computes the full MD5 digest of the data read from the given input port.
// Initializes the K constant table, then repeatedly fetches 512-bit message
// blocks via get512bits() and runs the 64-round MD5 compression on each block,
// updating the running digest words (a0, b0, c0, d0).
//
// For each round i (0..63):
//   - Selects the auxiliary function (F/G/H/I) and message word index g.
//   - Computes F = F + A + K[i] + M[g], rotates left by SHIFT_ARRAY[i], and
//     accumulates into B. All arithmetic is masked to 32 bits.
//
// After all blocks are processed, the final A,B,C,D words are stored into the
// global MD5_hashcode array.
//
// Arguments:
//   port — input port to read the raw byte stream from.
//
// Returns:
//   r0 = address of MD5_hashcode (four 32-bit digest words).
///////////////////////////////////////////////////////////////////////////////

#def_func   md5(port)
    #var    i, i1, M, p, a, b, c, d, F, g
    load    p, port
#ifndef USE_PRECOMPUTED_K_ARRAY
    #call   compute_k_array()
#endif
    load    r0, a0init
    store   r0, a0
    load    r0, b0init
    store   r0, b0
    load    r0, c0init
    store   r0, c0
    load    r0, d0init
    store   r0, d0
    store   0, INPUT_LENGTH
    store   FALSE, INPUT_DONE
    store   FALSE, NEEDS_EXTRA_BUFFER

    #call   get512bits(p)
    #while  r0
        move    M, r0
        load    a, a0
        load    b, b0
        load    c, c0
        load    d, d0
        // main loop
        #for    0, i < 64, 1
            #if_cond    i < 16
                // F := (B and C) or ((not B) and D)
                // alternatly F := D xor (B and (C xor D))
                xor     F, c, d
                and     F, b
                xor     F, d
                // g := i
                move    g, i
            #else_if_cond    i < 32
                // F := (D and B) or ((not D) and C)
                // alternatly F := C xor (D and (B xor C))
                xor     F, b, c
                and     F, d
                xor     F, c
                // g := (5×i + 1) mod 16
                mult    r0, 5, i
                add     r0, 1
                div     r0, g, r0, 16
            #else_if_cond    i < 48
                // F := B xor C xor D
                xor     r0, b, c
                xor     F, r0, d
                // g := (3×i + 5) mod 16
                mult    r0, 3, i
                add     r0, 5
                div     r0, g, r0, 16
            #else_cond
                // F := C xor (B or (not D))
                move    r0, d
                compl   r0
                or      r0, b
                xor     F, c, r0
                // g := (7×i) mod 16
                mult    r0, 7, i
                div     r0, g, r0, 16
            #end_cond
            // F := F + A + K[i] + M[g]  // M[g] must be a 32-bits block
            add     F, a
            add     i1, i, 1
            load    r0, K_ARRAY[i1]
            add     F, r0
            load    r0, M[g]
            add     F, r0
            and     F, 0xffffffff
            move    a, d
            move    d, c
            move    c, b
            // 32-bit left rotate
            load    r0, SHIFT_ARRAY[i1]
            lshift  F, r0
            move    r0, F
            rshift  r0, 32
            or      F, r0
            and     F, 0xffffffff
            add     b, F
            and     b, 0xffffffff
        #end_for
        
        // add a into a0, b into b0, etc.
        load    r0, a0
        add     a, r0
        and     a, 0xffffffff
        store   a, a0
        load    r0, b0
        add     b, r0
        and     b, 0xffffffff
        store   b, b0
        load    r0, c0
        add     c, r0
        and     c, 0xffffffff
        store   c, c0
        load    r0, d0
        add     d, r0
        and     d, 0xffffffff
        store   d, d0
        
        #call   get512bits(p)
    #end_while

    load    r0, a0
    store   r0, MD5_hashcode[0]
    load    r0, b0
    store   r0, MD5_hashcode[1]
    load    r0, c0
    store   r0, MD5_hashcode[2]
    load    r0, d0
    store   r0, MD5_hashcode[3]
    #return MD5_hashcode
#end_func

///////////////////////////////////////////////////////////////////////////////
// printbits(bits, num)
//
// Prints a sequence of bits as little-endian hexadecimal to STDOUT. The data
// at address 'bits' is treated as consecutive 32-bit words. The routine emits
// each 32-bit word as 4 little-endian hex bytes (8 hex digits) until 'num'
// bits have been printed. Used to display the final 128-bit MD5 digest.
//
// Arguments:
//   bits — address of the first 32-bit word to print.
//   num  — total number of bits to print (e.g. 128 for an MD5 digest).
//
// No return value (output goes to STDOUT).
///////////////////////////////////////////////////////////////////////////////

#def_func   printbits(bits, num)
    #var    b, n, i, v
    load    b, bits
    load    n, num
    #call   put_nl()
    #for    0, i < n, 32
        load    v, b
        #call   put_hex_little_endian(v, 4)
        add     b, 1
    #end_for
#end_func

#global INPUT_LENGTH: .dci 0
// input is sixteen 32-bit words
#global BUFFER: .dca 16
#global INPUT_DONE: .dci FALSE
#global NEEDS_EXTRA_BUFFER: .dci FALSE

///////////////////////////////////////////////////////////////////////////////
// storeByteInBuffer(v, length)
//
// Stores a single byte into the global BUFFER at the position implied by the
// given byte offset within the current 512-bit block. The offset is reduced
// modulo 64 to find the position inside BUFFER, then split into a word index
// (offset / 4) and a byte position within that word (offset % 4). The byte is
// shifted into place and OR-ed into the existing 32-bit word, packing bytes in
// little-endian order within each word.
//
// Performance Optimization: We implment this as a macro to avoid function call 
// overhead on code that is called for every byte in the file.
//
// Arguments:
//   v      — byte value to store (masked to 8 bits).
//   length — absolute byte offset; only its position within the block matters.
//
// No return value (modifies BUFFER in place).
///////////////////////////////////////////////////////////////////////////////

#ifdef INLINE_STORE_BYTE_IN_BUFFER
#def_macro  storeByteInBuffer(v, length)
// r1 = wordNum
// r2 = shiftBits
// r3 = byteValue
// r4 = len
    move    r4, ${length}       // load    len, length
    move    r3, ${v}            // load    byteValue, v
    and     r3, 0xff            // and     byteValue, 0xff
    and     r4, 63              // len = len % 64
    move    r1, r4              // move    wordNum, len
    rshift  r1, 2               // wordNum = len / 4
    and     r2, r4, 3           // shiftBits = len % 4
    lshift  r2, 3               // shiftBits = shiftBits * 8
    load    r0, BUFFER[r1]      // load    r0, BUFFER[wordNum]
    lshift  r3, r2              // lshift  byteValue, shiftBits
    or      r0, r3              // or      r0, byteValue
    store   r0, BUFFER[r1]      // store   r0, BUFFER[wordNum]
#end_macro

#else
#def_macro  storeByteInBuffer(v, length)
    #call   storeByteInBufferFunc(${v}, ${length})
#end_macro
// Original Function form of storeByteInBuffer()
#def_func storeByteInBufferFunc(v, length)
    #var    byteValue, len, wordNum, shiftBits
    load    len, length
    load    byteValue, v
    and     byteValue, 0xff
    and     len, 63                     // len = len % 64
    move    wordNum, len
    rshift  wordNum, 2                  // wordNum = len / 4
    and     shiftBits, len, 3           // shiftBits = len % 4
    lshift  shiftBits, 3                // shiftBits = shiftBits * 8
    load    r0, BUFFER[wordNum]
    lshift  byteValue, shiftBits
    or      r0, byteValue
    store   r0, BUFFER[wordNum]
#end_func
#endif

///////////////////////////////////////////////////////////////////////////////
// get512bits(port)
//
// Fetches the next 512-bit (64-byte) MD5 message block from the input port,
// performing the RFC 1321 padding as the end of the stream is reached. The
// block is assembled in the global BUFFER (sixteen 32-bit words) and the
// running total of bytes consumed is tracked in INPUT_LENGTH.
//
// Behavior:
//   - Clears BUFFER, then reads up to 64 bytes from the port.
//   - On EOF, appends the 0x80 padding byte and, if there is room (offset
//     <= 55), appends the 64-bit little-endian message length, completing the
//     final block.
//   - If the 0x80 byte does not leave room for the length, sets
//     NEEDS_EXTRA_BUFFER so the next call emits a final padding-only block
//     containing just the length, then sets INPUT_DONE.
//
// State (globals):
//   INPUT_LENGTH       — total message length in bytes processed so far.
//   INPUT_DONE         — TRUE once the final block has been returned.
//   NEEDS_EXTRA_BUFFER — TRUE when an additional length-only block is required.
//
// Arguments:
//   port — input port to read the raw byte stream from.
//
// Returns:
//   r0 = address of BUFFER for a valid block, or 0 when no more blocks remain.
///////////////////////////////////////////////////////////////////////////////

#def_func   get512bits(port)
    #var    i, byteRead, p, len
    load    p, port
    
    #for    0, i < 16, 1
        store   0, BUFFER[i]
    #end_for
    load    r0, NEEDS_EXTRA_BUFFER
    #if_cond    r0
        load    r0, INPUT_LENGTH
        #call   storeLengthInBuffer(r0)
        store   FALSE, NEEDS_EXTRA_BUFFER
        #return BUFFER
    #end_cond

    load    r0, INPUT_DONE
    #if_cond    r0
        #return 0
    #end_cond

    #do_while
        // Read a byte
        #macro  IN1(byteRead, p)
        // If it is -1 we are at EOF.
        #if_cond    byteRead == -1
            #break
        #end_cond
        // add to buffer
        load    len, INPUT_LENGTH
        #macro   storeByteInBuffer(byteRead, len)    
        add     len, 1
        store   len, INPUT_LENGTH
        and     r2, len, 63             // r2 = len % 64
    #end_do_while   r2

    #if_cond    byteRead == -1
        // padd with zeros and length
        load    r0, INPUT_LENGTH
        and     r2, r0, 63              // r2 = INPUT_LENGTH % 64
        #if_cond    r2 <= 55
            #macro   storeByteInBuffer(0x80, r2)
            load    r0, INPUT_LENGTH
            #call   storeLengthInBuffer(r0)
        #else_cond
            #macro   storeByteInBuffer(0x80, r2)
            store   TRUE, NEEDS_EXTRA_BUFFER
        #end_cond
        store   TRUE, INPUT_DONE
    #end_cond
    #return BUFFER
#end_func

///////////////////////////////////////////////////////////////////////////////
// storeLengthInBuffer(length)
//
// Appends the original message length, in bits, to the end of the final
// 512-bit block as a 64-bit little-endian integer occupying byte offsets
// 56..63. The byte length is multiplied by 8 to convert to bits, then each of
// the eight bytes is written low-to-high via storeByteInBuffer().
//
// Arguments:
//   length — total message length in bytes.
//
// No return value (modifies BUFFER in place).
///////////////////////////////////////////////////////////////////////////////

#def_func storeLengthInBuffer(length)
    #var    len, b, i
    load    len, length
    mult    len, 8
    #for    56, i < 64, 1
        and     b, len, 0xff
        #macro   storeByteInBuffer(b, i)
        rshift  len, 8
    #end_for
#end_func

///////////////////////////////////////////////////////////////////////////////
// put_hex_little_endian(value, sizeInBytes)
//
// Prints the low 'sizeInBytes' bytes of a value to STDOUT in little-endian
// order, each byte rendered as two zero-padded hex digits. The least
// significant byte is printed first, producing the standard MD5 byte ordering
// when applied to each 32-bit digest word.
//
// Arguments:
//   value       — the value whose bytes are to be printed.
//   sizeInBytes — number of low-order bytes to emit (e.g. 4 for a 32-bit word).
//
// No return value (output goes to STDOUT).
///////////////////////////////////////////////////////////////////////////////

#def_func   put_hex_little_endian(value, sizeInBytes)
    #var    i, v, size
    load    v, value
    load    size, sizeInBytes
    #for    0, i < size, 1
        move    r0, v
        and     r0, 0xff
        #call   put_hex_size(r0, 2)
        rshift  v, 8
    #end_for
#end_func
    stop
    stop