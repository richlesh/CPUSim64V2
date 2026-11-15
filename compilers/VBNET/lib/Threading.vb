' Threading.vb - System.Threading.Thread for the CPUSim64 VB.NET compiler.
'
' A Thread runs its delegate in a simulator thread (Utils.ThreadStart, in
' rt/vb_thread.c).  Interlocked and Monitor (SyncLock) are built into the
' compiler (src/sema_sym.c), as is CPUSim64.Process (Fork, Wait, WaitAll).
'
' Threads share the heap and the Shared fields.  While other threads run the
' garbage collector waits; it runs again when they have all ended.

Imports Microsoft.VisualBasic.CompilerServices

Namespace System.Threading
    Public Delegate Sub ThreadStart()
    Public Delegate Sub ParameterizedThreadStart(obj As Object)

    Public Class ThreadStateException
        Inherits SystemException
        Public Sub New()
            MyBase.New("Thread is running or terminated; it cannot restart.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    Public NotInheritable Class Thread
        Private _start As ThreadStart
        Private _pstart As ParameterizedThreadStart
        Private _id As Integer
        Private _started As Boolean

        Public Sub New(start As ThreadStart)
            If start Is Nothing Then Throw New ArgumentNullException("start")
            _start = start
        End Sub

        Public Sub New(start As ParameterizedThreadStart)
            If start Is Nothing Then Throw New ArgumentNullException("start")
            _pstart = start
        End Sub

        Private Sub MarkStarted()
            If _started Then Throw New ThreadStateException()
            _started = True
        End Sub

        Public Sub Start()
            MarkStarted()
            If _start IsNot Nothing Then
                _id = Utils.ThreadStart(_start, Nothing, False)
            Else
                _id = Utils.ThreadStart(_pstart, Nothing, True)
            End If
        End Sub

        Public Sub Start(parameter As Object)
            If _pstart Is Nothing Then Throw New InvalidOperationException("The thread was created with a ThreadStart delegate that does not accept a parameter.")
            MarkStarted()
            _id = Utils.ThreadStart(_pstart, parameter, True)
        End Sub

        Public Sub Join()
            If Not _started Then Throw New ThreadStateException("Thread has not been started.")
            Utils.ThreadJoin(_id)
        End Sub

        ' the simulator's id of the thread (1 for the main thread); 0 before Start
        Public ReadOnly Property ManagedThreadId As Integer
            Get
                Return _id
            End Get
        End Property

        Public Shared ReadOnly Property CurrentThread As Thread
            Get
                Dim t As New Thread(CType(Nothing, ThreadStart), 0)
                t._id = Utils.ThreadId()
                t._started = True
                Return t
            End Get
        End Property

        Private Sub New(start As ThreadStart, unused As Integer)
        End Sub

        Public Shared Sub Sleep(millisecondsTimeout As Integer)
            Utils.ThreadSleep(millisecondsTimeout)
        End Sub
    End Class
End Namespace
