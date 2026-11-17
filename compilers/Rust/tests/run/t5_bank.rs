// bank account simulation: accounts, transactions, errors, audit log
use std::fmt;

#[derive(Debug, Clone, PartialEq)]
enum TxError {
    Insufficient { needed: u64, available: u64 },
    NoSuchAccount(u32),
    Frozen(u32),
    ZeroAmount,
}

impl fmt::Display for TxError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            TxError::Insufficient { needed, available } => {
                write!(f, "insufficient funds: need {} have {}", needed, available)
            }
            TxError::NoSuchAccount(id) => write!(f, "no such account #{}", id),
            TxError::Frozen(id) => write!(f, "account #{} is frozen", id),
            TxError::ZeroAmount => write!(f, "amount must be positive"),
        }
    }
}

#[derive(Debug)]
struct Account {
    id: u32,
    owner: String,
    balance: u64,
    frozen: bool,
    history: Vec<String>,
}

struct Bank {
    accounts: Vec<Account>,
    next_id: u32,
    log: Vec<String>,
}

impl Bank {
    fn new() -> Bank {
        Bank { accounts: Vec::new(), next_id: 100, log: Vec::new() }
    }

    fn open(&mut self, owner: &str, initial: u64) -> u32 {
        let id = self.next_id;
        self.next_id += 1;
        self.accounts.push(Account {
            id,
            owner: owner.to_string(),
            balance: initial,
            frozen: false,
            history: vec![format!("open {}", initial)],
        });
        self.log.push(format!("opened #{} for {}", id, owner));
        id
    }

    fn find(&mut self, id: u32) -> Result<&mut Account, TxError> {
        for a in self.accounts.iter_mut() {
            if a.id == id {
                return Ok(a);
            }
        }
        Err(TxError::NoSuchAccount(id))
    }

    fn deposit(&mut self, id: u32, amount: u64) -> Result<u64, TxError> {
        if amount == 0 {
            return Err(TxError::ZeroAmount);
        }
        let acct = self.find(id)?;
        if acct.frozen {
            return Err(TxError::Frozen(id));
        }
        acct.balance += amount;
        acct.history.push(format!("deposit {}", amount));
        Ok(acct.balance)
    }

    fn withdraw(&mut self, id: u32, amount: u64) -> Result<u64, TxError> {
        if amount == 0 {
            return Err(TxError::ZeroAmount);
        }
        let acct = self.find(id)?;
        if acct.frozen {
            return Err(TxError::Frozen(id));
        }
        if acct.balance < amount {
            return Err(TxError::Insufficient { needed: amount, available: acct.balance });
        }
        acct.balance -= amount;
        acct.history.push(format!("withdraw {}", amount));
        Ok(acct.balance)
    }

    fn transfer(&mut self, from: u32, to: u32, amount: u64) -> Result<(), TxError> {
        self.find(to)?;
        self.withdraw(from, amount)?;
        match self.deposit(to, amount) {
            Ok(_) => Ok(()),
            Err(e) => {
                // roll back
                self.find(from)?.balance += amount;
                Err(e)
            }
        }
    }

    fn total(&self) -> u64 {
        self.accounts.iter().map(|a| a.balance).sum()
    }
}

fn report(r: Result<u64, TxError>) {
    match r {
        Ok(b) => println!("ok, balance {}", b),
        Err(e) => println!("error: {}", e),
    }
}

fn main() {
    let mut bank = Bank::new();
    let alice = bank.open("alice", 500);
    let bob = bank.open("bob", 50);
    report(bank.deposit(alice, 250));
    report(bank.withdraw(bob, 80));
    report(bank.withdraw(bob, 20));
    report(bank.deposit(999, 5));
    report(bank.deposit(alice, 0));
    match bank.transfer(alice, bob, 300) {
        Ok(()) => println!("transfer ok"),
        Err(e) => println!("transfer failed: {}", e),
    }
    match bank.transfer(bob, alice, 1000) {
        Ok(()) => println!("transfer ok"),
        Err(e) => println!("transfer failed: {}", e),
    }
    bank.find(bob).unwrap().frozen = true;
    match bank.transfer(alice, bob, 10) {
        Ok(()) => println!("transfer ok"),
        Err(e) => println!("transfer failed: {}", e),
    }
    for a in &bank.accounts {
        println!("#{} {:<6} {:>6} {}", a.id, a.owner, a.balance, a.frozen);
        for h in &a.history {
            println!("    {}", h);
        }
    }
    println!("total {}", bank.total());
    println!("{}", bank.log.join("; "));
    println!("{:?}", TxError::Insufficient { needed: 1, available: 0 });
}
