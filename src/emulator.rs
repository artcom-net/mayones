use crate::bus;
use crate::cpu;
use crate::rom;

pub struct Emulator {
    cpu: cpu::Cpu,
}

impl Emulator {
    pub fn new(cartridge: rom::Cartridge, cpu_pc: Option<u16>) -> Self {
        let mut emulator = Self {
            cpu: cpu::Cpu::new(bus::CpuBus::new(cartridge)),
        };
        emulator.cpu.reset(cpu_pc);
        emulator
    }

    pub fn run(&mut self) {
        loop {
            self.cpu.step();
        }
    }

    pub fn run_trace(&mut self) {
        loop {
            let trace = self.cpu.trace_step();
            let operand = match trace.operand {
                Some(op) => format!("{:02X}", op),
                None => match trace.operand_address {
                    Some(addr) => format!("{addr:04X}"),
                    None => "".to_string(),
                },
            };
            println!(
                "{pc:04X} {opcode:02X} {mnemonic:>4} {operand:<8} \
                      A={a:02X} X={x:02X} Y={y:02X} P={p:02X} SP={sp:02X} \
                      CYC={cycles}",
                pc = trace.pc,
                opcode = trace.opcode,
                mnemonic = trace.mnemonic,
                operand = operand,
                a = trace.a,
                x = trace.x,
                y = trace.y,
                p = trace.p,
                sp = trace.sp,
                cycles = trace.cycles
            );
        }
    }
}
