' QuickSortFloat.vb
'
' Sorts 10 arrays of 9-14 random doubles in [0, 1) with quicksort and
' prints each array (6 decimals) before and after.  The arrays keep their
' length in element 0.  The numbers differ from run to run.
' VB.NET version of documentation/projects/QuickSortFloat.asm.
'
' Author:   Richard Lesh
Module QuickSortFloat
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

    Function Partition(a As Double(), lo As Integer, hi As Integer) As Integer
        Dim pivot As Double = a(hi)
        Dim i As Integer = lo - 1
        For j As Integer = lo To hi - 1
            If a(j) <= pivot Then
                i += 1
                Dim t As Double = a(i)
                a(i) = a(j)
                a(j) = t
            End If
        Next
        i += 1
        Dim u As Double = a(i)
        a(i) = a(hi)
        a(hi) = u
        Return i
    End Function

    Sub QuickSortRange(a As Double(), lo As Integer, hi As Integer)
        If lo < hi Then
            Dim p As Integer = Partition(a, lo, hi)
            QuickSortRange(a, lo, p - 1)
            QuickSortRange(a, p + 1, hi)
        End If
    End Sub

    Sub QuickSortFloat(a As Double())
        QuickSortRange(a, 1, CInt(a(0)))
    End Sub

    Function Main() As Integer
        For i As Integer = 1 To 10
            Dim a As Double() = CreateRandomArray(RandRange(9, 14))
            Console.WriteLine("Test {0}", i)
            Console.WriteLine("random: " & ArrayToString(a))
            QuickSortFloat(a)
            Console.WriteLine("sorted: " & ArrayToString(a))
        Next
        Return 0
    End Function
End Module
