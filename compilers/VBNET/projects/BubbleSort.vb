' BubbleSort.vb
'
' Sorts 10 arrays of 15-30 random integers in [1, 999] with bubble sort and
' prints each array before and after.  The arrays keep their length in
' element 0.  The numbers differ from run to run.
' VB.NET version of documentation/projects/BubbleSort.asm.
'
' Author:   Richard Lesh
Module BubbleSort
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

    Sub BubbleSort(a As Integer())
        Dim count As Integer = a(0)
        For i As Integer = 1 To count - 1
            For j As Integer = 1 To count - i
                If a(j) > a(j + 1) Then
                    Dim t As Integer = a(j)
                    a(j) = a(j + 1)
                    a(j + 1) = t
                End If
            Next
        Next
    End Sub

    Function Main() As Integer
        For i As Integer = 1 To 10
            Dim a As Integer() = CreateRandomArray(RandRange(15, 30))
            Console.WriteLine("Test {0}", i)
            Console.WriteLine("random: " & ArrayToString(a))
            BubbleSort(a)
            Console.WriteLine("sorted: " & ArrayToString(a))
        Next
        Return 0
    End Function
End Module
