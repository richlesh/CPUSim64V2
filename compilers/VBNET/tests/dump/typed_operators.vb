Module Ops
    Sub Main()
        Dim by As Byte = 1, sb As SByte = 1, sh As Short = 1, us As UShort = 1
        Dim i As Integer = 1, ui As UInteger = 1, l As Long = 1, ul As ULong = 1
        Dim f As Single = 1, d As Double = 1, b As Boolean = True, c As Char = "c"c, s As String = "s"
        Dim r01 = by + by
        Dim r02 = sb + by
        Dim r03 = sh + us
        Dim r04 = i + ui
        Dim r05 = by + sh
        Dim r06 = b + b
        Dim r07 = b + by
        Dim r08 = i + f
        Dim r09 = l + d
        Dim r10 = i / i
        Dim r11 = f / i
        Dim r12 = d \ i
        Dim r13 = d Mod i
        Dim r14 = by ^ by
        Dim r15 = i & d
        Dim r16 = sh << 2
        Dim r17 = d >> 1
        Dim r18 = Not d
        Dim r19 = Not by
        Dim r20 = -by
        Dim r21 = -us
        Dim r22 = s + i
        Dim r23 = c + c
        Dim r24 = s = c
        Dim r25 = i And ul
        Dim r26 = b And b
        Dim r27 = i AndAlso b
        Dim r28 = 7 \ 2 + 7 Mod -3 + -7 Mod 3
        Dim r29 = CInt(2.5) + CInt(3.5) + CInt(-2.5)
        Dim r30 = CByte(True) + CShort(True)
        Dim r31 = &HFFFF + &HFFFFS + &H8000_0000L + &O777UI
        Dim r32 = 1 << 33
        Dim r33 = CLng(Integer.MaxValue) + 1
        Dim r34 = "a" & "b"c & True
        Dim r35 = 1.5F + 2
        Dim r36 = If(b, i, l)
        Dim r37 = NameOf(r36)
        Dim r38 = 10 / 4 * 2
        Dim r39 = 1 = 1.0
        Dim r40 = UInteger.MaxValue + 1L
    End Sub
End Module
