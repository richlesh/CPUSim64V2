' Allocates about twice the simulator's memory (gc.mem: 2M words) in garbage,
' with linked lists and cycles, and checks that the live data survives.
Class Node
    Public Value As Integer
    Public NextNode As Node
    Public Data() As Integer
End Class

Structure Pair
    Public Name As String
    Public Link As Node
End Structure

Module Program
    Dim keep As Node
    Dim pairs(9) As Pair

    Function MakeList(n As Integer) As Node
        Dim head As Node = Nothing
        For i = 1 To n
            Dim nd As New Node()
            nd.Value = i
            nd.NextNode = head
            head = nd
        Next
        Return head
    End Function

    Function Sum(nd As Node) As Long
        Dim s As Long = 0
        While nd IsNot Nothing
            s += nd.Value
            nd = nd.NextNode
        End While
        Return s
    End Function

    Sub Main()
        keep = MakeList(1000)
        For i = 0 To 9
            pairs(i).Name = "pair" & i
            pairs(i).Link = MakeList(i + 1)
        Next
        Dim total As Long = 0
        For round = 1 To 500
            ' garbage: a list of 100 nodes with arrays, a cycle, strings
            Dim lst = MakeList(100)
            Dim nd = lst
            While nd IsNot Nothing
                ReDim nd.Data(50)
                nd.Data(50) = nd.Value
                nd = nd.NextNode
            End While
            lst.NextNode.NextNode = lst
            Dim s As String = ""
            For k = 1 To 20
                s &= k.ToString()
            Next
            total += s.Length + lst.Data(50)
        Next
        Console.WriteLine(total)
        Console.WriteLine(Sum(keep))
        Dim ok = True
        For i = 0 To 9
            If pairs(i).Name <> "pair" & i OrElse Sum(pairs(i).Link) <> (i + 1) * (i + 2) \ 2 Then ok = False
        Next
        Console.WriteLine(ok)
        GC.Collect()
        Dim mem = GC.GetTotalMemory(True)
        Console.WriteLine(mem < 2000000)
        Console.WriteLine(GC.CollectionCount(0) > 2)
    End Sub
End Module
