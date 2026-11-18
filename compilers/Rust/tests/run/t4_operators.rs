// operator overloading: Add, Sub, Mul, Neg, Index, IndexMut, AddAssign, PartialEq, PartialOrd, Not
use std::ops::{Add, AddAssign, Index, IndexMut, Mul, Neg, Not, Sub};

#[derive(Debug, Clone, Copy, PartialEq)]
struct V2 {
    x: i32,
    y: i32,
}

impl Add for V2 {
    type Output = V2;
    fn add(self, o: V2) -> V2 {
        V2 { x: self.x + o.x, y: self.y + o.y }
    }
}

impl Sub for V2 {
    type Output = V2;
    fn sub(self, o: V2) -> V2 {
        V2 { x: self.x - o.x, y: self.y - o.y }
    }
}

impl Mul<i32> for V2 {
    type Output = V2;
    fn mul(self, k: i32) -> V2 {
        V2 { x: self.x * k, y: self.y * k }
    }
}

impl Mul<V2> for V2 {
    type Output = i32;
    fn mul(self, o: V2) -> i32 {
        self.x * o.x + self.y * o.y
    }
}

impl Neg for V2 {
    type Output = V2;
    fn neg(self) -> V2 {
        V2 { x: -self.x, y: -self.y }
    }
}

impl AddAssign for V2 {
    fn add_assign(&mut self, o: V2) {
        self.x += o.x;
        self.y += o.y;
    }
}

struct Grid {
    w: usize,
    cells: Vec<i32>,
}

impl Index<(usize, usize)> for Grid {
    type Output = i32;
    fn index(&self, (r, c): (usize, usize)) -> &i32 {
        &self.cells[r * self.w + c]
    }
}

impl IndexMut<(usize, usize)> for Grid {
    fn index_mut(&mut self, (r, c): (usize, usize)) -> &mut i32 {
        &mut self.cells[r * self.w + c]
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct Flags(u8);

impl Not for Flags {
    type Output = Flags;
    fn not(self) -> Flags {
        Flags(!self.0)
    }
}

#[derive(Debug, Clone, Copy)]
struct Money {
    cents: i64,
}

impl PartialEq for Money {
    fn eq(&self, o: &Money) -> bool {
        self.cents == o.cents
    }
}

impl PartialOrd for Money {
    fn partial_cmp(&self, o: &Money) -> Option<std::cmp::Ordering> {
        self.cents.partial_cmp(&o.cents)
    }
}

impl<'a> Add<&'a Money> for &'a Money {
    type Output = Money;
    fn add(self, o: &Money) -> Money {
        Money { cents: self.cents + o.cents }
    }
}

fn main() {
    let a = V2 { x: 1, y: 2 };
    let b = V2 { x: 10, y: 20 };
    println!("{:?}", a + b);
    println!("{:?}", b - a);
    println!("{:?}", a * 3);
    println!("{}", a * b);
    println!("{:?}", -a);
    let mut c = a;
    c += b;
    c += b;
    println!("{:?}", c);
    println!("{}", a + b == V2 { x: 11, y: 22 });
    println!("{}", a != b);

    let mut g = Grid { w: 3, cells: vec![0; 6] };
    g[(1, 2)] = 7;
    g[(0, 0)] += 5;
    println!("{} {} {}", g[(1, 2)], g[(0, 0)], g[(0, 1)]);
    println!("{:?}", g.cells);
    println!("{:?}", !Flags(0b1010_0000));

    let m1 = Money { cents: 150 };
    let m2 = Money { cents: 275 };
    println!("{}", m1 < m2);
    println!("{}", m1 >= m2);
    println!("{:?}", &m1 + &m2);
    let total = [a, b, c].iter().fold(V2 { x: 0, y: 0 }, |acc, v| acc + *v);
    println!("{:?}", total);
}
