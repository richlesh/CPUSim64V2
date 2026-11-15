Structure Point
    Public X As Integer
    Public Y As Integer
    Public Sub New(x As Integer, y As Integer)
        Me.X = x
        Me.Y = y
    End Sub
    Public Sub Move(dx As Integer)
        X += dx
    End Sub
    Public Function Text() As String
        Return "(" & X & "," & Y & ")"
    End Function
End Structure

Structure Line
    Public A As Point
    Public B As Point
    Public Shared Count As Integer
End Structure

Class Shape
    Public Origin As Point
    Public Name As String = "shape"
End Class

Module Program
    Sub Shift(p As Point)
        p.X = 100
    End Sub

    Sub ShiftRef(ByRef p As Point)
        p.X = 100
    End Sub

    Function Make(n As Integer) As Point
        Dim p As Point
        p.X = n
        p.Y = n * 2
        Return p
    End Function

    Sub Main()
        Dim p As Point
        Console.WriteLine(p.Text())
        p.X = 1
        Dim q = p
        q.X = 2
        Console.WriteLine(p.Text() & " " & q.Text())
        Shift(p)
        Console.WriteLine(p.Text())
        ShiftRef(p)
        Console.WriteLine(p.Text())
        p.Move(5)
        Console.WriteLine(p.Text())
        Dim r As New Point(3, 4)
        Console.WriteLine(r.Text() & " " & Make(7).Text())
        Dim ln As Line
        ln.A = r
        ln.B.X = 9
        r.X = 0
        Dim ln2 = ln
        ln2.B.X = -1
        Console.WriteLine(ln.A.Text() & ln.B.Text() & ln2.B.Text())
        Dim pts(2) As Point
        pts(1).Y = 5
        For Each pt In pts
            Console.Write(pt.Text())
        Next
        Console.WriteLine()
        Dim arr() As Point = {New Point(1, 1), r}
        arr(1).X = 42
        Console.WriteLine(arr(1).Text() & " " & r.Text())
        Dim o As Object = r
        r.Y = 99
        Dim back = CType(o, Point)
        Console.WriteLine(back.Text() & " " & o.ToString())
        Dim s As New Shape
        s.Origin.X = 8
        Dim s2 = s
        Console.WriteLine(s2.Origin.Text() & " " & s.Name)
        p = Nothing
        Console.WriteLine(p.Text())
        Line.Count += 2
        Console.WriteLine(Line.Count)
    End Sub
End Module
