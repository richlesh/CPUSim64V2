Option Explicit On
Option Strict Off
Option Compare Text
Option Infer On
Imports System
Imports System.Collections.Generic, System.Linq
Imports IO = System.IO

<Assembly: CLSCompliant(True)>

Namespace Geometry.Shapes
    ''' A shape.
    Public MustInherit Class Shape
        Implements IComparable(Of Shape), IDisposable

        Public MustOverride Function Area() As Double
        Public MustOverride ReadOnly Property Name As String
        Protected Friend Overridable Sub Draw()
        End Sub
        Public Sub Dispose() Implements IDisposable.Dispose
        End Sub
        Public Function CompareTo(other As Shape) As Integer Implements IComparable(Of Shape).CompareTo
            Return Area().CompareTo(other.Area())
        End Function
    End Class

    <Serializable>
    Public NotInheritable Class Circle
        Inherits Shape
        Private ReadOnly _r As Double
        Public Shared Count As Integer = 0
        Private WithEvents timer As Object
        Public Const Pi As Double = 3.14159, Tau = 2 * Pi

        Public Sub New(r As Double)
            MyBase.New()
            _r = r
            Count += 1
        End Sub

        Shared Sub New()
        End Sub

        Public Overrides Function Area() As Double
            Return Pi * _r ^ 2
        End Function

        Public Overrides ReadOnly Property Name As String
            Get
                Return "circle"
            End Get
        End Property

        Default Public Property Item(index As Integer) As Double
            Get
                Return index * _r
            End Get
            Private Set(value As Double)
            End Set
        End Property

        Public Property Tag As String = "c"
        Public Property Items As New List(Of String) From {"a", "b"}
        Public Property Other As Object

        Public Shared Operator +(a As Circle, b As Circle) As Double
            Return a.Area() + b.Area()
        End Operator
        Public Shared Widening Operator CType(c As Circle) As Double
            Return c._r
        End Operator
        Public Shared Operator IsTrue(c As Circle) As Boolean
            Return c._r > 0
        End Operator
        Public Shared Operator IsFalse(c As Circle) As Boolean
            Return c._r <= 0
        End Operator

        Public Event Resized(sender As Object, e As EventArgs)
        Public Event Moved As EventHandler

        Public Custom Event Changed As EventHandler
            AddHandler(value As EventHandler)
            End AddHandler
            RemoveHandler(value As EventHandler)
            End RemoveHandler
            RaiseEvent(sender As Object, e As EventArgs)
            End RaiseEvent
        End Event

        Private Sub OnTick(sender As Object, e As EventArgs) Handles timer.Disposed, Me.Resized
        End Sub

        Public Function Map(Of T As {Class, New}, U)(x As T, ByRef y As U, Optional z As Integer = 3,
                                                     ParamArray rest() As Object) As U
            Return y
        End Function

        Public Class Nested
            Private Structure Inner
                Dim a, b As Integer
                Dim c() As String
            End Structure
        End Class
    End Class

    Public Structure Point
        Implements IEquatable(Of Point)
        Public X As Integer, Y As Integer
        Public Function Equals1(other As Point) As Boolean Implements IEquatable(Of Point).Equals
            Return X = other.X AndAlso Y = other.Y
        End Function
    End Structure

    Public Interface IShape(Of In T, Out U)
        Inherits IComparable, IDisposable
        Sub Resize(factor As Double)
        Function Scale(Of V)(by As V) As U
        ReadOnly Property Sides As Integer
        Property Label As String
        Event Changed(sender As Object)
        Class Helper
        End Class
    End Interface

    Public Enum Color As Byte
        Red = 1
        <Obsolete> Green
        Blue = Green + 2
    End Enum

    Public Delegate Function Mapper(Of T)(value As T) As T
    Delegate Sub Notify()

    Partial Class Circle2
    End Class
End Namespace

Namespace Global.Other
End Namespace

Friend Module Program
    Private counter As Integer
    Dim names() As String = {"x"}, total As Long

    Sub Main(args As String())
    End Sub

    Function Twice(x As Integer) As Integer
        Return 2 * x
    End Function

    Property Setting As Boolean
End Module
