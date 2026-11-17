// traits: definitions, default methods, impls for several types, static dispatch
trait Animal {
    fn name(&self) -> String;
    fn legs(&self) -> u32;
    fn describe(&self) -> String {
        format!("{} has {} legs", self.name(), self.legs())
    }
}

struct Dog;
struct Bird {
    species: String,
}

impl Animal for Dog {
    fn name(&self) -> String {
        "dog".to_string()
    }
    fn legs(&self) -> u32 {
        4
    }
}

impl Animal for Bird {
    fn name(&self) -> String {
        self.species.clone()
    }
    fn legs(&self) -> u32 {
        2
    }
    fn describe(&self) -> String {
        format!("a {} flies", self.species)
    }
}

fn show<T: Animal>(a: &T) {
    println!("{}", a.describe());
}

fn total_legs<A: Animal, B: Animal>(a: &A, b: &B) -> u32 {
    a.legs() + b.legs()
}

trait Zero {
    fn zero() -> Self;
    fn is_zero(&self) -> bool;
}

impl Zero for i32 {
    fn zero() -> i32 {
        0
    }
    fn is_zero(&self) -> bool {
        *self == 0
    }
}

impl Zero for f64 {
    fn zero() -> f64 {
        0.0
    }
    fn is_zero(&self) -> bool {
        *self == 0.0
    }
}

trait Double {
    fn double(&self) -> Self;
}

impl Double for i32 {
    fn double(&self) -> i32 {
        self * 2
    }
}

impl Double for String {
    fn double(&self) -> String {
        format!("{}{}", self, self)
    }
}

trait Named {
    const KIND: &'static str;
    fn kind(&self) -> &'static str {
        Self::KIND
    }
}

impl Named for Dog {
    const KIND: &'static str = "canine";
}

fn main() {
    let d = Dog;
    let b = Bird { species: "owl".to_string() };
    show(&d);
    show(&b);
    println!("{}", total_legs(&d, &b));
    println!("{} {}", i32::zero().is_zero(), 1.5f64.is_zero());
    println!("{} {}", 21.double(), "ab".to_string().double());
    println!("{}", d.kind());
    println!("{}", <Dog as Named>::KIND);
    println!("{}", <i32 as Zero>::zero());
}
