' The number guessing game: without Randomize, Rnd gives the same
' sequence as in .NET, so the secret number is 71.
Module Guess
    Function Main() As Integer
        Dim secret = CInt(Int(Rnd() * 100)) + 1
        Dim tries = 0
        Console.WriteLine("Guess my number (1-100).")
        Do
            Console.Write("Your guess: ")
            Dim input = Console.ReadLine()
            If input Is Nothing Then
                Console.WriteLine()
                Console.WriteLine("Giving up? It was " & secret & ".")
                Return 1
            End If
            Dim g As Integer
            If Not Integer.TryParse(input, g) OrElse g < 1 OrElse g > 100 Then
                Console.WriteLine($"'{input.Trim()}' is not a number from 1 to 100.")
                Continue Do
            End If
            tries += 1
            If g < secret Then
                Console.WriteLine("Too low.")
            ElseIf g > secret Then
                Console.WriteLine("Too high.")
            Else
                Console.WriteLine($"Correct! You needed {tries} {If(tries = 1, "try", "tries")}.")
                Exit Do
            End If
        Loop
        Return 0
    End Function
End Module
