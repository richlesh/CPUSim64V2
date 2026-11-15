Namespace Geometry
    Class Matrix
        Private data(,) As Double
        Public Sub New(n As Integer)
            ReDim data(n - 1, n - 1)
            Created += 1
        End Sub
        Public Property Cell(r As Integer, c As Integer) As Double
            Get
                Return data(r, c)
            End Get
            Set(value As Double)
                data(r, c) = value
            End Set
        End Property
        Public Shared Property Created As Integer
        Public ReadOnly Property Size As Integer
            Get
                Return 0
            End Get
        End Property
    End Class
End Namespace

Module Program
    Class Inner
        Public V As Integer
        Public Shared Sub Hello()
            Console.WriteLine("hello from Inner")
        End Sub
    End Class

    Sub Main()
        Dim m As New Geometry.Matrix(2)
        m.Cell(0, 1) = 2.5
        m.Cell(0, 1) += 1
        Console.WriteLine(m.Cell(0, 1) & " " & m.Cell(1, 1))
        Dim m2 As New Geometry.Matrix(3)
        Geometry.Matrix.Created += 10
        Console.WriteLine(Geometry.Matrix.Created)
        Dim i As New Inner With {.V = 7}
        Console.WriteLine(i.ToString() & " " & i.V)
        Console.WriteLine(CObj(m))
        Inner.Hello()
    End Sub
End Module
