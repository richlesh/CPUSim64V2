' Cubic.vb
'
' Solves the cubic equation A*x^3 + B*x^2 + C*x + D = 0 with Cardano's
' method (one real root and two complex ones) or the trigonometric method
' (three real roots).
' VB.NET version of documentation/projects/Cubic.asm.
'
' Usage:
'     Cubic A B C D
'
' Example:
'     > Cubic 1 -1 -2 2
'
' Author:   Richard Lesh
Module Cubic
    Const Epsilon As Double = 0.000000001

    ' the real cube root, also of negative numbers
    Function RealCbrt(x As Double) As Double
        Dim r As Double = Math.Pow(Math.Abs(x), 1.0 / 3)
        Return If(x < 0, -r, r)
    End Function

    Sub SolveCubic(a As Double, b As Double, c As Double, d As Double)
        b /= a
        c /= a
        d /= a
        Dim shift As Double = b / 3
        Dim p As Double = c - b * b / 3
        Dim q As Double = b * b * b * 2 / 27 - b * c / 3 + d
        Dim disc As Double = (q / 2) * (q / 2) + (p / 3) * (p / 3) * (p / 3)
        If disc > Epsilon Then
            Dim s As Double = Math.Sqrt(disc), h As Double = -(q / 2)
            Dim u As Double = RealCbrt(h + s), v As Double = RealCbrt(h - s)
            Dim real As Double = -((u + v) / 2) - shift
            Dim imag As Double = (u - v) * Math.Sqrt(3.0) / 2
            Console.WriteLine("x1 = {0:F6}", u + v - shift)
            Console.WriteLine("x2 = {0:F6} + {1:F6}i", real, imag)
            Console.WriteLine("x3 = {0:F6} - {1:F6}i", real, imag)
        ElseIf disc < -Epsilon Then
            Dim m As Double = Math.Sqrt(-(p / 3)) * 2
            Dim theta As Double = Math.Acos(q * 3 / (p * 2) * Math.Sqrt(-(3 / p))) / 3
            For k As Integer = 0 To 2
                Console.WriteLine("x{0} = {1:F6}", k + 1, Math.Cos(theta - Math.PI * 2 * k / 3) * m - shift)
            Next
        Else
            Dim u As Double = RealCbrt(-(q / 2))
            Console.WriteLine("x1 = {0:F6}", u * 2 - shift)
            Console.WriteLine("x2 = {0:F6}", -u - shift)
            Console.WriteLine("x3 = {0:F6}", -u - shift)
        End If
    End Sub

    Function Main(args As String()) As Integer
        If args.Length <> 4 Then
            Console.WriteLine("Syntax: Cubic A B C D   (solves A*x^3 + B*x^2 + C*x + D = 0)")
            Return 1
        End If
        Dim a As Double = Double.Parse(args(0))
        Dim b As Double = Double.Parse(args(1))
        Dim c As Double = Double.Parse(args(2))
        Dim d As Double = Double.Parse(args(3))
        If a = 0.0 Then
            Console.WriteLine("Error: coefficient A must be non-zero for a cubic equation.")
            Return 1
        End If
        Console.WriteLine("Solving {0:F6}*x^3 + {1:F6}*x^2 + {2:F6}*x + {3:F6} = 0", a, b, c, d)
        SolveCubic(a, b, c, d)
        Return 0
    End Function
End Module
