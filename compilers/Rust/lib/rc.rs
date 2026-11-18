// lib/rc.rs: Rc, Weak, Cell, RefCell, Arc, Mutex (single threaded). Parsed as part of crate `core`.

// Interior mutability helper: reinterpret a shared reference as a unique one.
pub mod cell {
    use ::core::clone::Clone;
    use ::core::cmp::{Eq, Ord, Ordering, PartialEq, PartialOrd};
    use ::core::convert::From;
    use ::core::default::Default;
    use ::core::fmt::{Debug, Display, Formatter};
    use ::core::intrinsics::{forget, read_ref, transmute, write_ref};
    use ::core::marker::Copy;
    use ::core::ops::{Deref, DerefMut, Drop};
    use ::core::option::Option::{self, Some, None};
    use ::core::result::Result::{self, Ok, Err};

    pub fn as_mut<'a, T>(r: &'a T) -> &'a mut T { transmute::<&'a T, &'a mut T>(r) }

    pub struct Cell<T> { value: T }

    impl<T> Cell<T> {
        pub fn new(value: T) -> Cell<T> { Cell { value: value } }
        pub fn replace(&self, val: T) -> T {
            let slot = as_mut::<T>(&self.value);
            let old = read_ref::<T>(slot);
            write_ref::<T>(slot, val);
            old
        }
        pub fn set(&self, val: T) {
            let _old = self.replace(val);
        }
        pub fn into_inner(self) -> T {
            let v = read_ref::<T>(&self.value);
            forget::<Cell<T>>(self);
            v
        }
        pub fn get_mut(&mut self) -> &mut T { &mut self.value }
        pub fn swap(&self, other: &Cell<T>) {
            let a = read_ref::<T>(&self.value);
            let b = read_ref::<T>(&other.value);
            write_ref::<T>(as_mut::<T>(&self.value), b);
            write_ref::<T>(as_mut::<T>(&other.value), a);
        }
    }
    impl<T: Copy> Cell<T> {
        pub fn get(&self) -> T { read_ref::<T>(&self.value) }
        pub fn update<F: ::core::ops::FnOnce(T) -> T>(&self, f: F) {
            let v = f(self.get());
            self.set(v);
        }
    }
    impl<T: Default> Cell<T> {
        pub fn take(&self) -> T { self.replace(T::default()) }
    }
    impl<T: Copy> Clone for Cell<T> {
        fn clone(&self) -> Cell<T> { Cell::new(self.get()) }
    }
    impl<T: Default> Default for Cell<T> {
        fn default() -> Cell<T> { Cell::new(T::default()) }
    }
    impl<T> From<T> for Cell<T> {
        fn from(t: T) -> Cell<T> { Cell::new(t) }
    }
    impl<T: Copy + PartialEq> PartialEq for Cell<T> {
        fn eq(&self, o: &Cell<T>) -> bool { self.get() == o.get() }
    }
    impl<T: Copy + Eq> Eq for Cell<T> {}
    impl<T: Copy + PartialOrd> PartialOrd for Cell<T> {
        fn partial_cmp(&self, o: &Cell<T>) -> Option<Ordering> { self.get().partial_cmp(&o.get()) }
    }
    impl<T: Copy + Ord> Ord for Cell<T> {
        fn cmp(&self, o: &Cell<T>) -> Ordering { self.get().cmp(&o.get()) }
    }
    impl<T: Copy + Debug> Debug for Cell<T> {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result {
            let v = self.get();
            f.debug_struct("Cell").field("value", &v).finish()
        }
    }

    pub struct BorrowError { _p: () }
    pub struct BorrowMutError { _p: () }
    impl Debug for BorrowError {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { f.write_str("BorrowError") }
    }
    impl Display for BorrowError {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { f.write_str("RefCell already mutably borrowed") }
    }
    impl Debug for BorrowMutError {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { f.write_str("BorrowMutError") }
    }
    impl Display for BorrowMutError {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { f.write_str("RefCell already borrowed") }
    }
    impl ::core::error::Error for BorrowError {}
    impl ::core::error::Error for BorrowMutError {}

    // flag: 0 unused, n > 0 n shared borrows, -1 one mutable borrow
    pub struct RefCell<T> { flag: isize, value: T }

    pub struct Ref<'b, T> { cell: &'b RefCell<T> }
    pub struct RefMut<'b, T> { cell: &'b RefCell<T> }

    impl<T> RefCell<T> {
        pub fn new(value: T) -> RefCell<T> { RefCell { flag: 0, value: value } }
        pub fn try_borrow(&self) -> Result<Ref<'_, T>, BorrowError> {
            if self.flag < 0 { return Err(BorrowError { _p: () }); }
            as_mut::<RefCell<T>>(self).flag += 1;
            Ok(Ref { cell: self })
        }
        pub fn try_borrow_mut(&self) -> Result<RefMut<'_, T>, BorrowMutError> {
            if self.flag != 0 { return Err(BorrowMutError { _p: () }); }
            as_mut::<RefCell<T>>(self).flag = -1;
            Ok(RefMut { cell: self })
        }
        pub fn borrow(&self) -> Ref<'_, T> {
            if self.flag < 0 {
                ::core::panicking::panic_str("library/core/src/cell.rs", 1, 1, "RefCell already mutably borrowed");
            }
            as_mut::<RefCell<T>>(self).flag += 1;
            Ref { cell: self }
        }
        pub fn borrow_mut(&self) -> RefMut<'_, T> {
            if self.flag != 0 {
                ::core::panicking::panic_str("library/core/src/cell.rs", 1, 1, "RefCell already borrowed");
            }
            as_mut::<RefCell<T>>(self).flag = -1;
            RefMut { cell: self }
        }
        pub fn replace(&self, t: T) -> T {
            let mut g = self.borrow_mut();
            ::core::mem::replace::<T>(&mut *g, t)
        }
        pub fn into_inner(self) -> T {
            let v = read_ref::<T>(&self.value);
            forget::<RefCell<T>>(self);
            v
        }
        pub fn get_mut(&mut self) -> &mut T { &mut self.value }
        pub fn swap(&self, other: &RefCell<T>) {
            let mut a = self.borrow_mut();
            let mut b = other.borrow_mut();
            ::core::mem::swap::<T>(&mut *a, &mut *b);
        }
    }
    impl<T: Default> RefCell<T> {
        pub fn take(&self) -> T { self.replace(T::default()) }
    }
    impl<T: Clone> Clone for RefCell<T> {
        fn clone(&self) -> RefCell<T> { RefCell::new((*self.borrow()).clone()) }
    }
    impl<T: Default> Default for RefCell<T> {
        fn default() -> RefCell<T> { RefCell::new(T::default()) }
    }
    impl<T> From<T> for RefCell<T> {
        fn from(t: T) -> RefCell<T> { RefCell::new(t) }
    }
    impl<T: PartialEq> PartialEq for RefCell<T> {
        fn eq(&self, o: &RefCell<T>) -> bool { *self.borrow() == *o.borrow() }
    }
    impl<T: Eq> Eq for RefCell<T> {}
    impl<T: PartialOrd> PartialOrd for RefCell<T> {
        fn partial_cmp(&self, o: &RefCell<T>) -> Option<Ordering> { (*self.borrow()).partial_cmp(&*o.borrow()) }
    }
    impl<T: Ord> Ord for RefCell<T> {
        fn cmp(&self, o: &RefCell<T>) -> Ordering { (*self.borrow()).cmp(&*o.borrow()) }
    }
    struct BorrowedPlaceholder;
    impl Debug for BorrowedPlaceholder {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { f.write_str("<borrowed>") }
    }
    impl<T: Debug> Debug for RefCell<T> {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result {
            if self.flag < 0 {
                f.debug_struct("RefCell").field("value", &BorrowedPlaceholder).finish()
            } else {
                let g = self.borrow();
                f.debug_struct("RefCell").field("value", &*g).finish()
            }
        }
    }

    impl<'b, T> Ref<'b, T> {
        pub fn clone(orig: &Ref<'b, T>) -> Ref<'b, T> {
            as_mut::<RefCell<T>>(orig.cell).flag += 1;
            Ref { cell: orig.cell }
        }
    }
    impl<'b, T> Deref for Ref<'b, T> {
        type Target = T;
        fn deref(&self) -> &T { &self.cell.value }
    }
    impl<'b, T> Drop for Ref<'b, T> {
        fn drop(&mut self) { as_mut::<RefCell<T>>(self.cell).flag -= 1; }
    }
    impl<'b, T: Debug> Debug for Ref<'b, T> {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { Debug::fmt(&self.cell.value, f) }
    }
    impl<'b, T: Display> Display for Ref<'b, T> {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { Display::fmt(&self.cell.value, f) }
    }
    impl<'b, T> Deref for RefMut<'b, T> {
        type Target = T;
        fn deref(&self) -> &T { &self.cell.value }
    }
    impl<'b, T> DerefMut for RefMut<'b, T> {
        fn deref_mut(&mut self) -> &mut T { as_mut::<T>(&self.cell.value) }
    }
    impl<'b, T> Drop for RefMut<'b, T> {
        fn drop(&mut self) { as_mut::<RefCell<T>>(self.cell).flag = 0; }
    }
    impl<'b, T: Debug> Debug for RefMut<'b, T> {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { Debug::fmt(&self.cell.value, f) }
    }
    impl<'b, T: Display> Display for RefMut<'b, T> {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { Display::fmt(&self.cell.value, f) }
    }
}

pub mod rc {
    use ::core::clone::Clone;
    use ::core::cmp::{Eq, Ord, Ordering, PartialEq, PartialOrd};
    use ::core::convert::From;
    use ::core::default::Default;
    use ::core::fmt::{Debug, Display, Formatter};
    use ::core::intrinsics::{elem_alloc, elem_free, elem_read, elem_ref, elem_ref_mut, elem_write, forget, read_ref, write_ref};
    use ::core::marker::PhantomData;
    use ::core::ops::{Deref, Drop};
    use ::core::option::Option::{self, Some, None};
    use ::core::result::Result::{self, Ok, Err};

    // heap block: strong count, weak count, value (one RcBox<T> allocated with elem_alloc)
    struct RcBox<T> { strong: usize, weak: usize, value: T }

    fn blk<'a, T>(p: usize) -> &'a mut RcBox<T> { elem_ref_mut::<RcBox<T>>(p, 0) }

    pub struct Rc<T> { ptr: usize, _m: PhantomData<T> }
    pub struct Weak<T> { ptr: usize, _m: PhantomData<T> }

    impl<T> Rc<T> {
        pub fn new(value: T) -> Rc<T> {
            let p = elem_alloc::<RcBox<T>>(1);
            elem_write::<RcBox<T>>(p, 0, RcBox { strong: 1, weak: 0, value: value });
            Rc { ptr: p, _m: PhantomData }
        }
        pub fn strong_count(this: &Rc<T>) -> usize { blk::<T>(this.ptr).strong }
        pub fn weak_count(this: &Rc<T>) -> usize { blk::<T>(this.ptr).weak }
        pub fn ptr_eq(a: &Rc<T>, b: &Rc<T>) -> bool { a.ptr == b.ptr }
        pub fn get_mut(this: &mut Rc<T>) -> Option<&mut T> {
            let b = blk::<T>(this.ptr);
            if b.strong == 1 && b.weak == 0 { Some(&mut b.value) } else { None }
        }
        pub fn try_unwrap(this: Rc<T>) -> Result<T, Rc<T>> {
            let b = blk::<T>(this.ptr);
            if b.strong != 1 { return Err(this); }
            let v = read_ref::<T>(&b.value);
            b.strong = 0;
            if b.weak == 0 { elem_free::<RcBox<T>>(this.ptr); }
            forget::<Rc<T>>(this);
            Ok(v)
        }
        pub fn into_inner(this: Rc<T>) -> Option<T> {
            match Rc::try_unwrap(this) { Ok(v) => Some(v), Err(_r) => None }
        }
        pub fn downgrade(this: &Rc<T>) -> Weak<T> {
            blk::<T>(this.ptr).weak += 1;
            Weak { ptr: this.ptr, _m: PhantomData }
        }
    }
    impl<T: Clone> Rc<T> {
        pub fn make_mut(this: &mut Rc<T>) -> &mut T {
            let b = blk::<T>(this.ptr);
            if b.strong != 1 {
                let copy = Rc::new(b.value.clone());
                *this = copy;
            } else if b.weak != 0 {
                // move the value into a fresh block; the old block lives on for its Weak pointers
                let v = read_ref::<T>(&b.value);
                b.strong = 0;
                let fresh = Rc::new(v);
                write_ref::<Rc<T>>(this, fresh);
            }
            &mut blk::<T>(this.ptr).value
        }
        pub fn unwrap_or_clone(this: Rc<T>) -> T {
            if blk::<T>(this.ptr).strong == 1 {
                match Rc::try_unwrap(this) { Ok(v) => v, Err(r) => (*r).clone() }
            } else {
                (*this).clone()
            }
        }
    }
    impl<T> Clone for Rc<T> {
        fn clone(&self) -> Rc<T> {
            blk::<T>(self.ptr).strong += 1;
            Rc { ptr: self.ptr, _m: PhantomData }
        }
    }
    impl<T> Deref for Rc<T> {
        type Target = T;
        fn deref(&self) -> &T { &elem_ref::<RcBox<T>>(self.ptr, 0).value }
    }
    impl<T> Drop for Rc<T> {
        fn drop(&mut self) {
            let b = blk::<T>(self.ptr);
            b.strong -= 1;
            if b.strong == 0 {
                b.weak += 1; // implicit weak while the value is dropped
                {
                    let _v = read_ref::<T>(&b.value);
                }
                b.weak -= 1;
                if b.weak == 0 { elem_free::<RcBox<T>>(self.ptr); }
            }
        }
    }
    impl<T: Default> Default for Rc<T> {
        fn default() -> Rc<T> { Rc::new(T::default()) }
    }
    impl<T> From<T> for Rc<T> {
        fn from(t: T) -> Rc<T> { Rc::new(t) }
    }
    impl<T: Display> Display for Rc<T> {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { Display::fmt(&**self, f) }
    }
    impl<T: Debug> Debug for Rc<T> {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { Debug::fmt(&**self, f) }
    }
    impl<T: PartialEq> PartialEq for Rc<T> {
        fn eq(&self, o: &Rc<T>) -> bool { **self == **o }
    }
    impl<T: Eq> Eq for Rc<T> {}
    impl<T: PartialOrd> PartialOrd for Rc<T> {
        fn partial_cmp(&self, o: &Rc<T>) -> Option<Ordering> { (**self).partial_cmp(&**o) }
    }
    impl<T: Ord> Ord for Rc<T> {
        fn cmp(&self, o: &Rc<T>) -> Ordering { (**self).cmp(&**o) }
    }

    impl<T> Weak<T> {
        pub fn new() -> Weak<T> { Weak { ptr: 0, _m: PhantomData } }
        pub fn upgrade(&self) -> Option<Rc<T>> {
            if self.ptr == 0 { return None; }
            let b = blk::<T>(self.ptr);
            if b.strong == 0 { return None; }
            b.strong += 1;
            Some(Rc { ptr: self.ptr, _m: PhantomData })
        }
        pub fn strong_count(&self) -> usize {
            if self.ptr == 0 { 0 } else { blk::<T>(self.ptr).strong }
        }
        pub fn weak_count(&self) -> usize {
            if self.ptr == 0 { return 0; }
            let b = blk::<T>(self.ptr);
            if b.strong == 0 { 0 } else { b.weak }
        }
        pub fn ptr_eq(&self, o: &Weak<T>) -> bool { self.ptr == o.ptr }
    }
    impl<T> Clone for Weak<T> {
        fn clone(&self) -> Weak<T> {
            if self.ptr != 0 { blk::<T>(self.ptr).weak += 1; }
            Weak { ptr: self.ptr, _m: PhantomData }
        }
    }
    impl<T> Default for Weak<T> {
        fn default() -> Weak<T> { Weak::new() }
    }
    impl<T> Drop for Weak<T> {
        fn drop(&mut self) {
            if self.ptr != 0 {
                let b = blk::<T>(self.ptr);
                b.weak -= 1;
                if b.weak == 0 && b.strong == 0 { elem_free::<RcBox<T>>(self.ptr); }
            }
        }
    }
    impl<T> Debug for Weak<T> {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { f.write_str("(Weak)") }
    }
}

pub mod sync {
    pub use ::core::rc::Rc as Arc;
    pub use ::core::rc::Weak;

    use ::core::cell::as_mut;
    use ::core::clone::Clone;
    use ::core::default::Default;
    use ::core::fmt::{Debug, Display, Formatter};
    use ::core::intrinsics::{forget, read_ref};
    use ::core::ops::{Deref, DerefMut, Drop};
    use ::core::result::Result::{self, Ok, Err};

    pub struct PoisonError<T> { guard: T }
    impl<T> PoisonError<T> {
        pub fn into_inner(self) -> T { self.guard }
    }
    impl<T> Debug for PoisonError<T> {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { f.write_str("PoisonError { .. }") }
    }
    impl<T> Display for PoisonError<T> {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { f.write_str("poisoned lock: another task failed inside") }
    }
    pub type LockResult<G> = Result<G, PoisonError<G>>;

    pub struct Mutex<T> { locked: bool, value: T }
    pub struct MutexGuard<'a, T> { m: &'a Mutex<T> }

    impl<T> Mutex<T> {
        pub fn new(value: T) -> Mutex<T> { Mutex { locked: false, value: value } }
        pub fn lock(&self) -> Result<MutexGuard<'_, T>, PoisonError<MutexGuard<'_, T>>> {
            if self.locked {
                ::core::panicking::panic_str("library/std/src/sync/mutex.rs", 1, 1, "deadlock: Mutex already locked by this thread");
            }
            as_mut::<Mutex<T>>(self).locked = true;
            Ok(MutexGuard { m: self })
        }
        pub fn into_inner(self) -> Result<T, PoisonError<T>> {
            let v = read_ref::<T>(&self.value);
            forget::<Mutex<T>>(self);
            Ok(v)
        }
        pub fn get_mut(&mut self) -> Result<&mut T, PoisonError<&mut T>> { Ok(&mut self.value) }
    }
    impl<T: Default> Default for Mutex<T> {
        fn default() -> Mutex<T> { Mutex::new(T::default()) }
    }
    impl<'a, T> Deref for MutexGuard<'a, T> {
        type Target = T;
        fn deref(&self) -> &T { &self.m.value }
    }
    impl<'a, T> DerefMut for MutexGuard<'a, T> {
        fn deref_mut(&mut self) -> &mut T { as_mut::<T>(&self.m.value) }
    }
    impl<'a, T> Drop for MutexGuard<'a, T> {
        fn drop(&mut self) { as_mut::<Mutex<T>>(self.m).locked = false; }
    }
    impl<'a, T: Debug> Debug for MutexGuard<'a, T> {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { Debug::fmt(&self.m.value, f) }
    }
    impl<'a, T: Display> Display for MutexGuard<'a, T> {
        fn fmt(&self, f: &mut Formatter<'_>) -> ::core::fmt::Result { Display::fmt(&self.m.value, f) }
    }
}
