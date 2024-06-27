#![allow(warnings)]

use std::io::{self, Write};

mod bus;
mod cpu;
mod emulator;
mod mapper;
mod rom;

fn main() {
    print!("ROM path: ");
    io::stdout().flush().unwrap();
    let mut rom_path = String::new();
    io::stdin().read_line(&mut rom_path).expect("reading rom path error");
    let cartridge = match rom::read(rom_path.trim()) {
        Ok(cart) => cart,
        Err(msg) => panic!("{}", msg),
    };
    let mut emulator = emulator::Emulator::new(cartridge, None);
    emulator.run_trace();
}
