///////////////////////////////////////////////////////////////////////////////
// hashcode.asm
//
// Computes a 64-bit hashcode over the bytes of a raw input stream and prints
// it in hexadecimal.
//
// The hash is a classic polynomial rolling hash (the same form used by Java's
// String.hashCode, but over raw bytes):
//     hash = hash * 31 + byte
// applied to every byte in the stream, starting from an initial hash of 0.
// Multiplication wraps within the 64-bit register, so the final value is the
// accumulated hash modulo 2^64.
//
// Input source:
//   - If a filename is supplied on the command line, that file is opened in
//     raw read mode and hashed.
//   - If no filename is supplied, the program reads from STDIN instead.
//
// Usage:
//     hashcode <filename>     Hash the contents of the named file.
//     hashcode                Hash data read from STDIN.
//
// Output:
//   With a filename:   "<16-hex-digit hashcode> <filename>"
//   With STDIN:        "<16-hex-digit hashcode>"
//   On open failure:   "Failed to open: <filename>"
//
// Author: Richard Lesh
// Modified: 2026/06/28
// Original: 2022/11/26
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/system.asm>

    #call   main()
    #call   exit(r0)

///////////////////////////////////////////////////////////////////////////////
// main()
// Program entry point.  Selects the input source (file or STDIN), computes the
// hashcode, and prints the result.
//
// Arguments:
//   None (reads the command line directly via iARGC / args()).
//
// Command line:
//   argv[1]  (optional) Path to a file to hash.  If absent, STDIN is hashed.
//
// Behavior:
//   - If no command line argument is supplied, hash STDIN and print the
//     hashcode.
//   - Otherwise open the named file in raw read mode.  On success, hash its
//     contents, print "<filename>: <hashcode>", and close the file.  On
//     failure, print an error message.
//
// Returns:
//   0 on success (in r0), which main() passes to exit().
///////////////////////////////////////////////////////////////////////////////

#def_func   main()
    #var    filename, port
// if (argc < 2) then no filename was supplied; use STDIN.
    int     iARGC
    cmp     r0, 2
    jump    ge, $PROCESS_FILE
// No command line argument: hash STDIN.
    #call   hashcode(STDIN)
    #call   printf("%016x\n", r0)
    #return 0
$PROCESS_FILE:
// Get the first command line argument and put it in filename.
    #call   args(1)
    move    filename, r0
// Open the file in raw read mode (no newline translation).
    #call   openRawFile(filename, READ_MODE)
// Save the port returned.
    move    port, r0
// A port of -1 indicates the open failed.
    #if_cond   port, ne, -1
// Hash the file contents and report the result.
        #call   hashcode(port)
        #call   printf("%016x %s\n", r0, filename)
// Close the file now that we are done with it.
        #call   closeFile(port)
    #else_cond
        #call   printf("Failed to open: %s\n", filename)
        #return 2
    #end_cond
    #return 0
#end_func

///////////////////////////////////////////////////////////////////////////////
// hashcode(port)
// Reads a raw byte stream to end-of-file and computes a 64-bit polynomial
// rolling hash over the bytes.
//
// For performance, bytes are read 8 at a time (one 64-bit word per IN8 call)
// for as long as full 8-byte reads succeed.  Each word is unpacked into its
// 8 constituent bytes (big-endian, most significant byte first) and each
// byte is folded into the hash.  Once a full 8-byte read can no longer be
// satisfied (fewer than 8 bytes remain, or end-of-file), the remainder of
// the stream is finished off one byte at a time using IN1, exactly as
// before.  This cuts the number of INTERRUPT calls - the most expensive
// part of the loop - by roughly a factor of 8 for the bulk of the stream.
//
// Algorithm:
//   hash = 0
//   for each byte b in the stream:
//       hash = hash * 31 + b
//   (arithmetic wraps within the 64-bit register, i.e. modulo 2^64)
//
// Arguments:
//   port  An open input port to read bytes from (e.g. STDIN or a port
//         returned by openRawFile).
//
// Returns:
//   The accumulated 64-bit hashcode in r0.  An empty stream yields 0.
///////////////////////////////////////////////////////////////////////////////

#def_func   hashcode(port)
    #var    byteRead, p, hash, w, b
    load    p, port
    clear   hash                // Start the accumulator at 0

// Bulk phase: read and fold 8 bytes at a time as long as a full 8-byte
// read succeeds.  A short/failed read returns -1, just like IN1 does.
$LOOP8:
    #macro  IN8(w, p)
    #macro  COMPARE(w, eq, -1)
    move    r2, r0
    #macro  FEOF(p)
    and     r1, r0, r2
    jump    nz, $TAIL

// Unpack the 8 bytes packed big-endian into "word" (most significant
// byte first) and fold each one into the rolling hash in turn.
    move    b, w
    rshift  b, 56
    and     b, 0xFF
    mult    hash, 31
    add     hash, b

    move    b, w
    rshift  b, 48
    and     b, 0xFF
    mult    hash, 31
    add     hash, b

    move    b, w
    rshift  b, 40
    and     b, 0xFF
    mult    hash, 31
    add     hash, b

    move    b, w
    rshift  b, 32
    and     b, 0xFF
    mult    hash, 31
    add     hash, b

    move    b, w
    rshift  b, 24
    and     b, 0xFF
    mult    hash, 31
    add     hash, b

    move    b, w
    rshift  b, 16
    and     b, 0xFF
    mult    hash, 31
    add     hash, b

    move    b, w
    rshift  b, 8
    and     b, 0xFF
    mult    hash, 31
    add     hash, b

    move    b, w
    and     b, 0xFF
    mult    hash, 31
    add     hash, b

    jump    $LOOP8

// Tail phase: fewer than 8 bytes remain (or we hit EOF); finish reading
// one byte at a time.
$TAIL:
$LOOP1:
// Read one byte from the port.
    #macro  IN1(byteRead,p)
// A value of -1 signals end-of-file; stop reading.
    cmp     byteRead, -1
    jump    eq, $LOOP_END1

    #call   printf("tail byte: %02x\n", byteRead)
// Fold the byte into the rolling hash: hash = hash * 31 + byte.
    mult    hash, 31
    add     hash, byteRead
    jump    $LOOP1
$LOOP_END1:
    #return hash
#end_func

    stop
    stop