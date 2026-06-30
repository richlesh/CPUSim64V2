///////////////////////////////////////////////////////////////////////////////
// holy_grail.asm
//
// A tribute to the "Bridge of Death" scene from Monty Python and the Holy
// Grail (1975).
//
//     BRIDGEKEEPER: "Stop! Who would cross the Bridge of Death must answer
//                    me these questions three, ere the other side he see."
//
// This program plays the role of the Bridgekeeper, asking the traveler the
// three legendary questions:
//
//     1. What is your name?
//     2. What is your quest?
//     3. What is the air-speed velocity of an unladen swallow?
//
// As in the film, the third question is the deciding one. The "correct"
// response is the famous deflection:
//
//     "What do you mean? An African or European swallow?"
//
// If the traveler's answer mentions BOTH "african" and "european", the
// Bridgekeeper is confused and hurled into the gorge himself. Otherwise the
// traveler is cast into the Gorge of Eternal Peril.
//
// Author:   Richard Lesh
// Original: 2026/06/29
///////////////////////////////////////////////////////////////////////////////


#include <system/ansi_color.asm>    // ANSI terminal color / style sequences
#include <system/io.asm>            // puts, put_nl, free, etc.
#include <system/string.def>        // TO_LOWER_STR, SUBSTRING_SEARCH macros
#include <system/system.def>        // COMPARE macro, iEXIT, iGET_LINE, etc.

    // Program entry point: run main() then exit with its return code (in r0).
    #call   main()
    int     iEXIT

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Drives the entire interaction:
//   * Prints the Bridgekeeper's introduction.
//   * Reads three lines of input from the user.
//   * Evaluates the third answer to decide the traveler's fate.
//   * Frees all heap-allocated input strings.
//
// Returns 0 in r0 on success.
///////////////////////////////////////////////////////////////////////////////
#def_func   main()
    #var    answer1, answer2, answer3, lowercase3

    // Print the Bridgekeeper's challenge, followed by a blank line.
    #call   puts(ANSI_INTRO)
    #call   put_nl()
    #call   put_nl()

    // ---- Question 1: Name ---------------------------------------------------
    // iGET_LINE with buffer (r2) = 0 allocates a fresh heap string and
    // returns its address in r0. The newline is stripped from the result.
    #call   puts("BRIDGEKEEPER: What... is your name? ")
    #call   fgetline(STDIN, 0)
    move    answer1, r0             // heap-allocated string (must be freed)

    // ---- Question 2: Quest --------------------------------------------------
    #call   puts("BRIDGEKEEPER: What... is your quest? ")
    #call   fgetline(STDIN, 0)
    move    answer2, r0             // heap-allocated string (must be freed)

    // ---- Question 3: Swallow ------------------------------------------------
    #call   puts("BRIDGEKEEPER: What... is the air-speed velocity of an unladen swallow? ")
    #call   fgetline(STDIN, 0)
    move    answer3, r0             // heap-allocated string (must be freed)

    #call   put_nl()

    // ---- Judge the third answer ---------------------------------------------
    // The crichlever response is:
    //   "What do you mean? An African or European swallow?"
    //
    // To make matching case-insensitive, we create a lowercase copy of the
    // answer and search it for the substrings "african" and "european".

    // Make a lowercase, heap-allocated copy of answer3 and keep that instead.
    // NOTE: this overwrites our pointer with the new copy; the original
    // answer3 string is freed below using this same pointer, which is fine
    // because the lowercase copy is what we now hold. (See cleanup note.)
    #macro  TO_LOWER_STR(answer3)
    move    lowercase3, r0          // lowercased copy (heap-allocated)

    // Search for "african": iSUBSTRING_SEARCH returns the index or -1.
    // COMPARE sets the result so r0 becomes nonzero (TRUE) when found.
    #macro  SUBSTRING_SEARCH(lowercase3, "african", 0)
    #macro  COMPARE(r0, ne, -1)
    move    r4, r0                  // r4 = (found "african") ? TRUE : FALSE

    // Search for "european" the same way.
    #macro  SUBSTRING_SEARCH(lowercase3, "european", 0)
    #macro  COMPARE(r0, ne, -1)
    move    r5, r0                  // r5 = (found "european") ? TRUE : FALSE

    // Both must be present for the witty answer to count.
    and     r4, r5                  // r4 = found "african" AND "european"

    // ---- Deliver the verdict in dramatic red, blinking text -----------------
    #call   puts(ANSI_COLOR$RED)
    #call   puts(ANSI_COLOR$BLINK)
    #if_cond    r4, ne, 0
        // Traveler outwitted the Bridgekeeper.
        #call   puts("BRIDGEKEEPER: Huh? I-- I don't know that... AAAAARGH!\n\n")
    #else_cond
        // Traveler answered poorly and meets a grim fate.
        #call   puts("BRIDGEKEEPER: You are cast into the Gorge of Eternal Peril!\n\n")
    #end_cond
    #call   puts(ANSI_COLOR$RESET_BLINK)     // stop blinking
    #call   puts(ANSI_COLOR$RESET_COLOR)     // restore default text color

    // ---- Clean up heap allocations ------------------------------------------
    // answer1 and answer2 hold their original input strings.
    // answer3 now holds the LOWERCASE COPY (see TO_LOWER_STR above).
    #call   free(answer1)
    #call   free(answer2)
    #call   free(answer3)
    #call   free(lowercase3)

    #return 0
#end_func

// --- Static data -------------------------------------------------------------
// The Bridgekeeper's opening challenge, stored as a UTF-8 string literal.
ANSI_INTRO: .DCS "STOP! Who would cross the Bridge of Death must answer me\nthese questions three, ere the other side he see."

    // Two STOP instructions mark the true end of the program for the
    // disassembler.
    stop
    stop