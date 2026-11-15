' Inheritance, interfaces, overriding and generics: shapes.
Interface IScalable
    Sub Scale(factor As Double)
End Interface

MustInherit Class Shape
    Implements IComparable(Of Shape)
    Public MustOverride ReadOnly Property Name As String
    Public MustOverride Function Area() As Double
    Public Function CompareTo(other As Shape) As Integer Implements IComparable(Of Shape).CompareTo
        Return Area().CompareTo(other.Area())
    End Function
    Public Overrides Function ToString() As String
        Return $"{Name,-10}{Area(),8:F2}"
    End Function
End Class

Class Circle
    Inherits Shape
    Implements IScalable
    Private r As Double
    Public Sub New(radius As Double)
        r = radius
    End Sub
    Public Overrides ReadOnly Property Name As String
        Get
            Return "circle"
        End Get
    End Property
    Public Overrides Function Area() As Double
        Return Math.PI * r * r
    End Function
    Public Sub Scale(factor As Double) Implements IScalable.Scale
        r *= factor
    End Sub
End Class

Class Rectangle
    Inherits Shape
    Implements IScalable
    Protected w As Double, h As Double
    Public Sub New(w As Double, h As Double)
        Me.w = w
        Me.h = h
    End Sub
    Public Overrides ReadOnly Property Name As String
        Get
            Return "rectangle"
        End Get
    End Property
    Public Overrides Function Area() As Double
        Return w * h
    End Function
    Public Sub Scale(factor As Double) Implements IScalable.Scale
        w *= factor
        h *= factor
    End Sub
End Class

Class Square
    Inherits Rectangle
    Public Sub New(side As Double)
        MyBase.New(side, side)
    End Sub
    Public Overrides ReadOnly Property Name As String
        Get
            Return "square"
        End Get
    End Property
End Class

Module Shapes
    Function Largest(Of T As IComparable(Of T))(items As IEnumerable(Of T)) As T
        Dim best As T = Nothing
        Dim first = True
        For Each x In items
            If first OrElse x.CompareTo(best) > 0 Then best = x
            first = False
        Next
        Return best
    End Function

    Sub Main()
        Dim shapes As New List(Of Shape) From {New Circle(1.5), New Rectangle(2, 3.5), New Square(2.5)}
        For Each s In shapes
            Console.WriteLine(s)
        Next
        Console.WriteLine("largest: " & Largest(shapes).Name)
        For Each s In shapes
            If TypeOf s Is IScalable Then DirectCast(s, IScalable).Scale(2)
        Next
        shapes.Sort()
        Console.WriteLine("scaled by 2, sorted by area:")
        For Each s In shapes
            Console.WriteLine(s)
        Next
    End Sub
End Module
