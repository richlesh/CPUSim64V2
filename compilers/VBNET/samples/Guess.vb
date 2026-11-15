' The number guessing game (Random, Integer.TryParse, Do loops).
Module Guess
    Function Main() As Integer
        Dim secret = New Random().Next(1, 101)
        Dim tries = 0
        Console.WriteLine("I am thinking of a number from 1 to 100.")
        Do
            Console.Write("Your guess: ")
            Dim input = Console.ReadLine()
            If input Is Nothing Then
                Console.WriteLine()
                Console.WriteLine($"Giving up? It was {secret}.")
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
                Return 0
            End If
        Loop
    End Function
End Module
