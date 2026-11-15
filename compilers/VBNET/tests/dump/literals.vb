' literal forms
Dim a = 42 + 0 + 2147483647 + 18446744073709551615UL
Dim b = &HFF + &hffS + &O777 + &B1010 + &H7FFF_FFFFUI + 1_000_000
Dim c = 1S + 2US + 3I + 4UI + 5L + 6UL + 7% + 8&
Dim d = 1.5 + .25 + 1E3 + 2.5e-3 + 1F + 2R + 3D + 4! + 5# + 6@ + 1.5E+10R
Dim e = "Hello" & "say ""hi""" & "" & “smart” 
Dim f = "x"c & """"C
Dim g = $"a{1 + 2}b{{c}}{s("}"):N2}"
Dim h = True Or False Or Nothing
