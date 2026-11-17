trait Speak {
    fn speak(&self) -> String;
}

struct Dog;
struct Rock;

impl Speak for Dog {
    fn speak(&self) -> String {
        String::from("woof")
    }
}

fn pet() -> impl Speak {
    Rock
}

fn main() {
    let _d = Dog;
    println!("{}", pet().speak());
}
