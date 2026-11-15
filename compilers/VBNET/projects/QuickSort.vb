' QuickSort.vb
'
' Sorts 10 arrays of 15-30 random integers in [1, 999] with quicksort and
' prints each array before and after.  The arrays keep their length in
' element 0.  The numbers differ from run to run.
' VB.NET version of documentation/projects/QuickSort.asm.
'
' Author:   Richard Lesh
Module QuickSort
    Dim rng As New Random()

    ' a random integer in [low, high]
    Function RandRange(low As Integer, high As Integer) As Integer
        Return rng.Next(low, high + 1)
    End Function

    ' an array of n random integers in [1, 999]; a(0) holds the length
    Function CreateRandomArray(n As Integer) As Integer()
        Dim a(n) As Integer
        a(0) = n
        For i As Integer = 1 To n
            a(i) = RandRange(1, 999)
        Next
        Return a
    End Function

    ' the elements separated by commas
    Function ArrayToString(a As Integer()) As String
        Dim sb As New System.Text.StringBuilder()
        For i As Integer = 1 To a(0)
            If i > 1 Then sb.Append(",")
            sb.Append(a(i))
        Next
        Return sb.ToString()
    End Function

    Function Partition(a As Integer(), lo As Integer, hi As Integer) As Integer
        Dim pivot As Integer = a(hi)
        Dim i As Integer = lo - 1
        For j As Integer = lo To hi - 1
            If a(j) <= pivot Then
                i += 1
                Dim t As Integer = a(i)
                a(i) = a(j)
                a(j) = t
            End If
        Next
        i += 1
        Dim u As Integer = a(i)
        a(i) = a(hi)
        a(hi) = u
        Return i
    End Function

    Sub QuickSortRange(a As Integer(), lo As Integer, hi As Integer)
        If lo < hi Then
            Dim p As Integer = Partition(a, lo, hi)
            QuickSortRange(a, lo, p - 1)
            QuickSortRange(a, p + 1, hi)
        End If
    End Sub

    Sub QuickSort(a As Integer())
        QuickSortRange(a, 1, a(0))
    End Sub

    Function Main() As Integer
        For i As Integer = 1 To 10
            Dim a As Integer() = CreateRandomArray(RandRange(15, 30))
            Console.WriteLine("Test {0}", i)
            Console.WriteLine("random: " & ArrayToString(a))
            QuickSort(a)
            Console.WriteLine("sorted: " & ArrayToString(a))
        Next
        Return 0
    End Function
End Module
