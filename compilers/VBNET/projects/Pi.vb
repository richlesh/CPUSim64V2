' Pi.vb
'
' Computes pi with the Bailey-Borwein-Plouffe series
'     pi = sum[k=0..] 16^-k * (4/(8k+1) - 2/(8k+4) - 1/(8k+5) - 1/(8k+6))
' for a number of terms (1 to 11) and prints it with 16 decimals.
' VB.NET version of documentation/projects/Pi.asm.
'
' Usage:
'     Pi <terms>
'
' Example:
'     > Pi 11
'     PI(11): 3.1415926535897931
'
' Author:   Richard Lesh
Module Pi
    Function ComputePi(terms As Long) As Double
        Dim sum As Double = 0.0
        For k As Long = 0 To terms - 1
            Dim t As Double = 4.0 / (8 * k + 1) - 2.0 / (8 * k + 4) - 1.0 / (8 * k + 5) - 1.0 / (8 * k + 6)
            sum += t / Math.Pow(16.0, CDbl(k))
        Next
        Return sum
    End Function

    Function Main(args As String()) As Integer
        If args.Length <> 1 Then
            Console.WriteLine("You must supply the number of terms 1-11")
            Return 1
        End If
        Dim terms As Long = Long.Parse(args(0))
        If Not (0 < terms AndAlso terms <= 11) Then
            Console.WriteLine("You must supply the number of terms in the range [1-11]")
            Return 1
        End If
        Console.WriteLine("PI({0}): {1:F16}", terms, ComputePi(terms))
        Return 0
    End Function
End Module
