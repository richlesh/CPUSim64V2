' System.vb - the exception classes of the .NET library, part of every
' program compiled by the CPUSim64 VB.NET compiler (lib/ is searched next to
' rt/).  The messages are those of .NET.  The run-time library creates the
' exceptions of its errors (overflow, bad index, Nothing, ...) through
' VBRuntimeHelpers.MakeException.

Namespace System
    Public Interface IDisposable
        Sub Dispose()
    End Interface

    ' The primitive types satisfy constraints with these interfaces (their
    ' CompareTo and Equals are built in).
    Public Interface IComparable
        Function CompareTo(obj As Object) As Integer
    End Interface

    Public Interface IComparable(Of T)
        Function CompareTo(other As T) As Integer
    End Interface

    Public Interface IEquatable(Of T)
        Function Equals(other As T) As Boolean
    End Interface

    ' The base class of the delegate types: the target object (Nothing for a
    ' Shared method), the method, and the next delegate of the invocation
    ' list.  The compiler knows the order of the fields.
    Public MustInherit Class [Delegate]
        Friend _target As Object
        Friend _method As Long
        Friend _next As [Delegate]

        Public ReadOnly Property Target As Object
            Get
                Return _target
            End Get
        End Property

        Private Shared Function Copy(d As [Delegate]) As [Delegate]
            Dim c = DirectCast(Microsoft.VisualBasic.CompilerServices.Utils.CloneObject(d), [Delegate])
            c._next = Nothing
            Return c
        End Function

        ' a + b: the invocation list of a, then that of b (copies)
        Public Shared Function Combine(a As [Delegate], b As [Delegate]) As [Delegate]
            If a Is Nothing Then Return b
            If b Is Nothing Then Return a
            Dim head As [Delegate] = Nothing, tail As [Delegate] = Nothing
            For pass = 1 To 2
                Dim d = If(pass = 1, a, b)
                While d IsNot Nothing
                    Dim c = Copy(d)
                    If head Is Nothing Then head = c Else tail._next = c
                    tail = c
                    d = d._next
                End While
            Next
            Return head
        End Function

        ' a - b: without the last occurrence of b (a single delegate)
        Public Shared Function Remove(a As [Delegate], b As [Delegate]) As [Delegate]
            If a Is Nothing OrElse b Is Nothing Then Return a
            Dim last = -1, i = 0
            Dim d = a
            While d IsNot Nothing
                If d._target Is b._target AndAlso d._method = b._method Then last = i
                i += 1
                d = d._next
            End While
            If last < 0 Then Return a
            Dim head As [Delegate] = Nothing, tail As [Delegate] = Nothing
            d = a
            i = 0
            While d IsNot Nothing
                If i <> last Then
                    Dim c = Copy(d)
                    If head Is Nothing Then head = c Else tail._next = c
                    tail = c
                End If
                i += 1
                d = d._next
            End While
            Return head
        End Function
    End Class

    Public MustInherit Class MulticastDelegate
        Inherits [Delegate]
    End Class

    Public Delegate Sub Action()
    Public Delegate Sub Action(Of T)(obj As T)
    Public Delegate Sub Action(Of T1, T2)(arg1 As T1, arg2 As T2)
    Public Delegate Sub Action(Of T1, T2, T3)(arg1 As T1, arg2 As T2, arg3 As T3)
    Public Delegate Function Func(Of TResult)() As TResult
    Public Delegate Function Func(Of T, TResult)(arg As T) As TResult
    Public Delegate Function Func(Of T1, T2, TResult)(arg1 As T1, arg2 As T2) As TResult
    Public Delegate Function Func(Of T1, T2, T3, TResult)(arg1 As T1, arg2 As T2, arg3 As T3) As TResult
    Public Delegate Function Predicate(Of T)(obj As T) As Boolean
    Public Delegate Function Comparison(Of T)(x As T, y As T) As Integer
    Public Delegate Function Converter(Of TInput, TOutput)(input As TInput) As TOutput

    Public Class EventArgs
        Public Shared ReadOnly Empty As New EventArgs()
    End Class

    Public Delegate Sub EventHandler(sender As Object, e As EventArgs)
    Public Delegate Sub EventHandler(Of TEventArgs)(sender As Object, e As TEventArgs)

    Public Class Exception
        Private ReadOnly _message As String
        Private ReadOnly _inner As Exception

        Public Sub New()
        End Sub

        Public Sub New(message As String)
            _message = message
        End Sub

        Public Sub New(message As String, innerException As Exception)
            _message = message
            _inner = innerException
        End Sub

        Public Overridable ReadOnly Property Message As String
            Get
                If _message Is Nothing Then
                    Return "Exception of type '" & Microsoft.VisualBasic.CompilerServices.Utils.FullTypeName(Me) & "' was thrown."
                End If
                Return _message
            End Get
        End Property

        Public ReadOnly Property InnerException As Exception
            Get
                Return _inner
            End Get
        End Property

        Public Overridable ReadOnly Property StackTrace As String
            Get
                Return Nothing
            End Get
        End Property

        Public Function GetBaseException() As Exception
            Dim e As Exception = Me
            While e.InnerException IsNot Nothing
                e = e.InnerException
            End While
            Return e
        End Function

        Public Overrides Function ToString() As String
            Dim s As String = Microsoft.VisualBasic.CompilerServices.Utils.FullTypeName(Me)
            Dim m As String = Message
            If m IsNot Nothing AndAlso m.Length > 0 Then s &= ": " & m
            If _inner IsNot Nothing Then
                s &= " ---> " & _inner.ToString() & Microsoft.VisualBasic.Constants.vbLf & "   --- End of inner exception stack trace ---"
            End If
            Return s
        End Function
    End Class

    Public Class SystemException
        Inherits Exception
        Public Sub New()
            MyBase.New("System error.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
        Public Sub New(message As String, innerException As Exception)
            MyBase.New(message, innerException)
        End Sub
    End Class

    Public Class ApplicationException
        Inherits Exception
        Public Sub New()
            MyBase.New("Error in the application.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
        Public Sub New(message As String, innerException As Exception)
            MyBase.New(message, innerException)
        End Sub
    End Class

    Public Class ArgumentException
        Inherits SystemException
        Private ReadOnly _param As String
        Public Sub New()
            MyBase.New("Value does not fall within the expected range.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
        Public Sub New(message As String, innerException As Exception)
            MyBase.New(message, innerException)
        End Sub
        Public Sub New(message As String, paramName As String)
            MyBase.New(message)
            _param = paramName
        End Sub
        Public Overridable ReadOnly Property ParamName As String
            Get
                Return _param
            End Get
        End Property
        Public Overrides ReadOnly Property Message As String
            Get
                If _param Is Nothing OrElse _param.Length = 0 Then Return MyBase.Message
                Return MyBase.Message & " (Parameter '" & _param & "')"
            End Get
        End Property
    End Class

    Public Class ArgumentNullException
        Inherits ArgumentException
        Public Sub New()
            MyBase.New("Value cannot be null.")
        End Sub
        Public Sub New(paramName As String)
            MyBase.New("Value cannot be null.", paramName)
        End Sub
        Public Sub New(paramName As String, message As String)
            MyBase.New(message, paramName)
        End Sub
    End Class

    Public Class ArgumentOutOfRangeException
        Inherits ArgumentException
        Public Sub New()
            MyBase.New("Specified argument was out of the range of valid values.")
        End Sub
        Public Sub New(paramName As String)
            MyBase.New("Specified argument was out of the range of valid values.", paramName)
        End Sub
        Public Sub New(paramName As String, message As String)
            MyBase.New(message, paramName)
        End Sub
    End Class

    Public Class ArithmeticException
        Inherits SystemException
        Public Sub New()
            MyBase.New("Overflow or underflow in the arithmetic operation.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    Public Class DivideByZeroException
        Inherits ArithmeticException
        Public Sub New()
            MyBase.New("Attempted to divide by zero.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    Public Class OverflowException
        Inherits ArithmeticException
        Public Sub New()
            MyBase.New("Arithmetic operation resulted in an overflow.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    Public Class FormatException
        Inherits SystemException
        Public Sub New()
            MyBase.New("One of the identified items was in an invalid format.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    Public Class IndexOutOfRangeException
        Inherits SystemException
        Public Sub New()
            MyBase.New("Index was outside the bounds of the array.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    Public Class InvalidCastException
        Inherits SystemException
        Public Sub New()
            MyBase.New("Specified cast is not valid.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    Public Class InvalidOperationException
        Inherits SystemException
        Public Sub New()
            MyBase.New("Operation is not valid due to the current state of the object.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
        Public Sub New(message As String, innerException As Exception)
            MyBase.New(message, innerException)
        End Sub
    End Class

    Public Class NotImplementedException
        Inherits SystemException
        Public Sub New()
            MyBase.New("The method or operation is not implemented.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    Public Class NotSupportedException
        Inherits SystemException
        Public Sub New()
            MyBase.New("Specified method is not supported.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    Public Class NullReferenceException
        Inherits SystemException
        Public Sub New()
            MyBase.New("Object reference not set to an instance of an object.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    Public Class OutOfMemoryException
        Inherits SystemException
        Public Sub New()
            MyBase.New("Insufficient memory to continue the execution of the program.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    Public Class ArrayTypeMismatchException
        Inherits SystemException
        Public Sub New()
            MyBase.New("Attempted to access an element as a type incompatible with the array.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    Public Class ObjectDisposedException
        Inherits InvalidOperationException
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    ' The run-time library's errors: the exception for an error code and
    ' message (Nothing: the default message).  The parameters are in reverse
    ' order because the library calls this function with the C convention.
    Friend Module VBRuntimeHelpers
        Function MakeException(message As String, code As Integer) As Exception
            Select Case code
                Case 1 : Return If(message Is Nothing, New OverflowException(), New OverflowException(message))
                Case 2 : Return If(message Is Nothing, New DivideByZeroException(), New DivideByZeroException(message))
                Case 3 : Return If(message Is Nothing, New IndexOutOfRangeException(), New IndexOutOfRangeException(message))
                Case 4 : Return If(message Is Nothing, New NullReferenceException(), New NullReferenceException(message))
                Case 5 : Return If(message Is Nothing, New InvalidCastException(), New InvalidCastException(message))
                Case 6 : Return If(message Is Nothing, New FormatException(), New FormatException(message))
                Case 7 : Return New ArgumentOutOfRangeException(Nothing, message)
                Case 8 : Return New ArgumentException(message)
                Case 9 : Return New ArgumentNullException(Nothing, message)
                Case 10 : Return New OutOfMemoryException()
                Case 11 : Return New InvalidOperationException(message)
                Case 12 : Return New ArrayTypeMismatchException(message)
                Case 13 : Return New ArithmeticException(message)
                Case 15 : Return New NotSupportedException(message)
                Case 16 : Return New NotImplementedException(message)
                Case 17 : Return New System.IO.FileNotFoundException(message)
                Case 18 : Return New System.IO.IOException(message)
                Case 19 : Return New ObjectDisposedException(message)
            End Select
            Return New Exception(message)
        End Function
    End Module
End Namespace

Namespace System.Collections
    Public Interface IEnumerator
        Function MoveNext() As Boolean
        ReadOnly Property Current As Object
        Sub Reset()
    End Interface

    Public Interface IEnumerable
        Function GetEnumerator() As IEnumerator
    End Interface
End Namespace

Namespace System.Collections.Generic
    Public Interface IEnumerator(Of T)
        Inherits System.Collections.IEnumerator, System.IDisposable
        Overloads ReadOnly Property Current As T
    End Interface

    Public Interface IEnumerable(Of T)
        Inherits System.Collections.IEnumerable
        Overloads Function GetEnumerator() As IEnumerator(Of T)
    End Interface
End Namespace

Namespace System.IO
    Public Class IOException
        Inherits SystemException
        Public Sub New()
            MyBase.New("I/O error occurred.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    Public Class FileNotFoundException
        Inherits IOException
        Public Sub New()
            MyBase.New("Unable to find the specified file.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class
End Namespace

Namespace System.Runtime.CompilerServices
    ' <Extension()> marks extension methods (the compiler recognizes the name)
    Public NotInheritable Class ExtensionAttribute
    End Class
End Namespace
