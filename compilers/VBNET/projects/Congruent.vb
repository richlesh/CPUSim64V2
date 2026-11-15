' Congruent.vb
'
' Tests whether n is a congruent number (the area of a right triangle with
' rational sides) with Tunnell's theorem: for a squarefree odd m, m is
' congruent if (assuming the Birch and Swinnerton-Dyer conjecture) the number
' of solutions of 2x^2 + y^2 + 8z^2 = m is twice that of
' 2x^2 + y^2 + 32z^2 = m; for an even m, 8x^2 + 2y^2 + 16z^2 = m against
' 8x^2 + 2y^2 + 64z^2 = m.
' VB.NET version of documentation/projects/Congruent.asm.
'
' Usage:
'     Congruent <n>
'
' Example:
'     > Congruent 6
'     n = 6 (squarefree kernel = 6)
'     t1 = 4, 2*t2 = 4
'     6 IS a congruent number (assuming BSD)
'
' Author:   Richard Lesh
Module Congruent
    ' n divided by its largest square factor
    Function SquarefreeKernel(n As Long) As Long
        Dim m As Long = n, i As Long = 2
        While i * i <= m
            If m Mod (i * i) = 0 Then
                m \= i * i
            Else
                i += 1
            End If
        End While
        Return m
    End Function

    Function ISqrt(n As Long) As Long
        Return CLng(Math.Floor(Math.Sqrt(CDbl(n))))
    End Function

    ' floor(sqrt(target / divisor))
    Function ComputeLimit(target As Long, divisor As Long) As Long
        Return CLng(Math.Floor(Math.Sqrt(CDbl(target) / CDbl(divisor))))
    End Function

    ' the number of integer solutions (x, y, z) of a*x^2 + b*y^2 + c*z^2 = target
    Function CountSolutions(target As Long, a As Long, b As Long, c As Long) As Long
        Dim count As Long = 0
        Dim xlim As Long = ComputeLimit(target, a)
        For x As Long = -xlim To xlim
            Dim remX As Long = a * x * x
            If remX > target Then Continue For
            remX = target - remX
            Dim ylim As Long = ComputeLimit(remX, b)
            For y As Long = -ylim To ylim
                Dim remXY As Long = b * y * y
                If remXY > remX Then Continue For
                remXY = remX - remXY
                If remXY Mod c = 0 Then
                    Dim zsq As Long = remXY \ c
                    Dim z As Long = ISqrt(zsq)
                    If z * z = zsq Then count += If(z = 0, 1L, 2L)      ' z and -z
                End If
            Next
        Next
        Return count
    End Function

    Function Main(args As String()) As Integer
        If args.Length < 1 Then
            Console.WriteLine("Usage: Congruent <n>")
            Return 1
        End If
        Dim n As Long = Long.Parse(args(0))
        If n <= 0 Then
            Console.WriteLine("n must be a positive integer")
            Return 1
        End If
        Dim m As Long = SquarefreeKernel(n)
        Dim t1 As Long, t2 As Long
        If (m And 1) <> 0 Then
            t1 = CountSolutions(m, 2, 1, 8)
            t2 = CountSolutions(m, 2, 1, 32)
        Else
            t1 = CountSolutions(m, 8, 2, 16)
            t2 = CountSolutions(m, 8, 2, 64)
        End If
        Console.WriteLine("n = {0} (squarefree kernel = {1})", n, m)
        Console.WriteLine("t1 = {0}, 2*t2 = {1}", t1, 2 * t2)
        If t1 = 2 * t2 Then
            Console.WriteLine("{0} IS a congruent number (assuming BSD)", n)
        Else
            Console.WriteLine("{0} is NOT a congruent number", n)
        End If
        Return 0
    End Function
End Module
