' Collections.vb - the generic collections of System.Collections.Generic,
' part of every program compiled by the CPUSim64 VB.NET compiler (only the
' parts that a program uses are compiled into it).  They behave as in .NET:
' the same order of enumeration, the same exceptions and messages.

Namespace System.Collections.Generic
    Public Class KeyNotFoundException
        Inherits SystemException
        Public Sub New()
            MyBase.New("The given key was not present in the dictionary.")
        End Sub
        Public Sub New(message As String)
            MyBase.New(message)
        End Sub
    End Class

    ' Equality, hashing and ordering of the elements of any type T.
    Friend Module CollectionHelpers
        Function AreEqual(a As Object, b As Object) As Boolean
            If a Is Nothing Then Return b Is Nothing
            If b Is Nothing Then Return False
            Return a.Equals(b)
        End Function

        Function HashOf(a As Object) As Integer
            If a Is Nothing Then Return 0
            Return a.GetHashCode() And &H7FFFFFFF
        End Function

        Function CompareOf(Of T)(a As T, b As T) As Integer
            Dim oa As Object = a, ob As Object = b
            If oa Is Nothing Then Return If(ob Is Nothing, 0, -1)
            If ob Is Nothing Then Return 1
            Dim g = TryCast(oa, IComparable(Of T))
            If g IsNot Nothing Then Return g.CompareTo(b)
            Dim p = TryCast(oa, IComparable)
            If p IsNot Nothing Then Return p.CompareTo(ob)
            Return Microsoft.VisualBasic.CompilerServices.Utils.CompareObjects(oa, ob)
        End Function

        ' a stable merge sort of a(lo .. lo + n - 1)
        Sub SortRange(Of T)(a() As T, lo As Integer, n As Integer)
            If n < 2 Then Return
            Dim tmp(n - 1) As T
            MergeSort(a, tmp, lo, lo + n - 1)
        End Sub

        Private Sub MergeSort(Of T)(a() As T, tmp() As T, lo As Integer, hi As Integer)
            If hi - lo < 1 Then Return
            If hi - lo < 8 Then
                For i = lo + 1 To hi
                    Dim x = a(i)
                    Dim j = i - 1
                    While j >= lo AndAlso CompareOf(a(j), x) > 0
                        a(j + 1) = a(j)
                        j -= 1
                    End While
                    a(j + 1) = x
                Next
                Return
            End If
            Dim mid = (lo + hi) \ 2
            MergeSort(a, tmp, lo, mid)
            MergeSort(a, tmp, mid + 1, hi)
            Dim p = lo, q = mid + 1, k = 0
            While p <= mid AndAlso q <= hi
                If CompareOf(a(q), a(p)) < 0 Then
                    tmp(k) = a(q)
                    q += 1
                Else
                    tmp(k) = a(p)
                    p += 1
                End If
                k += 1
            End While
            While p <= mid
                tmp(k) = a(p)
                p += 1
                k += 1
            End While
            While q <= hi
                tmp(k) = a(q)
                q += 1
                k += 1
            End While
            For i = 0 To k - 1
                a(lo + i) = tmp(i)
            Next
        End Sub

        ' a stable merge sort with a Comparison
        Sub SortWith(Of T)(a() As T, lo As Integer, n As Integer, cmp As Comparison(Of T))
            If n < 2 Then Return
            Dim tmp(n - 1) As T
            MergeWith(a, tmp, lo, lo + n - 1, cmp)
        End Sub

        Private Sub MergeWith(Of T)(a() As T, tmp() As T, lo As Integer, hi As Integer, cmp As Comparison(Of T))
            If hi - lo < 1 Then Return
            Dim mid = (lo + hi) \ 2
            MergeWith(a, tmp, lo, mid, cmp)
            MergeWith(a, tmp, mid + 1, hi, cmp)
            Dim p = lo, q = mid + 1, k = 0
            While p <= mid AndAlso q <= hi
                If cmp(a(q), a(p)) < 0 Then
                    tmp(k) = a(q)
                    q += 1
                Else
                    tmp(k) = a(p)
                    p += 1
                End If
                k += 1
            End While
            While p <= mid
                tmp(k) = a(p)
                p += 1
                k += 1
            End While
            While q <= hi
                tmp(k) = a(q)
                q += 1
                k += 1
            End While
            For i = 0 To k - 1
                a(lo + i) = tmp(i)
            Next
        End Sub

        Function KeyText(key As Object) As String
            If key Is Nothing Then Return ""
            Return key.ToString()
        End Function

        Sub CheckVersion(expected As Integer, actual As Integer)
            If expected <> actual Then
                Throw New InvalidOperationException("Collection was modified; enumeration operation may not execute.")
            End If
        End Sub
    End Module

    ' An array as an IEnumerable(Of T): the compiler converts T() to
    ' IEnumerable(Of T) (and IEnumerable) with this wrapper.
    Friend NotInheritable Class ArrayWrapper(Of T)
        Implements IEnumerable(Of T)
        Private ReadOnly _a() As T
        Public Sub New(a() As T)
            _a = a
        End Sub
        Public Function GetEnumerator() As IEnumerator(Of T) Implements IEnumerable(Of T).GetEnumerator
            Return New ArrayEnumerator(Of T)(_a)
        End Function
        Private Function GetEnumerator2() As System.Collections.IEnumerator Implements System.Collections.IEnumerable.GetEnumerator
            Return New ArrayEnumerator(Of T)(_a)
        End Function
    End Class

    Friend NotInheritable Class ArrayEnumerator(Of T)
        Implements IEnumerator(Of T)
        Private ReadOnly _a() As T
        Private _i As Integer = -1
        Public Sub New(a() As T)
            _a = a
        End Sub
        Public Function MoveNext() As Boolean Implements System.Collections.IEnumerator.MoveNext
            _i += 1
            Return _i < _a.Length
        End Function
        Public ReadOnly Property Current As T Implements IEnumerator(Of T).Current
            Get
                Return _a(_i)
            End Get
        End Property
        Private ReadOnly Property Current1 As Object Implements System.Collections.IEnumerator.Current
            Get
                Return _a(_i)
            End Get
        End Property
        Public Sub Reset() Implements System.Collections.IEnumerator.Reset
            _i = -1
        End Sub
        Public Sub Dispose() Implements IDisposable.Dispose
        End Sub
    End Class

    ' ---------------------------------------------------------------- List

    Public Class List(Of T)
        Implements IEnumerable(Of T)
        Private _items() As T
        Private _size As Integer
        Private _version As Integer

        Public Sub New()
            ReDim _items(-1)
        End Sub

        Public Sub New(capacity As Integer)
            If capacity < 0 Then Throw New ArgumentOutOfRangeException("capacity", "Non-negative number required.")
            ReDim _items(capacity - 1)
        End Sub

        Public Sub New(collection As IEnumerable(Of T))
            ReDim _items(-1)
            AddRange(collection)
        End Sub

        Private Sub EnsureCapacity(min As Integer)
            If min <= _items.Length Then Return
            Dim cap = If(_items.Length = 0, 4, _items.Length * 2)
            If cap < min Then cap = min
            ReDim Preserve _items(cap - 1)
        End Sub

        Private Sub CheckIndex(index As Integer)
            If index < 0 OrElse index >= _size Then
                Throw New ArgumentOutOfRangeException("index", "Index was out of range. Must be non-negative and less than the size of the collection.")
            End If
        End Sub

        Public Property Capacity As Integer
            Get
                Return _items.Length
            End Get
            Set(value As Integer)
                If value < _size Then Throw New ArgumentOutOfRangeException("value", "capacity was less than the current size.")
                ReDim Preserve _items(value - 1)
            End Set
        End Property

        Public ReadOnly Property Count As Integer
            Get
                Return _size
            End Get
        End Property

        Default Public Property Item(index As Integer) As T
            Get
                CheckIndex(index)
                Return _items(index)
            End Get
            Set(value As T)
                CheckIndex(index)
                _items(index) = value
                _version += 1
            End Set
        End Property

        Public Sub Add(item As T)
            EnsureCapacity(_size + 1)
            _items(_size) = item
            _size += 1
            _version += 1
        End Sub

        Public Sub AddRange(collection As IEnumerable(Of T))
            If collection Is Nothing Then Throw New ArgumentNullException("collection")
            For Each x In ToArrayOf(collection)
                Add(x)
            Next
        End Sub

        Private Shared Function ToArrayOf(collection As IEnumerable(Of T)) As T()
            Dim tmp As New List(Of T)(4)
            For Each x In collection
                tmp.Add(x)
            Next
            Return tmp.ToArray()
        End Function

        Public Sub Insert(index As Integer, item As T)
            If index < 0 OrElse index > _size Then
                Throw New ArgumentOutOfRangeException("index", "Index must be within the bounds of the List.")
            End If
            EnsureCapacity(_size + 1)
            For i = _size To index + 1 Step -1
                _items(i) = _items(i - 1)
            Next
            _items(index) = item
            _size += 1
            _version += 1
        End Sub

        Public Sub RemoveAt(index As Integer)
            CheckIndex(index)
            For i = index To _size - 2
                _items(i) = _items(i + 1)
            Next
            _size -= 1
            _items(_size) = Nothing
            _version += 1
        End Sub

        Public Sub RemoveRange(index As Integer, count As Integer)
            If index < 0 OrElse count < 0 OrElse index + count > _size Then
                Throw New ArgumentException("Offset and length were out of bounds for the array or count is greater than the number of elements from index to the end of the source collection.")
            End If
            For i = index To _size - count - 1
                _items(i) = _items(i + count)
            Next
            For i = _size - count To _size - 1
                _items(i) = Nothing
            Next
            _size -= count
            _version += 1
        End Sub

        Public Function Remove(item As T) As Boolean
            Dim i = IndexOf(item)
            If i < 0 Then Return False
            RemoveAt(i)
            Return True
        End Function

        Public Function IndexOf(item As T) As Integer
            For i = 0 To _size - 1
                If AreEqual(_items(i), item) Then Return i
            Next
            Return -1
        End Function

        Public Function LastIndexOf(item As T) As Integer
            For i = _size - 1 To 0 Step -1
                If AreEqual(_items(i), item) Then Return i
            Next
            Return -1
        End Function

        Public Function Contains(item As T) As Boolean
            Return IndexOf(item) >= 0
        End Function

        Public Sub Clear()
            For i = 0 To _size - 1
                _items(i) = Nothing
            Next
            _size = 0
            _version += 1
        End Sub

        Public Sub Reverse()
            Dim i = 0, j = _size - 1
            While i < j
                Dim t = _items(i)
                _items(i) = _items(j)
                _items(j) = t
                i += 1
                j -= 1
            End While
            _version += 1
        End Sub

        Public Sub Sort()
            SortRange(_items, 0, _size)
            _version += 1
        End Sub

        Public Function BinarySearch(item As T) As Integer
            Dim lo = 0, hi = _size - 1
            While lo <= hi
                Dim m = lo + (hi - lo) \ 2
                Dim c = CompareOf(_items(m), item)
                If c = 0 Then Return m
                If c < 0 Then lo = m + 1 Else hi = m - 1
            End While
            Return Not lo
        End Function

        Public Sub Sort(comparison As Comparison(Of T))
            If comparison Is Nothing Then Throw New ArgumentNullException("comparison")
            SortWith(_items, 0, _size, comparison)
            _version += 1
        End Sub

        Public Function Find(match As Predicate(Of T)) As T
            For i = 0 To _size - 1
                If match(_items(i)) Then Return _items(i)
            Next
            Return Nothing
        End Function

        Public Function FindLast(match As Predicate(Of T)) As T
            For i = _size - 1 To 0 Step -1
                If match(_items(i)) Then Return _items(i)
            Next
            Return Nothing
        End Function

        Public Function FindIndex(match As Predicate(Of T)) As Integer
            For i = 0 To _size - 1
                If match(_items(i)) Then Return i
            Next
            Return -1
        End Function

        Public Function FindAll(match As Predicate(Of T)) As List(Of T)
            Dim r As New List(Of T)
            For i = 0 To _size - 1
                If match(_items(i)) Then r.Add(_items(i))
            Next
            Return r
        End Function

        Public Function Exists(match As Predicate(Of T)) As Boolean
            Return FindIndex(match) >= 0
        End Function

        Public Function TrueForAll(match As Predicate(Of T)) As Boolean
            For i = 0 To _size - 1
                If Not match(_items(i)) Then Return False
            Next
            Return True
        End Function

        Public Function RemoveAll(match As Predicate(Of T)) As Integer
            Dim k = 0
            For i = 0 To _size - 1
                If Not match(_items(i)) Then
                    _items(k) = _items(i)
                    k += 1
                End If
            Next
            Dim removed = _size - k
            For i = k To _size - 1
                _items(i) = Nothing
            Next
            _size = k
            _version += 1
            Return removed
        End Function

        Public Sub ForEach(action As Action(Of T))
            Dim v = _version
            For i = 0 To _size - 1
                CheckVersion(v, _version)
                action(_items(i))
            Next
        End Sub

        Public Function ConvertAll(Of TOutput)(converter As Converter(Of T, TOutput)) As List(Of TOutput)
            Dim r As New List(Of TOutput)(_size)
            For i = 0 To _size - 1
                r.Add(converter(_items(i)))
            Next
            Return r
        End Function

        Public Function GetRange(index As Integer, count As Integer) As List(Of T)
            If index < 0 OrElse count < 0 OrElse index + count > _size Then
                Throw New ArgumentException("Offset and length were out of bounds for the array or count is greater than the number of elements from index to the end of the source collection.")
            End If
            Dim r As New List(Of T)(count)
            For i = index To index + count - 1
                r.Add(_items(i))
            Next
            Return r
        End Function

        Public Function ToArray() As T()
            Dim a(_size - 1) As T
            For i = 0 To _size - 1
                a(i) = _items(i)
            Next
            Return a
        End Function

        Public Function GetEnumerator() As Enumerator
            Return New Enumerator(Me)
        End Function

        Private Function GetEnumerator1() As IEnumerator(Of T) Implements IEnumerable(Of T).GetEnumerator
            Return New Enumerator(Me)
        End Function

        Private Function GetEnumerator2() As System.Collections.IEnumerator Implements System.Collections.IEnumerable.GetEnumerator
            Return New Enumerator(Me)
        End Function

        Public Class Enumerator
            Implements IEnumerator(Of T)
            Private ReadOnly _list As List(Of T)
            Private ReadOnly _version As Integer
            Private _index As Integer = -1

            Friend Sub New(list As List(Of T))
                _list = list
                _version = list._version
            End Sub

            Public Function MoveNext() As Boolean Implements System.Collections.IEnumerator.MoveNext
                CheckVersion(_version, _list._version)
                _index += 1
                Return _index < _list._size
            End Function

            Public ReadOnly Property Current As T Implements IEnumerator(Of T).Current
                Get
                    Return _list._items(_index)
                End Get
            End Property

            Private ReadOnly Property Current1 As Object Implements System.Collections.IEnumerator.Current
                Get
                    Return _list._items(_index)
                End Get
            End Property

            Public Sub Reset() Implements System.Collections.IEnumerator.Reset
                _index = -1
            End Sub

            Public Sub Dispose() Implements IDisposable.Dispose
            End Sub
        End Class
    End Class

    ' ---------------------------------------------------------------- Dictionary

    Public Structure KeyValuePair(Of TKey, TValue)
        Private ReadOnly _key As TKey
        Private ReadOnly _value As TValue

        Public Sub New(key As TKey, value As TValue)
            _key = key
            _value = value
        End Sub

        Public ReadOnly Property Key As TKey
            Get
                Return _key
            End Get
        End Property

        Public ReadOnly Property Value As TValue
            Get
                Return _value
            End Get
        End Property

        Public Overrides Function ToString() As String
            Return "[" & KeyText(_key) & ", " & KeyText(_value) & "]"
        End Function
    End Structure

    Public Class Dictionary(Of TKey, TValue)
        Implements IEnumerable(Of KeyValuePair(Of TKey, TValue))
        Private _buckets() As Integer       ' the first entry of each bucket + 1 (0: empty)
        Private _hashes() As Integer        ' -1: a free entry
        Private _nexts() As Integer
        Private _keys() As TKey
        Private _values() As TValue
        Private _count As Integer           ' the entries used so far (with the free ones)
        Private _freeList As Integer = -1
        Private _freeCount As Integer
        Private _version As Integer

        Public Sub New()
            Init(0)
        End Sub

        Public Sub New(capacity As Integer)
            If capacity < 0 Then Throw New ArgumentOutOfRangeException("capacity", "Non-negative number required.")
            Init(capacity)
        End Sub

        Private Sub Init(capacity As Integer)
            ReDim _buckets(If(capacity < 3, 2, capacity - 1))
            ReDim _hashes(capacity - 1)
            ReDim _nexts(capacity - 1)
            ReDim _keys(capacity - 1)
            ReDim _values(capacity - 1)
        End Sub

        Public ReadOnly Property Count As Integer
            Get
                Return _count - _freeCount
            End Get
        End Property

        Private Function FindEntry(key As TKey) As Integer
            Dim ko As Object = key
            If ko Is Nothing Then Throw New ArgumentNullException("key")
            Dim h = HashOf(ko)
            Dim i = _buckets(h Mod _buckets.Length) - 1
            While i >= 0
                If _hashes(i) = h AndAlso AreEqual(_keys(i), ko) Then Return i
                i = _nexts(i)
            End While
            Return -1
        End Function

        Private Sub Resize()
            Dim size = If(_keys.Length < 2, 4, _keys.Length * 2)
            ReDim Preserve _hashes(size - 1)
            ReDim Preserve _nexts(size - 1)
            ReDim Preserve _keys(size - 1)
            ReDim Preserve _values(size - 1)
            ReDim _buckets(size - 1)
            For i = 0 To _count - 1
                If _hashes(i) >= 0 Then
                    Dim b = _hashes(i) Mod size
                    _nexts(i) = _buckets(b) - 1
                    _buckets(b) = i + 1
                End If
            Next
        End Sub

        Private Sub Insert(key As TKey, value As TValue, add As Boolean)
            Dim i = FindEntry(key)
            If i >= 0 Then
                If add Then Throw New ArgumentException("An item with the same key has already been added. Key: " & KeyText(key))
                _values(i) = value
                _version += 1
                Return
            End If
            Dim index As Integer
            If _freeCount > 0 Then
                index = _freeList
                _freeList = _nexts(index)
                _freeCount -= 1
            Else
                If _count = _keys.Length Then Resize()
                index = _count
                _count += 1
            End If
            Dim h = HashOf(key)
            Dim b = h Mod _buckets.Length
            _hashes(index) = h
            _keys(index) = key
            _values(index) = value
            _nexts(index) = _buckets(b) - 1
            _buckets(b) = index + 1
            _version += 1
        End Sub

        Public Sub Add(key As TKey, value As TValue)
            Insert(key, value, True)
        End Sub

        Public Function TryAdd(key As TKey, value As TValue) As Boolean
            If FindEntry(key) >= 0 Then Return False
            Insert(key, value, True)
            Return True
        End Function

        Default Public Property Item(key As TKey) As TValue
            Get
                Dim i = FindEntry(key)
                If i < 0 Then Throw New KeyNotFoundException("The given key '" & KeyText(key) & "' was not present in the dictionary.")
                Return _values(i)
            End Get
            Set(value As TValue)
                Insert(key, value, False)
            End Set
        End Property

        Public Function ContainsKey(key As TKey) As Boolean
            Return FindEntry(key) >= 0
        End Function

        Public Function ContainsValue(value As TValue) As Boolean
            For i = 0 To _count - 1
                If _hashes(i) >= 0 AndAlso AreEqual(_values(i), value) Then Return True
            Next
            Return False
        End Function

        Public Function TryGetValue(key As TKey, ByRef value As TValue) As Boolean
            Dim i = FindEntry(key)
            If i >= 0 Then
                value = _values(i)
                Return True
            End If
            value = Nothing
            Return False
        End Function

        Public Function Remove(key As TKey) As Boolean
            Dim ko As Object = key
            If ko Is Nothing Then Throw New ArgumentNullException("key")
            Dim h = HashOf(ko)
            Dim b = h Mod _buckets.Length
            Dim prev = -1
            Dim i = _buckets(b) - 1
            While i >= 0
                If _hashes(i) = h AndAlso AreEqual(_keys(i), ko) Then
                    If prev < 0 Then _buckets(b) = _nexts(i) + 1 Else _nexts(prev) = _nexts(i)
                    _hashes(i) = -1
                    _keys(i) = Nothing
                    _values(i) = Nothing
                    _nexts(i) = _freeList
                    _freeList = i
                    _freeCount += 1
                    _version += 1
                    Return True
                End If
                prev = i
                i = _nexts(i)
            End While
            Return False
        End Function

        Public Sub Clear()
            Init(0)
            _count = 0
            _freeList = -1
            _freeCount = 0
            _version += 1
        End Sub

        Public ReadOnly Property Keys As List(Of TKey)
            Get
                Dim r As New List(Of TKey)(Count)
                For i = 0 To _count - 1
                    If _hashes(i) >= 0 Then r.Add(_keys(i))
                Next
                Return r
            End Get
        End Property

        Public ReadOnly Property Values As List(Of TValue)
            Get
                Dim r As New List(Of TValue)(Count)
                For i = 0 To _count - 1
                    If _hashes(i) >= 0 Then r.Add(_values(i))
                Next
                Return r
            End Get
        End Property

        Public Function GetEnumerator() As Enumerator
            Return New Enumerator(Me)
        End Function

        Private Function GetEnumerator1() As IEnumerator(Of KeyValuePair(Of TKey, TValue)) Implements IEnumerable(Of KeyValuePair(Of TKey, TValue)).GetEnumerator
            Return New Enumerator(Me)
        End Function

        Private Function GetEnumerator2() As System.Collections.IEnumerator Implements System.Collections.IEnumerable.GetEnumerator
            Return New Enumerator(Me)
        End Function

        Public Class Enumerator
            Implements IEnumerator(Of KeyValuePair(Of TKey, TValue))
            Private ReadOnly _d As Dictionary(Of TKey, TValue)
            Private ReadOnly _version As Integer
            Private _index As Integer = -1

            Friend Sub New(d As Dictionary(Of TKey, TValue))
                _d = d
                _version = d._version
            End Sub

            Public Function MoveNext() As Boolean Implements System.Collections.IEnumerator.MoveNext
                CheckVersion(_version, _d._version)
                Do
                    _index += 1
                Loop While _index < _d._count AndAlso _d._hashes(_index) < 0
                Return _index < _d._count
            End Function

            Public ReadOnly Property Current As KeyValuePair(Of TKey, TValue) Implements IEnumerator(Of KeyValuePair(Of TKey, TValue)).Current
                Get
                    Return New KeyValuePair(Of TKey, TValue)(_d._keys(_index), _d._values(_index))
                End Get
            End Property

            Private ReadOnly Property Current1 As Object Implements System.Collections.IEnumerator.Current
                Get
                    Return Current
                End Get
            End Property

            Public Sub Reset() Implements System.Collections.IEnumerator.Reset
                _index = -1
            End Sub

            Public Sub Dispose() Implements IDisposable.Dispose
            End Sub
        End Class
    End Class

    ' ---------------------------------------------------------------- HashSet

    Public Class HashSet(Of T)
        Implements IEnumerable(Of T)
        Private ReadOnly _d As New Dictionary(Of T, Boolean)

        Public Sub New()
        End Sub

        Public Sub New(collection As IEnumerable(Of T))
            UnionWith(collection)
        End Sub

        Public ReadOnly Property Count As Integer
            Get
                Return _d.Count
            End Get
        End Property

        Public Function Add(item As T) As Boolean
            Return _d.TryAdd(item, True)
        End Function

        Public Function Contains(item As T) As Boolean
            Return _d.ContainsKey(item)
        End Function

        Public Function Remove(item As T) As Boolean
            Return _d.Remove(item)
        End Function

        Public Sub Clear()
            _d.Clear()
        End Sub

        Public Sub UnionWith(other As IEnumerable(Of T))
            For Each x In other
                _d.TryAdd(x, True)
            Next
        End Sub

        Public Sub IntersectWith(other As IEnumerable(Of T))
            Dim keep As New HashSet(Of T)(other)
            For Each x In _d.Keys
                If Not keep.Contains(x) Then _d.Remove(x)
            Next
        End Sub

        Public Sub ExceptWith(other As IEnumerable(Of T))
            For Each x In other
                _d.Remove(x)
            Next
        End Sub

        Public Function GetEnumerator() As IEnumerator(Of T) Implements IEnumerable(Of T).GetEnumerator
            Return _d.Keys.GetEnumerator()
        End Function

        Private Function GetEnumerator2() As System.Collections.IEnumerator Implements System.Collections.IEnumerable.GetEnumerator
            Return GetEnumerator()
        End Function
    End Class

    ' ---------------------------------------------------------------- Stack, Queue

    Public Class Stack(Of T)
        Implements IEnumerable(Of T)
        Private ReadOnly _list As New List(Of T)

        Public ReadOnly Property Count As Integer
            Get
                Return _list.Count
            End Get
        End Property

        Public Sub Push(item As T)
            _list.Add(item)
        End Sub

        Public Function Pop() As T
            If _list.Count = 0 Then Throw New InvalidOperationException("Stack empty.")
            Dim x = _list(_list.Count - 1)
            _list.RemoveAt(_list.Count - 1)
            Return x
        End Function

        Public Function Peek() As T
            If _list.Count = 0 Then Throw New InvalidOperationException("Stack empty.")
            Return _list(_list.Count - 1)
        End Function

        Public Function Contains(item As T) As Boolean
            Return _list.Contains(item)
        End Function

        Public Sub Clear()
            _list.Clear()
        End Sub

        ' from the top of the stack down, as in .NET
        Public Function ToArray() As T()
            Dim a = _list.ToArray()
            System.Array.Reverse(a)
            Return a
        End Function

        Public Function GetEnumerator() As IEnumerator(Of T) Implements IEnumerable(Of T).GetEnumerator
            Return New List(Of T)(ToArray()).GetEnumerator()
        End Function

        Private Function GetEnumerator2() As System.Collections.IEnumerator Implements System.Collections.IEnumerable.GetEnumerator
            Return GetEnumerator()
        End Function
    End Class

    Public Class Queue(Of T)
        Implements IEnumerable(Of T)
        Private _items(3) As T
        Private _head As Integer
        Private _size As Integer

        Public ReadOnly Property Count As Integer
            Get
                Return _size
            End Get
        End Property

        Public Sub Enqueue(item As T)
            If _size = _items.Length Then
                Dim a(2 * _items.Length - 1) As T
                For i = 0 To _size - 1
                    a(i) = _items((_head + i) Mod _items.Length)
                Next
                _items = a
                _head = 0
            End If
            _items((_head + _size) Mod _items.Length) = item
            _size += 1
        End Sub

        Public Function Dequeue() As T
            If _size = 0 Then Throw New InvalidOperationException("Queue empty.")
            Dim x = _items(_head)
            _items(_head) = Nothing
            _head = (_head + 1) Mod _items.Length
            _size -= 1
            Return x
        End Function

        Public Function Peek() As T
            If _size = 0 Then Throw New InvalidOperationException("Queue empty.")
            Return _items(_head)
        End Function

        Public Function Contains(item As T) As Boolean
            For i = 0 To _size - 1
                If AreEqual(_items((_head + i) Mod _items.Length), item) Then Return True
            Next
            Return False
        End Function

        Public Sub Clear()
            ReDim _items(3)
            _head = 0
            _size = 0
        End Sub

        Public Function ToArray() As T()
            Dim a(_size - 1) As T
            For i = 0 To _size - 1
                a(i) = _items((_head + i) Mod _items.Length)
            Next
            Return a
        End Function

        Public Function GetEnumerator() As IEnumerator(Of T) Implements IEnumerable(Of T).GetEnumerator
            Return New List(Of T)(ToArray()).GetEnumerator()
        End Function

        Private Function GetEnumerator2() As System.Collections.IEnumerator Implements System.Collections.IEnumerable.GetEnumerator
            Return GetEnumerator()
        End Function
    End Class
End Namespace

Namespace System
    ' ---------------------------------------------------------------- Array

    Public NotInheritable Class Array
        Private Sub New()
        End Sub

        Public Shared Sub Sort(Of T)(array() As T)
            If array Is Nothing Then Throw New ArgumentNullException("array")
            System.Collections.Generic.CollectionHelpers.SortRange(array, 0, array.Length)
        End Sub

        Public Shared Sub Sort(Of T)(array() As T, index As Integer, length As Integer)
            If index < 0 OrElse length < 0 OrElse index + length > array.Length Then
                Throw New ArgumentException("Offset and length were out of bounds for the array or count is greater than the number of elements from index to the end of the source collection.")
            End If
            System.Collections.Generic.CollectionHelpers.SortRange(array, index, length)
        End Sub

        Public Shared Sub Sort(Of T)(array() As T, comparison As Comparison(Of T))
            System.Collections.Generic.CollectionHelpers.SortWith(array, 0, array.Length, comparison)
        End Sub

        Public Shared Function Find(Of T)(array() As T, match As Predicate(Of T)) As T
            For Each x In array
                If match(x) Then Return x
            Next
            Return Nothing
        End Function

        Public Shared Function FindAll(Of T)(array() As T, match As Predicate(Of T)) As T()
            Dim r As New System.Collections.Generic.List(Of T)
            For Each x In array
                If match(x) Then r.Add(x)
            Next
            Return r.ToArray()
        End Function

        Public Shared Function FindIndex(Of T)(array() As T, match As Predicate(Of T)) As Integer
            For i = 0 To array.Length - 1
                If match(array(i)) Then Return i
            Next
            Return -1
        End Function

        Public Shared Function Exists(Of T)(array() As T, match As Predicate(Of T)) As Boolean
            Return FindIndex(array, match) >= 0
        End Function

        Public Shared Function TrueForAll(Of T)(array() As T, match As Predicate(Of T)) As Boolean
            For Each x In array
                If Not match(x) Then Return False
            Next
            Return True
        End Function

        Public Shared Sub ForEach(Of T)(array() As T, action As Action(Of T))
            For Each x In array
                action(x)
            Next
        End Sub

        Public Shared Function ConvertAll(Of TInput, TOutput)(array() As TInput, converter As Converter(Of TInput, TOutput)) As TOutput()
            Dim r(array.Length - 1) As TOutput
            For i = 0 To array.Length - 1
                r(i) = converter(array(i))
            Next
            Return r
        End Function

        Public Shared Sub Reverse(Of T)(array() As T)
            Dim i = 0, j = array.Length - 1
            While i < j
                Dim x = array(i)
                array(i) = array(j)
                array(j) = x
                i += 1
                j -= 1
            End While
        End Sub

        Public Shared Function IndexOf(Of T)(array() As T, value As T) As Integer
            For i = 0 To array.Length - 1
                If System.Collections.Generic.CollectionHelpers.AreEqual(array(i), value) Then Return i
            Next
            Return -1
        End Function

        Public Shared Function LastIndexOf(Of T)(array() As T, value As T) As Integer
            For i = array.Length - 1 To 0 Step -1
                If System.Collections.Generic.CollectionHelpers.AreEqual(array(i), value) Then Return i
            Next
            Return -1
        End Function

        Public Shared Function BinarySearch(Of T)(array() As T, value As T) As Integer
            Dim lo = 0, hi = array.Length - 1
            While lo <= hi
                Dim m = lo + (hi - lo) \ 2
                Dim c = System.Collections.Generic.CollectionHelpers.CompareOf(array(m), value)
                If c = 0 Then Return m
                If c < 0 Then lo = m + 1 Else hi = m - 1
            End While
            Return Not lo
        End Function

        Public Shared Sub Resize(Of T)(ByRef array() As T, newSize As Integer)
            If newSize < 0 Then Throw New ArgumentOutOfRangeException("newSize", "Non-negative number required.")
            If array Is Nothing Then
                ReDim array(newSize - 1)
            Else
                ReDim Preserve array(newSize - 1)
            End If
        End Sub

        Public Shared Sub Copy(Of T)(sourceArray() As T, destinationArray() As T, length As Integer)
            For i = 0 To length - 1
                destinationArray(i) = sourceArray(i)
            Next
        End Sub

        Public Shared Sub Copy(Of T)(sourceArray() As T, sourceIndex As Integer, destinationArray() As T, destinationIndex As Integer, length As Integer)
            If sourceIndex < destinationIndex Then
                For i = length - 1 To 0 Step -1
                    destinationArray(destinationIndex + i) = sourceArray(sourceIndex + i)
                Next
            Else
                For i = 0 To length - 1
                    destinationArray(destinationIndex + i) = sourceArray(sourceIndex + i)
                Next
            End If
        End Sub

        Public Shared Sub Clear(Of T)(array() As T, index As Integer, length As Integer)
            For i = index To index + length - 1
                array(i) = Nothing
            Next
        End Sub
    End Class

    ' ---------------------------------------------------------------- String

    ' Shared members of String written in VB: the compiler adds them to the
    ' built-in members of String (a class <Type>_Members of System extends
    ' the built-in type <Type>).
    Friend NotInheritable Class String_Members
        Public Shared Function Join(Of T)(separator As String, values As System.Collections.Generic.IEnumerable(Of T)) As String
            Dim sb As New System.Text.StringBuilder()
            Dim first = True
            For Each v In values
                If Not first Then sb.Append(separator)
                first = False
                Dim o As Object = v
                If o IsNot Nothing Then sb.Append(o.ToString())
            Next
            Return sb.ToString()
        End Function

        Public Shared Function Concat(Of T)(values As System.Collections.Generic.IEnumerable(Of T)) As String
            Return Join("", values)
        End Function
    End Class

    ' ---------------------------------------------------------------- Tuple

    Public Class Tuple(Of T1, T2)
        Public ReadOnly Property Item1 As T1
        Public ReadOnly Property Item2 As T2
        Public Sub New(item1 As T1, item2 As T2)
            _Item1 = item1
            _Item2 = item2
        End Sub
        Public Overrides Function ToString() As String
            Return "(" & System.Collections.Generic.CollectionHelpers.KeyText(Item1) & ", " &
                   System.Collections.Generic.CollectionHelpers.KeyText(Item2) & ")"
        End Function
    End Class

    Public Class Tuple(Of T1, T2, T3)
        Public ReadOnly Property Item1 As T1
        Public ReadOnly Property Item2 As T2
        Public ReadOnly Property Item3 As T3
        Public Sub New(item1 As T1, item2 As T2, item3 As T3)
            _Item1 = item1
            _Item2 = item2
            _Item3 = item3
        End Sub
        Public Overrides Function ToString() As String
            Return "(" & System.Collections.Generic.CollectionHelpers.KeyText(Item1) & ", " &
                   System.Collections.Generic.CollectionHelpers.KeyText(Item2) & ", " &
                   System.Collections.Generic.CollectionHelpers.KeyText(Item3) & ")"
        End Function
    End Class

    Public NotInheritable Class Tuple
        Private Sub New()
        End Sub
        Public Shared Function Create(Of T1, T2)(item1 As T1, item2 As T2) As Tuple(Of T1, T2)
            Return New Tuple(Of T1, T2)(item1, item2)
        End Function
        Public Shared Function Create(Of T1, T2, T3)(item1 As T1, item2 As T2, item3 As T3) As Tuple(Of T1, T2, T3)
            Return New Tuple(Of T1, T2, T3)(item1, item2, item3)
        End Function
    End Class

    ' ---------------------------------------------------------------- Random

    ' The subtractive generator of .NET (Knuth), so that New Random(seed) gives
    ' the same numbers as in .NET.
    Public Class Random
        Private Const MBIG As Integer = Integer.MaxValue
        Private Const MSEED As Integer = 161803398
        Private ReadOnly _seedArray(55) As Integer
        Private _inext As Integer
        Private _inextp As Integer

        Public Sub New()
            Me.New(CInt(Microsoft.VisualBasic.CompilerServices.Utils.TickSeed()))
        End Sub

        Public Sub New(seed As Integer)
            ' (in .NET this arithmetic is unchecked Integer arithmetic: Long here)
            Dim subtraction As Long = If(seed = Integer.MinValue, Integer.MaxValue, Math.Abs(seed))
            Dim mj As Long = MSEED - subtraction
            _seedArray(55) = CInt(mj)
            Dim mk As Long = 1
            For i = 1 To 54
                Dim ii = (21 * i) Mod 55
                _seedArray(ii) = CInt(mk)
                mk = Wrap32(mj - mk)
                If mk < 0 Then mk += MBIG
                mj = _seedArray(ii)
            Next
            For k = 1 To 4
                For i = 1 To 55
                    Dim v = Wrap32(CLng(_seedArray(i)) - _seedArray(1 + (i + 30) Mod 55))
                    If v < 0 Then v += MBIG
                    _seedArray(i) = CInt(v)
                Next
            Next
            _inext = 0
            _inextp = 21
        End Sub

        ' the value of a 32-bit two's complement result (unchecked arithmetic)
        Private Shared Function Wrap32(v As Long) As Long
            v = v And &HFFFFFFFFL
            If v > Integer.MaxValue Then v -= &H100000000L
            Return v
        End Function

        Private Function InternalSample() As Integer
            Dim locINext = _inext + 1
            If locINext >= 56 Then locINext = 1
            Dim locINextp = _inextp + 1
            If locINextp >= 56 Then locINextp = 1
            Dim retVal = CLng(_seedArray(locINext)) - _seedArray(locINextp)
            If retVal = MBIG Then retVal -= 1
            If retVal < 0 Then retVal += MBIG
            _seedArray(locINext) = CInt(retVal)
            _inext = locINext
            _inextp = locINextp
            Return CInt(retVal)
        End Function

        Protected Overridable Function Sample() As Double
            Return InternalSample() * (1.0 / MBIG)
        End Function

        Public Overridable Function [Next]() As Integer
            Return InternalSample()
        End Function

        Public Overridable Function [Next](maxValue As Integer) As Integer
            If maxValue < 0 Then Throw New ArgumentOutOfRangeException("maxValue", "'maxValue' must be greater than zero.")
            Return CInt(Int(Sample() * maxValue))
        End Function

        Public Overridable Function [Next](minValue As Integer, maxValue As Integer) As Integer
            If minValue > maxValue Then
                Throw New ArgumentOutOfRangeException("minValue", "'minValue' cannot be greater than maxValue.")
            End If
            Dim range = CLng(maxValue) - minValue
            If range <= Integer.MaxValue Then Return CInt(Int(Sample() * range)) + minValue
            Return CInt(CLng(Int(LargeSample() * range)) + minValue)
        End Function

        Private Function LargeSample() As Double
            Dim result = InternalSample()
            If InternalSample() Mod 2 = 0 Then result = -result
            Dim d As Double = result
            d += Integer.MaxValue - 1
            d /= 2.0 * Integer.MaxValue - 1
            Return d
        End Function

        Public Overridable Function NextDouble() As Double
            Return Sample()
        End Function
    End Class
End Namespace

Namespace System.Text
    ' ---------------------------------------------------------------- StringBuilder

    Public NotInheritable Class StringBuilder
        Private _chars() As Char
        Private _length As Integer

        Public Sub New()
            ReDim _chars(15)
        End Sub

        Public Sub New(capacity As Integer)
            ReDim _chars(If(capacity < 1, 0, capacity - 1))
        End Sub

        Public Sub New(value As String)
            ReDim _chars(15)
            Append(value)
        End Sub

        Private Sub Ensure(n As Integer)
            If n <= _chars.Length Then Return
            Dim cap = _chars.Length * 2
            If cap < n Then cap = n
            ReDim Preserve _chars(cap - 1)
        End Sub

        Public Property Length As Integer
            Get
                Return _length
            End Get
            Set(value As Integer)
                If value < 0 Then Throw New ArgumentOutOfRangeException("value", "Length cannot be less than zero.")
                Ensure(value)
                For i = _length To value - 1
                    _chars(i) = ChrW(0)
                Next
                _length = value
            End Set
        End Property

        Public ReadOnly Property Capacity As Integer
            Get
                Return _chars.Length
            End Get
        End Property

        Default Public Property Chars(index As Integer) As Char
            Get
                If index < 0 OrElse index >= _length Then Throw New IndexOutOfRangeException()
                Return _chars(index)
            End Get
            Set(value As Char)
                If index < 0 OrElse index >= _length Then Throw New ArgumentOutOfRangeException("index", "Index was out of range. Must be non-negative and less than the size of the collection.")
                _chars(index) = value
            End Set
        End Property

        Public Function Append(value As String) As StringBuilder
            If value Is Nothing Then Return Me
            Ensure(_length + value.Length)
            For i = 0 To value.Length - 1
                _chars(_length + i) = value(i)
            Next
            _length += value.Length
            Return Me
        End Function

        Public Function Append(value As Char) As StringBuilder
            Ensure(_length + 1)
            _chars(_length) = value
            _length += 1
            Return Me
        End Function

        Public Function Append(value As Char, repeatCount As Integer) As StringBuilder
            For i = 1 To repeatCount
                Append(value)
            Next
            Return Me
        End Function

        Public Function Append(value As Object) As StringBuilder
            If value Is Nothing Then Return Me
            Return Append(value.ToString())
        End Function

        Public Function AppendLine() As StringBuilder
            Return Append(Microsoft.VisualBasic.Constants.vbLf)
        End Function

        Public Function AppendLine(value As String) As StringBuilder
            Append(value)
            Return Append(Microsoft.VisualBasic.Constants.vbLf)
        End Function

        Public Function AppendFormat(format As String, ParamArray args() As Object) As StringBuilder
            Return Append(String.Format(format, args))
        End Function

        Public Function Insert(index As Integer, value As String) As StringBuilder
            If index < 0 OrElse index > _length Then Throw New ArgumentOutOfRangeException("index", "Index was out of range. Must be non-negative and less than or equal to the size of the collection.")
            Dim s = ToString()
            Clear()
            Append(s.Substring(0, index))
            Append(value)
            Return Append(s.Substring(index))
        End Function

        Public Function Remove(startIndex As Integer, length As Integer) As StringBuilder
            If startIndex < 0 OrElse length < 0 OrElse startIndex + length > _length Then
                Throw New ArgumentOutOfRangeException("length", "Index was out of range. Must be non-negative and less than the size of the collection.")
            End If
            For i = startIndex To _length - length - 1
                _chars(i) = _chars(i + length)
            Next
            _length -= length
            Return Me
        End Function

        Public Function Replace(oldValue As String, newValue As String) As StringBuilder
            Dim s = ToString().Replace(oldValue, If(newValue, ""))
            Clear()
            Return Append(s)
        End Function

        Public Function Clear() As StringBuilder
            _length = 0
            Return Me
        End Function

        Public Overrides Function ToString() As String
            Return Microsoft.VisualBasic.CompilerServices.Utils.CharsToString(_chars, 0, _length)
        End Function
    End Class
End Namespace

Namespace System.IO
    ' ---------------------------------------------------------------- files

    Public NotInheritable Class File
        Private Sub New()
        End Sub

        Public Shared Function ReadAllText(path As String) As String
            Return Microsoft.VisualBasic.CompilerServices.Utils.FileReadAll(path)
        End Function

        Public Shared Function ReadAllLines(path As String) As String()
            Dim lines As New System.Collections.Generic.List(Of String)
            Using r As New StreamReader(path)
                Dim line = r.ReadLine()
                While line IsNot Nothing
                    lines.Add(line)
                    line = r.ReadLine()
                End While
            End Using
            Return lines.ToArray()
        End Function

        Public Shared Sub WriteAllText(path As String, contents As String)
            Microsoft.VisualBasic.CompilerServices.Utils.FileWriteAll(path, contents, False)
        End Sub

        Public Shared Sub AppendAllText(path As String, contents As String)
            Microsoft.VisualBasic.CompilerServices.Utils.FileWriteAll(path, contents, True)
        End Sub

        Public Shared Sub WriteAllLines(path As String, contents As System.Collections.Generic.IEnumerable(Of String))
            Using w As New StreamWriter(path)
                For Each line In contents
                    w.WriteLine(line)
                Next
            End Using
        End Sub

        Public Shared Function Exists(path As String) As Boolean
            Return Microsoft.VisualBasic.CompilerServices.Utils.FileExists(path)
        End Function

        Public Shared Sub Delete(path As String)
            Microsoft.VisualBasic.CompilerServices.Utils.FileDelete(path)
        End Sub
    End Class

    Public Class StreamReader
        Implements IDisposable
        Private _f As Long

        Public Sub New(path As String)
            _f = Microsoft.VisualBasic.CompilerServices.Utils.FileOpen(path, 0)
        End Sub

        Public Function ReadLine() As String
            Return Microsoft.VisualBasic.CompilerServices.Utils.FileReadLine(_f)
        End Function

        Public Function ReadToEnd() As String
            Return Microsoft.VisualBasic.CompilerServices.Utils.FileReadToEnd(_f)
        End Function

        Public Function Read() As Integer
            Return Microsoft.VisualBasic.CompilerServices.Utils.FileRead(_f)
        End Function

        Public Function Peek() As Integer
            Return Microsoft.VisualBasic.CompilerServices.Utils.FilePeek(_f)
        End Function

        Public ReadOnly Property EndOfStream As Boolean
            Get
                Return Peek() < 0
            End Get
        End Property

        Public Sub Close()
            Microsoft.VisualBasic.CompilerServices.Utils.FileClose(_f)
            _f = 0
        End Sub

        Public Sub Dispose() Implements IDisposable.Dispose
            Close()
        End Sub
    End Class

    Public Class StreamWriter
        Implements IDisposable
        Private _f As Long

        Public Sub New(path As String)
            _f = Microsoft.VisualBasic.CompilerServices.Utils.FileOpen(path, 1)
        End Sub

        Public Sub New(path As String, append As Boolean)
            _f = Microsoft.VisualBasic.CompilerServices.Utils.FileOpen(path, If(append, 2, 1))
        End Sub

        Public Sub Write(value As String)
            Microsoft.VisualBasic.CompilerServices.Utils.FileWrite(_f, value)
        End Sub

        Public Sub Write(value As Object)
            If value IsNot Nothing Then Write(value.ToString())
        End Sub

        Public Sub WriteLine()
            Write(Microsoft.VisualBasic.Constants.vbLf)
        End Sub

        Public Sub WriteLine(value As String)
            Write(value)
            WriteLine()
        End Sub

        Public Sub WriteLine(value As Object)
            Write(value)
            WriteLine()
        End Sub

        Public Sub Flush()
        End Sub

        Public Sub Close()
            Microsoft.VisualBasic.CompilerServices.Utils.FileClose(_f)
            _f = 0
        End Sub

        Public Sub Dispose() Implements IDisposable.Dispose
            Close()
        End Sub
    End Class
End Namespace

Namespace System
    ' ---------------------------------------------------------------- Nullable

    ' T? : a value of T or no value.  The compiler converts T to T? (and
    ' Nothing to an empty T?), T? to T (through Value), and compares T? with
    ' Nothing (Is / IsNot).
    Public Structure Nullable(Of T As Structure)
        Private ReadOnly _hasValue As Boolean
        Private ReadOnly _value As T

        Public Sub New(value As T)
            _value = value
            _hasValue = True
        End Sub

        Public ReadOnly Property HasValue As Boolean
            Get
                Return _hasValue
            End Get
        End Property

        Public ReadOnly Property Value As T
            Get
                If Not _hasValue Then Throw New InvalidOperationException("Nullable object must have a value.")
                Return _value
            End Get
        End Property

        Public Function GetValueOrDefault() As T
            Return _value
        End Function

        Public Function GetValueOrDefault(defaultValue As T) As T
            Return If(_hasValue, _value, defaultValue)
        End Function

        Public Overrides Function ToString() As String
            If Not _hasValue Then Return ""
            Dim o As Object = _value
            Return o.ToString()
        End Function

        Public Overrides Function Equals(other As Object) As Boolean
            If Not _hasValue Then Return other Is Nothing
            If other Is Nothing Then Return False
            Dim o As Object = _value
            Return o.Equals(other)
        End Function

        Public Overrides Function GetHashCode() As Integer
            If Not _hasValue Then Return 0
            Dim o As Object = _value
            Return o.GetHashCode()
        End Function
    End Structure
End Namespace
