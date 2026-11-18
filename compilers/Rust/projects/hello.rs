struct Greeting {
    language: &'static str,
    text: &'static str,
}

fn main() {
    let greetings = [
        Greeting { language: "English",  text: "Hello, world!" },
        Greeting { language: "Spanish",  text: "¡Hola, mundo!" },
        Greeting { language: "French",   text: "Bonjour le monde !" },
        Greeting { language: "Greek",    text: "Γειά σου, κόσμε!" },
        Greeting { language: "Russian",  text: "Привет, мир!" },
        Greeting { language: "Japanese", text: "こんにちは、世界！" },
        Greeting { language: "Chinese",  text: "你好，世界！" },
    ];

    for (i, g) in greetings.iter().enumerate() {
        let label = format!("{}:", g.language);
        println!("{:<9} {}", label, g.text);
    }

    println!("\nThat's {} languages!", greetings.len());
}
