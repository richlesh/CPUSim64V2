///////////////////////////////////////////////////////////////////////////////
// packdemo.asm
//
// Demonstrates two different ways to build a UTF-8 packed string containing
// the 95 printable ASCII characters ' ' (0x20) through '~' (0x7E), and two
// different ways to print each one.
//
// Packed string format (same as .dcs):
//   word 0      = number of UTF-8 bytes (95)
//   words 1..12 = data bytes, 8 per word, big-endian (byte 0 is the most
//                 significant byte).  The last word holds only 7 bytes, so
//                 its lowest byte is padding (0).
//
// Test 1: Build the string with PACK / PACK64.
//         Print it 8 bytes at a time (OUT8 of whole words).
//         Print it 1 byte at a time by splitting words with UNPACK64 / UNPACK.
//
// Test 2: Build the string with STB (store byte).
//         Print it 8 bytes at a time (OUT8 of whole words).
//         Print it 1 byte at a time with LDB (load byte).
//
// Finally the two strings are compared to confirm they are identical.
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/string.def>
#include <system/system.def>
#include <system/system.asm>

#define FIRST_CHAR  0x20        // ' '
#define LAST_CHAR   0x7E        // '~'
#define NUM_CHARS   95          // LAST_CHAR - FIRST_CHAR + 1
#define NUM_WORDS   12          // ceil(NUM_CHARS / 8) data words
#define BUF_WORDS   13          // NUM_WORDS + 1 (length word)

///////////////////////////////////////////////////////////////////////////////
// NEXT_BYTE(dest, ch)
// Moves the character ch into dest if it is still <= LAST_CHAR, otherwise
// moves 0 (padding) into dest.  Then advances ch to the next character.
///////////////////////////////////////////////////////////////////////////////
#def_macro  NEXT_BYTE(dest, ch)
    cmp     ${ch}, LAST_CHAR
    move    le, ${dest}, ${ch}, 0   // conditional move: char or padding
    add     ${ch}, 1
#end_macro

///////////////////////////////////////////////////////////////////////////////
// EMIT_BYTE(b, count, len)
// Outputs byte b to STDOUT only if fewer than len bytes have been output so
// far (this skips the padding bytes in the last word).  Advances count.
///////////////////////////////////////////////////////////////////////////////
#def_macro  EMIT_BYTE(b, count, len)
    #if_cond    ${count} < ${len}
        #macro  OUT1(${b}, STDOUT)
    #end_cond
    add     ${count}, 1
#end_macro

    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
// Runs both tests and verifies that both construction methods produce the
// same packed string.
///////////////////////////////////////////////////////////////////////////////
#def_func   main()
    #var    str1, str2

    // ---------------- Test 1: PACK / PACK64 ----------------
    #call   putline("=== Test 1: Build with PACK / PACK64 ===")
    #call   buildWithPack()
    move    str1, r0
    #call   dumpHex(str1)
    #call   putline("8 bytes at a time (OUT8):")
    #call   printWords(str1)
    #call   putline("1 byte at a time (UNPACK64 / UNPACK):")
    #call   printBytesUnpack(str1)
    #call   put_nl()

    // ---------------- Test 2: STB / LDB ----------------
    #call   putline("=== Test 2: Build with STB ===")
    #call   buildWithStb()
    move    str2, r0
    #call   dumpHex(str2)
    #call   putline("8 bytes at a time (OUT8):")
    #call   printWords(str2)
    #call   putline("1 byte at a time (LDB):")
    #call   printBytesLdb(str2)
    #call   put_nl()

    // ---------------- Verify ----------------
    move    r1, str1
    move    r2, str2
    int     iSTRCMP
    #if_cond    r0 == 0
        #call   putline("Both methods produced identical strings.")
    #else_cond
        #call   putline("ERROR: The two strings differ!")
    #end_cond

    #call   free(str1)
    #call   free(str2)
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// buildWithPack()
// Builds the packed string a whole word at a time.  For each word, eight
// bytes are generated, PACK combines them into two 32-bit halves, and PACK64
// joins the halves into one 64-bit word.
//
// Returns: address of the heap allocated string in r0.
///////////////////////////////////////////////////////////////////////////////
#def_func   buildWithPack()
    #var    buf, c, w, b0, b1, b2, b3, b4, b5, b6, b7
    move    r1, BUF_WORDS
    int     iALLOC
    move    buf, r0
    move    r0, NUM_CHARS
    store   r0, buf[0]              // byte count in word 0

    move    c, FIRST_CHAR
    #for    1, w <= NUM_WORDS, 1
        #macro  NEXT_BYTE(b0, c)
        #macro  NEXT_BYTE(b1, c)
        #macro  NEXT_BYTE(b2, c)
        #macro  NEXT_BYTE(b3, c)
        #macro  NEXT_BYTE(b4, c)
        #macro  NEXT_BYTE(b5, c)
        #macro  NEXT_BYTE(b6, c)
        #macro  NEXT_BYTE(b7, c)
        pack    b0, b1, b2, b3      // b0 = high 32 bits (bytes 0-3)
        pack    b4, b5, b6, b7      // b4 = low 32 bits  (bytes 4-7)
        pack64  b0, b4              // b0 = full 64-bit word
        store   b0, buf[w]
    #end_for
    #return buf
#end_func

///////////////////////////////////////////////////////////////////////////////
// buildWithStb()
// Builds the packed string one byte at a time with STB.  The data starts at
// byte address (buf + 1) * 8.  The buffer is cleared first so the unused
// padding byte in the last word is 0.
//
// Returns: address of the heap allocated string in r0.
///////////////////////////////////////////////////////////////////////////////
#def_func   buildWithStb()
    #var    buf, c, addr
    move    r1, BUF_WORDS
    int     iALLOC
    move    buf, r0
    move    r1, buf
    move    r2, BUF_WORDS
    int     iMEMCLEAR               // zero the whole block
    move    r0, NUM_CHARS
    store   r0, buf[0]              // byte count in word 0

    add     addr, buf, 1            // first data word
    lshift  addr, 3                 // convert to byte address (* 8)
    #for    FIRST_CHAR, c <= LAST_CHAR, 1
        stb     c, addr             // store low 8 bits of c
        add     addr, 1             // next byte address
    #end_for
    #return buf
#end_func

///////////////////////////////////////////////////////////////////////////////
// dumpHex(str)
// Prints the length word and each data word of the string in hexadecimal so
// the big-endian packing can be seen.
///////////////////////////////////////////////////////////////////////////////
#def_func   dumpHex(str)
    #var    s, len, words, i, w
    load    s, str
    load    len, s[0]
    #call   printf("Length word: %d bytes\n", len)
    add     words, len, 7
    div     words, 8                // words = ceil(len / 8)
    #for    1, i <= words, 1
        load    w, s[i]
        #call   printf("  word %d: %016x\n", i, w)
    #end_for
#end_func

///////////////////////////////////////////////////////////////////////////////
// printWords(str)
// Outputs the string 8 bytes at a time with OUT8.  Every complete word is
// written whole.  The final partial word (if any) is finished byte by byte so
// that the zero padding byte is not written to the console.
///////////////////////////////////////////////////////////////////////////////
#def_func   printWords(str)
    #var    s, len, full, rem, i, w, j, shift
    load    s, str
    load    len, s[0]
    div     full, rem, len, 8       // full words and leftover bytes

    #for    1, i <= full, 1
        load    w, s[i]
        #macro  OUT8(w, STDOUT)     // 8 bytes, big-endian
    #end_for

    #if_cond    rem > 0
        add     i, full, 1          // index of the partial word
        load    w, s[i]
        move    shift, 56           // start with the most significant byte
        #for    0, j < rem, 1
            rshift  r0, w, shift
            and     r0, 0xFF
            #macro  OUT1(r0, STDOUT)
            sub     shift, 8
        #end_for
    #end_cond
    #call   put_nl()
#end_func

///////////////////////////////////////////////////////////////////////////////
// printBytesUnpack(str)
// Outputs the string one byte at a time.  Each word is split into two 32-bit
// halves with UNPACK64, and each half into four bytes with UNPACK.
///////////////////////////////////////////////////////////////////////////////
#def_func   printBytesUnpack(str)
    #var    s, len, words, i, count, b0, b1, b2, b3, b4, b5, b6, b7
    load    s, str
    load    len, s[0]
    add     words, len, 7
    div     words, 8                // words = ceil(len / 8)
    clear   count

    #for    1, i <= words, 1
        load    b0, s[i]
        unpack64 b0, b4             // b0 = high 32 bits, b4 = low 32 bits
        unpack  b0, b1, b2, b3      // bytes 0-3
        unpack  b4, b5, b6, b7      // bytes 4-7
        #macro  EMIT_BYTE(b0, count, len)
        #macro  EMIT_BYTE(b1, count, len)
        #macro  EMIT_BYTE(b2, count, len)
        #macro  EMIT_BYTE(b3, count, len)
        #macro  EMIT_BYTE(b4, count, len)
        #macro  EMIT_BYTE(b5, count, len)
        #macro  EMIT_BYTE(b6, count, len)
        #macro  EMIT_BYTE(b7, count, len)
    #end_for
    #call   put_nl()
#end_func

///////////////////////////////////////////////////////////////////////////////
// printBytesLdb(str)
// Outputs the string one byte at a time using LDB with byte addressing.
///////////////////////////////////////////////////////////////////////////////
#def_func   printBytesLdb(str)
    #var    s, len, addr, i, b
    load    s, str
    load    len, s[0]
    add     addr, s, 1              // first data word
    lshift  addr, 3                 // convert to byte address (* 8)
    #for    0, i < len, 1
        ldb     b, addr[i]          // EA = addr + i, zero-extended byte
        #macro  OUT1(b, STDOUT)
    #end_for
    #call   put_nl()
#end_func

    stop
    stop