' InsertSorted.vb
'
' Inserts 30 random integers in [1, 999] one at a time into a sorted array
' with binary search, and prints the array after each insertion.  The array
' keeps its length in element 0 and grows by doubling.  The numbers differ
' from run to run.
' VB.NET version of documentation/projects/InsertSorted.asm.
'
' Author:   Richard Lesh
Module InsertSorted
    Dim rng As New Random()

    ' a growing array: a(0) holds the length, the elements are a(1..length)
    Class SortedArray
        Public A(1) As Integer

        Public Sub Insert(value As Integer)
            Dim n As Integer = A(0)
            If A.Length < n + 2 Then
                Dim newCap As Integer = A.Length * 2
                If newCap < n + 2 Then newCap = n + 2
                ReDim Preserve A(newCap - 1)
            End If
            ' the first position whose element is greater than value
            Dim lo As Integer = 1, hi As Integer = n
            While lo <= hi
                Dim mid As Integer = (lo + hi) \ 2
                If A(mid) <= value Then
                    lo = mid + 1
                Else
                    hi = mid - 1
                End If
            End While
            For i As Integer = n To lo Step -1
                A(i + 1) = A(i)
            Next
            A(lo) = value
            A(0) = n + 1
        End Sub

        Public Overrides Function ToString() As String
            Dim sb As New System.Text.StringBuilder()
            For i As Integer = 1 To A(0)
                If i > 1 Then sb.Append(",")
                sb.Append(A(i))
            Next
            Return sb.ToString()
        End Function
    End Class

    Function Main() As Integer
        Dim s As New SortedArray()
        Console.WriteLine("Inserting 30 random integers [1,999] one at a time:")
        For i As Integer = 1 To 30
            Dim v As Integer = rng.Next(1, 1000)
            s.Insert(v)
            Console.WriteLine("Insert {0,3}: {1}", v, s.ToString())
        Next
        Return 0
    End Function
End Module
