namespace std {
    typedef unsigned long size_t;
    template<class T> struct remove_reference { typedef T type; };
    template<class T> struct remove_reference<T &> { typedef T type; };
    template<class T> struct remove_reference<T &&> { typedef T type; };
    template<class T> typename remove_reference<T>::type &&move(T &&t) noexcept {
        return static_cast<typename remove_reference<T>::type &&>(t);
    }
    template<class T, T v> struct integral_constant { static constexpr T value = v; typedef T value_type; };
    typedef integral_constant<bool, true> true_type;
    typedef integral_constant<bool, false> false_type;
    template<bool B, class T = void> struct enable_if { };
    template<class T> struct enable_if<true, T> { typedef T type; };
    template<class T> struct is_pointer : false_type { };
    template<class T> struct is_pointer<T *> : true_type { };
    template<class It> struct iterator_traits {
        typedef typename It::value_type value_type;
        typedef typename It::difference_type difference_type;
    };
    template<class T> struct iterator_traits<T *> { typedef T value_type; typedef long difference_type; };

    template<class T> class allocator {
    public:
        typedef T value_type;
        T *allocate(size_t n) { return static_cast<T *>(::operator new(n * sizeof(T))); }
        void deallocate(T *p, size_t) { ::operator delete(p); }
    };

    template<class T, class A = allocator<T>> class vector {
    public:
        typedef T value_type;
        typedef T *iterator;
        typedef const T *const_iterator;
        typedef size_t size_type;
        vector() : b(nullptr), e(nullptr), c(nullptr) {}
        vector(size_type n, const T &v = T());
        vector(const vector &o);
        vector(vector &&o) noexcept : b(o.b), e(o.e), c(o.c) { o.b = o.e = o.c = nullptr; }
        ~vector() { clear(); }
        vector &operator=(vector o) { swap(o); return *this; }
        iterator begin() { return b; }
        iterator end() { return e; }
        const_iterator begin() const { return b; }
        size_type size() const { return static_cast<size_type>(e - b); }
        bool empty() const { return b == e; }
        T &operator[](size_type i) { return b[i]; }
        const T &operator[](size_type i) const { return b[i]; }
        void push_back(const T &v) { if (e == c) grow(); new (e) T(v); ++e; }
        void push_back(T &&v) { if (e == c) grow(); new (e) T(move(v)); ++e; }
        template<class... Args> void emplace_back(Args &&... args) { if (e == c) grow(); new (e) T(static_cast<Args &&>(args)...); ++e; }
        void clear() { while (e != b) (--e)->~T(); }
        void swap(vector &o) { T *t = b; b = o.b; o.b = t; }
    private:
        void grow();
        T *b, *e, *c;
        A alloc;
    };
    template<class T, class A> vector<T, A>::vector(size_type n, const T &v) : b(nullptr), e(nullptr), c(nullptr) {
        for (size_type i = 0; i < n; ++i) push_back(v);
    }
    template<class T, class A> void vector<T, A>::grow() {
        size_type n = size() ? size() * 2 : 4;
        T *nb = alloc.allocate(n);
        for (size_type i = 0; i < size(); ++i) new (nb + i) T(move(b[i]));
        b = nb; e = nb + n / 2; c = nb + n;
    }

    template<class It, class Pred> It find_if(It first, It last, Pred p) {
        for (; first != last; ++first) if (p(*first)) return first;
        return last;
    }
    template<class It, class Cmp> void sort(It first, It last, Cmp cmp) {
        for (It i = first; i != last; ++i)
            for (It j = i + 1; j != last; ++j)
                if (cmp(*j, *i)) { auto t = *i; *i = *j; *j = t; }
    }
    template<class T> struct less { bool operator()(const T &a, const T &b) const { return a < b; } };
    template<class A, class B> struct pair {
        A first; B second;
        pair() : first(), second() {}
        pair(const A &a, const B &b) : first(a), second(b) {}
    };
    template<class A, class B> pair<A, B> make_pair(A a, B b) { return pair<A, B>(a, b); }
}

int use_library() {
    std::vector<int> v(3, 1);
    v.push_back(4);
    std::vector<std::vector<int>> vv;
    vv.emplace_back(2, 5);
    std::sort(v.begin(), v.end(), std::less<int>());
    auto it = std::find_if(v.begin(), v.end(), [](int x) { return x > 2; });
    std::pair<int, double> p = std::make_pair(1, 2.5);
    int total = 0;
    for (std::vector<int>::iterator i = v.begin(); i != v.end(); ++i) total += *i;
    for (int x : v) total += x;
    return total + (it != v.end()) + p.first + (std::is_pointer<int *>::value ? 1 : 0);
}
