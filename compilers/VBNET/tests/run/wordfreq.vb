' The word frequencies of the standard input, most frequent first (the
' plan's demo for Dictionary and List).
Class WordCount
    Implements IComparable(Of WordCount)
    Public ReadOnly Word As String
    Public Count As Integer
    Public Sub New(word As String)
        Me.Word = word
    End Sub
    Public Function CompareTo(other As WordCount) As Integer Implements IComparable(Of WordCount).CompareTo
        If Count <> other.Count Then Return other.Count.CompareTo(Count)
        Return String.Compare(Word, other.Word)
    End Function
End Class

Module Program
    Sub Main()
        Dim counts As New Dictionary(Of String, WordCount)
        Dim total = 0
        Dim line = Console.ReadLine()
        While line IsNot Nothing
            For Each raw In line.Split(" "c)
                Dim w = raw.Trim(","c, "."c, ";"c, "!"c, "?"c).ToLower()
                If w = "" Then Continue For
                total += 1
                Dim wc As WordCount = Nothing
                If Not counts.TryGetValue(w, wc) Then
                    wc = New WordCount(w)
                    counts.Add(w, wc)
                End If
                wc.Count += 1
            Next
            line = Console.ReadLine()
        End While
        Dim list As New List(Of WordCount)
        For Each kv In counts
            list.Add(kv.Value)
        Next
        list.Sort()
        Console.WriteLine($"{total} words, {counts.Count} different")
        For i = 0 To Math.Min(4, list.Count - 1)
            Console.WriteLine($"{list(i).Word,-6}{list(i).Count,3}")
        Next
    End Sub
End Module
