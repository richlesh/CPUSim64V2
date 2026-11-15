Module EnumTest
    Enum Color
        Red = 1
        Green
        Blue = Green + 5
    End Enum

    Enum Flags As Byte
        None = 0
        A = 1
        B = 2
        C = 4
    End Enum

    Sub Main()
        Dim c As Color = Color.Green
        Console.WriteLine(c)
        Console.WriteLine(c.ToString())
        Console.WriteLine(Color.Blue.ToString() & " " & CInt(Color.Blue))
        Console.WriteLine($"{c} and {Color.Red}")
        Dim o As Object = Color.Red
        Console.WriteLine(o)
        Dim f = Flags.A Or Flags.C
        Console.WriteLine(f & " " & f.ToString())
        Console.WriteLine((f And Flags.C) = Flags.C)
        For k As Color = Color.Red To Color.Blue Step 3
            Console.Write(k.ToString() & " ")
        Next
        Console.WriteLine()
        Select Case c
            Case Color.Red : Console.WriteLine("red")
            Case Color.Green : Console.WriteLine("green")
        End Select
        Dim n As Integer = c + 1
        Console.WriteLine(n)
        c = CType(7, Color)
        Console.WriteLine(c = Color.Blue)
        Console.WriteLine(String.Format("{0}|{1,6}|", Color.Green, Color.Blue))
    End Sub
End Module
