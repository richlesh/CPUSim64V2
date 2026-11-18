///////////////////////////////////////////////////////////////////////////////
// hello_languages.asm
//
// Prints "Hello, world!" in several different languages, one per line, with
// the language name in the first column and the greeting in the second.
//
// String literals are UTF-8 encoded, so greetings in non-Latin scripts
// (Greek, Cyrillic, Japanese, Chinese) can be written directly in the source.
//
// Usage:
//     > run.sh hello
//
// Output:
//     English    Hello, World!
//     Spanish    ¡Hola, Mundo!
//     French     Bonjour, le monde !
//     Greek      Γεια σου, Κόσμε!
//     Russian    Привет, мир!
//     Japanese   こんにちは、世界！
//     Chinese    你好，世界！
///////////////////////////////////////////////////////////////////////////////

#include <system/io.asm>
#include <system/system.asm>

    #call   main()
    #call   exit(r0)                    // Exit to OS with main's return value

///////////////////////////////////////////////////////////////////////////////
// main()
//
// Prints each language name (padded to a fixed width) followed by the
// greeting in that language.
//
// Returns: 0 in r0
///////////////////////////////////////////////////////////////////////////////
#def_func   main()
    #call   puts("English    ")
    #call   putline("Hello, World!")

    #call   puts("Spanish    ")
    #call   putline("¡Hola, Mundo!")

    #call   puts("French     ")
    #call   putline("Bonjour, le monde !")

    #call   puts("Greek      ")
    #call   putline("Γεια σου, Κόσμε!")

    #call   puts("Russian    ")
    #call   putline("Привет, мир!")

    #call   puts("Japanese   ")
    #call   putline("こんにちは、世界！")

    #call   puts("Chinese    ")
    #call   putline("你好，世界！")

    #return 0
#end_func

    stop
    stop