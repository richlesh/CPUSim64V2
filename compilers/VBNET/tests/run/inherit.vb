Interface IShape
    Function Area() As Double
    ReadOnly Property Name As String
End Interface

Interface IScalable
    Sub Scale(f As Double)
End Interface

Interface IThing
    Inherits IScalable
    Function Id() As Integer
End Interface

MustInherit Class Shape
    Implements IShape
    Protected label As String
    Private Shared created As Integer

    Public Sub New(label As String)
        Me.label = label
        created += 1
    End Sub

    Public MustOverride Function Area() As Double Implements IShape.Area

    Public Overridable ReadOnly Property Name As String Implements IShape.Name
        Get
            Return "shape " & label
        End Get
    End Property

    Public Overrides Function ToString() As String
        Return $"{Name} ({Area():F2})"
    End Function

    Public Shared Function Count() As Integer
        Return created
    End Function
End Class

Class Circle
    Inherits Shape
    Implements IScalable
    Public R As Double

    Public Sub New(r As Double)
        MyBase.New("circle")
        Me.R = r
    End Sub

    Public Overrides Function Area() As Double
        Return Math.PI * R * R
    End Function

    Public Sub Scale(f As Double) Implements IScalable.Scale
        R *= f
    End Sub
End Class

Class Square
    Inherits Shape
    Public Side As Double = 1

    Public Sub New()
        MyBase.New("square")
    End Sub

    Public Sub New(side As Double)
        Me.New()
        Me.Side = side
    End Sub

    Public Overrides Function Area() As Double
        Return Side * Side
    End Function

    Public Overrides ReadOnly Property Name As String
        Get
            Return MyBase.Name.ToUpper()
        End Get
    End Property

    Public Overrides Function Equals(obj As Object) As Boolean
        Dim other = TryCast(obj, Square)
        Return other IsNot Nothing AndAlso other.Side = Side
    End Function
End Class

Structure Tag
    Implements IThing
    Public N As Integer
    Public Function Id() As Integer Implements IThing.Id
        Return N
    End Function
    Public Sub Scale(f As Double) Implements IScalable.Scale
        N = CInt(N * f)
    End Sub
End Structure

Module Program
    Sub Main()
        Dim shapes() As Shape = {New Circle(1), New Square(2), New Square()}
        For Each s In shapes
            Console.WriteLine(s)
        Next
        Dim total As Double = 0
        For Each s As IShape In shapes
            total += s.Area()
        Next
        Console.WriteLine($"total {total:F3}, {Shape.Count()} shapes")
        Dim c As New Circle(2)
        Dim sc As IScalable = c
        sc.Scale(1.5)
        Console.WriteLine(c.R & " " & c.Name & " " & CType(c, IShape).Name)
        Console.WriteLine(shapes(1).Equals(New Square(2)) & " " & shapes(1).Equals(shapes(2)) & " " & shapes(0).Equals(shapes(0)))
        Console.WriteLine((TypeOf shapes(0) Is Circle) & " " & (TypeOf shapes(0) Is Square) & " " & (TypeOf shapes(1) IsNot Circle) & " " & (TypeOf shapes(0) Is IScalable))
        Dim o As Object = shapes(1)
        Dim sq = DirectCast(o, Square)
        Console.WriteLine(sq.Side & " " & (TryCast(o, Circle) Is Nothing))
        Dim t As New Tag With {.N = 4}
        Dim th As IThing = t
        th.Scale(2.5)
        Console.WriteLine(th.Id() & " " & t.N & " " & (TypeOf th Is IScalable))
        Dim base As Shape = c
        Console.WriteLine(base.Area().ToString("F4"))
        Try_Bad_Cast(o)
    End Sub

    Sub Try_Bad_Cast(o As Object)
        Dim cc As Circle = CType(o, Circle)
    End Sub
End Module
