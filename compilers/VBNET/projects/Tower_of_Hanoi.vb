' Tower_of_Hanoi.vb
'
' Solves the Tower of Hanoi puzzle for n discs by recursion and prints the
' three pegs after about every tenth move.
' VB.NET version of documentation/projects/Tower_of_Hanoi.asm.
'
' Usage:
'     Tower_of_Hanoi <discs>
'
' Author:   Richard Lesh
Module Tower_of_Hanoi
    Dim numDiscs As Long
    Dim moveNumber As Long
    Dim printMod As Long = 1            ' the pegs are printed after every printMod moves
    Dim pegs As Long()                  ' pegs(peg * numDiscs + slot)

    Function GetDisk(peg As Long, slot As Long) As Long
        Return pegs(CInt(peg * numDiscs + slot))
    End Function

    Sub SetDisk(peg As Long, slot As Long, v As Long)
        pegs(CInt(peg * numDiscs + slot)) = v
    End Sub

    Sub PrintPegs()
        Console.WriteLine("Move: {0}", moveNumber)
        For i As Long = 0 To numDiscs - 1
            For j As Long = 0 To 2
                Dim disk As Long = GetDisk(j, i)
                If disk = 0 Then
                    Console.Write("|")
                Else
                    Console.Write(disk)
                End If
                Console.Write("   ")
            Next
            Console.WriteLine()
        Next
        Console.WriteLine("=========")
        Console.WriteLine()
    End Sub

    ' moves the top disk of src to the top of dst
    Sub MakeMove(src As Long, dst As Long)
        Dim disk As Long = 0, pos As Long = numDiscs - 1
        moveNumber += 1
        For i As Long = 0 To numDiscs - 1
            If GetDisk(src, i) <> 0 Then
                disk = GetDisk(src, i)
                SetDisk(src, i, 0)
                Exit For
            End If
        Next
        For i As Long = 0 To numDiscs - 1
            If GetDisk(dst, i) <> 0 Then
                pos = i - 1
                Exit For
            End If
        Next
        SetDisk(dst, pos, disk)
        If moveNumber Mod printMod = 0 Then PrintPegs()
    End Sub

    Sub Solve(n As Long, src As Long, dst As Long, aux As Long)
        If n = 1 Then
            MakeMove(src, dst)
        Else
            Solve(n - 1, src, aux, dst)
            MakeMove(src, dst)
            Solve(n - 1, aux, dst, src)
        End If
    End Sub

    Function Main(args As String()) As Integer
        If args.Length <> 1 Then
            Console.WriteLine("Syntax: Hanoi [disks]")
            Return 1
        End If
        Dim n As Long = Long.Parse(args(0))
        If n <= 0 Then n = 3
        numDiscs = n
        pegs = New Long(CInt(n * 3) - 1) {}
        For i As Long = 0 To n - 1
            SetDisk(0, i, i + 1)
        Next
        ' print about 10 times: every steps/10 moves, rounded down to a multiple of 5 (100 for n > 10)
        Dim steps As Long = (1L << CInt(n)) - 1
        Dim pm As Long = steps \ 10 + 2
        pm -= pm Mod If(n > 10, 100L, 5L)
        If pm < 1 Then pm = 1
        printMod = pm
        PrintPegs()
        Solve(n, 0, 2, 1)
        If moveNumber Mod printMod <> 0 Then PrintPegs()
        Return 0
    End Function
End Module
