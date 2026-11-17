// small state machines: a traffic light, a vending machine, a text parser with states
#[derive(Debug, Clone, Copy, PartialEq)]
enum Light {
    Red,
    Green,
    Yellow,
}

impl Light {
    fn next(self) -> Light {
        match self {
            Light::Red => Light::Green,
            Light::Green => Light::Yellow,
            Light::Yellow => Light::Red,
        }
    }
    fn secs(self) -> u32 {
        match self {
            Light::Red => 30,
            Light::Green => 25,
            Light::Yellow => 5,
        }
    }
}

#[derive(Debug, PartialEq, Clone, Copy)]
enum Vend {
    Idle,
    HasMoney(u32),
    Dispensing(u32),
    OutOfOrder,
}

#[derive(Debug, Clone, Copy)]
enum Event {
    Coin(u32),
    Select,
    Take,
    Kick,
}

const PRICE: u32 = 75;

fn step(s: Vend, e: Event) -> Vend {
    match (s, e) {
        (Vend::OutOfOrder, _) => Vend::OutOfOrder,
        (_, Event::Kick) => Vend::OutOfOrder,
        (Vend::Idle, Event::Coin(c)) => Vend::HasMoney(c),
        (Vend::HasMoney(m), Event::Coin(c)) => Vend::HasMoney(m + c),
        (Vend::HasMoney(m), Event::Select) if m >= PRICE => Vend::Dispensing(m - PRICE),
        (Vend::HasMoney(m), Event::Select) => Vend::HasMoney(m),
        (Vend::Dispensing(change), Event::Take) => {
            if change > 0 {
                Vend::HasMoney(change)
            } else {
                Vend::Idle
            }
        }
        (other, _) => other,
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
enum St {
    Start,
    InNumber,
    InWord,
    InSpace,
}

fn classify(s: &str) -> (u32, u32) {
    let mut st = St::Start;
    let mut nums = 0;
    let mut words = 0;
    for c in s.chars() {
        let next = if c.is_ascii_digit() {
            match st {
                St::InWord => St::InWord,
                _ => St::InNumber,
            }
        } else if c.is_alphabetic() {
            St::InWord
        } else {
            St::InSpace
        };
        if next != st {
            match next {
                St::InNumber => nums += 1,
                St::InWord => words += 1,
                _ => {}
            }
        }
        st = next;
    }
    (nums, words)
}

fn main() {
    let mut l = Light::Red;
    let mut t = 0;
    for _ in 0..7 {
        print!("{:?}({}) ", l, l.secs());
        t += l.secs();
        l = l.next();
    }
    println!();
    println!("{}", t);

    let events = [
        Event::Select,
        Event::Coin(50),
        Event::Select,
        Event::Coin(50),
        Event::Select,
        Event::Take,
        Event::Take,
        Event::Coin(100),
        Event::Select,
        Event::Take,
        Event::Take,
        Event::Kick,
        Event::Coin(5),
    ];
    let mut s = Vend::Idle;
    for e in events.iter() {
        let n = step(s, *e);
        println!("{:?} --{:?}--> {:?}", s, e, n);
        s = n;
    }
    println!("{:?}", classify("abc 123 def45 6 x"));
    println!("{:?}", classify(""));
}
