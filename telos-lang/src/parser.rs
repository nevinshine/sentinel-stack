use pest::Parser;
use pest_derive::Parser;

#[derive(Parser)]
#[grammar = "telos.pest"] // Binds the PEG to this struct
pub struct TelosParser;

pub fn parse_telos_file(source: &str) {
    println!("Tokenizing source code...");

    let parse_result = TelosParser::parse(Rule::program, source);

    match parse_result {
        Ok(mut pairs) => {
            println!("[PASS] Syntax is valid.");
            let program = pairs.next().unwrap();

            for rule in program.into_inner() {
                match rule.as_rule() {
                    Rule::policy_block => println!("-> Found Policy Block"),
                    Rule::fn_decl => {
                        println!("-> Found Function Declaration");
                        for inner in rule.into_inner() {
                            if inner.as_rule() == Rule::effect {
                                println!("   Effect Declared: {}", inner.as_str());
                            }
                        }
                    }
                    Rule::EOI => (),
                    _ => unreachable!(),
                }
            }
        }
        Err(e) => {
            eprintln!("[FAIL] Syntax Error:\n{}", e);
            std::process::exit(1);
        }
    }
}
