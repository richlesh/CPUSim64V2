' Unicode.vb
'
' Prints some blocks of Unicode characters, 32 to a line: ASCII, Latin
' supplements, math symbols, box drawing, and symbols and dingbats.  The
' assembly version also prints the icons from U+1F300; those are outside the
' Basic Multilingual Plane, where ChrW fails in VB.NET (a character is a
' 16-bit UTF-16 unit) and this compiler has no surrogate pair output, so
' they are left out.
' VB.NET version of documentation/projects/Unicode.asm.
'
' Author:   Richard Lesh
Module Unicode
    Sub PrintRange(first As Integer, last As Integer)
        For c As Integer = first To last
            Console.Write(ChrW(c))
            If (c And 31) = 31 Then Console.WriteLine()
        Next
    End Sub

    Function Main() As Integer
        Console.WriteLine("ASCII")
        PrintRange(&H20, &H7F)
        Console.WriteLine("Basic Multilingual")
        PrintRange(&HA0, &H1FF)
        Console.WriteLine("Math Symbols")
        PrintRange(&H2200, &H22FF)
        Console.WriteLine("Drawing")
        PrintRange(&H2500, &H25FF)
        Console.WriteLine("Symbols")
        PrintRange(&H2600, &H27FF)
        Return 0
    End Function
End Module
