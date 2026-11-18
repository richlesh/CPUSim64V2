Module HelloWorld

    Sub Main()
        Dim languages As String() = {"English", "Spanish", "French", "Greek", "Russian", "Japanese", "Chinese"}
        Dim greetings As String() = {
            "Hello, world!",
            "¡Hola, mundo!",
            "Bonjour le monde !",
            "Γειά σου, κόσμε!",
            "Привет, мир!",
            "こんにちは、世界！",
            "你好，世界！"
        }

        For i As Integer = 0 To languages.Length - 1
            Console.WriteLine($"{languages(i) & ":",-10}{greetings(i)}")
        Next
    End Sub

End Module
