' holy_grail.vb
'
' The Bridge of Death scene from Monty Python and the Holy Grail: asks three
' questions and answers in red, blinking text.  An answer to the third
' question that names both an African and a European swallow stumps the
' bridgekeeper.
' VB.NET version of documentation/projects/holy_grail.asm.
'
' Author:   Richard Lesh
Module holy_grail
    Dim esc As String = ChrW(27)

    Function ReadAnswer() As String
        Dim line As String = Console.ReadLine()
        If line Is Nothing Then Return ""
        Return line
    End Function

    Function Main() As Integer
        Console.WriteLine("STOP! Who would cross the Bridge of Death must answer me" & vbLf &
                          "these questions three, ere the other side he see.")
        Console.WriteLine()
        Console.Write("BRIDGEKEEPER: What... is your name? ")
        Dim name As String = ReadAnswer()
        Console.Write("BRIDGEKEEPER: What... is your quest? ")
        Dim quest As String = ReadAnswer()
        Console.Write("BRIDGEKEEPER: What... is the air-speed velocity of an unladen swallow? ")
        Dim speed As String = ReadAnswer().ToLower()
        Console.WriteLine()
        Dim african As Boolean = speed.Contains("african")
        Dim european As Boolean = speed.Contains("european")
        Console.Write(esc & "[31m")           ' red
        Console.Write(esc & "[5m")            ' blink
        If african AndAlso european Then
            Console.Write("BRIDGEKEEPER: Huh? I-- I don't know that... AAAAARGH!" & vbLf & vbLf)
        Else
            Console.Write("BRIDGEKEEPER: You are cast into the Gorge of Eternal Peril!" & vbLf & vbLf)
        End If
        Console.Write(esc & "[25m")
        Console.Write(esc & "[39m")
        Return 0
    End Function
End Module
