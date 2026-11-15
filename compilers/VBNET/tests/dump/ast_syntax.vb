Option Strict On
Imports System.Collections.Generic
Imports SB = System.Text.StringBuilder

Namespace Demo.Shapes
    Public MustInherit Class Shape
        Implements IComparable(Of Shape)
        Public MustOverride Function Area() As Double
        Public Property Name As String = "shape"
        Private _id As Integer
        Public ReadOnly Property Id As Integer
            Get
                Return _id
            End Get
        End Property
        Public Function CompareTo(other As Shape) As Integer Implements IComparable(Of Shape).CompareTo
            Return Area().CompareTo(other.Area())
        End Function
        Public Shared Operator +(a As Shape, b As Shape) As Double
            Return a.Area() + b.Area()
        End Operator
        Public Event Changed(sender As Object)
    End Class

    Public Class Circle : Inherits Shape
        Private r As Double
        Sub New(r As Double)
            Me.r = r
        End Sub
        Public Overrides Function Area() As Double
            Return Math.PI * r ^ 2
        End Function
    End Class

    Enum Color As Byte
        Red = 1
        Green
        Blue = Green + 2
    End Enum

    Structure Point
        Public X, Y As Integer
    End Structure

    Interface IThing(Of In T)
        Sub DoIt(x As T)
        ReadOnly Property Count As Integer
    End Interface

    Delegate Function Mapper(Of T)(x As T) As T
End Namespace

Module Program
    Sub Main(args As String())
        Dim a(9) As Integer, b() As String = {"x", "y"}
        Dim m As New Dictionary(Of String, List(Of Integer))
        Const Limit As Integer = 10
        Static count%
        For i As Integer = 0 To 9 Step 2
            a(i) = i * i
        Next
        For Each s In b : Console.WriteLine(s) : Next
        Dim n = 0
        Do While n < 5
            n += 1
            If n = 3 Then Continue Do
        Loop
        Do : n -= 1 : Loop Until n <= 0
        While n < 3 : n = n + 1 : End While
        Select Case n
            Case 1, 2 To 4, Is > 10
                Console.WriteLine("a")
            Case Else
                Console.WriteLine("b")
        End Select
        If n > 1 AndAlso Not n = 2 Then
            n = -2 ^ 2
        ElseIf n < 0 Then
            n = 7 \ 2 Mod 3
        Else
            n = If(n > 0, 1, 2)
        End If
        If n = 1 Then n = 2 Else n = 3
        Try
            Throw New InvalidOperationException("bad")
        Catch ex As InvalidOperationException When ex.Message <> ""
            Console.WriteLine(ex.Message)
        Catch
        Finally
            n = 0
        End Try
        Dim sq = Function(x As Integer) x * x
        Dim pr = Sub(x) Console.WriteLine(x)
        Dim big = Function(x As Integer) As Integer
                      Return x + 1
                  End Function
        Dim q = From x In a Where x > 2 Order By x Descending Select x * 2
        Dim s2 = $"n={n,5:D2} sum={sq(3) + 1}{{}}"
        ReDim Preserve a(0 To 19)
        Dim p As New Point With {.X = 1, .Y = 2}
        Dim o = New With {Key .A = 1, .B = "b"}
        Dim arr = New Integer(,) {{1, 2}, {3, 4}}
        Dim c As Char = "x"c
        With p
            .X = .Y + 1
        End With
        Using sbx As New SB()
        End Using
        AddHandler obj.Changed, AddressOf Handler
        Console.WriteLine(TypeOf o Is Object)
        Call Foo
        GoTo done
done:
        Exit Sub
    End Sub
End Module
