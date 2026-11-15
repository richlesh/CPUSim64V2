Class TemperatureEventArgs
    Inherits EventArgs
    Public ReadOnly Celsius As Double
    Public Sub New(c As Double)
        Celsius = c
    End Sub
End Class

Class Thermometer
    Public Event Changed(sender As Object, e As TemperatureEventArgs)
    Public Event Alarm As EventHandler
    Public Shared Event Created(name As String)
    Private _t As Double
    Public ReadOnly Name As String

    Public Sub New(name As String)
        Me.Name = name
        RaiseEvent Created(name)
    End Sub

    Public Property Temperature As Double
        Get
            Return _t
        End Get
        Set(value As Double)
            _t = value
            RaiseEvent Changed(Me, New TemperatureEventArgs(value))
            If value > 30 Then RaiseEvent Alarm(Me, EventArgs.Empty)
        End Set
    End Property

    Private Sub SelfCheck(sender As Object, e As TemperatureEventArgs) Handles Me.Changed
        Console.WriteLine("  self check: " & e.Celsius)
    End Sub
End Class

Class Display
    Private WithEvents sensor As Thermometer
    Public Sub Attach(t As Thermometer)
        sensor = t
    End Sub
    Private Sub OnChanged(sender As Object, e As TemperatureEventArgs) Handles sensor.Changed
        Console.WriteLine("  display " & DirectCast(sender, Thermometer).Name & ": " & e.Celsius.ToString("F1"))
    End Sub
    Private Sub OnAlarm(sender As Object, e As EventArgs) Handles sensor.Alarm
        Console.WriteLine("  display ALARM")
    End Sub
End Class

Module Program
    Dim count As Integer

    Sub Logger(sender As Object, e As TemperatureEventArgs)
        count += 1
        Console.WriteLine("  logger #" & count & " " & e.Celsius)
    End Sub

    Sub Main()
        AddHandler Thermometer.Created, Sub(n) Console.WriteLine("created " & n)
        Dim t As New Thermometer("kitchen")
        t.Temperature = 20
        AddHandler t.Changed, AddressOf Logger
        t.Temperature = 21.5
        Dim d As New Display()
        d.Attach(t)
        t.Temperature = 35
        Dim t2 As New Thermometer("garage")
        d.Attach(t2)
        t.Temperature = 22
        t2.Temperature = 5
        RemoveHandler t.Changed, AddressOf Logger
        t.Temperature = 23
        Dim alarms = 0
        AddHandler t.Alarm, Sub(s, e) alarms += 1
        t.Temperature = 40
        t.Temperature = 41
        Console.WriteLine("alarms " & alarms & ", logged " & count)
    End Sub
End Module
