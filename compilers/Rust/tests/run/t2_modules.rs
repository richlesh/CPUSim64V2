// modules, use, pub, nested paths, consts in impls, associated consts
mod geometry {
    pub const UNIT: i32 = 10;

    pub struct Square {
        pub side: i32,
    }

    impl Square {
        pub const SIDES: u32 = 4;
        pub fn area(&self) -> i32 {
            self.side * self.side
        }
        pub fn perimeter(&self) -> i32 {
            Self::SIDES as i32 * self.side
        }
    }

    pub mod util {
        pub fn double(x: i32) -> i32 {
            x * 2
        }
        pub fn scaled(x: i32) -> i32 {
            double(x) * super::UNIT
        }
    }
}

mod counter {
    pub struct Counter {
        n: u32,
    }
    impl Counter {
        pub fn new() -> Counter {
            Counter { n: 0 }
        }
        pub fn tick(&mut self) -> u32 {
            self.n += 1;
            self.n
        }
    }
}

use geometry::util::double;
use geometry::Square;

fn main() {
    let s = Square { side: 6 };
    println!("{} {}", s.area(), s.perimeter());
    println!("{}", Square::SIDES);
    println!("{}", double(21));
    println!("{}", geometry::util::scaled(3));
    println!("{}", geometry::UNIT);
    let mut c = counter::Counter::new();
    c.tick();
    c.tick();
    println!("{}", c.tick());
}
