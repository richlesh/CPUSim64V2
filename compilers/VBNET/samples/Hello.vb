' Hello, World! in VB.NET for CPUSim64.
Module Hello
    Sub Main()
        Console.WriteLine("Hello, World!")
        Console.Write("What is your name? ")
        Dim name = Console.ReadLine()
        If String.IsNullOrWhiteSpace(name) Then name = "stranger"
        Console.WriteLine($"Nice to meet you, {name.Trim()}.")
    End Sub
End Module
