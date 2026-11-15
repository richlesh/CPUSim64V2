' BubbleSortFloat.vb
'
' Sorts 10 arrays of 9-14 random doubles in [0, 1) with bubble sort and
' prints each array (6 decimals) before and after.  The arrays keep their
' length in element 0.  The numbers differ from run to run.
' VB.NET version of documentation/projects/BubbleSortFloat.asm.
'
' Author:   Richard Lesh
Module BubbleSortFloat
    Dim rng As New Random()

    ' a random integer in [low, high]
    Function RandRange(low As Integer, high As Integer) As Integer
        Return rng.Next(low, high + 1)
    End Function

    ' an array of n random doubles in [0, 1); a(0) holds the length
    Function CreateRandomArray(n As Integer) As Double()
        Dim a(n) As Double
        a(0) = n
        For i As Integer = 1 To n
            a(i) = rng.NextDouble()
        Next
        Return a
    End Function

    ' the elements with 6 decimals, separated by commas
    Function ArrayToString(a As Double()) As String
        Dim sb As New System.Text.StringBuilder()
        For i As Integer = 1 To CInt(a(0))
            If i > 1 Then sb.Append(",")
            sb.Append(a(i).ToString("F6"))
        Next
        Return sb.ToString()
    End Function

    Sub BubbleSortFloat(a As Double())
        Dim count As Integer = CInt(a(0))
        For i As Integer = 1 To count - 1
            For j As Integer = 1 To count - i
                If a(j) > a(j + 1) Then
                    Dim t As Double = a(j)
                    a(j) = a(j + 1)
                    a(j + 1) = t
                End If
            Next
        Next
    End Sub

    Function Main() As Integer
        For i As Integer = 1 To 10
            Dim a As Double() = CreateRandomArray(RandRange(9, 14))
            Console.WriteLine("Test {0}", i)
            Console.WriteLine("random: " & ArrayToString(a))
            BubbleSortFloat(a)
            Console.WriteLine("sorted: " & ArrayToString(a))
        Next
        Return 0
    End Function
End Module
