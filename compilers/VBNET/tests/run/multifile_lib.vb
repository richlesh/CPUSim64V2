Namespace MathLib.Vectors
    Partial Public Class Vec
        Public X As Double, Y As Double

        Public Sub New(x As Double, y As Double)
            Me.X = x
            Me.Y = y
        End Sub

        Public Shared Operator +(a As Vec, b As Vec) As Vec
            Return New Vec(a.X + b.X, a.Y + b.Y)
        End Operator

        Public Shared Operator -(a As Vec) As Vec
            Return New Vec(-a.X, -a.Y)
        End Operator

        Public Shared Operator *(a As Vec, k As Double) As Vec
            Return New Vec(a.X * k, a.Y * k)
        End Operator

        Public Shared Operator =(a As Vec, b As Vec) As Boolean
            Return a.X = b.X AndAlso a.Y = b.Y
        End Operator

        Public Shared Operator <>(a As Vec, b As Vec) As Boolean
            Return Not (a = b)
        End Operator

        Public Shared Widening Operator CType(v As Vec) As Double
            Return Math.Sqrt(v.X * v.X + v.Y * v.Y)
        End Operator

        Public Shared Narrowing Operator CType(n As Integer) As Vec
            Return New Vec(n, n)
        End Operator
    End Class

    Partial Class Vec
        Public Overrides Function ToString() As String
            Return $"<{X}, {Y}>"
        End Function

        Public Function Describe() As String
            Return "vector of length " & CType(Me, Double).ToString("F2")
        End Function
    End Class
End Namespace

Namespace MathLib
    Public Class BaseLog
        Protected items As Integer
        Public Sub Add(s As String)
            items += 1
        End Sub
    End Class

    Public Class Logger
        Inherits BaseLog
        Public Overloads Sub Add(s As String, times As Integer)
            items += times
        End Sub
        Public Overloads Sub Add(d As Double)
            items += 100
        End Sub
        Public ReadOnly Property Count As Integer
            Get
                Return items
            End Get
        End Property
    End Class
End Namespace
