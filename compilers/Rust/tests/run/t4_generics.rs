// generics: functions, structs, impls with bounds, where clauses, PhantomData-free
use std::fmt::Debug;
use std::fmt::Display;

fn largest<T: PartialOrd + Copy>(v: &[T]) -> T {
    let mut m = v[0];
    for &x in v {
        if x > m {
            m = x;
        }
    }
    m
}

fn pair_up<A: Clone, B: Clone>(a: &A, b: &B) -> (A, B) {
    (a.clone(), b.clone())
}

fn print_all<T>(items: &[T])
where
    T: Display,
{
    let mut first = true;
    for it in items {
        if !first {
            print!(", ");
        }
        print!("{}", it);
        first = false;
    }
    println!();
}

fn debug_str<T: Debug>(x: T) -> String {
    format!("<{:?}>", x)
}

struct Stack<T> {
    items: Vec<T>,
}

impl<T> Stack<T> {
    fn new() -> Self {
        Stack { items: Vec::new() }
    }
    fn push(&mut self, x: T) {
        self.items.push(x);
    }
    fn pop(&mut self) -> Option<T> {
        self.items.pop()
    }
    fn peek(&self) -> Option<&T> {
        self.items.last()
    }
    fn len(&self) -> usize {
        self.items.len()
    }
    fn is_empty(&self) -> bool {
        self.items.is_empty()
    }
}

impl<T: Display> Stack<T> {
    fn render(&self) -> String {
        let parts: Vec<String> = self.items.iter().map(|x| x.to_string()).collect();
        format!("[{}]", parts.join("|"))
    }
}

struct Pair<T> {
    a: T,
    b: T,
}

impl<T: PartialOrd + Copy> Pair<T> {
    fn max(&self) -> T {
        if self.a > self.b {
            self.a
        } else {
            self.b
        }
    }
    fn swap(self) -> Pair<T> {
        Pair { a: self.b, b: self.a }
    }
}

enum Either<L, R> {
    Left(L),
    Right(R),
}

impl<L: Display, R: Display> Either<L, R> {
    fn show(&self) -> String {
        match self {
            Either::Left(l) => format!("L({})", l),
            Either::Right(r) => format!("R({})", r),
        }
    }
}

fn map_opt<T, U, F: Fn(T) -> U>(o: Option<T>, f: F) -> Option<U> {
    match o {
        Some(x) => Some(f(x)),
        None => None,
    }
}

fn main() {
    println!("{} {} {}", largest(&[3, 9, 2]), largest(&[1.5, 0.5]), largest(&['x', 'b']));
    println!("{:?}", pair_up(&1, &"s".to_string()));
    print_all(&[1, 2, 3]);
    print_all(&["a", "b"]);
    println!("{} {}", debug_str("q"), debug_str(vec![1, 2]));
    let mut s: Stack<i32> = Stack::new();
    s.push(1);
    s.push(2);
    s.push(3);
    println!("{} {:?} {}", s.len(), s.peek(), s.render());
    println!("{:?}", s.pop());
    println!("{}", s.is_empty());
    let mut names = Stack::new();
    names.push(String::from("x"));
    println!("{}", names.render());
    let p = Pair { a: 3, b: 8 };
    println!("{}", p.max());
    let q = p.swap();
    println!("{} {}", q.a, q.b);
    let e1: Either<i32, String> = Either::Left(5);
    let e2: Either<i32, String> = Either::Right("r".to_string());
    println!("{} {}", e1.show(), e2.show());
    println!("{:?}", map_opt(Some(4), |x| x as f64 / 8.0));
}
