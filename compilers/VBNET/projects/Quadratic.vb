' Quadratic.vb
'
' Solves the quadratic equation A*x^2 + B*x + C = 0, with real or complex
' roots.
' VB.NET version of documentation/projects/Quadratic.asm.
'
' Usage:
'     Quadratic A B C
'
' Example:
'     > Quadratic 1 -1 -2
'     Two real roots: x1 = 2.000000, x2 = -1.000000
'
' Author:   Richard Lesh
Module Quadratic
    Sub SolveQuadratic(a As Double, b As Double, c As Double)
        If a = 0.0 Then
            Console.WriteLine("Not a quadratic equation: coefficient A must be non-zero.")
            Return
        End If
        Dim disc As Double = b * b - a * c * 4
        Dim twoA As Double = a * 2
        If disc > 0.0 Then
            Dim sqrtD As Double = Math.Sqrt(disc)
            Console.WriteLine("Two real roots: x1 = {0:F6}, x2 = {1:F6}", (-b + sqrtD) / twoA, (-b - sqrtD) / twoA)
        ElseIf disc = 0.0 Then
            Console.WriteLine("One repeated (double) root: x = {0:F6}", -b / twoA)
        Else
            Dim imag As Double = Math.Abs(Math.Sqrt(-disc) / twoA)
            Dim real As Double = -b / twoA
            Console.WriteLine("Two complex roots: x1 = {0:F6} + {1:F6}i, x2 = {0:F6} - {1:F6}i", real, imag)
        End If
    End Sub

    Function Main(args As String()) As Integer
        If args.Length <> 3 Then
            Console.WriteLine("Syntax: Quadratic A B C   (solves A*x^2 + B*x + C = 0)")
            Return 1
        End If
        SolveQuadratic(Double.Parse(args(0)), Double.Parse(args(1)), Double.Parse(args(2)))
        Return 0
    End Function
End Module
