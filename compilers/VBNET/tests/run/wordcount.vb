' Counts the lines, words and characters of the standard input and finds
' the longest word.
Module WordCount
    Sub Main()
        Dim lines = 0, words = 0, chars = 0
        Dim longest = ""
        Dim line = Console.ReadLine()
        While line IsNot Nothing
            lines += 1
            chars += line.Length
            For Each w In line.Split(" "c)
                Dim word = w.Trim(","c, "."c, "!"c, "?"c)
                If word.Length = 0 Then Continue For
                words += 1
                If word.Length > longest.Length Then longest = word
            Next
            line = Console.ReadLine()
        End While
        Console.WriteLine("{0} lines, {1} words, {2} characters", lines, words, chars)
        Console.WriteLine($"Longest word: {longest} ({longest.Length} letters)")
        Console.WriteLine("Average word length: {0:F2}", (chars - (words - lines)) / words)
    End Sub
End Module
