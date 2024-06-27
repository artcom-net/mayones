use std::fmt::{Debug, Formatter};

use crate::bus;

#[derive(Copy, Clone)]
enum AddressMode {
    Accumulator,
    Implied,
    Immediate,
    Relative,
    Zeropage,
    ZeropageX,
    ZeropageY,
    Absolute,
    AbsoluteX,
    AbsoluteY,
    Indirect,
    IndirectX,
    IndirectY,
}

struct Instruction<'a> {
    opcode: u8,
    mnemonic: &'a str,
    cycles: u8,
    address_mode: AddressMode,
    check_page_cross: bool,
    func: fn(&mut Cpu),
}

pub struct Cpu {
    a: u8,
    x: u8,
    y: u8,
    p: u8,
    sp: u8,
    pc: u16,
    bus: bus::CpuBus,
    curr_cycles: u8,
    total_cycles: usize,
    operand: Option<u16>,
    operand_address: Option<i32>,
    address_mode: AddressMode,
    is_page_crossed: bool,
}

pub struct TraceEntry {
    pub opcode: u8,
    pub mnemonic: String,
    pub operand: Option<u16>,
    pub operand_address: Option<i32>,
    pub a: u8,
    pub x: u8,
    pub y: u8,
    pub p: u8,
    pub pc: u16,
    pub sp: u8,
    pub cycles: usize,
}

impl PartialEq for TraceEntry {
    fn eq(&self, other: &Self) -> bool {
        self.opcode == other.opcode
            && self.mnemonic == other.mnemonic
            && self.a == other.a
            && self.x == other.x
            && self.y == other.y
            && self.p == other.p
            && self.pc == other.pc
            && self.sp == other.sp
            && self.cycles == other.cycles
    }
}

impl Debug for TraceEntry {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "TraceEntry {{ \
                opcode: {:02X}, mnemonic: \"{}\", operand: {:?}, operand_address: {:?}, \
                a: {:02X}, x: {:02X}, y: {:02X}, p: {:02X}, pc: {:04X}, sp: {:04X}, cycles: {} \
            }}", 
               self.opcode, self.mnemonic, self.operand, self.operand_address, 
               self.a, self.x, self.y, self.p, self.pc, self.sp, self.cycles)
    }
}

impl<'a> Cpu {
    const STACK_BASE_ADDR: u16 = 0x0100;
    const NMI_VECTOR_ADDR: u16 = 0xFFFA;
    const RESET_VECTOR_ADDR: u16 = 0xFFFC;
    const IRQ_VECTOR_ADDR: u16 = 0xFFFE;

    const ACCUMULATOR_ADDR: i32 = -1;

    const CARRY_FLAG: u8 = 1 << 0;
    const ZERO_FLAG: u8 = 1 << 1;
    const INTERRUPT_FLAG: u8 = 1 << 2;
    const DECIMAL_FLAG: u8 = 1 << 3;
    const BREAK_FLAG: u8 = 1 << 4;
    const UNUSED_FLAG: u8 = 1 << 5;
    const OVERFLOW_FLAG: u8 = 1 << 6;
    const NEGATIVE_FLAG: u8 = 1 << 7;

    const INVALID_INSTRUCTION: Instruction<'a> = Instruction {
        opcode: 0,
        mnemonic: "",
        cycles: 0,
        address_mode: AddressMode::Implied,
        check_page_cross: false,
        func: Self::invalid_opcode,
    };
    const INSTRUCTIONS: [Instruction<'a>; 0x100] = [
        Instruction {
            opcode: 0x00,
            mnemonic: "BRK",
            cycles: 8,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::brk,
        },
        Instruction {
            opcode: 0x01,
            mnemonic: "ORA",
            cycles: 6,
            address_mode: AddressMode::IndirectX,
            check_page_cross: false,
            func: Self::ora,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x05,
            mnemonic: "ORA",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::ora,
        },
        Instruction {
            opcode: 0x06,
            mnemonic: "ASL",
            cycles: 5,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::asl,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x08,
            mnemonic: "PHP",
            cycles: 3,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::php,
        },
        Instruction {
            opcode: 0x09,
            mnemonic: "ORA",
            cycles: 2,
            address_mode: AddressMode::Immediate,
            check_page_cross: false,
            func: Self::ora,
        },
        Instruction {
            opcode: 0x0A,
            mnemonic: "ASL",
            cycles: 2,
            address_mode: AddressMode::Accumulator,
            check_page_cross: false,
            func: Self::asl,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x0D,
            mnemonic: "ORA",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::ora,
        },
        Instruction {
            opcode: 0x0E,
            mnemonic: "ASL",
            cycles: 6,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::asl,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x10,
            mnemonic: "BPL",
            cycles: 2,
            address_mode: AddressMode::Relative,
            check_page_cross: false,
            func: Self::bpl,
        },
        Instruction {
            opcode: 0x11,
            mnemonic: "ORA",
            cycles: 5,
            address_mode: AddressMode::IndirectY,
            check_page_cross: true,
            func: Self::ora,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x15,
            mnemonic: "ORA",
            cycles: 4,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::ora,
        },
        Instruction {
            opcode: 0x16,
            mnemonic: "ASL",
            cycles: 6,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::asl,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x18,
            mnemonic: "CLC",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::clc,
        },
        Instruction {
            opcode: 0x19,
            mnemonic: "ORA",
            cycles: 4,
            address_mode: AddressMode::AbsoluteY,
            check_page_cross: true,
            func: Self::ora,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x1D,
            mnemonic: "ORA",
            cycles: 4,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: true,
            func: Self::ora,
        },
        Instruction {
            opcode: 0x1E,
            mnemonic: "ASL",
            cycles: 7,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: false,
            func: Self::asl,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x20,
            mnemonic: "JSR",
            cycles: 6,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::jsr,
        },
        Instruction {
            opcode: 0x21,
            mnemonic: "AND",
            cycles: 6,
            address_mode: AddressMode::IndirectX,
            check_page_cross: false,
            func: Self::and,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x24,
            mnemonic: "BIT",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::bit,
        },
        Instruction {
            opcode: 0x25,
            mnemonic: "AND",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::and,
        },
        Instruction {
            opcode: 0x26,
            mnemonic: "ROL",
            cycles: 5,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::rol,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x28,
            mnemonic: "PLP",
            cycles: 4,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::plp,
        },
        Instruction {
            opcode: 0x29,
            mnemonic: "AND",
            cycles: 2,
            address_mode: AddressMode::Immediate,
            check_page_cross: false,
            func: Self::and,
        },
        Instruction {
            opcode: 0x2A,
            mnemonic: "ROL",
            cycles: 2,
            address_mode: AddressMode::Accumulator,
            check_page_cross: false,
            func: Self::rol,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x2C,
            mnemonic: "BIT",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::bit,
        },
        Instruction {
            opcode: 0x2D,
            mnemonic: "AND",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::and,
        },
        Instruction {
            opcode: 0x2E,
            mnemonic: "ROL",
            cycles: 6,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::rol,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x30,
            mnemonic: "BMI",
            cycles: 2,
            address_mode: AddressMode::Relative,
            check_page_cross: false,
            func: Self::bmi,
        },
        Instruction {
            opcode: 0x31,
            mnemonic: "AND",
            cycles: 5,
            address_mode: AddressMode::IndirectY,
            check_page_cross: true,
            func: Self::and,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x35,
            mnemonic: "AND",
            cycles: 4,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::and,
        },
        Instruction {
            opcode: 0x36,
            mnemonic: "ROL",
            cycles: 6,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::rol,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x38,
            mnemonic: "SEC",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::sec,
        },
        Instruction {
            opcode: 0x39,
            mnemonic: "AND",
            cycles: 4,
            address_mode: AddressMode::AbsoluteY,
            check_page_cross: true,
            func: Self::and,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x3D,
            mnemonic: "AND",
            cycles: 4,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: true,
            func: Self::and,
        },
        Instruction {
            opcode: 0x3E,
            mnemonic: "ROL",
            cycles: 7,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: false,
            func: Self::rol,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x40,
            mnemonic: "RTI",
            cycles: 6,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::rti,
        },
        Instruction {
            opcode: 0x41,
            mnemonic: "EOR",
            cycles: 6,
            address_mode: AddressMode::IndirectX,
            check_page_cross: false,
            func: Self::eor,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x45,
            mnemonic: "EOR",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::eor,
        },
        Instruction {
            opcode: 0x46,
            mnemonic: "LSR",
            cycles: 5,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::lsr,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x48,
            mnemonic: "PHA",
            cycles: 3,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::pha,
        },
        Instruction {
            opcode: 0x49,
            mnemonic: "EOR",
            cycles: 2,
            address_mode: AddressMode::Immediate,
            check_page_cross: false,
            func: Self::eor,
        },
        Instruction {
            opcode: 0x4A,
            mnemonic: "LSR",
            cycles: 2,
            address_mode: AddressMode::Accumulator,
            check_page_cross: false,
            func: Self::lsr,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x4C,
            mnemonic: "JMP",
            cycles: 3,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::jmp,
        },
        Instruction {
            opcode: 0x4D,
            mnemonic: "EOR",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::eor,
        },
        Instruction {
            opcode: 0x4E,
            mnemonic: "LSR",
            cycles: 6,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::lsr,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x50,
            mnemonic: "BVC",
            cycles: 2,
            address_mode: AddressMode::Relative,
            check_page_cross: false,
            func: Self::bvc,
        },
        Instruction {
            opcode: 0x51,
            mnemonic: "EOR",
            cycles: 5,
            address_mode: AddressMode::IndirectY,
            check_page_cross: true,
            func: Self::eor,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x55,
            mnemonic: "EOR",
            cycles: 4,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::eor,
        },
        Instruction {
            opcode: 0x56,
            mnemonic: "LSR",
            cycles: 6,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::lsr,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x58,
            mnemonic: "CLI",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::cli,
        },
        Instruction {
            opcode: 0x59,
            mnemonic: "EOR",
            cycles: 4,
            address_mode: AddressMode::AbsoluteY,
            check_page_cross: true,
            func: Self::eor,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x5D,
            mnemonic: "EOR",
            cycles: 4,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: true,
            func: Self::eor,
        },
        Instruction {
            opcode: 0x5E,
            mnemonic: "LSR",
            cycles: 7,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: false,
            func: Self::lsr,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x60,
            mnemonic: "RTS",
            cycles: 6,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::rts,
        },
        Instruction {
            opcode: 0x61,
            mnemonic: "ADC",
            cycles: 6,
            address_mode: AddressMode::IndirectX,
            check_page_cross: false,
            func: Self::adc,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x65,
            mnemonic: "ADC",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::adc,
        },
        Instruction {
            opcode: 0x66,
            mnemonic: "ROR",
            cycles: 5,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::ror,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x68,
            mnemonic: "PLA",
            cycles: 4,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::pla,
        },
        Instruction {
            opcode: 0x69,
            mnemonic: "ADC",
            cycles: 2,
            address_mode: AddressMode::Immediate,
            check_page_cross: false,
            func: Self::adc,
        },
        Instruction {
            opcode: 0x6A,
            mnemonic: "ROR",
            cycles: 2,
            address_mode: AddressMode::Accumulator,
            check_page_cross: false,
            func: Self::ror,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x6C,
            mnemonic: "JMP",
            cycles: 5,
            address_mode: AddressMode::Indirect,
            check_page_cross: false,
            func: Self::jmp,
        },
        Instruction {
            opcode: 0x6D,
            mnemonic: "ADC",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::adc,
        },
        Instruction {
            opcode: 0x6E,
            mnemonic: "ROR",
            cycles: 6,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::ror,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x70,
            mnemonic: "BVS",
            cycles: 2,
            address_mode: AddressMode::Relative,
            check_page_cross: false,
            func: Self::bvs,
        },
        Instruction {
            opcode: 0x71,
            mnemonic: "ADC",
            cycles: 5,
            address_mode: AddressMode::IndirectY,
            check_page_cross: true,
            func: Self::adc,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x75,
            mnemonic: "ADC",
            cycles: 4,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::adc,
        },
        Instruction {
            opcode: 0x76,
            mnemonic: "ROR",
            cycles: 6,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::ror,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x78,
            mnemonic: "SEI",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::sei,
        },
        Instruction {
            opcode: 0x79,
            mnemonic: "ADC",
            cycles: 4,
            address_mode: AddressMode::AbsoluteY,
            check_page_cross: true,
            func: Self::adc,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x7D,
            mnemonic: "ADC",
            cycles: 4,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: true,
            func: Self::adc,
        },
        Instruction {
            opcode: 0x7E,
            mnemonic: "ROR",
            cycles: 7,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: false,
            func: Self::ror,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x81,
            mnemonic: "STA",
            cycles: 6,
            address_mode: AddressMode::IndirectX,
            check_page_cross: false,
            func: Self::sta,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x84,
            mnemonic: "STY",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::sty,
        },
        Instruction {
            opcode: 0x85,
            mnemonic: "STA",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::sta,
        },
        Instruction {
            opcode: 0x86,
            mnemonic: "STX",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::stx,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x88,
            mnemonic: "DEY",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::dey,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x8A,
            mnemonic: "TXA",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::txa,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x8C,
            mnemonic: "STY",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::sty,
        },
        Instruction {
            opcode: 0x8D,
            mnemonic: "STA",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::sta,
        },
        Instruction {
            opcode: 0x8E,
            mnemonic: "STX",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::stx,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x90,
            mnemonic: "BCC",
            cycles: 2,
            address_mode: AddressMode::Relative,
            check_page_cross: false,
            func: Self::bcc,
        },
        Instruction {
            opcode: 0x91,
            mnemonic: "STA",
            cycles: 6,
            address_mode: AddressMode::IndirectY,
            check_page_cross: false,
            func: Self::sta,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x94,
            mnemonic: "STY",
            cycles: 4,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::sty,
        },
        Instruction {
            opcode: 0x95,
            mnemonic: "STA",
            cycles: 4,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::sta,
        },
        Instruction {
            opcode: 0x96,
            mnemonic: "STX",
            cycles: 4,
            address_mode: AddressMode::ZeropageY,
            check_page_cross: false,
            func: Self::stx,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x98,
            mnemonic: "TYA",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::tya,
        },
        Instruction {
            opcode: 0x99,
            mnemonic: "STA",
            cycles: 5,
            address_mode: AddressMode::AbsoluteY,
            check_page_cross: false,
            func: Self::sta,
        },
        Instruction {
            opcode: 0x9A,
            mnemonic: "TXS",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::txs,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0x9D,
            mnemonic: "STA",
            cycles: 5,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: false,
            func: Self::sta,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xA0,
            mnemonic: "LDY",
            cycles: 2,
            address_mode: AddressMode::Immediate,
            check_page_cross: false,
            func: Self::ldy,
        },
        Instruction {
            opcode: 0xA1,
            mnemonic: "LDA",
            cycles: 6,
            address_mode: AddressMode::IndirectX,
            check_page_cross: false,
            func: Self::lda,
        },
        Instruction {
            opcode: 0xA2,
            mnemonic: "LDX",
            cycles: 2,
            address_mode: AddressMode::Immediate,
            check_page_cross: false,
            func: Self::ldx,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xA4,
            mnemonic: "LDY",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::ldy,
        },
        Instruction {
            opcode: 0xA5,
            mnemonic: "LDA",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::lda,
        },
        Instruction {
            opcode: 0xA6,
            mnemonic: "LDX",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::ldx,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xA8,
            mnemonic: "TAY",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::tay,
        },
        Instruction {
            opcode: 0xA9,
            mnemonic: "LDA",
            cycles: 2,
            address_mode: AddressMode::Immediate,
            check_page_cross: false,
            func: Self::lda,
        },
        Instruction {
            opcode: 0xAA,
            mnemonic: "TAX",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::tax,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xAC,
            mnemonic: "LDY",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::ldy,
        },
        Instruction {
            opcode: 0xAD,
            mnemonic: "LDA",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::lda,
        },
        Instruction {
            opcode: 0xAE,
            mnemonic: "LDX",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::ldx,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xB0,
            mnemonic: "BCS",
            cycles: 2,
            address_mode: AddressMode::Relative,
            check_page_cross: false,
            func: Self::bcs,
        },
        Instruction {
            opcode: 0xB1,
            mnemonic: "LDA",
            cycles: 5,
            address_mode: AddressMode::IndirectY,
            check_page_cross: true,
            func: Self::lda,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xB4,
            mnemonic: "LDY",
            cycles: 4,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::ldy,
        },
        Instruction {
            opcode: 0xB5,
            mnemonic: "LDA",
            cycles: 4,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::lda,
        },
        Instruction {
            opcode: 0xB6,
            mnemonic: "LDX",
            cycles: 4,
            address_mode: AddressMode::ZeropageY,
            check_page_cross: false,
            func: Self::ldx,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xB8,
            mnemonic: "CLV",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::clv,
        },
        Instruction {
            opcode: 0xB9,
            mnemonic: "LDA",
            cycles: 4,
            address_mode: AddressMode::AbsoluteY,
            check_page_cross: true,
            func: Self::lda,
        },
        Instruction {
            opcode: 0xBA,
            mnemonic: "TSX",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::tsx,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xBC,
            mnemonic: "LDY",
            cycles: 4,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: true,
            func: Self::ldy,
        },
        Instruction {
            opcode: 0xBD,
            mnemonic: "LDA",
            cycles: 4,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: true,
            func: Self::lda,
        },
        Instruction {
            opcode: 0xBE,
            mnemonic: "LDX",
            cycles: 4,
            address_mode: AddressMode::AbsoluteY,
            check_page_cross: true,
            func: Self::ldx,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xC0,
            mnemonic: "CPY",
            cycles: 2,
            address_mode: AddressMode::Immediate,
            check_page_cross: false,
            func: Self::cpy,
        },
        Instruction {
            opcode: 0xC1,
            mnemonic: "CMP",
            cycles: 6,
            address_mode: AddressMode::IndirectX,
            check_page_cross: false,
            func: Self::cmp,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xC4,
            mnemonic: "CPY",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::cpy,
        },
        Instruction {
            opcode: 0xC5,
            mnemonic: "CMP",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::cmp,
        },
        Instruction {
            opcode: 0xC6,
            mnemonic: "DEC",
            cycles: 5,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::dec,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xC8,
            mnemonic: "INY",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::iny,
        },
        Instruction {
            opcode: 0xC9,
            mnemonic: "CMP",
            cycles: 2,
            address_mode: AddressMode::Immediate,
            check_page_cross: false,
            func: Self::cmp,
        },
        Instruction {
            opcode: 0xCA,
            mnemonic: "DEX",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::dex,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xCC,
            mnemonic: "CPY",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::cpy,
        },
        Instruction {
            opcode: 0xCD,
            mnemonic: "CMP",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::cmp,
        },
        Instruction {
            opcode: 0xCE,
            mnemonic: "DEC",
            cycles: 6,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::dec,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xD0,
            mnemonic: "BNE",
            cycles: 2,
            address_mode: AddressMode::Relative,
            check_page_cross: false,
            func: Self::bne,
        },
        Instruction {
            opcode: 0xD1,
            mnemonic: "CMP",
            cycles: 5,
            address_mode: AddressMode::IndirectY,
            check_page_cross: true,
            func: Self::cmp,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xD5,
            mnemonic: "CMP",
            cycles: 4,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::cmp,
        },
        Instruction {
            opcode: 0xD6,
            mnemonic: "DEC",
            cycles: 6,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::dec,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xD8,
            mnemonic: "CLD",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::cld,
        },
        Instruction {
            opcode: 0xD9,
            mnemonic: "CMP",
            cycles: 4,
            address_mode: AddressMode::AbsoluteY,
            check_page_cross: true,
            func: Self::cmp,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xDD,
            mnemonic: "CMP",
            cycles: 4,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: true,
            func: Self::cmp,
        },
        Instruction {
            opcode: 0xDE,
            mnemonic: "DEC",
            cycles: 7,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: false,
            func: Self::dec,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xE0,
            mnemonic: "CPX",
            cycles: 2,
            address_mode: AddressMode::Immediate,
            check_page_cross: false,
            func: Self::cpx,
        },
        Instruction {
            opcode: 0xE1,
            mnemonic: "SBC",
            cycles: 6,
            address_mode: AddressMode::IndirectX,
            check_page_cross: false,
            func: Self::sbc,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xE4,
            mnemonic: "CPX",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::cpx,
        },
        Instruction {
            opcode: 0xE5,
            mnemonic: "SBC",
            cycles: 3,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::sbc,
        },
        Instruction {
            opcode: 0xE6,
            mnemonic: "INC",
            cycles: 5,
            address_mode: AddressMode::Zeropage,
            check_page_cross: false,
            func: Self::inc,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xE8,
            mnemonic: "INX",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::inx,
        },
        Instruction {
            opcode: 0xE9,
            mnemonic: "SBC",
            cycles: 2,
            address_mode: AddressMode::Immediate,
            check_page_cross: false,
            func: Self::sbc,
        },
        Instruction {
            opcode: 0xEA,
            mnemonic: "NOP",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::nop,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xEC,
            mnemonic: "CPX",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::cpx,
        },
        Instruction {
            opcode: 0xED,
            mnemonic: "SBC",
            cycles: 4,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::sbc,
        },
        Instruction {
            opcode: 0xEE,
            mnemonic: "INC",
            cycles: 6,
            address_mode: AddressMode::Absolute,
            check_page_cross: false,
            func: Self::inc,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xF0,
            mnemonic: "BEQ",
            cycles: 2,
            address_mode: AddressMode::Relative,
            check_page_cross: false,
            func: Self::beq,
        },
        Instruction {
            opcode: 0xF1,
            mnemonic: "SBC",
            cycles: 5,
            address_mode: AddressMode::IndirectY,
            check_page_cross: true,
            func: Self::sbc,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xF5,
            mnemonic: "SBC",
            cycles: 4,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::sbc,
        },
        Instruction {
            opcode: 0xF6,
            mnemonic: "INC",
            cycles: 6,
            address_mode: AddressMode::ZeropageX,
            check_page_cross: false,
            func: Self::inc,
        },
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xF8,
            mnemonic: "SED",
            cycles: 2,
            address_mode: AddressMode::Implied,
            check_page_cross: false,
            func: Self::sed,
        },
        Instruction {
            opcode: 0xF9,
            mnemonic: "SBC",
            cycles: 4,
            address_mode: AddressMode::AbsoluteY,
            check_page_cross: true,
            func: Self::sbc,
        },
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Self::INVALID_INSTRUCTION,
        Instruction {
            opcode: 0xFD,
            mnemonic: "SBC",
            cycles: 4,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: true,
            func: Self::sbc,
        },
        Instruction {
            opcode: 0xFE,
            mnemonic: "INC",
            cycles: 7,
            address_mode: AddressMode::AbsoluteX,
            check_page_cross: false,
            func: Self::inc,
        },
        Self::INVALID_INSTRUCTION,
    ];

    pub fn new(bus: bus::CpuBus) -> Self {
        Self {
            a: 0,
            x: 0,
            y: 0,
            p: 0,
            sp: 0,
            pc: 0,
            bus: bus,
            curr_cycles: 0,
            total_cycles: 0,
            operand: None,
            operand_address: None,
            address_mode: AddressMode::Implied,
            is_page_crossed: false,
        }
    }

    pub fn reset(&mut self, pc: Option<u16>) {
        self.a = 0;
        self.x = 0;
        self.y = 0;
        self.sp = 0xFD;
        self.total_cycles = 7;
        self.p = Self::INTERRUPT_FLAG | Self::UNUSED_FLAG;
        self.pc = match pc {
            Some(addr) => addr,
            None => {
                self.bus.read(Self::RESET_VECTOR_ADDR) as u16
                    | (self.bus.read(Self::RESET_VECTOR_ADDR + 1) as u16) << 8
            }
        }
    }

    pub fn step(&mut self) -> u8 {
        self.curr_cycles = 0;
        let opcode = self.bus.read(self.pc);
        self.pc += 1;
        let instruction = Self::INSTRUCTIONS
            .get(opcode as usize)
            .unwrap_or(&Self::INVALID_INSTRUCTION);
        self.address_mode = instruction.address_mode;
        (self.operand, self.operand_address) = match self.address_mode {
            AddressMode::Accumulator => (None, Some(Self::ACCUMULATOR_ADDR)),
            AddressMode::Implied => (None, None),
            AddressMode::Immediate => self.resolve_immediate(),
            AddressMode::Relative => self.resolve_relative(),
            AddressMode::Zeropage => self.resolve_zeropage(0),
            AddressMode::ZeropageX => self.resolve_zeropage(self.x),
            AddressMode::ZeropageY => self.resolve_zeropage(self.y),
            AddressMode::Absolute => self.resolve_absolute(0),
            AddressMode::AbsoluteX => self.resolve_absolute(self.x),
            AddressMode::AbsoluteY => self.resolve_absolute(self.y),
            AddressMode::Indirect => self.resolve_indirect(),
            AddressMode::IndirectX => self.resolve_indirect_x(),
            AddressMode::IndirectY => self.resolve_indirect_y(),
        };
        (instruction.func)(self);
        self.curr_cycles += instruction.cycles;
        if instruction.check_page_cross && self.is_page_crossed {
            self.curr_cycles += 1;
            self.is_page_crossed = false;
        }
        self.total_cycles += self.curr_cycles as usize;
        self.curr_cycles
    }

    pub fn trace_step(&mut self) -> TraceEntry {
        let a = self.a;
        let x = self.x;
        let y = self.y;
        let p = self.p;
        let pc = self.pc;
        let sp = self.sp;
        let cycles = self.total_cycles;
        let opcode = self.bus.read(self.pc);
        let instruction = Self::INSTRUCTIONS
            .get(opcode as usize)
            .unwrap_or(&Self::INVALID_INSTRUCTION);
        self.step();
        TraceEntry {
            opcode: opcode,
            mnemonic: instruction.mnemonic.to_string(),
            operand: self.operand,
            operand_address: self.operand_address,
            a: a,
            x: x,
            y: y,
            p: p,
            pc: pc,
            sp: sp,
            cycles,
        }
    }

    fn resolve_immediate(&mut self) -> (Option<u16>, Option<i32>) {
        let operand = self.bus.read(self.pc);
        self.pc += 1;
        (Some(operand as u16), None)
    }

    fn resolve_relative(&mut self) -> (Option<u16>, Option<i32>) {
        let offset = self.bus.read(self.pc);
        self.pc += 1;
        (Some(offset as u16), None)
    }

    fn resolve_zeropage(&mut self, index: u8) -> (Option<u16>, Option<i32>) {
        let base_addr = self.bus.read(self.pc);
        self.pc += 1;
        (
            Some(base_addr as u16),
            Some(base_addr.wrapping_add(index) as i32),
        )
    }

    fn is_page_crossed(address1: u16, address2: u16) -> bool {
        address1 & 0xFF00 != address2 & 0xFF00
    }

    fn read_address_around_page(&self, address: u16) -> u16 {
        let mut pointer = self.bus.read(address) as u16;
        if Self::is_page_crossed(address, address + 1) {
            pointer |= (self.bus.read(address & 0xFF00) as u16) << 8;
        } else {
            pointer |= (self.bus.read(address + 1) as u16) << 8;
        }
        pointer
    }

    fn resolve_absolute(&mut self, index: u8) -> (Option<u16>, Option<i32>) {
        let base_addr = self.bus.read(self.pc) as u16 | (self.bus.read(self.pc + 1) as u16) << 8;
        self.pc += 2;
        let effective_addr = base_addr.wrapping_add(index as u16);
        self.is_page_crossed = Self::is_page_crossed(base_addr, effective_addr);
        (Some(base_addr), Some(effective_addr as i32))
    }

    fn resolve_indirect(&mut self) -> (Option<u16>, Option<i32>) {
        let pointer = self.bus.read(self.pc) as u16 | (self.bus.read(self.pc + 1) as u16) << 8;
        self.pc += 2;
        let effective_addr = self.read_address_around_page(pointer);
        (Some(pointer), Some(effective_addr as i32))
    }

    fn resolve_indirect_x(&mut self) -> (Option<u16>, Option<i32>) {
        let base_addr = self.bus.read(self.pc);
        self.pc += 1;
        let zeropage_addr = base_addr.wrapping_add(self.x);
        let effective_addr = self.read_address_around_page(zeropage_addr as u16);
        (Some(base_addr as u16), Some(effective_addr as i32))
    }

    fn resolve_indirect_y(&mut self) -> (Option<u16>, Option<i32>) {
        let pointer = self.bus.read(self.pc) as u16;
        self.pc += 1;
        let base_addr = self.read_address_around_page(pointer);
        let effective_addr = base_addr.wrapping_add(self.y as u16);
        self.is_page_crossed = Self::is_page_crossed(base_addr, effective_addr);
        (Some(pointer), Some(effective_addr as i32))
    }

    fn get_operand(&self) -> u16 {
        match self.operand_address {
            Some(addr) => match addr {
                Self::ACCUMULATOR_ADDR => self.a as u16,
                _ => self.bus.read(addr as u16) as u16,
            },
            None => self.operand.unwrap(),
        }
    }

    fn store(&mut self, address: Option<i32>, data: u8) {
        match self.address_mode {
            AddressMode::Accumulator => self.a = data,
            _ => match address {
                Some(addr) => self.bus.write(addr as u16, data),
                None => panic!("expected address got None"),
            },
        }
    }

    fn push_stack(&mut self, data: u8) {
        self.bus.write(Self::STACK_BASE_ADDR | self.sp as u16, data);
        self.sp -= 1;
    }

    fn pop_stack(&mut self) -> u8 {
        self.sp += 1;
        self.bus.read(Self::STACK_BASE_ADDR | self.sp as u16)
    }

    fn set_flag(&mut self, flag: u8, is_need_set: bool) {
        if is_need_set {
            self.p |= flag;
        } else {
            self.p &= !flag;
        }
    }

    fn set_nz_flags(&mut self, data: u8) {
        self.set_flag(Self::ZERO_FLAG, data == 0);
        self.set_flag(Self::NEGATIVE_FLAG, (data >> 7) == 1);
    }

    fn brk(&mut self) {
        let return_addr = self.pc + 1;
        self.push_stack((return_addr >> 8) as u8);
        self.push_stack(return_addr as u8);
        self.push_stack(self.p | Self::BREAK_FLAG);
        self.p |= Self::INTERRUPT_FLAG;
        self.pc = self.bus.read(Self::IRQ_VECTOR_ADDR) as u16
            | (self.bus.read(Self::IRQ_VECTOR_ADDR + 1) as u16) << 8;
    }

    fn php(&mut self) {
        self.push_stack(self.p | Self::BREAK_FLAG);
    }

    fn plp(&mut self) {
        self.p = self.pop_stack() & !Self::BREAK_FLAG | Self::UNUSED_FLAG;
    }

    fn pha(&mut self) {
        self.push_stack(self.a);
    }

    fn pla(&mut self) {
        self.a = self.pop_stack();
        self.set_nz_flags(self.a);
    }

    fn clc(&mut self) {
        self.p &= !Self::CARRY_FLAG;
    }

    fn sec(&mut self) {
        self.p |= Self::CARRY_FLAG;
    }

    fn cli(&mut self) {
        self.p &= !Self::INTERRUPT_FLAG;
    }

    fn sei(&mut self) {
        self.p |= Self::INTERRUPT_FLAG;
    }

    fn clv(&mut self) {
        self.p &= !Self::OVERFLOW_FLAG;
    }

    fn cld(&mut self) {
        self.p &= !Self::DECIMAL_FLAG;
    }

    fn sed(&mut self) {
        self.p |= Self::DECIMAL_FLAG;
    }

    fn bit(&mut self) {
        let operand = self.get_operand() as u8;
        self.set_flag(Self::ZERO_FLAG, (self.a & operand) == 0);
        self.set_flag(Self::OVERFLOW_FLAG, ((operand >> 6) & 1) == 1);
        self.set_flag(Self::NEGATIVE_FLAG, (operand >> 7) == 1);
    }

    fn rol(&mut self) {
        let operand = self.get_operand() as u8;
        let rotated = operand << 1 | self.p & Self::CARRY_FLAG;
        self.set_flag(Self::CARRY_FLAG, (operand >> 7) == 1);
        self.set_nz_flags(rotated);
        self.store(self.operand_address, rotated);
    }

    fn ror(&mut self) {
        let operand = self.get_operand() as u8;
        let rotated = operand >> 1 | (self.p & Self::CARRY_FLAG) << 7;
        self.set_flag(Self::CARRY_FLAG, (operand & 1) == 1);
        self.set_nz_flags(rotated);
        self.store(self.operand_address, rotated);
    }

    fn rti(&mut self) {
        self.p = self.pop_stack() | Self::UNUSED_FLAG;
        self.pc = self.pop_stack() as u16 | (self.pop_stack() as u16) << 8;
    }

    fn rts(&mut self) {
        self.pc = (self.pop_stack() as u16 | (self.pop_stack() as u16) << 8) + 1;
    }

    fn jmp(&mut self) {
        self.pc = self.operand_address.unwrap() as u16;
    }

    fn jsr(&mut self) {
        let return_addr = self.pc - 1;
        self.push_stack((return_addr >> 8) as u8);
        self.push_stack(return_addr as u8);
        self.pc = self.operand_address.unwrap() as u16;
    }

    fn ora(&mut self) {
        self.a |= self.get_operand() as u8;
        self.set_nz_flags(self.a);
    }

    fn asl(&mut self) {
        let operand = self.get_operand() as u8;
        let result = operand << 1;
        self.set_flag(Self::CARRY_FLAG, (operand >> 7) == 1);
        self.set_nz_flags(result);
        self.store(self.operand_address, result);
    }

    fn and(&mut self) {
        self.a &= self.get_operand() as u8;
        self.set_nz_flags(self.a);
    }

    fn eor(&mut self) {
        self.a ^= self.get_operand() as u8;
        self.set_nz_flags(self.a);
    }

    fn lsr(&mut self) {
        let operand = self.get_operand() as u8;
        let result = operand >> 1;
        self.set_flag(Self::CARRY_FLAG, (operand & 1) == 1);
        self.set_nz_flags(result);
        self.store(self.operand_address, result);
    }

    fn adc_(&mut self, operand: u16) {
        let result = self.a as u16 + operand + (self.p & Self::CARRY_FLAG) as u16;
        self.set_flag(Self::CARRY_FLAG, result > 0xFF);
        self.set_flag(
            Self::OVERFLOW_FLAG,
            (((self.a as u16 ^ result) & (operand ^ result) & 0x80) >> 7) == 1,
        );
        self.set_nz_flags(result as u8);
        self.a = result as u8;
    }

    fn adc(&mut self) {
        self.adc_(self.get_operand());
    }

    fn sbc(&mut self) {
        self.adc_(self.get_operand() ^ 0xFF);
    }

    fn sta(&mut self) {
        self.store(self.operand_address, self.a);
    }

    fn sty(&mut self) {
        self.store(self.operand_address, self.y);
    }

    fn stx(&mut self) {
        self.store(self.operand_address, self.x);
    }

    fn inx(&mut self) {
        self.x = self.x.wrapping_add(1);
        self.set_nz_flags(self.x);
    }

    fn dex(&mut self) {
        self.x = self.x.wrapping_sub(1);
        self.set_nz_flags(self.x);
    }

    fn iny(&mut self) {
        self.y = self.y.wrapping_add(1);
        self.set_nz_flags(self.y);
    }

    fn dey(&mut self) {
        self.y = self.y.wrapping_sub(1);
        self.set_nz_flags(self.y);
    }

    fn inc(&mut self) {
        let result = (self.get_operand() as u8).wrapping_add(1);
        self.set_nz_flags(result);
        self.store(self.operand_address, result);
    }

    fn dec(&mut self) {
        let result = (self.get_operand() as u8).wrapping_sub(1);
        self.set_nz_flags(result);
        self.store(self.operand_address, result);
    }

    fn txa(&mut self) {
        self.a = self.x;
        self.set_nz_flags(self.a);
    }

    fn tya(&mut self) {
        self.a = self.y;
        self.set_nz_flags(self.a);
    }

    fn txs(&mut self) {
        self.sp = self.x;
    }

    fn tax(&mut self) {
        self.x = self.a;
        self.set_nz_flags(self.x);
    }

    fn tsx(&mut self) {
        self.x = self.sp;
        self.set_nz_flags(self.x);
    }

    fn ldx(&mut self) {
        self.x = self.get_operand() as u8;
        self.set_nz_flags(self.x);
    }

    fn ldy(&mut self) {
        self.y = self.get_operand() as u8;
        self.set_nz_flags(self.y);
    }

    fn lda(&mut self) {
        self.a = self.get_operand() as u8;
        self.set_nz_flags(self.a);
    }

    fn tay(&mut self) {
        self.y = self.a;
        self.set_nz_flags(self.y);
    }

    fn cmp_(&mut self, register: u8) {
        let operand = self.get_operand() as u8;
        let result = register.wrapping_sub(operand);
        self.set_flag(Self::CARRY_FLAG, register >= operand);
        self.set_nz_flags(result);
    }

    fn cmp(&mut self) {
        self.cmp_(self.a);
    }

    fn cpx(&mut self) {
        self.cmp_(self.x);
    }

    fn cpy(&mut self) {
        self.cmp_(self.y);
    }

    fn branch(&mut self, condition: bool) {
        if !condition {
            return;
        }
        self.curr_cycles += 1;
        let operand = self.get_operand() as i16;
        let offset = match operand & 0x80 {
            0x80 => -(0x100 - operand),
            _ => operand,
        };
        let address = (self.pc as i16 + offset) as u16; // TODO: use i32 instead
        self.is_page_crossed = Self::is_page_crossed(self.pc, address);
        self.pc = address;
    }

    fn bpl(&mut self) {
        self.branch((self.p & Self::NEGATIVE_FLAG) == 0);
    }

    fn bmi(&mut self) {
        self.branch((self.p & Self::NEGATIVE_FLAG) != 0);
    }

    fn bvc(&mut self) {
        self.branch((self.p & Self::OVERFLOW_FLAG) == 0);
    }

    fn bvs(&mut self) {
        self.branch((self.p & Self::OVERFLOW_FLAG) != 0);
    }

    fn bcc(&mut self) {
        self.branch((self.p & Self::CARRY_FLAG) == 0);
    }

    fn bcs(&mut self) {
        self.branch((self.p & Self::CARRY_FLAG) != 0);
    }

    fn bne(&mut self) {
        self.branch((self.p & Self::ZERO_FLAG) == 0);
    }

    fn beq(&mut self) {
        self.branch((self.p & Self::ZERO_FLAG) != 0);
    }

    fn nop(&mut self) {}

    fn invalid_opcode(&mut self) {
        panic!("illegal opcode")
    }
}

#[cfg(test)]
mod tests {
    use std::collections::LinkedList;
    use std::fs::File;
    use std::io::{BufRead, BufReader};
    use std::{panic, path};

    use super::*;
    use crate::{bus, rom};
    
    const NESTEST_PC: u16 = 0xC000;
    const NESTEST_ROM_PATH: &str = "./rom/nestest.nes";
    const NESTEST_TRACE_PATH: &str = "./rom/nestest_official.trace";

    fn parse_register_data<T: num_traits::Num>(str_val: &str, radix: u32) -> Result<T, T::FromStrRadixErr> {
        T::from_str_radix(str_val.split(':').nth(1).unwrap(), radix)
    }

    fn parse_nestest_line(line: String) -> TraceEntry {
        let chunks: Vec<&str> = line
            .split(' ')
            .filter(|&c| !c.is_empty())
            .collect();
        let pc = u16::from_str_radix(chunks[0], 16).unwrap();
        let opcode = u8::from_str_radix(chunks[1], 16).unwrap();
        let mut operand: Option<u16> = None;
        let mut index = 2;
        let mut stack: LinkedList<u8> = LinkedList::new();
        while chunks[index].len() == 2 {
            let operand_byte = u8::from_str_radix(chunks[index], 16).unwrap();
            stack.push_back(operand_byte);
            index += 1;
        }
        if !stack.is_empty() {
            let mut operand_val: u16 = 0;
            while !stack.is_empty() {
                operand_val = operand_val << 8 | stack.pop_back().unwrap() as u16;
            }
            operand = Some(operand_val);
        }
        
        let mnemonic = chunks[index].to_string();
        index += 1;
        while !chunks[index].starts_with("A:") {
            index += 1;
        }

        let a: u8;
        let x: u8;
        let y: u8;
        let p: u8;
        let sp: u8;
        let cycles: usize;

        if let [a_str, x_str, y_str, p_str, sp_str, .., cycles_str] = &chunks[index..] {
            a = parse_register_data(a_str, 16).unwrap();
            x = parse_register_data(x_str, 16).unwrap();
            y = parse_register_data(y_str, 16).unwrap();
            p = parse_register_data(p_str, 16).unwrap();
            sp = parse_register_data(sp_str, 16).unwrap();
            cycles = parse_register_data(cycles_str, 10).unwrap();
        } else {
            panic!("parsing registers data error");
        }
        TraceEntry {
            opcode: opcode,
            mnemonic: mnemonic,
            operand: operand,
            operand_address: None,
            a: a,
            x: x,
            y: y,
            p: p,
            pc: pc,
            sp: sp,
            cycles: cycles
        }
    }
    
    fn parse_nestest_trace(trace_path: &str) -> Vec<TraceEntry> {
        let path = path::Path::new(trace_path).canonicalize().unwrap();
        let file = File::open(path).unwrap();
        let reader = BufReader::new(file);
        let mut trace: Vec<TraceEntry> = Vec::new();
        for line in reader.lines() {
            match line {  
                Ok(line) => trace.push(parse_nestest_line(line)),
                Err(err) => panic!("reading line error: {}", err),
            }
        }
        trace
    }
    
    fn run_nestest(rom_path: &str, limit: usize) -> Vec<TraceEntry> {
        let result = panic::catch_unwind(|| {
            let cartridge = rom::read(rom_path).unwrap();
            let mut cpu = Cpu::new(bus::CpuBus::new(cartridge));
            cpu.reset(Some(NESTEST_PC));
            let mut trace: Vec<TraceEntry> = Vec::new();
            for _ in 0..limit {
                trace.push(cpu.trace_step());
            }
            trace
        });
        result.unwrap()
    }
    
    #[test]
    fn test_nestest() {
        let nestest_trace = parse_nestest_trace(NESTEST_TRACE_PATH);
        let cpu_trace = run_nestest(NESTEST_ROM_PATH, nestest_trace.len());
        for (cpu_tr, nestest_tr) in std::iter::zip(cpu_trace, nestest_trace) {
            assert_eq!(cpu_tr, nestest_tr);
        }
    }
}
