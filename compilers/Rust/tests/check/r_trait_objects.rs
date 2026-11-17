trait Animal {
    fn name(&self) -> String;
    fn sound(&self) -> String;
    fn legs(&self) -> u32 { 4 }
    fn describe(&self) -> String { format!("{} says {} on {} legs", self.name(), self.sound(), self.legs()) }
}
struct Dog { name: String }
struct Bird;
struct Cow { weight: f64 }
impl Animal for Dog { fn name(&self) -> String { self.name.clone() } fn sound(&self) -> String { "woof".to_string() } }
impl Animal for Bird { fn name(&self) -> String { "bird".into() } fn sound(&self) -> String { String::from("tweet") } fn legs(&self) -> u32 { 2 } }
impl Animal for Cow { fn name(&self) -> String { "cow".to_string() } fn sound(&self) -> String { "moo".to_string() } }
fn loudest(zoo: &Vec<Box<dyn Animal>>) -> Option<&Box<dyn Animal>> {
    let mut best: Option<&Box<dyn Animal>> = None;
    for a in zoo.iter() {
        if best.is_none() || a.sound().len() > best.unwrap().sound().len() { best = Some(a); }
    }
    best
}
fn total_legs(zoo: &[Box<dyn Animal>]) -> u32 { zoo.iter().map(|a| a.legs()).sum() }
fn main() {
    let mut zoo: Vec<Box<dyn Animal>> = Vec::new();
    zoo.push(Box::new(Dog { name: "rex".to_string() }));
    zoo.push(Box::new(Bird));
    zoo.push(Box::new(Cow { weight: 500.0 }));
    for a in &zoo { println!("{}", a.describe()); }
    println!("{} {}", total_legs(&zoo), loudest(&zoo).map(|a| a.name()).unwrap_or_default());
}
