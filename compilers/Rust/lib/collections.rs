// lib/collections.rs: hashing and the std collections.
//
// Parsed as part of the crate `core` (after core.rs), so paths are absolute
// (`::core::...`) as in core.rs and no `unsafe` is written. Provides
// `std::hash`, `std::collections::{HashMap, HashSet, BTreeMap, BTreeSet,
// VecDeque, BinaryHeap}` and the submodules `hash_map`, `hash_set`,
// `btree_map`, `btree_set`, `vec_deque`, `binary_heap`.
//
// Differences from std: the maps and sets have no hasher parameter (they always
// use the deterministic DefaultHasher), so iteration order is deterministic but
// unspecified; `Borrow` lookups are done through the local trait `hash::KeyBorrow`.

// ---------------------------------------------------------------- hash

pub mod hash {
    use ::core::boxed::Box;
    use ::core::option::Option::{self, Some, None};
    use ::core::string::String;
    use ::core::vec::Vec;

    pub trait Hasher {
        fn finish(&self) -> u64;
        fn write(&mut self, bytes: &[u8]);
        fn write_u8(&mut self, i: u8) { let b = [i]; let s: &[u8] = &b; self.write(s); }
        fn write_u16(&mut self, i: u16) { self.write_u64(i as u64); }
        fn write_u32(&mut self, i: u32) { self.write_u64(i as u64); }
        fn write_u64(&mut self, i: u64) {
            let b = [i as u8, (i >> 8) as u8, (i >> 16) as u8, (i >> 24) as u8,
                     (i >> 32) as u8, (i >> 40) as u8, (i >> 48) as u8, (i >> 56) as u8];
            let s: &[u8] = &b;
            self.write(s);
        }
        fn write_usize(&mut self, i: usize) { self.write_u64(i as u64); }
        fn write_i8(&mut self, i: i8) { self.write_u64(i as u64); }
        fn write_i16(&mut self, i: i16) { self.write_u64(i as u64); }
        fn write_i32(&mut self, i: i32) { self.write_u64(i as u64); }
        fn write_i64(&mut self, i: i64) { self.write_u64(i as u64); }
        fn write_isize(&mut self, i: isize) { self.write_u64(i as u64); }
    }

    pub trait Hash {
        fn hash<H: Hasher>(&self, state: &mut H);
    }

    pub trait BuildHasher {
        type Hasher: Hasher;
        fn build_hasher(&self) -> Self::Hasher;
        fn hash_one<T: Hash>(&self, x: T) -> u64 where Self: Sized {
            let mut h = self.build_hasher();
            x.hash(&mut h);
            h.finish()
        }
    }

    // Deterministic 64 bit hasher (rotate, xor, multiply per word; mixed at the end).
    pub struct DefaultHasher { state: u64 }
    impl DefaultHasher {
        pub fn new() -> DefaultHasher { DefaultHasher { state: 0x243f6a8885a308d3 } }
        fn add(&mut self, w: u64) {
            let s = self.state;
            self.state = ((s << 5) | (s >> 59)) ^ w;
            self.state = self.state.wrapping_mul(0x517cc1b727220a95);
        }
    }
    impl ::core::default::Default for DefaultHasher { fn default() -> DefaultHasher { DefaultHasher::new() } }
    impl ::core::clone::Clone for DefaultHasher { fn clone(&self) -> DefaultHasher { DefaultHasher { state: self.state } } }
    impl Hasher for DefaultHasher {
        fn finish(&self) -> u64 {
            let mut h = self.state;
            h = h ^ (h >> 32);
            h = h.wrapping_mul(0x2545f4914f6cdd1d);
            h = h ^ (h >> 29);
            h
        }
        fn write(&mut self, bytes: &[u8]) {
            let n = bytes.len();
            let mut i = 0usize;
            while i < n { self.add(bytes[i] as u64); i += 1; }
        }
        fn write_u8(&mut self, i: u8) { self.add(i as u64); }
        fn write_u16(&mut self, i: u16) { self.add(i as u64); }
        fn write_u32(&mut self, i: u32) { self.add(i as u64); }
        fn write_u64(&mut self, i: u64) { self.add(i); }
        fn write_usize(&mut self, i: usize) { self.add(i as u64); }
        fn write_i8(&mut self, i: i8) { self.add(i as u64); }
        fn write_i16(&mut self, i: i16) { self.add(i as u64); }
        fn write_i32(&mut self, i: i32) { self.add(i as u64); }
        fn write_i64(&mut self, i: i64) { self.add(i as u64); }
        fn write_isize(&mut self, i: isize) { self.add(i as u64); }
    }

    pub struct RandomState;
    impl RandomState { pub fn new() -> RandomState { RandomState } }
    impl ::core::default::Default for RandomState { fn default() -> RandomState { RandomState } }
    impl ::core::clone::Clone for RandomState { fn clone(&self) -> RandomState { RandomState } }
    impl BuildHasher for RandomState {
        type Hasher = DefaultHasher;
        fn build_hasher(&self) -> DefaultHasher { DefaultHasher::new() }
    }

    // The hash stored in a table slot: never 0 (0 marks an empty slot).
    pub fn make_hash<Q: Hash + ?Sized>(q: &Q) -> u64 {
        let mut h = DefaultHasher::new();
        q.hash(&mut h);
        (h.finish() >> 1) | 9223372036854775808
    }

    // `K: KeyBorrow<Q>` stands for std's `K: Borrow<Q>` in map lookups.
    pub trait KeyBorrow<Q: ?Sized> {
        fn key_borrow(&self) -> &Q;
    }
    impl<T: ?Sized> KeyBorrow<T> for T {
        fn key_borrow(&self) -> &T { self }
    }
    impl KeyBorrow<str> for String {
        fn key_borrow(&self) -> &str { self.as_str() }
    }
    impl<T> KeyBorrow<[T]> for Vec<T> {
        fn key_borrow(&self) -> &[T] { self.as_slice() }
    }

    impl Hash for i8 { fn hash<H: Hasher>(&self, state: &mut H) { state.write_i8(*self); } }
    impl Hash for i16 { fn hash<H: Hasher>(&self, state: &mut H) { state.write_i16(*self); } }
    impl Hash for i32 { fn hash<H: Hasher>(&self, state: &mut H) { state.write_i32(*self); } }
    impl Hash for i64 { fn hash<H: Hasher>(&self, state: &mut H) { state.write_i64(*self); } }
    impl Hash for isize { fn hash<H: Hasher>(&self, state: &mut H) { state.write_isize(*self); } }
    impl Hash for u8 { fn hash<H: Hasher>(&self, state: &mut H) { state.write_u8(*self); } }
    impl Hash for u16 { fn hash<H: Hasher>(&self, state: &mut H) { state.write_u16(*self); } }
    impl Hash for u32 { fn hash<H: Hasher>(&self, state: &mut H) { state.write_u32(*self); } }
    impl Hash for u64 { fn hash<H: Hasher>(&self, state: &mut H) { state.write_u64(*self); } }
    impl Hash for usize { fn hash<H: Hasher>(&self, state: &mut H) { state.write_usize(*self); } }
    impl Hash for bool { fn hash<H: Hasher>(&self, state: &mut H) { state.write_u8(*self as u8); } }
    impl Hash for char { fn hash<H: Hasher>(&self, state: &mut H) { state.write_u32(*self as u32); } }
    impl Hash for () { fn hash<H: Hasher>(&self, state: &mut H) {} }
    impl Hash for str {
        fn hash<H: Hasher>(&self, state: &mut H) { state.write(self.as_bytes()); state.write_u8(255); }
    }
    impl Hash for String {
        fn hash<H: Hasher>(&self, state: &mut H) { Hash::hash(self.as_str(), state); }
    }
    impl<'a, T: Hash + ?Sized> Hash for &'a T {
        fn hash<H: Hasher>(&self, state: &mut H) { Hash::hash(&**self, state); }
    }
    impl<'a, T: Hash + ?Sized> Hash for &'a mut T {
        fn hash<H: Hasher>(&self, state: &mut H) { Hash::hash(&**self, state); }
    }
    impl<T: Hash> Hash for [T] {
        fn hash<H: Hasher>(&self, state: &mut H) {
            let n = self.len();
            state.write_usize(n);
            let mut i = 0usize;
            while i < n { Hash::hash(&self[i], state); i += 1; }
        }
    }
    impl<T: Hash, const N: usize> Hash for [T; N] {
        fn hash<H: Hasher>(&self, state: &mut H) { let s: &[T] = self; Hash::hash(s, state); }
    }
    impl<T: Hash> Hash for Vec<T> {
        fn hash<H: Hasher>(&self, state: &mut H) { Hash::hash(self.as_slice(), state); }
    }
    impl<T: Hash> Hash for Option<T> {
        fn hash<H: Hasher>(&self, state: &mut H) {
            match self {
                None => { state.write_usize(0); }
                Some(x) => { state.write_usize(1); Hash::hash(x, state); }
            }
        }
    }
    impl<T: Hash + ?Sized> Hash for Box<T> {
        fn hash<H: Hasher>(&self, state: &mut H) { Hash::hash(&**self, state); }
    }
    impl<T: Hash> Hash for ::core::cmp::Reverse<T> {
        fn hash<H: Hasher>(&self, state: &mut H) { Hash::hash(&self.0, state); }
    }
    impl<A: Hash> Hash for (A,) {
        fn hash<H: Hasher>(&self, state: &mut H) { Hash::hash(&self.0, state); }
    }
    impl<A: Hash, B: Hash> Hash for (A, B) {
        fn hash<H: Hasher>(&self, state: &mut H) { Hash::hash(&self.0, state); Hash::hash(&self.1, state); }
    }
    impl<A: Hash, B: Hash, C: Hash> Hash for (A, B, C) {
        fn hash<H: Hasher>(&self, state: &mut H) {
            Hash::hash(&self.0, state); Hash::hash(&self.1, state); Hash::hash(&self.2, state);
        }
    }
    impl<A: Hash, B: Hash, C: Hash, D: Hash> Hash for (A, B, C, D) {
        fn hash<H: Hasher>(&self, state: &mut H) {
            Hash::hash(&self.0, state); Hash::hash(&self.1, state);
            Hash::hash(&self.2, state); Hash::hash(&self.3, state);
        }
    }
}

// ---------------------------------------------------------------- collections

pub mod collections {
    pub use ::core::collections::hash_map::HashMap;
    pub use ::core::collections::hash_set::HashSet;
    pub use ::core::collections::btree_map::BTreeMap;
    pub use ::core::collections::btree_set::BTreeSet;
    pub use ::core::collections::vec_deque::VecDeque;
    pub use ::core::collections::binary_heap::BinaryHeap;

    // `{:?}` / `{:#?}` of a set or a map: `{a, b}` / `{k: v, ..}`.
    pub mod dbg {
        use ::core::fmt::{Debug, Formatter, Result, Write};
        use ::core::option::Option::{self, Some, None};
        use ::core::string::String;

        // pretty text of one entry, `k: v` when `v` is given
        fn entry_text<K: Debug + ?Sized, V: Debug + ?Sized>(k: &K, v: Option<&V>) -> String {
            let mut s = String::new();
            {
                let mut fm = Formatter::new(&mut s);
                fm.set_spec(' ', 0, 0, true, false, 0, false, 0, false);
                let _ = Debug::fmt(k, &mut fm);
                if let Some(x) = v {
                    let _ = fm.write_str(": ");
                    let _ = Debug::fmt(x, &mut fm);
                }
            }
            s
        }

        fn write_indented(f: &mut Formatter<'_>, text: &str) -> Result {
            f.write_str("    ")?;
            let mut it = text.chars();
            while let Some(c) = it.next() {
                f.write_char(c)?;
                if c == '\n' { f.write_str("    ")?; }
            }
            f.write_str(",\n")
        }

        pub fn set<'a, T: Debug + 'a, I: ::core::iter::Iterator<Item = &'a T>>(f: &mut Formatter<'_>, it: I) -> Result {
            let mut it = it;
            f.write_str("{")?;
            if f.alternate() {
                let mut first = true;
                while let Some(x) = it.next() {
                    if first { f.write_str("\n")?; first = false; }
                    let t = entry_text::<T, T>(x, None);
                    write_indented(f, t.as_str())?;
                }
            } else {
                let mut first = true;
                while let Some(x) = it.next() {
                    if !first { f.write_str(", ")?; }
                    first = false;
                    Debug::fmt(x, f)?;
                }
            }
            f.write_str("}")
        }

        pub fn map<'a, K: Debug + 'a, V: Debug + 'a, I: ::core::iter::Iterator<Item = (&'a K, &'a V)>>(f: &mut Formatter<'_>, it: I) -> Result {
            let mut it = it;
            f.write_str("{")?;
            if f.alternate() {
                let mut first = true;
                while let Some((k, v)) = it.next() {
                    if first { f.write_str("\n")?; first = false; }
                    let t = entry_text::<K, V>(k, Some(v));
                    write_indented(f, t.as_str())?;
                }
            } else {
                let mut first = true;
                while let Some((k, v)) = it.next() {
                    if !first { f.write_str(", ")?; }
                    first = false;
                    Debug::fmt(k, f)?;
                    f.write_str(": ")?;
                    Debug::fmt(v, f)?;
                }
            }
            f.write_str("}")
        }
    }

    // ------------------------------------------------------------ HashMap

    pub mod hash_map {
        use ::core::clone::Clone;
        use ::core::cmp::{Eq, PartialEq};
        use ::core::default::Default;
        use ::core::hash::{make_hash, Hash, KeyBorrow};
        use ::core::intrinsics::{elem_alloc, elem_free, elem_read, elem_ref, elem_ref_mut, elem_write};
        use ::core::iter::{Extend, FromIterator, IntoIterator, Iterator};
        use ::core::ops::{Drop, FnMut, FnOnce, Index};
        use ::core::option::Option::{self, Some, None};
        pub use ::core::hash::{DefaultHasher, RandomState};

        // Open addressing with linear probing. `hashes[i] == 0` marks an empty slot;
        // keys and values live in raw storage and are initialised only where the hash is not 0.
        pub struct HashMap<K, V> { hashes: usize, keys: usize, vals: usize, cap: usize, len: usize }

        fn zero_hashes(base: usize, n: usize) {
            let mut i = 0usize;
            while i < n { elem_write::<u64>(base, i, 0); i += 1; }
        }

        impl<K, V> HashMap<K, V> {
            pub fn new() -> HashMap<K, V> { HashMap { hashes: 0, keys: 0, vals: 0, cap: 0, len: 0 } }
            pub fn with_capacity(n: usize) -> HashMap<K, V> {
                let mut m = HashMap::new();
                if n > 0 {
                    let mut c = 8usize;
                    while c * 3 < n * 4 { c *= 2; }
                    m.hashes = elem_alloc::<u64>(c);
                    zero_hashes(m.hashes, c);
                    m.keys = elem_alloc::<K>(c);
                    m.vals = elem_alloc::<V>(c);
                    m.cap = c;
                }
                m
            }
            pub fn len(&self) -> usize { self.len }
            pub fn is_empty(&self) -> bool { self.len == 0 }
            pub fn capacity(&self) -> usize { self.cap * 3 / 4 }

            fn grow(&mut self) {
                let nc = if self.cap == 0 { 8 } else { self.cap * 2 };
                let nh = elem_alloc::<u64>(nc);
                zero_hashes(nh, nc);
                let nk = elem_alloc::<K>(nc);
                let nv = elem_alloc::<V>(nc);
                let mask = nc - 1;
                let mut i = 0usize;
                while i < self.cap {
                    let h = elem_read::<u64>(self.hashes, i);
                    if h != 0 {
                        let mut j = (h as usize) & mask;
                        while elem_read::<u64>(nh, j) != 0 { j = (j + 1) & mask; }
                        elem_write::<u64>(nh, j, h);
                        elem_write::<K>(nk, j, elem_read::<K>(self.keys, i));
                        elem_write::<V>(nv, j, elem_read::<V>(self.vals, i));
                    }
                    i += 1;
                }
                if self.cap > 0 {
                    elem_free::<u64>(self.hashes);
                    elem_free::<K>(self.keys);
                    elem_free::<V>(self.vals);
                }
                self.hashes = nh; self.keys = nk; self.vals = nv; self.cap = nc;
            }

            // stores a new entry (the key must not be present); returns its slot
            fn insert_new(&mut self, h: u64, k: K, v: V) -> usize {
                if (self.len + 1) * 4 > self.cap * 3 { self.grow(); }
                let mask = self.cap - 1;
                let mut j = (h as usize) & mask;
                while elem_read::<u64>(self.hashes, j) != 0 { j = (j + 1) & mask; }
                elem_write::<u64>(self.hashes, j, h);
                elem_write::<K>(self.keys, j, k);
                elem_write::<V>(self.vals, j, v);
                self.len += 1;
                j
            }

            // removes slot `i` (backward shift deletion) and returns its entry
            fn remove_at(&mut self, i: usize) -> (K, V) {
                let k = elem_read::<K>(self.keys, i);
                let v = elem_read::<V>(self.vals, i);
                let mask = self.cap - 1;
                let mut hole = i;
                let mut j = i;
                loop {
                    j = (j + 1) & mask;
                    let hj = elem_read::<u64>(self.hashes, j);
                    if hj == 0 { break; }
                    let ideal = (hj as usize) & mask;
                    if ((j + self.cap - ideal) & mask) >= ((j + self.cap - hole) & mask) {
                        elem_write::<u64>(self.hashes, hole, hj);
                        elem_write::<K>(self.keys, hole, elem_read::<K>(self.keys, j));
                        elem_write::<V>(self.vals, hole, elem_read::<V>(self.vals, j));
                        hole = j;
                    }
                }
                elem_write::<u64>(self.hashes, hole, 0);
                self.len -= 1;
                (k, v)
            }

            pub fn clear(&mut self) {
                let mut i = 0usize;
                while i < self.cap {
                    if elem_read::<u64>(self.hashes, i) != 0 {
                        let _k = elem_read::<K>(self.keys, i);
                        let _v = elem_read::<V>(self.vals, i);
                        elem_write::<u64>(self.hashes, i, 0);
                    }
                    i += 1;
                }
                self.len = 0;
            }

            pub fn iter(&self) -> Iter<'_, K, V> { Iter { map: self, pos: 0, left: self.len } }
            pub fn iter_mut(&mut self) -> IterMut<'_, K, V> {
                let n = self.len;
                IterMut { hashes: self.hashes, keys: self.keys, vals: self.vals, cap: self.cap, pos: 0, left: n, map: self }
            }
            pub fn keys(&self) -> Keys<'_, K, V> { Keys { inner: self.iter() } }
            pub fn values(&self) -> Values<'_, K, V> { Values { inner: self.iter() } }
            pub fn values_mut(&mut self) -> ValuesMut<'_, K, V> { ValuesMut { inner: self.iter_mut() } }
            pub fn into_keys(self) -> IntoKeys<K, V> { IntoKeys { inner: self.into_iter() } }
            pub fn into_values(self) -> IntoValues<K, V> { IntoValues { inner: self.into_iter() } }
            pub fn drain(&mut self) -> IntoIter<K, V> {
                let it = IntoIter { hashes: self.hashes, keys: self.keys, vals: self.vals, cap: self.cap, pos: 0, left: self.len };
                self.hashes = 0; self.keys = 0; self.vals = 0; self.cap = 0; self.len = 0;
                it
            }
        }

        impl<K: Hash + Eq, V> HashMap<K, V> {
            fn find<Q: ?Sized + Hash + Eq>(&self, k: &Q, h: u64) -> Option<usize> where K: KeyBorrow<Q> {
                if self.cap == 0 { return None; }
                let mask = self.cap - 1;
                let mut i = (h as usize) & mask;
                loop {
                    let hh = elem_read::<u64>(self.hashes, i);
                    if hh == 0 { return None; }
                    if hh == h {
                        let kk: &K = elem_ref::<K>(self.keys, i);
                        if PartialEq::eq(kk.key_borrow(), k) { return Some(i); }
                    }
                    i = (i + 1) & mask;
                }
            }

            pub fn insert(&mut self, k: K, v: V) -> Option<V> {
                let h = make_hash(&k);
                match self.find::<K>(&k, h) {
                    Some(i) => Some(::core::mem::replace(elem_ref_mut::<V>(self.vals, i), v)),
                    None => { self.insert_new(h, k, v); None }
                }
            }
            pub fn get<Q: ?Sized + Hash + Eq>(&self, k: &Q) -> Option<&V> where K: KeyBorrow<Q> {
                match self.find::<Q>(k, make_hash(k)) {
                    Some(i) => Some(elem_ref::<V>(self.vals, i)),
                    None => None,
                }
            }
            pub fn get_mut<Q: ?Sized + Hash + Eq>(&mut self, k: &Q) -> Option<&mut V> where K: KeyBorrow<Q> {
                match self.find::<Q>(k, make_hash(k)) {
                    Some(i) => Some(elem_ref_mut::<V>(self.vals, i)),
                    None => None,
                }
            }
            pub fn get_key_value<Q: ?Sized + Hash + Eq>(&self, k: &Q) -> Option<(&K, &V)> where K: KeyBorrow<Q> {
                match self.find::<Q>(k, make_hash(k)) {
                    Some(i) => Some((elem_ref::<K>(self.keys, i), elem_ref::<V>(self.vals, i))),
                    None => None,
                }
            }
            pub fn contains_key<Q: ?Sized + Hash + Eq>(&self, k: &Q) -> bool where K: KeyBorrow<Q> {
                match self.find::<Q>(k, make_hash(k)) { Some(_) => true, None => false }
            }
            pub fn remove<Q: ?Sized + Hash + Eq>(&mut self, k: &Q) -> Option<V> where K: KeyBorrow<Q> {
                match self.find::<Q>(k, make_hash(k)) {
                    Some(i) => { let (_k, v) = self.remove_at(i); Some(v) }
                    None => None,
                }
            }
            pub fn remove_entry<Q: ?Sized + Hash + Eq>(&mut self, k: &Q) -> Option<(K, V)> where K: KeyBorrow<Q> {
                match self.find::<Q>(k, make_hash(k)) {
                    Some(i) => Some(self.remove_at(i)),
                    None => None,
                }
            }
            pub fn entry(&mut self, key: K) -> Entry<'_, K, V> {
                let h = make_hash(&key);
                match self.find::<K>(&key, h) {
                    Some(i) => Entry::Occupied(OccupiedEntry { map: self, idx: i, key: key }),
                    None => Entry::Vacant(VacantEntry { map: self, hash: h, key: key }),
                }
            }
            pub fn reserve(&mut self, extra: usize) {
                while (self.len + extra) * 4 > self.cap * 3 { self.grow(); }
            }
            pub fn retain<F: FnMut(&K, &mut V) -> bool>(&mut self, mut f: F) {
                let mut i = 0usize;
                // mark rejected slots by collecting their hashes' indices, then delete from the end
                let mut doomed: ::core::vec::Vec<usize> = ::core::vec::Vec::new();
                while i < self.cap {
                    if elem_read::<u64>(self.hashes, i) != 0 {
                        let keep = f(elem_ref::<K>(self.keys, i), elem_ref_mut::<V>(self.vals, i));
                        if !keep { doomed.push(i); }
                    }
                    i += 1;
                }
                // rebuild without the doomed slots
                if doomed.len() > 0 {
                    let oh = self.hashes; let ok = self.keys; let ov = self.vals; let oc = self.cap;
                    self.hashes = 0; self.keys = 0; self.vals = 0; self.cap = 0; self.len = 0;
                    let mut d = 0usize;
                    let mut j = 0usize;
                    while j < oc {
                        let h = elem_read::<u64>(oh, j);
                        if h != 0 {
                            if d < doomed.len() && doomed[d] == j {
                                d += 1;
                                let _k = elem_read::<K>(ok, j);
                                let _v = elem_read::<V>(ov, j);
                            } else {
                                self.insert_new(h, elem_read::<K>(ok, j), elem_read::<V>(ov, j));
                            }
                        }
                        j += 1;
                    }
                    elem_free::<u64>(oh);
                    elem_free::<K>(ok);
                    elem_free::<V>(ov);
                }
            }
        }

        impl<K, V> Drop for HashMap<K, V> {
            fn drop(&mut self) {
                if self.cap > 0 {
                    self.clear();
                    elem_free::<u64>(self.hashes);
                    elem_free::<K>(self.keys);
                    elem_free::<V>(self.vals);
                }
            }
        }

        // ---- entry API
        pub enum Entry<'a, K, V> {
            Occupied(OccupiedEntry<'a, K, V>),
            Vacant(VacantEntry<'a, K, V>),
        }
        pub struct OccupiedEntry<'a, K, V> { map: &'a mut HashMap<K, V>, idx: usize, key: K }
        pub struct VacantEntry<'a, K, V> { map: &'a mut HashMap<K, V>, hash: u64, key: K }

        impl<'a, K, V> Entry<'a, K, V> {
            pub fn or_insert(self, default: V) -> &'a mut V {
                match self {
                    Entry::Occupied(e) => e.into_mut(),
                    Entry::Vacant(e) => e.insert(default),
                }
            }
            pub fn or_insert_with<F: FnOnce() -> V>(self, f: F) -> &'a mut V {
                match self {
                    Entry::Occupied(e) => e.into_mut(),
                    Entry::Vacant(e) => e.insert(f()),
                }
            }
            pub fn or_insert_with_key<F: FnOnce(&K) -> V>(self, f: F) -> &'a mut V {
                match self {
                    Entry::Occupied(e) => e.into_mut(),
                    Entry::Vacant(e) => { let v = f(&e.key); e.insert(v) }
                }
            }
            pub fn or_default(self) -> &'a mut V where V: Default {
                match self {
                    Entry::Occupied(e) => e.into_mut(),
                    Entry::Vacant(e) => e.insert(V::default()),
                }
            }
            pub fn and_modify<F: FnOnce(&mut V)>(self, f: F) -> Entry<'a, K, V> {
                match self {
                    Entry::Occupied(mut e) => { f(e.get_mut()); Entry::Occupied(e) }
                    Entry::Vacant(e) => Entry::Vacant(e),
                }
            }
            pub fn key(&self) -> &K {
                match self {
                    Entry::Occupied(e) => &e.key,
                    Entry::Vacant(e) => &e.key,
                }
            }
        }
        impl<'a, K, V> OccupiedEntry<'a, K, V> {
            pub fn key(&self) -> &K { elem_ref::<K>(self.map.keys, self.idx) }
            pub fn get(&self) -> &V { elem_ref::<V>(self.map.vals, self.idx) }
            pub fn get_mut(&mut self) -> &mut V { elem_ref_mut::<V>(self.map.vals, self.idx) }
            pub fn into_mut(self) -> &'a mut V { elem_ref_mut::<V>(self.map.vals, self.idx) }
            pub fn insert(&mut self, v: V) -> V { ::core::mem::replace(elem_ref_mut::<V>(self.map.vals, self.idx), v) }
            pub fn remove(self) -> V { let (_k, v) = self.map.remove_at(self.idx); v }
            pub fn remove_entry(self) -> (K, V) { self.map.remove_at(self.idx) }
        }
        impl<'a, K, V> VacantEntry<'a, K, V> {
            pub fn key(&self) -> &K { &self.key }
            pub fn into_key(self) -> K { self.key }
            pub fn insert(self, v: V) -> &'a mut V {
                let i = self.map.insert_new(self.hash, self.key, v);
                elem_ref_mut::<V>(self.map.vals, i)
            }
        }

        // ---- iterators
        pub struct Iter<'a, K, V> { map: &'a HashMap<K, V>, pos: usize, left: usize }
        impl<'a, K, V> Iterator for Iter<'a, K, V> {
            type Item = (&'a K, &'a V);
            fn next(&mut self) -> Option<(&'a K, &'a V)> {
                while self.pos < self.map.cap {
                    let i = self.pos;
                    self.pos += 1;
                    if elem_read::<u64>(self.map.hashes, i) != 0 {
                        self.left -= 1;
                        return Some((elem_ref::<K>(self.map.keys, i), elem_ref::<V>(self.map.vals, i)));
                    }
                }
                None
            }
        }
        impl<'a, K, V> Iter<'a, K, V> {
            pub fn len(&self) -> usize { self.left }
        }
        impl<'a, K, V> Clone for Iter<'a, K, V> {
            fn clone(&self) -> Iter<'a, K, V> { Iter { map: self.map, pos: self.pos, left: self.left } }
        }
        pub struct IterMut<'a, K, V> { map: &'a mut HashMap<K, V>, hashes: usize, keys: usize, vals: usize, cap: usize, pos: usize, left: usize }
        impl<'a, K, V> Iterator for IterMut<'a, K, V> {
            type Item = (&'a K, &'a mut V);
            fn next(&mut self) -> Option<(&'a K, &'a mut V)> {
                while self.pos < self.cap {
                    let i = self.pos;
                    self.pos += 1;
                    if elem_read::<u64>(self.hashes, i) != 0 {
                        self.left -= 1;
                        return Some((elem_ref::<K>(self.keys, i), elem_ref_mut::<V>(self.vals, i)));
                    }
                }
                None
            }
        }
        pub struct Keys<'a, K, V> { inner: Iter<'a, K, V> }
        impl<'a, K, V> Iterator for Keys<'a, K, V> {
            type Item = &'a K;
            fn next(&mut self) -> Option<&'a K> {
                match self.inner.next() { Some((k, _v)) => Some(k), None => None }
            }
        }
        impl<'a, K, V> Keys<'a, K, V> { pub fn len(&self) -> usize { self.inner.left } }
        pub struct Values<'a, K, V> { inner: Iter<'a, K, V> }
        impl<'a, K, V> Iterator for Values<'a, K, V> {
            type Item = &'a V;
            fn next(&mut self) -> Option<&'a V> {
                match self.inner.next() { Some((_k, v)) => Some(v), None => None }
            }
        }
        impl<'a, K, V> Values<'a, K, V> { pub fn len(&self) -> usize { self.inner.left } }
        pub struct ValuesMut<'a, K, V> { inner: IterMut<'a, K, V> }
        impl<'a, K, V> Iterator for ValuesMut<'a, K, V> {
            type Item = &'a mut V;
            fn next(&mut self) -> Option<&'a mut V> {
                match self.inner.next() { Some((_k, v)) => Some(v), None => None }
            }
        }

        pub struct IntoIter<K, V> { hashes: usize, keys: usize, vals: usize, cap: usize, pos: usize, left: usize }
        impl<K, V> Iterator for IntoIter<K, V> {
            type Item = (K, V);
            fn next(&mut self) -> Option<(K, V)> {
                while self.pos < self.cap {
                    let i = self.pos;
                    self.pos += 1;
                    if elem_read::<u64>(self.hashes, i) != 0 {
                        self.left -= 1;
                        return Some((elem_read::<K>(self.keys, i), elem_read::<V>(self.vals, i)));
                    }
                }
                None
            }
        }
        impl<K, V> Drop for IntoIter<K, V> {
            fn drop(&mut self) {
                if self.cap > 0 {
                    while self.pos < self.cap {
                        let i = self.pos;
                        self.pos += 1;
                        if elem_read::<u64>(self.hashes, i) != 0 {
                            let _k = elem_read::<K>(self.keys, i);
                            let _v = elem_read::<V>(self.vals, i);
                        }
                    }
                    elem_free::<u64>(self.hashes);
                    elem_free::<K>(self.keys);
                    elem_free::<V>(self.vals);
                }
            }
        }
        pub struct IntoKeys<K, V> { inner: IntoIter<K, V> }
        impl<K, V> Iterator for IntoKeys<K, V> {
            type Item = K;
            fn next(&mut self) -> Option<K> { match self.inner.next() { Some((k, _v)) => Some(k), None => None } }
        }
        pub struct IntoValues<K, V> { inner: IntoIter<K, V> }
        impl<K, V> Iterator for IntoValues<K, V> {
            type Item = V;
            fn next(&mut self) -> Option<V> { match self.inner.next() { Some((_k, v)) => Some(v), None => None } }
        }

        impl<K, V> IntoIterator for HashMap<K, V> {
            type Item = (K, V);
            type IntoIter = IntoIter<K, V>;
            fn into_iter(self) -> IntoIter<K, V> {
                let it = IntoIter { hashes: self.hashes, keys: self.keys, vals: self.vals, cap: self.cap, pos: 0, left: self.len };
                ::core::mem::forget(self);
                it
            }
        }
        impl<'a, K, V> IntoIterator for &'a HashMap<K, V> {
            type Item = (&'a K, &'a V);
            type IntoIter = Iter<'a, K, V>;
            fn into_iter(self) -> Iter<'a, K, V> { self.iter() }
        }
        impl<'a, K, V> IntoIterator for &'a mut HashMap<K, V> {
            type Item = (&'a K, &'a mut V);
            type IntoIter = IterMut<'a, K, V>;
            fn into_iter(self) -> IterMut<'a, K, V> { self.iter_mut() }
        }

        impl<K: Hash + Eq, V> FromIterator<(K, V)> for HashMap<K, V> {
            fn from_iter<I: IntoIterator<Item = (K, V)>>(iter: I) -> HashMap<K, V> {
                let mut m = HashMap::new();
                let mut it = iter.into_iter();
                while let Some((k, v)) = it.next() { m.insert(k, v); }
                m
            }
        }
        impl<K: Hash + Eq, V> Extend<(K, V)> for HashMap<K, V> {
            fn extend<I: IntoIterator<Item = (K, V)>>(&mut self, iter: I) {
                let mut it = iter.into_iter();
                while let Some((k, v)) = it.next() { self.insert(k, v); }
            }
        }
        impl<K: Hash + Eq, V, const N: usize> ::core::convert::From<[(K, V); N]> for HashMap<K, V> {
            fn from(a: [(K, V); N]) -> HashMap<K, V> {
                let mut m = HashMap::new();
                let mut it = ::core::vec::Vec::from_array(a).into_iter();
                while let Some((k, v)) = it.next() { m.insert(k, v); }
                m
            }
        }
        impl<'a, K: Hash + Eq + KeyBorrow<Q>, Q: ?Sized + Hash + Eq, V> Index<&'a Q> for HashMap<K, V> {
            type Output = V;
            fn index(&self, k: &'a Q) -> &V {
                match self.get::<Q>(k) {
                    Some(v) => v,
                    None => ::core::panicking::panic_str("library/std/src/collections/hash/map.rs", 1, 1, "no entry found for key"),
                }
            }
        }
        impl<K, V> Default for HashMap<K, V> { fn default() -> HashMap<K, V> { HashMap::new() } }
        impl<K: Clone, V: Clone> Clone for HashMap<K, V> {
            fn clone(&self) -> HashMap<K, V> {
                let mut m: HashMap<K, V> = HashMap::new();
                if self.cap > 0 {
                    m.hashes = elem_alloc::<u64>(self.cap);
                    zero_hashes(m.hashes, self.cap);
                    m.keys = elem_alloc::<K>(self.cap);
                    m.vals = elem_alloc::<V>(self.cap);
                    m.cap = self.cap;
                    let mut i = 0usize;
                    while i < self.cap {
                        let h = elem_read::<u64>(self.hashes, i);
                        if h != 0 {
                            elem_write::<u64>(m.hashes, i, h);
                            elem_write::<K>(m.keys, i, elem_ref::<K>(self.keys, i).clone());
                            elem_write::<V>(m.vals, i, elem_ref::<V>(self.vals, i).clone());
                            m.len += 1;
                        }
                        i += 1;
                    }
                }
                m
            }
        }
        impl<K: Hash + Eq, V: PartialEq> PartialEq for HashMap<K, V> {
            fn eq(&self, o: &HashMap<K, V>) -> bool {
                if self.len != o.len { return false; }
                let mut it = self.iter();
                while let Some((k, v)) = it.next() {
                    match o.get::<K>(k) {
                        Some(w) => { if !PartialEq::eq(v, w) { return false; } }
                        None => { return false; }
                    }
                }
                true
            }
        }
        impl<K: Hash + Eq, V: Eq> Eq for HashMap<K, V> {}
        impl<K: ::core::fmt::Debug, V: ::core::fmt::Debug> ::core::fmt::Debug for HashMap<K, V> {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                ::core::collections::dbg::map::<K, V, Iter<'_, K, V>>(f, self.iter())
            }
        }
    }

    // ------------------------------------------------------------ HashSet

    pub mod hash_set {
        use ::core::clone::Clone;
        use ::core::cmp::{Eq, PartialEq};
        use ::core::collections::hash_map::{self, HashMap};
        use ::core::default::Default;
        use ::core::hash::{Hash, KeyBorrow};
        use ::core::iter::{Extend, FromIterator, IntoIterator, Iterator};
        use ::core::option::Option::{self, Some, None};

        pub struct HashSet<T> { map: HashMap<T, ()> }

        impl<T> HashSet<T> {
            pub fn new() -> HashSet<T> { HashSet { map: HashMap::new() } }
            pub fn with_capacity(n: usize) -> HashSet<T> { HashSet { map: HashMap::with_capacity(n) } }
            pub fn len(&self) -> usize { self.map.len() }
            pub fn is_empty(&self) -> bool { self.map.is_empty() }
            pub fn capacity(&self) -> usize { self.map.capacity() }
            pub fn clear(&mut self) { self.map.clear(); }
            pub fn iter(&self) -> Iter<'_, T> { Iter { inner: self.map.keys() } }
            pub fn drain(&mut self) -> IntoIter<T> { IntoIter { inner: self.map.drain() } }
        }

        impl<T: Hash + Eq> HashSet<T> {
            pub fn insert(&mut self, v: T) -> bool {
                match self.map.insert(v, ()) { Some(_) => false, None => true }
            }
            pub fn replace(&mut self, v: T) -> Option<T> {
                let old = self.map.remove_entry::<T>(&v);
                self.map.insert(v, ());
                match old { Some((k, _u)) => Some(k), None => None }
            }
            pub fn contains<Q: ?Sized + Hash + Eq>(&self, v: &Q) -> bool where T: KeyBorrow<Q> {
                self.map.contains_key::<Q>(v)
            }
            pub fn get<Q: ?Sized + Hash + Eq>(&self, v: &Q) -> Option<&T> where T: KeyBorrow<Q> {
                match self.map.get_key_value::<Q>(v) { Some((k, _u)) => Some(k), None => None }
            }
            pub fn remove<Q: ?Sized + Hash + Eq>(&mut self, v: &Q) -> bool where T: KeyBorrow<Q> {
                match self.map.remove::<Q>(v) { Some(_) => true, None => false }
            }
            pub fn take<Q: ?Sized + Hash + Eq>(&mut self, v: &Q) -> Option<T> where T: KeyBorrow<Q> {
                match self.map.remove_entry::<Q>(v) { Some((k, _u)) => Some(k), None => None }
            }
            pub fn reserve(&mut self, n: usize) { self.map.reserve(n); }
            pub fn retain<F: ::core::ops::FnMut(&T) -> bool>(&mut self, mut f: F) {
                self.map.retain(|k, _u| f(k));
            }
            pub fn union<'a>(&'a self, other: &'a HashSet<T>) -> Union<'a, T> {
                Union { a: self.iter(), b: other.difference(self) }
            }
            pub fn intersection<'a>(&'a self, other: &'a HashSet<T>) -> Intersection<'a, T> {
                Intersection { iter: self.iter(), other: other }
            }
            pub fn difference<'a>(&'a self, other: &'a HashSet<T>) -> Difference<'a, T> {
                Difference { iter: self.iter(), other: other }
            }
            pub fn symmetric_difference<'a>(&'a self, other: &'a HashSet<T>) -> SymmetricDifference<'a, T> {
                SymmetricDifference { a: self.difference(other), b: other.difference(self) }
            }
            pub fn is_subset(&self, other: &HashSet<T>) -> bool {
                if self.len() > other.len() { return false; }
                let mut it = self.iter();
                while let Some(x) = it.next() { if !other.contains::<T>(x) { return false; } }
                true
            }
            pub fn is_superset(&self, other: &HashSet<T>) -> bool { other.is_subset(self) }
            pub fn is_disjoint(&self, other: &HashSet<T>) -> bool {
                let mut it = self.iter();
                while let Some(x) = it.next() { if other.contains::<T>(x) { return false; } }
                true
            }
        }

        pub struct Iter<'a, T> { inner: hash_map::Keys<'a, T, ()> }
        impl<'a, T> Iterator for Iter<'a, T> {
            type Item = &'a T;
            fn next(&mut self) -> Option<&'a T> { self.inner.next() }
        }
        impl<'a, T> Iter<'a, T> { pub fn len(&self) -> usize { self.inner.len() } }
        pub struct IntoIter<T> { inner: hash_map::IntoIter<T, ()> }
        impl<T> Iterator for IntoIter<T> {
            type Item = T;
            fn next(&mut self) -> Option<T> { match self.inner.next() { Some((k, _u)) => Some(k), None => None } }
        }

        pub struct Difference<'a, T> { iter: Iter<'a, T>, other: &'a HashSet<T> }
        impl<'a, T: Hash + Eq> Iterator for Difference<'a, T> {
            type Item = &'a T;
            fn next(&mut self) -> Option<&'a T> {
                loop {
                    match self.iter.next() {
                        Some(x) => { if !self.other.contains::<T>(x) { return Some(x); } }
                        None => { return None; }
                    }
                }
            }
        }
        pub struct Intersection<'a, T> { iter: Iter<'a, T>, other: &'a HashSet<T> }
        impl<'a, T: Hash + Eq> Iterator for Intersection<'a, T> {
            type Item = &'a T;
            fn next(&mut self) -> Option<&'a T> {
                loop {
                    match self.iter.next() {
                        Some(x) => { if self.other.contains::<T>(x) { return Some(x); } }
                        None => { return None; }
                    }
                }
            }
        }
        pub struct Union<'a, T> { a: Iter<'a, T>, b: Difference<'a, T> }
        impl<'a, T: Hash + Eq> Iterator for Union<'a, T> {
            type Item = &'a T;
            fn next(&mut self) -> Option<&'a T> {
                match self.a.next() { Some(x) => Some(x), None => self.b.next() }
            }
        }
        pub struct SymmetricDifference<'a, T> { a: Difference<'a, T>, b: Difference<'a, T> }
        impl<'a, T: Hash + Eq> Iterator for SymmetricDifference<'a, T> {
            type Item = &'a T;
            fn next(&mut self) -> Option<&'a T> {
                match self.a.next() { Some(x) => Some(x), None => self.b.next() }
            }
        }

        impl<T> IntoIterator for HashSet<T> {
            type Item = T;
            type IntoIter = IntoIter<T>;
            fn into_iter(self) -> IntoIter<T> { IntoIter { inner: self.map.into_iter() } }
        }
        impl<'a, T> IntoIterator for &'a HashSet<T> {
            type Item = &'a T;
            type IntoIter = Iter<'a, T>;
            fn into_iter(self) -> Iter<'a, T> { self.iter() }
        }
        impl<T: Hash + Eq> FromIterator<T> for HashSet<T> {
            fn from_iter<I: IntoIterator<Item = T>>(iter: I) -> HashSet<T> {
                let mut s = HashSet::new();
                let mut it = iter.into_iter();
                while let Some(x) = it.next() { s.insert(x); }
                s
            }
        }
        impl<T: Hash + Eq> Extend<T> for HashSet<T> {
            fn extend<I: IntoIterator<Item = T>>(&mut self, iter: I) {
                let mut it = iter.into_iter();
                while let Some(x) = it.next() { self.insert(x); }
            }
        }
        impl<T: Hash + Eq, const N: usize> ::core::convert::From<[T; N]> for HashSet<T> {
            fn from(a: [T; N]) -> HashSet<T> {
                let mut s = HashSet::new();
                let mut it = ::core::vec::Vec::from_array(a).into_iter();
                while let Some(x) = it.next() { s.insert(x); }
                s
            }
        }
        impl<T> Default for HashSet<T> { fn default() -> HashSet<T> { HashSet::new() } }
        impl<T: Hash + Eq + Clone> Clone for HashSet<T> {
            fn clone(&self) -> HashSet<T> {
                let mut s = HashSet::with_capacity(self.len());
                let mut it = self.iter();
                while let Some(x) = it.next() { s.insert(x.clone()); }
                s
            }
        }
        impl<T: Hash + Eq> PartialEq for HashSet<T> {
            fn eq(&self, o: &HashSet<T>) -> bool { self.len() == o.len() && self.is_subset(o) }
        }
        impl<T: Hash + Eq> Eq for HashSet<T> {}
        impl<T: ::core::fmt::Debug> ::core::fmt::Debug for HashSet<T> {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                ::core::collections::dbg::set::<T, Iter<'_, T>>(f, self.iter())
            }
        }
        impl<'a, 'b, T: Hash + Eq + Clone> ::core::ops::BitOr<&'b HashSet<T>> for &'a HashSet<T> {
            type Output = HashSet<T>;
            fn bitor(self, o: &'b HashSet<T>) -> HashSet<T> { self.union(o).cloned().collect() }
        }
        impl<'a, 'b, T: Hash + Eq + Clone> ::core::ops::BitAnd<&'b HashSet<T>> for &'a HashSet<T> {
            type Output = HashSet<T>;
            fn bitand(self, o: &'b HashSet<T>) -> HashSet<T> { self.intersection(o).cloned().collect() }
        }
        impl<'a, 'b, T: Hash + Eq + Clone> ::core::ops::BitXor<&'b HashSet<T>> for &'a HashSet<T> {
            type Output = HashSet<T>;
            fn bitxor(self, o: &'b HashSet<T>) -> HashSet<T> { self.symmetric_difference(o).cloned().collect() }
        }
        impl<'a, 'b, T: Hash + Eq + Clone> ::core::ops::Sub<&'b HashSet<T>> for &'a HashSet<T> {
            type Output = HashSet<T>;
            fn sub(self, o: &'b HashSet<T>) -> HashSet<T> { self.difference(o).cloned().collect() }
        }
    }

    // ------------------------------------------------------------ BTreeMap (sorted parallel Vecs)

    pub mod btree_map {
        use ::core::clone::Clone;
        use ::core::cmp::{Eq, Ord, Ordering, PartialEq};
        use ::core::default::Default;
        use ::core::hash::KeyBorrow;
        use ::core::intrinsics::elem_ref_mut;
        use ::core::iter::{DoubleEndedIterator, Extend, FromIterator, IntoIterator, Iterator};
        use ::core::ops::{FnMut, FnOnce, Index};
        use ::core::option::Option::{self, Some, None};
        use ::core::result::Result::{self, Ok, Err};
        use ::core::vec::Vec;

        pub struct BTreeMap<K, V> { keys: Vec<K>, vals: Vec<V> }

        impl<K, V> BTreeMap<K, V> {
            pub fn new() -> BTreeMap<K, V> { BTreeMap { keys: Vec::new(), vals: Vec::new() } }
            pub fn len(&self) -> usize { self.keys.len() }
            pub fn is_empty(&self) -> bool { self.keys.len() == 0 }
            pub fn clear(&mut self) { self.keys.clear(); self.vals.clear(); }
            pub fn iter(&self) -> Iter<'_, K, V> {
                Iter { keys: self.keys.as_slice(), vals: self.vals.as_slice(), front: 0, back: self.keys.len() }
            }
            pub fn iter_mut(&mut self) -> IterMut<'_, K, V> {
                let n = self.keys.len();
                IterMut { keys: self.keys.as_slice(), vals: self.vals.ptr, front: 0, back: n }
            }
            pub fn keys(&self) -> Keys<'_, K, V> { Keys { inner: self.iter() } }
            pub fn values(&self) -> Values<'_, K, V> { Values { inner: self.iter() } }
            pub fn values_mut(&mut self) -> ValuesMut<'_, K, V> { ValuesMut { inner: self.iter_mut() } }
            pub fn into_keys(self) -> ::core::vec::IntoIter<K> { let m = self; m.keys.into_iter() }
            pub fn into_values(self) -> ::core::vec::IntoIter<V> { let m = self; m.vals.into_iter() }
            pub fn first_key_value(&self) -> Option<(&K, &V)> {
                if self.keys.len() == 0 { None } else { Some((&self.keys[0], &self.vals[0])) }
            }
            pub fn last_key_value(&self) -> Option<(&K, &V)> {
                let n = self.keys.len();
                if n == 0 { None } else { Some((&self.keys[n - 1], &self.vals[n - 1])) }
            }
            pub fn pop_first(&mut self) -> Option<(K, V)> {
                if self.keys.len() == 0 { None } else { Some((self.keys.remove(0), self.vals.remove(0))) }
            }
            pub fn pop_last(&mut self) -> Option<(K, V)> {
                match self.keys.pop() {
                    Some(k) => match self.vals.pop() { Some(v) => Some((k, v)), None => None },
                    None => None,
                }
            }
        }

        impl<K: Ord, V> BTreeMap<K, V> {
            // Ok(index) when found, Err(insertion point) otherwise
            fn search<Q: ?Sized + Ord>(&self, k: &Q) -> Result<usize, usize> where K: KeyBorrow<Q> {
                let mut lo = 0usize;
                let mut hi = self.keys.len();
                if hi > 0 {
                    // fast path: appending in order
                    match Ord::cmp(self.keys[hi - 1].key_borrow(), k) {
                        Ordering::Less => { return Err(hi); }
                        Ordering::Equal => { return Ok(hi - 1); }
                        Ordering::Greater => {}
                    }
                }
                while lo < hi {
                    let mid = lo + (hi - lo) / 2;
                    match Ord::cmp(self.keys[mid].key_borrow(), k) {
                        Ordering::Less => { lo = mid + 1; }
                        Ordering::Greater => { hi = mid; }
                        Ordering::Equal => { return Ok(mid); }
                    }
                }
                Err(lo)
            }
            pub fn insert(&mut self, k: K, v: V) -> Option<V> {
                match self.search::<K>(&k) {
                    Ok(i) => Some(::core::mem::replace(&mut self.vals[i], v)),
                    Err(i) => { self.keys.insert(i, k); self.vals.insert(i, v); None }
                }
            }
            pub fn get<Q: ?Sized + Ord>(&self, k: &Q) -> Option<&V> where K: KeyBorrow<Q> {
                match self.search::<Q>(k) { Ok(i) => Some(&self.vals[i]), Err(_) => None }
            }
            pub fn get_mut<Q: ?Sized + Ord>(&mut self, k: &Q) -> Option<&mut V> where K: KeyBorrow<Q> {
                match self.search::<Q>(k) { Ok(i) => Some(&mut self.vals[i]), Err(_) => None }
            }
            pub fn get_key_value<Q: ?Sized + Ord>(&self, k: &Q) -> Option<(&K, &V)> where K: KeyBorrow<Q> {
                match self.search::<Q>(k) { Ok(i) => Some((&self.keys[i], &self.vals[i])), Err(_) => None }
            }
            pub fn contains_key<Q: ?Sized + Ord>(&self, k: &Q) -> bool where K: KeyBorrow<Q> {
                match self.search::<Q>(k) { Ok(_) => true, Err(_) => false }
            }
            pub fn remove<Q: ?Sized + Ord>(&mut self, k: &Q) -> Option<V> where K: KeyBorrow<Q> {
                match self.search::<Q>(k) {
                    Ok(i) => { let _k = self.keys.remove(i); Some(self.vals.remove(i)) }
                    Err(_) => None,
                }
            }
            pub fn remove_entry<Q: ?Sized + Ord>(&mut self, k: &Q) -> Option<(K, V)> where K: KeyBorrow<Q> {
                match self.search::<Q>(k) {
                    Ok(i) => Some((self.keys.remove(i), self.vals.remove(i))),
                    Err(_) => None,
                }
            }
            pub fn entry(&mut self, key: K) -> Entry<'_, K, V> {
                match self.search::<K>(&key) {
                    Ok(i) => Entry::Occupied(OccupiedEntry { map: self, idx: i }),
                    Err(i) => Entry::Vacant(VacantEntry { map: self, idx: i, key: key }),
                }
            }
            pub fn retain<F: FnMut(&K, &mut V) -> bool>(&mut self, mut f: F) {
                let mut i = 0usize;
                while i < self.keys.len() {
                    if f(&self.keys[i], &mut self.vals[i]) { i += 1; }
                    else { let _k = self.keys.remove(i); let _v = self.vals.remove(i); }
                }
            }
            pub fn append(&mut self, other: &mut BTreeMap<K, V>) {
                let ks = ::core::mem::replace(&mut other.keys, Vec::new());
                let vs = ::core::mem::replace(&mut other.vals, Vec::new());
                let mut ki = ks.into_iter();
                let mut vi = vs.into_iter();
                loop {
                    match ki.next() {
                        Some(k) => match vi.next() { Some(v) => { self.insert(k, v); } None => { return; } },
                        None => { return; }
                    }
                }
            }
        }

        pub enum Entry<'a, K, V> {
            Occupied(OccupiedEntry<'a, K, V>),
            Vacant(VacantEntry<'a, K, V>),
        }
        pub struct OccupiedEntry<'a, K, V> { map: &'a mut BTreeMap<K, V>, idx: usize }
        pub struct VacantEntry<'a, K, V> { map: &'a mut BTreeMap<K, V>, idx: usize, key: K }
        impl<'a, K, V> Entry<'a, K, V> {
            pub fn or_insert(self, default: V) -> &'a mut V {
                match self {
                    Entry::Occupied(e) => e.into_mut(),
                    Entry::Vacant(e) => e.insert(default),
                }
            }
            pub fn or_insert_with<F: FnOnce() -> V>(self, f: F) -> &'a mut V {
                match self {
                    Entry::Occupied(e) => e.into_mut(),
                    Entry::Vacant(e) => e.insert(f()),
                }
            }
            pub fn or_default(self) -> &'a mut V where V: Default {
                match self {
                    Entry::Occupied(e) => e.into_mut(),
                    Entry::Vacant(e) => e.insert(V::default()),
                }
            }
            pub fn and_modify<F: FnOnce(&mut V)>(self, f: F) -> Entry<'a, K, V> {
                match self {
                    Entry::Occupied(mut e) => { f(e.get_mut()); Entry::Occupied(e) }
                    Entry::Vacant(e) => Entry::Vacant(e),
                }
            }
            pub fn key(&self) -> &K {
                match self {
                    Entry::Occupied(e) => e.key(),
                    Entry::Vacant(e) => &e.key,
                }
            }
        }
        impl<'a, K, V> OccupiedEntry<'a, K, V> {
            pub fn key(&self) -> &K { &self.map.keys[self.idx] }
            pub fn get(&self) -> &V { &self.map.vals[self.idx] }
            pub fn get_mut(&mut self) -> &mut V { elem_ref_mut::<V>(self.map.vals.ptr, self.idx) }
            pub fn into_mut(self) -> &'a mut V { elem_ref_mut::<V>(self.map.vals.ptr, self.idx) }
            pub fn insert(&mut self, v: V) -> V { ::core::mem::replace(elem_ref_mut::<V>(self.map.vals.ptr, self.idx), v) }
            pub fn remove(self) -> V { let _k = self.map.keys.remove(self.idx); self.map.vals.remove(self.idx) }
        }
        impl<'a, K, V> VacantEntry<'a, K, V> {
            pub fn key(&self) -> &K { &self.key }
            pub fn insert(self, v: V) -> &'a mut V {
                self.map.keys.insert(self.idx, self.key);
                self.map.vals.insert(self.idx, v);
                elem_ref_mut::<V>(self.map.vals.ptr, self.idx)
            }
        }

        pub struct Iter<'a, K, V> { keys: &'a [K], vals: &'a [V], front: usize, back: usize }
        impl<'a, K, V> Iterator for Iter<'a, K, V> {
            type Item = (&'a K, &'a V);
            fn next(&mut self) -> Option<(&'a K, &'a V)> {
                if self.front < self.back {
                    let i = self.front;
                    self.front += 1;
                    Some((&self.keys[i], &self.vals[i]))
                } else { None }
            }
        }
        impl<'a, K, V> DoubleEndedIterator for Iter<'a, K, V> {
            fn next_back(&mut self) -> Option<(&'a K, &'a V)> {
                if self.front < self.back {
                    self.back -= 1;
                    let i = self.back;
                    Some((&self.keys[i], &self.vals[i]))
                } else { None }
            }
        }
        impl<'a, K, V> Iter<'a, K, V> { pub fn len(&self) -> usize { self.back - self.front } }
        impl<'a, K, V> Clone for Iter<'a, K, V> {
            fn clone(&self) -> Iter<'a, K, V> { Iter { keys: self.keys, vals: self.vals, front: self.front, back: self.back } }
        }
        pub struct IterMut<'a, K, V> { keys: &'a [K], vals: usize, front: usize, back: usize }
        impl<'a, K, V> Iterator for IterMut<'a, K, V> {
            type Item = (&'a K, &'a mut V);
            fn next(&mut self) -> Option<(&'a K, &'a mut V)> {
                if self.front < self.back {
                    let i = self.front;
                    self.front += 1;
                    Some((&self.keys[i], elem_ref_mut::<V>(self.vals, i)))
                } else { None }
            }
        }
        impl<'a, K, V> DoubleEndedIterator for IterMut<'a, K, V> {
            fn next_back(&mut self) -> Option<(&'a K, &'a mut V)> {
                if self.front < self.back {
                    self.back -= 1;
                    let i = self.back;
                    Some((&self.keys[i], elem_ref_mut::<V>(self.vals, i)))
                } else { None }
            }
        }
        pub struct Keys<'a, K, V> { inner: Iter<'a, K, V> }
        impl<'a, K, V> Iterator for Keys<'a, K, V> {
            type Item = &'a K;
            fn next(&mut self) -> Option<&'a K> { match self.inner.next() { Some((k, _v)) => Some(k), None => None } }
        }
        impl<'a, K, V> DoubleEndedIterator for Keys<'a, K, V> {
            fn next_back(&mut self) -> Option<&'a K> { match self.inner.next_back() { Some((k, _v)) => Some(k), None => None } }
        }
        impl<'a, K, V> Keys<'a, K, V> { pub fn len(&self) -> usize { self.inner.len() } }
        pub struct Values<'a, K, V> { inner: Iter<'a, K, V> }
        impl<'a, K, V> Iterator for Values<'a, K, V> {
            type Item = &'a V;
            fn next(&mut self) -> Option<&'a V> { match self.inner.next() { Some((_k, v)) => Some(v), None => None } }
        }
        impl<'a, K, V> DoubleEndedIterator for Values<'a, K, V> {
            fn next_back(&mut self) -> Option<&'a V> { match self.inner.next_back() { Some((_k, v)) => Some(v), None => None } }
        }
        impl<'a, K, V> Values<'a, K, V> { pub fn len(&self) -> usize { self.inner.len() } }
        pub struct ValuesMut<'a, K, V> { inner: IterMut<'a, K, V> }
        impl<'a, K, V> Iterator for ValuesMut<'a, K, V> {
            type Item = &'a mut V;
            fn next(&mut self) -> Option<&'a mut V> { match self.inner.next() { Some((_k, v)) => Some(v), None => None } }
        }
        impl<'a, K, V> DoubleEndedIterator for ValuesMut<'a, K, V> {
            fn next_back(&mut self) -> Option<&'a mut V> { match self.inner.next_back() { Some((_k, v)) => Some(v), None => None } }
        }

        pub struct IntoIter<K, V> { keys: ::core::vec::IntoIter<K>, vals: ::core::vec::IntoIter<V> }
        impl<K, V> Iterator for IntoIter<K, V> {
            type Item = (K, V);
            fn next(&mut self) -> Option<(K, V)> {
                match self.keys.next() {
                    Some(k) => match self.vals.next() { Some(v) => Some((k, v)), None => None },
                    None => None,
                }
            }
        }
        impl<K, V> DoubleEndedIterator for IntoIter<K, V> {
            fn next_back(&mut self) -> Option<(K, V)> {
                match self.keys.next_back() {
                    Some(k) => match self.vals.next_back() { Some(v) => Some((k, v)), None => None },
                    None => None,
                }
            }
        }
        impl<K, V> IntoIterator for BTreeMap<K, V> {
            type Item = (K, V);
            type IntoIter = IntoIter<K, V>;
            fn into_iter(self) -> IntoIter<K, V> {
                let m = self;
                IntoIter { keys: m.keys.into_iter(), vals: m.vals.into_iter() }
            }
        }
        impl<'a, K, V> IntoIterator for &'a BTreeMap<K, V> {
            type Item = (&'a K, &'a V);
            type IntoIter = Iter<'a, K, V>;
            fn into_iter(self) -> Iter<'a, K, V> { self.iter() }
        }
        impl<'a, K, V> IntoIterator for &'a mut BTreeMap<K, V> {
            type Item = (&'a K, &'a mut V);
            type IntoIter = IterMut<'a, K, V>;
            fn into_iter(self) -> IterMut<'a, K, V> { self.iter_mut() }
        }
        impl<K: Ord, V> FromIterator<(K, V)> for BTreeMap<K, V> {
            fn from_iter<I: IntoIterator<Item = (K, V)>>(iter: I) -> BTreeMap<K, V> {
                let mut m = BTreeMap::new();
                let mut it = iter.into_iter();
                while let Some((k, v)) = it.next() { m.insert(k, v); }
                m
            }
        }
        impl<K: Ord, V> Extend<(K, V)> for BTreeMap<K, V> {
            fn extend<I: IntoIterator<Item = (K, V)>>(&mut self, iter: I) {
                let mut it = iter.into_iter();
                while let Some((k, v)) = it.next() { self.insert(k, v); }
            }
        }
        impl<K: Ord, V, const N: usize> ::core::convert::From<[(K, V); N]> for BTreeMap<K, V> {
            fn from(a: [(K, V); N]) -> BTreeMap<K, V> {
                let mut m = BTreeMap::new();
                let mut it = Vec::from_array(a).into_iter();
                while let Some((k, v)) = it.next() { m.insert(k, v); }
                m
            }
        }
        impl<'a, K: Ord + KeyBorrow<Q>, Q: ?Sized + Ord, V> Index<&'a Q> for BTreeMap<K, V> {
            type Output = V;
            fn index(&self, k: &'a Q) -> &V {
                match self.get::<Q>(k) {
                    Some(v) => v,
                    None => ::core::panicking::panic_str("library/alloc/src/collections/btree/map.rs", 1, 1, "no entry found for key"),
                }
            }
        }
        impl<K, V> Default for BTreeMap<K, V> { fn default() -> BTreeMap<K, V> { BTreeMap::new() } }
        impl<K: Clone, V: Clone> Clone for BTreeMap<K, V> {
            fn clone(&self) -> BTreeMap<K, V> { BTreeMap { keys: self.keys.clone(), vals: self.vals.clone() } }
        }
        impl<K: PartialEq, V: PartialEq> PartialEq for BTreeMap<K, V> {
            fn eq(&self, o: &BTreeMap<K, V>) -> bool { self.keys == o.keys && self.vals == o.vals }
        }
        impl<K: Eq, V: Eq> Eq for BTreeMap<K, V> {}
        impl<K: ::core::fmt::Debug, V: ::core::fmt::Debug> ::core::fmt::Debug for BTreeMap<K, V> {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                ::core::collections::dbg::map::<K, V, Iter<'_, K, V>>(f, self.iter())
            }
        }
    }

    // ------------------------------------------------------------ BTreeSet (sorted Vec)

    pub mod btree_set {
        use ::core::clone::Clone;
        use ::core::cmp::{Eq, Ord, Ordering, PartialEq};
        use ::core::default::Default;
        use ::core::hash::KeyBorrow;
        use ::core::iter::{DoubleEndedIterator, Extend, FromIterator, IntoIterator, Iterator};
        use ::core::option::Option::{self, Some, None};
        use ::core::result::Result::{self, Ok, Err};
        use ::core::vec::Vec;

        pub struct BTreeSet<T> { items: Vec<T> }

        impl<T> BTreeSet<T> {
            pub fn new() -> BTreeSet<T> { BTreeSet { items: Vec::new() } }
            pub fn len(&self) -> usize { self.items.len() }
            pub fn is_empty(&self) -> bool { self.items.len() == 0 }
            pub fn clear(&mut self) { self.items.clear(); }
            pub fn iter(&self) -> Iter<'_, T> { Iter { items: self.items.as_slice(), front: 0, back: self.items.len() } }
            pub fn first(&self) -> Option<&T> { self.items.first() }
            pub fn last(&self) -> Option<&T> { self.items.last() }
            pub fn pop_first(&mut self) -> Option<T> {
                if self.items.len() == 0 { None } else { Some(self.items.remove(0)) }
            }
            pub fn pop_last(&mut self) -> Option<T> { self.items.pop() }
        }

        impl<T: Ord> BTreeSet<T> {
            fn search<Q: ?Sized + Ord>(&self, k: &Q) -> Result<usize, usize> where T: KeyBorrow<Q> {
                let mut lo = 0usize;
                let mut hi = self.items.len();
                if hi > 0 {
                    match Ord::cmp(self.items[hi - 1].key_borrow(), k) {
                        Ordering::Less => { return Err(hi); }
                        Ordering::Equal => { return Ok(hi - 1); }
                        Ordering::Greater => {}
                    }
                }
                while lo < hi {
                    let mid = lo + (hi - lo) / 2;
                    match Ord::cmp(self.items[mid].key_borrow(), k) {
                        Ordering::Less => { lo = mid + 1; }
                        Ordering::Greater => { hi = mid; }
                        Ordering::Equal => { return Ok(mid); }
                    }
                }
                Err(lo)
            }
            pub fn insert(&mut self, v: T) -> bool {
                match self.search::<T>(&v) {
                    Ok(_) => false,
                    Err(i) => { self.items.insert(i, v); true }
                }
            }
            pub fn replace(&mut self, v: T) -> Option<T> {
                match self.search::<T>(&v) {
                    Ok(i) => Some(::core::mem::replace(&mut self.items[i], v)),
                    Err(i) => { self.items.insert(i, v); None }
                }
            }
            pub fn contains<Q: ?Sized + Ord>(&self, v: &Q) -> bool where T: KeyBorrow<Q> {
                match self.search::<Q>(v) { Ok(_) => true, Err(_) => false }
            }
            pub fn get<Q: ?Sized + Ord>(&self, v: &Q) -> Option<&T> where T: KeyBorrow<Q> {
                match self.search::<Q>(v) { Ok(i) => Some(&self.items[i]), Err(_) => None }
            }
            pub fn remove<Q: ?Sized + Ord>(&mut self, v: &Q) -> bool where T: KeyBorrow<Q> {
                match self.search::<Q>(v) { Ok(i) => { let _x = self.items.remove(i); true } Err(_) => false }
            }
            pub fn take<Q: ?Sized + Ord>(&mut self, v: &Q) -> Option<T> where T: KeyBorrow<Q> {
                match self.search::<Q>(v) { Ok(i) => Some(self.items.remove(i)), Err(_) => None }
            }
            pub fn retain<F: ::core::ops::FnMut(&T) -> bool>(&mut self, mut f: F) {
                let mut i = 0usize;
                while i < self.items.len() {
                    if f(&self.items[i]) { i += 1; } else { let _x = self.items.remove(i); }
                }
            }
            pub fn union<'a>(&'a self, other: &'a BTreeSet<T>) -> Union<'a, T> {
                Union { a: self.items.as_slice(), b: other.items.as_slice(), i: 0, j: 0 }
            }
            pub fn intersection<'a>(&'a self, other: &'a BTreeSet<T>) -> Intersection<'a, T> {
                Intersection { a: self.items.as_slice(), b: other.items.as_slice(), i: 0, j: 0 }
            }
            pub fn difference<'a>(&'a self, other: &'a BTreeSet<T>) -> Difference<'a, T> {
                Difference { a: self.items.as_slice(), b: other.items.as_slice(), i: 0, j: 0 }
            }
            pub fn symmetric_difference<'a>(&'a self, other: &'a BTreeSet<T>) -> SymmetricDifference<'a, T> {
                SymmetricDifference { a: self.items.as_slice(), b: other.items.as_slice(), i: 0, j: 0 }
            }
            pub fn is_subset(&self, other: &BTreeSet<T>) -> bool {
                let mut it = self.iter();
                while let Some(x) = it.next() { if !other.contains::<T>(x) { return false; } }
                true
            }
            pub fn is_superset(&self, other: &BTreeSet<T>) -> bool { other.is_subset(self) }
            pub fn is_disjoint(&self, other: &BTreeSet<T>) -> bool {
                let mut it = self.iter();
                while let Some(x) = it.next() { if other.contains::<T>(x) { return false; } }
                true
            }
            pub fn append(&mut self, other: &mut BTreeSet<T>) {
                let v = ::core::mem::replace(&mut other.items, Vec::new());
                let mut it = v.into_iter();
                while let Some(x) = it.next() { self.insert(x); }
            }
        }

        pub struct Iter<'a, T> { items: &'a [T], front: usize, back: usize }
        impl<'a, T> Iterator for Iter<'a, T> {
            type Item = &'a T;
            fn next(&mut self) -> Option<&'a T> {
                if self.front < self.back { let i = self.front; self.front += 1; Some(&self.items[i]) } else { None }
            }
        }
        impl<'a, T> DoubleEndedIterator for Iter<'a, T> {
            fn next_back(&mut self) -> Option<&'a T> {
                if self.front < self.back { self.back -= 1; Some(&self.items[self.back]) } else { None }
            }
        }
        impl<'a, T> Iter<'a, T> { pub fn len(&self) -> usize { self.back - self.front } }
        impl<'a, T> Clone for Iter<'a, T> {
            fn clone(&self) -> Iter<'a, T> { Iter { items: self.items, front: self.front, back: self.back } }
        }

        pub struct Union<'a, T> { a: &'a [T], b: &'a [T], i: usize, j: usize }
        impl<'a, T: Ord> Iterator for Union<'a, T> {
            type Item = &'a T;
            fn next(&mut self) -> Option<&'a T> {
                if self.i >= self.a.len() {
                    if self.j >= self.b.len() { return None; }
                    self.j += 1; return Some(&self.b[self.j - 1]);
                }
                if self.j >= self.b.len() { self.i += 1; return Some(&self.a[self.i - 1]); }
                match Ord::cmp(&self.a[self.i], &self.b[self.j]) {
                    Ordering::Less => { self.i += 1; Some(&self.a[self.i - 1]) }
                    Ordering::Greater => { self.j += 1; Some(&self.b[self.j - 1]) }
                    Ordering::Equal => { self.i += 1; self.j += 1; Some(&self.a[self.i - 1]) }
                }
            }
        }
        pub struct Intersection<'a, T> { a: &'a [T], b: &'a [T], i: usize, j: usize }
        impl<'a, T: Ord> Iterator for Intersection<'a, T> {
            type Item = &'a T;
            fn next(&mut self) -> Option<&'a T> {
                while self.i < self.a.len() && self.j < self.b.len() {
                    match Ord::cmp(&self.a[self.i], &self.b[self.j]) {
                        Ordering::Less => { self.i += 1; }
                        Ordering::Greater => { self.j += 1; }
                        Ordering::Equal => { self.i += 1; self.j += 1; return Some(&self.a[self.i - 1]); }
                    }
                }
                None
            }
        }
        pub struct Difference<'a, T> { a: &'a [T], b: &'a [T], i: usize, j: usize }
        impl<'a, T: Ord> Iterator for Difference<'a, T> {
            type Item = &'a T;
            fn next(&mut self) -> Option<&'a T> {
                while self.i < self.a.len() {
                    if self.j >= self.b.len() { self.i += 1; return Some(&self.a[self.i - 1]); }
                    match Ord::cmp(&self.a[self.i], &self.b[self.j]) {
                        Ordering::Less => { self.i += 1; return Some(&self.a[self.i - 1]); }
                        Ordering::Greater => { self.j += 1; }
                        Ordering::Equal => { self.i += 1; self.j += 1; }
                    }
                }
                None
            }
        }
        pub struct SymmetricDifference<'a, T> { a: &'a [T], b: &'a [T], i: usize, j: usize }
        impl<'a, T: Ord> Iterator for SymmetricDifference<'a, T> {
            type Item = &'a T;
            fn next(&mut self) -> Option<&'a T> {
                loop {
                    if self.i >= self.a.len() {
                        if self.j >= self.b.len() { return None; }
                        self.j += 1; return Some(&self.b[self.j - 1]);
                    }
                    if self.j >= self.b.len() { self.i += 1; return Some(&self.a[self.i - 1]); }
                    match Ord::cmp(&self.a[self.i], &self.b[self.j]) {
                        Ordering::Less => { self.i += 1; return Some(&self.a[self.i - 1]); }
                        Ordering::Greater => { self.j += 1; return Some(&self.b[self.j - 1]); }
                        Ordering::Equal => { self.i += 1; self.j += 1; }
                    }
                }
            }
        }

        impl<T> IntoIterator for BTreeSet<T> {
            type Item = T;
            type IntoIter = ::core::vec::IntoIter<T>;
            fn into_iter(self) -> ::core::vec::IntoIter<T> { let s = self; s.items.into_iter() }
        }
        impl<'a, T> IntoIterator for &'a BTreeSet<T> {
            type Item = &'a T;
            type IntoIter = Iter<'a, T>;
            fn into_iter(self) -> Iter<'a, T> { self.iter() }
        }
        impl<T: Ord> FromIterator<T> for BTreeSet<T> {
            fn from_iter<I: IntoIterator<Item = T>>(iter: I) -> BTreeSet<T> {
                let mut s = BTreeSet::new();
                let mut it = iter.into_iter();
                while let Some(x) = it.next() { s.insert(x); }
                s
            }
        }
        impl<T: Ord> Extend<T> for BTreeSet<T> {
            fn extend<I: IntoIterator<Item = T>>(&mut self, iter: I) {
                let mut it = iter.into_iter();
                while let Some(x) = it.next() { self.insert(x); }
            }
        }
        impl<T: Ord, const N: usize> ::core::convert::From<[T; N]> for BTreeSet<T> {
            fn from(a: [T; N]) -> BTreeSet<T> {
                let mut s = BTreeSet::new();
                let mut it = Vec::from_array(a).into_iter();
                while let Some(x) = it.next() { s.insert(x); }
                s
            }
        }
        impl<T> Default for BTreeSet<T> { fn default() -> BTreeSet<T> { BTreeSet::new() } }
        impl<T: Clone> Clone for BTreeSet<T> { fn clone(&self) -> BTreeSet<T> { BTreeSet { items: self.items.clone() } } }
        impl<T: PartialEq> PartialEq for BTreeSet<T> { fn eq(&self, o: &BTreeSet<T>) -> bool { self.items == o.items } }
        impl<T: Eq> Eq for BTreeSet<T> {}
        impl<T: ::core::fmt::Debug> ::core::fmt::Debug for BTreeSet<T> {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                ::core::collections::dbg::set::<T, Iter<'_, T>>(f, self.iter())
            }
        }
    }

    // ------------------------------------------------------------ VecDeque (ring buffer)

    pub mod vec_deque {
        use ::core::clone::Clone;
        use ::core::cmp::{Eq, PartialEq};
        use ::core::default::Default;
        use ::core::intrinsics::{elem_alloc, elem_free, elem_read, elem_ref, elem_ref_mut, elem_write};
        use ::core::iter::{DoubleEndedIterator, Extend, FromIterator, IntoIterator, Iterator};
        use ::core::ops::{Drop, Index, IndexMut};
        use ::core::option::Option::{self, Some, None};
        use ::core::vec::Vec;

        // `cap` is 0 or a power of two
        pub struct VecDeque<T> { buf: usize, cap: usize, head: usize, len: usize }

        impl<T> VecDeque<T> {
            pub fn new() -> VecDeque<T> { VecDeque { buf: 0, cap: 0, head: 0, len: 0 } }
            pub fn with_capacity(n: usize) -> VecDeque<T> {
                let mut d = VecDeque::new();
                if n > 0 {
                    let mut c = 4usize;
                    while c < n { c *= 2; }
                    d.buf = elem_alloc::<T>(c);
                    d.cap = c;
                }
                d
            }
            pub fn len(&self) -> usize { self.len }
            pub fn is_empty(&self) -> bool { self.len == 0 }
            pub fn capacity(&self) -> usize { self.cap }

            fn phys(&self, i: usize) -> usize { (self.head + i) & (self.cap - 1) }
            fn grow(&mut self) {
                let nc = if self.cap == 0 { 4 } else { self.cap * 2 };
                let nb = elem_alloc::<T>(nc);
                let mut i = 0usize;
                while i < self.len {
                    elem_write::<T>(nb, i, elem_read::<T>(self.buf, self.phys(i)));
                    i += 1;
                }
                if self.cap > 0 { elem_free::<T>(self.buf); }
                self.buf = nb; self.cap = nc; self.head = 0;
            }
            pub fn push_back(&mut self, v: T) {
                if self.len == self.cap { self.grow(); }
                let p = self.phys(self.len);
                elem_write::<T>(self.buf, p, v);
                self.len += 1;
            }
            pub fn push_front(&mut self, v: T) {
                if self.len == self.cap { self.grow(); }
                self.head = (self.head + self.cap - 1) & (self.cap - 1);
                elem_write::<T>(self.buf, self.head, v);
                self.len += 1;
            }
            pub fn pop_front(&mut self) -> Option<T> {
                if self.len == 0 { return None; }
                let v = elem_read::<T>(self.buf, self.head);
                self.head = (self.head + 1) & (self.cap - 1);
                self.len -= 1;
                Some(v)
            }
            pub fn pop_back(&mut self) -> Option<T> {
                if self.len == 0 { return None; }
                self.len -= 1;
                Some(elem_read::<T>(self.buf, self.phys(self.len)))
            }
            pub fn front(&self) -> Option<&T> { self.get(0) }
            pub fn back(&self) -> Option<&T> { if self.len == 0 { None } else { self.get(self.len - 1) } }
            pub fn front_mut(&mut self) -> Option<&mut T> { self.get_mut(0) }
            pub fn back_mut(&mut self) -> Option<&mut T> {
                if self.len == 0 { None } else { let i = self.len - 1; self.get_mut(i) }
            }
            pub fn get(&self, i: usize) -> Option<&T> {
                if i < self.len { Some(elem_ref::<T>(self.buf, self.phys(i))) } else { None }
            }
            pub fn get_mut(&mut self, i: usize) -> Option<&mut T> {
                if i < self.len { Some(elem_ref_mut::<T>(self.buf, self.phys(i))) } else { None }
            }
            pub fn clear(&mut self) { while self.len > 0 { let _x = self.pop_back(); } self.head = 0; }
            pub fn truncate(&mut self, n: usize) { while self.len > n { let _x = self.pop_back(); } }
            pub fn swap(&mut self, a: usize, b: usize) {
                if a >= self.len || b >= self.len {
                    ::core::panicking::panic_str("library/alloc/src/collections/vec_deque/mod.rs", 1, 1, "assertion failed: i < self.len()");
                }
                let pa = self.phys(a);
                let pb = self.phys(b);
                let x = elem_read::<T>(self.buf, pa);
                let y = elem_read::<T>(self.buf, pb);
                elem_write::<T>(self.buf, pa, y);
                elem_write::<T>(self.buf, pb, x);
            }
            pub fn iter(&self) -> Iter<'_, T> { Iter { dq: self, front: 0, back: self.len } }
            pub fn iter_mut(&mut self) -> IterMut<'_, T> {
                let n = self.len;
                IterMut { buf: self.buf, cap: self.cap, head: self.head, front: 0, back: n, dq: self }
            }
            pub fn contains(&self, x: &T) -> bool where T: PartialEq {
                let mut i = 0usize;
                while i < self.len { if *elem_ref::<T>(self.buf, self.phys(i)) == *x { return true; } i += 1; }
                false
            }
            pub fn append(&mut self, other: &mut VecDeque<T>) {
                while let Some(x) = other.pop_front() { self.push_back(x); }
            }
            pub fn rotate_left(&mut self, n: usize) {
                let mut k = 0usize;
                while k < n { match self.pop_front() { Some(x) => { self.push_back(x); } None => {} } k += 1; }
            }
            pub fn rotate_right(&mut self, n: usize) {
                let mut k = 0usize;
                while k < n { match self.pop_back() { Some(x) => { self.push_front(x); } None => {} } k += 1; }
            }
            pub fn make_contiguous(&mut self) -> &mut [T] {
                if self.head + self.len > self.cap {
                    let nb = elem_alloc::<T>(self.cap);
                    let mut i = 0usize;
                    while i < self.len { elem_write::<T>(nb, i, elem_read::<T>(self.buf, self.phys(i))); i += 1; }
                    elem_free::<T>(self.buf);
                    self.buf = nb; self.head = 0;
                }
                if self.cap == 0 { return ::core::intrinsics::slice_from_raw_mut::<T>(0, 0); }
                ::core::intrinsics::slice_from_raw_mut::<T>(::core::intrinsics::elem_offset::<T>(self.buf, self.head), self.len)
            }
            pub fn retain<F: ::core::ops::FnMut(&T) -> bool>(&mut self, mut f: F) {
                let n = self.len;
                let mut k = 0usize;
                while k < n {
                    match self.pop_front() {
                        Some(x) => { if f(&x) { self.push_back(x); } }
                        None => {}
                    }
                    k += 1;
                }
            }
        }

        impl<T> Drop for VecDeque<T> {
            fn drop(&mut self) {
                self.clear();
                if self.cap > 0 { elem_free::<T>(self.buf); }
            }
        }
        impl<T> Index<usize> for VecDeque<T> {
            type Output = T;
            fn index(&self, i: usize) -> &T {
                if i >= self.len { ::core::panicking::panic_str("library/alloc/src/collections/vec_deque/mod.rs", 1, 1, "Out of bounds access"); }
                elem_ref::<T>(self.buf, self.phys(i))
            }
        }
        impl<T> IndexMut<usize> for VecDeque<T> {
            fn index_mut(&mut self, i: usize) -> &mut T {
                if i >= self.len { ::core::panicking::panic_str("library/alloc/src/collections/vec_deque/mod.rs", 1, 1, "Out of bounds access"); }
                elem_ref_mut::<T>(self.buf, self.phys(i))
            }
        }

        pub struct Iter<'a, T> { dq: &'a VecDeque<T>, front: usize, back: usize }
        impl<'a, T> Iterator for Iter<'a, T> {
            type Item = &'a T;
            fn next(&mut self) -> Option<&'a T> {
                if self.front < self.back {
                    let i = self.front;
                    self.front += 1;
                    Some(elem_ref::<T>(self.dq.buf, self.dq.phys(i)))
                } else { None }
            }
        }
        impl<'a, T> DoubleEndedIterator for Iter<'a, T> {
            fn next_back(&mut self) -> Option<&'a T> {
                if self.front < self.back {
                    self.back -= 1;
                    Some(elem_ref::<T>(self.dq.buf, self.dq.phys(self.back)))
                } else { None }
            }
        }
        impl<'a, T> Iter<'a, T> { pub fn len(&self) -> usize { self.back - self.front } }
        impl<'a, T> Clone for Iter<'a, T> {
            fn clone(&self) -> Iter<'a, T> { Iter { dq: self.dq, front: self.front, back: self.back } }
        }
        pub struct IterMut<'a, T> { dq: &'a mut VecDeque<T>, buf: usize, cap: usize, head: usize, front: usize, back: usize }
        impl<'a, T> Iterator for IterMut<'a, T> {
            type Item = &'a mut T;
            fn next(&mut self) -> Option<&'a mut T> {
                if self.front < self.back {
                    let i = self.front;
                    self.front += 1;
                    Some(elem_ref_mut::<T>(self.buf, (self.head + i) & (self.cap - 1)))
                } else { None }
            }
        }
        impl<'a, T> DoubleEndedIterator for IterMut<'a, T> {
            fn next_back(&mut self) -> Option<&'a mut T> {
                if self.front < self.back {
                    self.back -= 1;
                    Some(elem_ref_mut::<T>(self.buf, (self.head + self.back) & (self.cap - 1)))
                } else { None }
            }
        }
        pub struct IntoIter<T> { dq: VecDeque<T> }
        impl<T> Iterator for IntoIter<T> {
            type Item = T;
            fn next(&mut self) -> Option<T> { self.dq.pop_front() }
        }
        impl<T> DoubleEndedIterator for IntoIter<T> {
            fn next_back(&mut self) -> Option<T> { self.dq.pop_back() }
        }
        impl<T> IntoIterator for VecDeque<T> {
            type Item = T;
            type IntoIter = IntoIter<T>;
            fn into_iter(self) -> IntoIter<T> { IntoIter { dq: self } }
        }
        impl<'a, T> IntoIterator for &'a VecDeque<T> {
            type Item = &'a T;
            type IntoIter = Iter<'a, T>;
            fn into_iter(self) -> Iter<'a, T> { self.iter() }
        }
        impl<'a, T> IntoIterator for &'a mut VecDeque<T> {
            type Item = &'a mut T;
            type IntoIter = IterMut<'a, T>;
            fn into_iter(self) -> IterMut<'a, T> { self.iter_mut() }
        }
        impl<T> FromIterator<T> for VecDeque<T> {
            fn from_iter<I: IntoIterator<Item = T>>(iter: I) -> VecDeque<T> {
                let mut d = VecDeque::new();
                let mut it = iter.into_iter();
                while let Some(x) = it.next() { d.push_back(x); }
                d
            }
        }
        impl<T> Extend<T> for VecDeque<T> {
            fn extend<I: IntoIterator<Item = T>>(&mut self, iter: I) {
                let mut it = iter.into_iter();
                while let Some(x) = it.next() { self.push_back(x); }
            }
        }
        impl<T> ::core::convert::From<Vec<T>> for VecDeque<T> {
            fn from(v: Vec<T>) -> VecDeque<T> {
                let mut d = VecDeque::with_capacity(v.len());
                let mut it = v.into_iter();
                while let Some(x) = it.next() { d.push_back(x); }
                d
            }
        }
        impl<T, const N: usize> ::core::convert::From<[T; N]> for VecDeque<T> {
            fn from(a: [T; N]) -> VecDeque<T> { VecDeque::from(Vec::from_array(a)) }
        }
        impl<T> ::core::convert::From<VecDeque<T>> for Vec<T> {
            fn from(d: VecDeque<T>) -> Vec<T> {
                let mut v = Vec::with_capacity(d.len());
                let mut it = d.into_iter();
                while let Some(x) = it.next() { v.push(x); }
                v
            }
        }
        impl<T> Default for VecDeque<T> { fn default() -> VecDeque<T> { VecDeque::new() } }
        impl<T: Clone> Clone for VecDeque<T> {
            fn clone(&self) -> VecDeque<T> {
                let mut d = VecDeque::with_capacity(self.len);
                let mut it = self.iter();
                while let Some(x) = it.next() { d.push_back(x.clone()); }
                d
            }
        }
        impl<T: PartialEq> PartialEq for VecDeque<T> {
            fn eq(&self, o: &VecDeque<T>) -> bool {
                if self.len != o.len { return false; }
                let mut i = 0usize;
                while i < self.len {
                    if *elem_ref::<T>(self.buf, self.phys(i)) != *elem_ref::<T>(o.buf, o.phys(i)) { return false; }
                    i += 1;
                }
                true
            }
        }
        impl<T: Eq> Eq for VecDeque<T> {}
        impl<T: ::core::fmt::Debug> ::core::fmt::Debug for VecDeque<T> {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                f.debug_list().entries(self.iter()).finish()
            }
        }
    }

    // ------------------------------------------------------------ BinaryHeap (max-heap)

    pub mod binary_heap {
        use ::core::clone::Clone;
        use ::core::cmp::{Ord, Ordering};
        use ::core::default::Default;
        use ::core::iter::{Extend, FromIterator, IntoIterator, Iterator};
        use ::core::option::Option::{self, Some, None};
        use ::core::vec::Vec;

        pub struct BinaryHeap<T> { data: Vec<T> }

        impl<T> BinaryHeap<T> {
            pub fn len(&self) -> usize { self.data.len() }
            pub fn is_empty(&self) -> bool { self.data.len() == 0 }
            pub fn clear(&mut self) { self.data.clear(); }
            pub fn peek(&self) -> Option<&T> { self.data.first() }
            pub fn iter(&self) -> ::core::slice::Iter<'_, T> { self.data.iter() }
            pub fn into_vec(self) -> Vec<T> { let h = self; h.data }
            pub fn capacity(&self) -> usize { self.data.capacity() }
        }
        impl<T: Ord> BinaryHeap<T> {
            pub fn new() -> BinaryHeap<T> { BinaryHeap { data: Vec::new() } }
            pub fn with_capacity(n: usize) -> BinaryHeap<T> { BinaryHeap { data: Vec::with_capacity(n) } }
            fn less(&self, a: usize, b: usize) -> bool {
                match Ord::cmp(&self.data[a], &self.data[b]) { Ordering::Less => true, _ => false }
            }
            fn sift_up(&mut self, start: usize) {
                let mut i = start;
                while i > 0 {
                    let p = (i - 1) / 2;
                    if self.less(p, i) { self.data.swap(p, i); i = p; } else { break; }
                }
            }
            fn sift_down(&mut self, start: usize) {
                let n = self.data.len();
                let mut i = start;
                loop {
                    let l = 2 * i + 1;
                    if l >= n { break; }
                    let mut c = l;
                    if l + 1 < n && self.less(l, l + 1) { c = l + 1; }
                    if self.less(i, c) { self.data.swap(i, c); i = c; } else { break; }
                }
            }
            pub fn push(&mut self, v: T) {
                self.data.push(v);
                let n = self.data.len();
                self.sift_up(n - 1);
            }
            pub fn pop(&mut self) -> Option<T> {
                let n = self.data.len();
                if n == 0 { return None; }
                self.data.swap(0, n - 1);
                let v = self.data.pop();
                if n > 1 { self.sift_down(0); }
                v
            }
            pub fn into_sorted_vec(self) -> Vec<T> {
                let h = self;
                let mut v = h.data;
                v.sort();
                v
            }
            pub fn append(&mut self, other: &mut BinaryHeap<T>) {
                while let Some(x) = other.data.pop() { self.push(x); }
            }
            pub fn drain(&mut self) -> ::core::vec::IntoIter<T> {
                ::core::mem::replace(&mut self.data, Vec::new()).into_iter()
            }
        }
        impl<T: Ord> FromIterator<T> for BinaryHeap<T> {
            fn from_iter<I: IntoIterator<Item = T>>(iter: I) -> BinaryHeap<T> {
                let mut h = BinaryHeap::new();
                let mut it = iter.into_iter();
                while let Some(x) = it.next() { h.push(x); }
                h
            }
        }
        impl<T: Ord> Extend<T> for BinaryHeap<T> {
            fn extend<I: IntoIterator<Item = T>>(&mut self, iter: I) {
                let mut it = iter.into_iter();
                while let Some(x) = it.next() { self.push(x); }
            }
        }
        impl<T: Ord> ::core::convert::From<Vec<T>> for BinaryHeap<T> {
            fn from(v: Vec<T>) -> BinaryHeap<T> {
                let mut h = BinaryHeap { data: v };
                let n = h.data.len();
                let mut i = n / 2;
                while i > 0 { i -= 1; h.sift_down(i); }
                h
            }
        }
        impl<T: Ord, const N: usize> ::core::convert::From<[T; N]> for BinaryHeap<T> {
            fn from(a: [T; N]) -> BinaryHeap<T> { BinaryHeap::from(Vec::from_array(a)) }
        }
        impl<T> IntoIterator for BinaryHeap<T> {
            type Item = T;
            type IntoIter = ::core::vec::IntoIter<T>;
            fn into_iter(self) -> ::core::vec::IntoIter<T> { let h = self; h.data.into_iter() }
        }
        impl<'a, T> IntoIterator for &'a BinaryHeap<T> {
            type Item = &'a T;
            type IntoIter = ::core::slice::Iter<'a, T>;
            fn into_iter(self) -> ::core::slice::Iter<'a, T> { self.data.iter() }
        }
        impl<T: Ord> Default for BinaryHeap<T> { fn default() -> BinaryHeap<T> { BinaryHeap::new() } }
        impl<T: Clone> Clone for BinaryHeap<T> { fn clone(&self) -> BinaryHeap<T> { BinaryHeap { data: self.data.clone() } } }
        impl<T: ::core::fmt::Debug> ::core::fmt::Debug for BinaryHeap<T> {
            fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
                f.debug_list().entries(self.data.iter()).finish()
            }
        }
    }
}
