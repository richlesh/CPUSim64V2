Module Arith
    Function Fact(n As Integer) As Long
        If n <= 1 Then Return 1
        Return n * Fact(n - 1)
    End Function

    Sub Main()
        Dim a As Integer = 17, b As Integer = 5
        Console.WriteLine(a + b)
        Console.WriteLine(a - b * 2)
        Console.WriteLine(a \ b)
        Console.WriteLine(a Mod b)
        Console.WriteLine(-a \ b)
        Console.WriteLine(-a Mod b)
        Console.WriteLine(a / b)
        Console.WriteLine(a ^ 2)
        Console.WriteLine(2 ^ 0.5)
        Console.WriteLine(1 / 3)
        Console.WriteLine(0.1 + 0.2)
        Console.WriteLine(1.0E+15)
        Console.WriteLine(123456789012345.0)
        Console.WriteLine(0.0001)
        Console.WriteLine(0.00001)
        Console.WriteLine(-1.5E-300 * 1.0E-300)
        Console.WriteLine(1 / 0.0)
        Console.WriteLine(Fact(20))
        Dim x As Long = Long.MaxValue
        Console.WriteLine(x)
        Console.WriteLine(Long.MinValue)
        Dim by As Byte = 200
        Console.WriteLine(by + 55)
        Dim sh As Short = -32768
        Console.WriteLine(sh)
        Dim u As UInteger = 4000000000UI
        Console.WriteLine(u + 1)
        Dim ul As ULong = 18446744073709551615UL
        Console.WriteLine(ul)
        Console.WriteLine(ul \ 10UL)
        Console.WriteLine(True)
        Console.WriteLine(Not True)
        Console.WriteLine(5 > 3 AndAlso 2 > 3)
        Console.WriteLine("x"c)
        Console.WriteLine(CInt(2.5) & " " & CInt(3.5) & " " & CInt(-2.5))
        Console.WriteLine(CLng(1.0E+18))
        Console.WriteLine(CDbl(7) / 2)
        Dim f As Single = 1.1F
        Console.WriteLine(f)
        Console.WriteLine(f * 3)
        Console.WriteLine(CSng(1) / 3)
        Console.WriteLine(&HFF And 15)
        Console.WriteLine(1 << 10)
        Console.WriteLine(-16 >> 2)
        Console.WriteLine(Integer.MaxValue)
        Console.Write("no newline ")
        Console.WriteLine()
        Console.WriteLine("{0} + {1} = {2}", a, b, a + b)
        Console.WriteLine("[{0,5}] [{1,-5}]", 42, "ab")
        Dim s As String = "abc"
        Console.WriteLine(s & s.Length)
        Console.WriteLine($"a={a}, b={b}, sum={a + b}")
        Console.WriteLine(17.0 Mod 5)
        Console.WriteLine(CObj(3.25))
    End Sub
End Module
