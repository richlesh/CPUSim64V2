#Const Level = 3
#Const Name = "vb"
#If Level > 2 AndAlso Name = "vb" Then
a = 1
#ElseIf Level = 2
a = 2
#Else
a = 3
#End If
#If Undefined Then
  this is skipped ( ) "
  #If True Then
  nested skipped
  #Else
  also skipped
  #End If
#ElseIf CPUSIM64 And VBC_VER >= 11 Then
b = 1
#End If
#Region "Helpers"
#If DEFINED_ON_CMDLINE = 7 Then
c = 7
#End If
#End Region
#If Not (1 + 2 * 3 = 7) Or -2 ^ 2 <> -4 Or 7 \ 2 <> 3 Or 7 Mod 3 <> 1 Or ("a" & 1) <> "a1" Then
bad = 1
#Else If If(Nothing, 5) = 5 And CInt("12" & "") = 12 Then
good = 1
#End If
