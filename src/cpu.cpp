#include <ios>
#include <iostream>

#include "cpu.h"

CPU::Instruction::Instruction() :
    mnemonic(""),
    addr_mode(CPU::AddressMode::NONE),
    cycles(0),
    check_page_cross(false),
    func(nullptr)
{
}

CPU::Instruction::Instruction(const std::string& mnemonic,
                              AddressMode addr_mode,
                              uint8_t cycles,
                              bool check_page_cross,
                              void (CPU::* func)()) :
    mnemonic(mnemonic),
    addr_mode(addr_mode),
    cycles(cycles),
    check_page_cross(check_page_cross),
    func(func)
{
}

const uint16_t CPU::STACK_BASE_ADDRESS = 0x0100;
const uint16_t CPU::NMI_VECTOR_ADDRESS = 0xFFFA;
const uint16_t CPU::RESET_VECTOR_ADDRESS = 0xFFFC;
const uint16_t CPU::IRQ_VECTOR_ADDRESS = 0xFFFE;

const CPU::Instruction CPU::INSTRUCTIONS[0xFF] = {
    {"BRK", CPU::AddressMode::IMPLIED, 7, false, &CPU::brk},
    {"ORA", CPU::AddressMode::X_INDIRECT, 6, false, &CPU::ora},
    {},
    {},
    {},
    {"ORA", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::ora},
    {"ASL", CPU::AddressMode::ZEROPAGE, 5, false, &CPU::asl},
    {},
    {"PHP", CPU::AddressMode::IMPLIED, 3, false, &CPU::php},
    {"ORA", CPU::AddressMode::IMMEDIATE, 2, false, &CPU::ora},
    {"ASL", CPU::AddressMode::ACCUMULATOR, 2, false, &CPU::asl},
    {},
    {},
    {"ORA", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::ora},
    {"ASL", CPU::AddressMode::ABSOLUTE, 6, false, &CPU::asl},
    {},
    {"BPL", CPU::AddressMode::RELATIVE, 2, false, &CPU::bpl},
    {"ORA", CPU::AddressMode::INDIRECT_Y, 5, true, &CPU::ora},
    {},
    {},
    {},
    {"ORA", CPU::AddressMode::ZEROPAGE_X, 4, false, &CPU::ora},
    {"ASL", CPU::AddressMode::ZEROPAGE_X, 6, false, &CPU::asl},
    {},
    {"CLC", CPU::AddressMode::IMPLIED, 2, false, &CPU::clc},
    {"ORA", CPU::AddressMode::ABSOLUTE_Y, 4, true, &CPU::ora},
    {},
    {},
    {},
    {"ORA", CPU::AddressMode::ABSOLUTE_X, 4, true, &CPU::ora},
    {"ASL", CPU::AddressMode::ABSOLUTE_X, 7, false, &CPU::asl},
    {},
    {"JSR", CPU::AddressMode::ABSOLUTE, 6, false, &CPU::jsr},
    {"AND", CPU::AddressMode::X_INDIRECT, 6, false, &CPU::and_},
    {},
    {},
    {"BIT", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::bit},
    {"AND", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::and_},
    {"ROL", CPU::AddressMode::ZEROPAGE, 5, false, &CPU::rol},
    {},
    {"PLP", CPU::AddressMode::IMPLIED, 4, false, &CPU::plp},
    {"AND", CPU::AddressMode::IMMEDIATE, 2, false, &CPU::and_},
    {"ROL", CPU::AddressMode::ACCUMULATOR, 2, false, &CPU::rol},
    {},
    {"BIT", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::bit},
    {"AND", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::and_},
    {"ROL", CPU::AddressMode::ABSOLUTE, 6, false, &CPU::rol},
    {},
    {"BMI", CPU::AddressMode::RELATIVE, 2, false, &CPU::bmi},
    {"AND", CPU::AddressMode::INDIRECT_Y, 5, true, &CPU::and_},
    {},
    {},
    {},
    {"AND", CPU::AddressMode::ZEROPAGE_X, 4, false, &CPU::and_},
    {"ROL", CPU::AddressMode::ZEROPAGE_X, 6, false, &CPU::rol},
    {},
    {"SEC", CPU::AddressMode::IMPLIED, 2, false, &CPU::sec},
    {"AND", CPU::AddressMode::ABSOLUTE_Y, 4, true, &CPU::and_},
    {},
    {},
    {},
    {"AND", CPU::AddressMode::ABSOLUTE_X, 4, true, &CPU::and_},
    {"ROL", CPU::AddressMode::ABSOLUTE_X, 7, false, &CPU::rol},
    {},
    {"RTI", CPU::AddressMode::IMPLIED, 6, false, &CPU::rti},
    {"EOR", CPU::AddressMode::X_INDIRECT, 6, false, &CPU::eor},
    {},
    {},
    {},
    {"EOR", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::eor},
    {"LSR", CPU::AddressMode::ZEROPAGE, 5, false, &CPU::lsr},
    {},
    {"PHA", CPU::AddressMode::IMPLIED, 3, false, &CPU::pha},
    {"EOR", CPU::AddressMode::IMMEDIATE, 2, false, &CPU::eor},
    {"LSR", CPU::AddressMode::ACCUMULATOR, 2, false, &CPU::lsr},
    {},
    {"JMP", CPU::AddressMode::ABSOLUTE, 3, false, &CPU::jmp},
    {"EOR", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::eor},
    {"LSR", CPU::AddressMode::ABSOLUTE, 6, false, &CPU::lsr},
    {},
    {"BVC", CPU::AddressMode::RELATIVE, 2, false, &CPU::bvc},
    {"EOR", CPU::AddressMode::INDIRECT_Y, 5, true, &CPU::eor},
    {},
    {},
    {},
    {"EOR", CPU::AddressMode::ZEROPAGE_X, 4, false, &CPU::eor},
    {"LSR", CPU::AddressMode::ZEROPAGE_X, 6, false, &CPU::lsr},
    {},
    {"CLI", CPU::AddressMode::IMPLIED, 2, false, &CPU::cli},
    {"EOR", CPU::AddressMode::ABSOLUTE_Y, 4, true, &CPU::eor},
    {},
    {},
    {},
    {"EOR", CPU::AddressMode::ABSOLUTE_X, 4, true, &CPU::eor},
    {"LSR", CPU::AddressMode::ABSOLUTE_X, 7, false, &CPU::lsr},
    {},
    {"RTS", CPU::AddressMode::IMPLIED, 6, false, &CPU::rts},
    {"ADC", CPU::AddressMode::X_INDIRECT, 6, false, &CPU::adc},
    {},
    {},
    {},
    {"ADC", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::adc},
    {"ROR", CPU::AddressMode::ZEROPAGE, 5, false, &CPU::ror},
    {},
    {"PLA", CPU::AddressMode::IMPLIED, 4, false, &CPU::pla},
    {"ADC", CPU::AddressMode::IMMEDIATE, 2, false, &CPU::adc},
    {"ROR", CPU::AddressMode::ACCUMULATOR, 2, false, &CPU::ror},
    {},
    {"JMP", CPU::AddressMode::INDIRECT, 5, false, &CPU::jmp},
    {"ADC", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::adc},
    {"ROR", CPU::AddressMode::ABSOLUTE, 6, false, &CPU::ror},
    {},
    {"BVS", CPU::AddressMode::RELATIVE, 2, false, &CPU::bvs},
    {"ADC", CPU::AddressMode::INDIRECT_Y, 5, true, &CPU::adc},
    {},
    {},
    {},
    {"ADC", CPU::AddressMode::ZEROPAGE_X, 4, false, &CPU::adc},
    {"ROR", CPU::AddressMode::ZEROPAGE_X, 6, false, &CPU::ror},
    {},
    {"SEI", CPU::AddressMode::IMPLIED, 2, false, &CPU::sei},
    {"ADC", CPU::AddressMode::ABSOLUTE_Y, 4, true, &CPU::adc},
    {},
    {},
    {},
    {"ADC", CPU::AddressMode::ABSOLUTE_X, 4, true, &CPU::adc},
    {"ROR", CPU::AddressMode::ABSOLUTE_X, 7, false, &CPU::ror},
    {},
    {},
    {"STA", CPU::AddressMode::X_INDIRECT, 6, false, &CPU::sta},
    {},
    {},
    {"STY", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::sty},
    {"STA", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::sta},
    {"STX", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::stx},
    {},
    {"DEY", CPU::AddressMode::IMPLIED, 2, false, &CPU::dey},
    {},
    {"TXA", CPU::AddressMode::IMPLIED, 2, false, &CPU::txa},
    {},
    {"STY", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::sty},
    {"STA", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::sta},
    {"STX", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::stx},
    {},
    {"BCC", CPU::AddressMode::RELATIVE, 2, false, &CPU::bcc},
    {"STA", CPU::AddressMode::INDIRECT_Y, 6, false, &CPU::sta},
    {},
    {},
    {"STY", CPU::AddressMode::ZEROPAGE_X, 4, false, &CPU::sty},
    {"STA", CPU::AddressMode::ZEROPAGE_X, 4, false, &CPU::sta},
    {"STX", CPU::AddressMode::ZEROPAGE_Y, 4, false, &CPU::stx},
    {},
    {"TYA", CPU::AddressMode::IMPLIED, 2, false, &CPU::tya},
    {"STA", CPU::AddressMode::ABSOLUTE_Y, 5, false, &CPU::sta},
    {"TXS", CPU::AddressMode::IMPLIED, 2, false, &CPU::txs},
    {},
    {},
    {"STA", CPU::AddressMode::ABSOLUTE_X, 5, false, &CPU::sta},
    {},
    {},
    {"LDY", CPU::AddressMode::IMMEDIATE, 2, false, &CPU::ldy},
    {"LDA", CPU::AddressMode::X_INDIRECT, 6, false, &CPU::lda},
    {"LDX", CPU::AddressMode::IMMEDIATE, 2, false, &CPU::ldx},
    {},
    {"LDY", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::ldy},
    {"LDA", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::lda},
    {"LDX", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::ldx},
    {},
    {"TAY", CPU::AddressMode::IMPLIED, 2, false, &CPU::tay},
    {"LDA", CPU::AddressMode::IMMEDIATE, 2, false, &CPU::lda},
    {"TAX", CPU::AddressMode::IMPLIED, 2, false, &CPU::tax},
    {},
    {"LDY", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::ldy},
    {"LDA", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::lda},
    {"LDX", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::ldx},
    {},
    {"BCS", CPU::AddressMode::RELATIVE, 2, false, &CPU::bcs},
    {"LDA", CPU::AddressMode::INDIRECT_Y, 5, true, &CPU::lda},
    {},
    {},
    {"LDY", CPU::AddressMode::ZEROPAGE_X, 4, false, &CPU::ldy},
    {"LDA", CPU::AddressMode::ZEROPAGE_X, 4, false, &CPU::lda},
    {"LDX", CPU::AddressMode::ZEROPAGE_Y, 4, false, &CPU::ldx},
    {},
    {"CLV", CPU::AddressMode::IMPLIED, 2, false, &CPU::clv},
    {"LDA", CPU::AddressMode::ABSOLUTE_Y, 4, true, &CPU::lda},
    {"TSX", CPU::AddressMode::IMPLIED, 2, false, &CPU::tsx},
    {},
    {"LDY", CPU::AddressMode::ABSOLUTE_X, 4, true, &CPU::ldy},
    {"LDA", CPU::AddressMode::ABSOLUTE_X, 4, true, &CPU::lda},
    {"LDX", CPU::AddressMode::ABSOLUTE_Y, 4, true, &CPU::ldx},
    {},
    {"CPY", CPU::AddressMode::IMMEDIATE, 2, false, &CPU::cpy},
    {"CMP", CPU::AddressMode::X_INDIRECT, 6, false, &CPU::cmp},
    {},
    {},
    {"CPY", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::cpy},
    {"CMP", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::cmp},
    {"DEC", CPU::AddressMode::ZEROPAGE, 5, false, &CPU::dec},
    {},
    {"INY", CPU::AddressMode::IMPLIED, 2, false, &CPU::iny},
    {"CMP", CPU::AddressMode::IMMEDIATE, 2, false, &CPU::cmp},
    {"DEX", CPU::AddressMode::IMPLIED, 2, false, &CPU::dex},
    {},
    {"CPY", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::cpy},
    {"CMP", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::cmp},
    {"DEC", CPU::AddressMode::ABSOLUTE, 6, false, &CPU::dec},
    {},
    {"BNE", CPU::AddressMode::RELATIVE, 2, false, &CPU::bne},
    {"CMP", CPU::AddressMode::INDIRECT_Y, 5, true, &CPU::cmp},
    {},
    {},
    {},
    {"CMP", CPU::AddressMode::ZEROPAGE_X, 4, false, &CPU::cmp},
    {"DEC", CPU::AddressMode::ZEROPAGE_X, 6, false, &CPU::dec},
    {},
    {"CLD", CPU::AddressMode::IMPLIED, 2, false, &CPU::cld},
    {"CMP", CPU::AddressMode::ABSOLUTE_Y, 4, true, &CPU::cmp},
    {},
    {},
    {},
    {"CMP", CPU::AddressMode::ABSOLUTE_X, 4, true, &CPU::cmp},
    {"DEC", CPU::AddressMode::ABSOLUTE_X, 7, false, &CPU::dec},
    {},
    {"CPX", CPU::AddressMode::IMMEDIATE, 2, false, &CPU::cpx},
    {"SBC", CPU::AddressMode::X_INDIRECT, 6, false, &CPU::sbc},
    {},
    {},
    {"CPX", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::cpx},
    {"SBC", CPU::AddressMode::ZEROPAGE, 3, false, &CPU::sbc},
    {"INC", CPU::AddressMode::ZEROPAGE, 5, false, &CPU::inc},
    {},
    {"INX", CPU::AddressMode::IMPLIED, 2, false, &CPU::inx},
    {"SBC", CPU::AddressMode::IMMEDIATE, 2, false, &CPU::sbc},
    {"NOP", CPU::AddressMode::IMPLIED, 2, false, &CPU::nop},
    {},
    {"CPX", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::cpx},
    {"SBC", CPU::AddressMode::ABSOLUTE, 4, false, &CPU::sbc},
    {"INC", CPU::AddressMode::ABSOLUTE, 6, false, &CPU::inc},
    {},
    {"BEQ", CPU::AddressMode::RELATIVE, 2, false, &CPU::beq},
    {"SBC", CPU::AddressMode::INDIRECT_Y, 5, true, &CPU::sbc},
    {},
    {},
    {},
    {"SBC", CPU::AddressMode::ZEROPAGE_X, 4, false, &CPU::sbc},
    {"INC", CPU::AddressMode::ZEROPAGE_X, 6, false, &CPU::inc},
    {},
    {"SED", CPU::AddressMode::IMPLIED, 2, false, &CPU::sed},
    {"SBC", CPU::AddressMode::ABSOLUTE_Y, 4, true, &CPU::sbc},
    {},
    {},
    {},
    {"SBC", CPU::AddressMode::ABSOLUTE_X, 4, true, &CPU::sbc},
    {"INC", CPU::AddressMode::ABSOLUTE_X, 7, false, &CPU::inc}
};


CPU::CPU(CPUMemoryBus& bus) :
    nmi(false),
    a(0),
    x(0),
    y(0),
    p(0),
    sp(0),
    pc(0),
    addr_mode(AddressMode::NONE),
    operand_addr(0),
    curr_cycle(0),
    total_cycles(0),
    page_crossed(false),
    bus(bus)
{
}

void CPU::reset()
{
    a = 0x00;
    x = 0x00;
    y = 0x00;
    sp = 0xFD;
    p = Flag::INTERRUPT | Flag::UNUSED;
    pc = bus.read(RESET_VECTOR_ADDRESS) | bus.read(RESET_VECTOR_ADDRESS + 1) << 8;
    total_cycles += 7;
}

size_t CPU::step()
{
    curr_cycle = 0;
    const Instruction& instruction = INSTRUCTIONS[bus.read(pc++)];
    std::cout << instruction.mnemonic << std::endl;
    execute(instruction);
    return curr_cycle;
}

void CPU::execute(const Instruction& instruction)
{
    addr_mode = instruction.addr_mode;
    switch (addr_mode)
    {
    case AddressMode::ACCUMULATOR:
    case AddressMode::IMPLIED:
        break;
    case AddressMode::IMMEDIATE:
        operand_addr = resolve_immediate();
        break;
    case AddressMode::ABSOLUTE:
        operand_addr = resolve_absolute(0);
        break;
    case AddressMode::ZEROPAGE:
        operand_addr = resolve_zeropage(0);
        break;
    case AddressMode::ABSOLUTE_X:
        operand_addr = resolve_absolute(x);
        break;
    case AddressMode::ABSOLUTE_Y:
        operand_addr = resolve_absolute(y);
        break;
    case AddressMode::ZEROPAGE_X:
        operand_addr = resolve_zeropage(x);
        break;
    case AddressMode::ZEROPAGE_Y:
        operand_addr = resolve_zeropage(y);
        break;
    case AddressMode::INDIRECT:
        operand_addr = resolve_indirect();
        break;
    case AddressMode::X_INDIRECT:
        operand_addr = resolve_preindex_indirect();
        break;
    case AddressMode::INDIRECT_Y:
        operand_addr = resolve_postindex_indirect();
        break;
    case AddressMode::RELATIVE:
        operand_addr = resolve_relative();
        break;
    default:
        break;
    }
    (this->*instruction.func)();
    if (instruction.check_page_cross && page_crossed)
    {
        ++curr_cycle;
        page_crossed = false;
    }
    curr_cycle += instruction.cycles;
    total_cycles += curr_cycle;
}

void CPU::push_stack(uint8_t data)
{
    bus.write(STACK_BASE_ADDRESS | sp--, data);
}

uint8_t CPU::pop_stack()
{
    return bus.read(STACK_BASE_ADDRESS | ++sp);
}

void CPU::set_flag(Flag flag, uint8_t value)
{
    if (value)
    {
        p |= flag;
    }
    else
    {
        p &= ~flag;
    }
}

void CPU::set_nz_flags(uint8_t data)
{
    set_flag(Flag::ZERO, data == 0 ? 1 : 0);
    set_flag(Flag::NEGATIVE, (data >> 7) & 1);
}

bool CPU::is_page_crossed(uint16_t address1, uint16_t address2) const
{
    return (address1 & 0xFF00) != (address2 & 0xFF00);
}

uint16_t CPU::read_wrapped_page(uint16_t address) const
{
    uint16_t pointer = bus.read(address);
    if (is_page_crossed(address, address + 1))
    {
        pointer |= bus.read(address & 0xFF00) << 8;
    }
    else
    {
        pointer |= bus.read(address + 1) << 8;
    }
    return pointer;
}

uint16_t CPU::resolve_immediate()
{
    return pc++;
}

uint16_t CPU::resolve_zeropage(uint8_t offset)
{
    return (bus.read(pc++) + offset) & 0xFF;
}

uint16_t CPU::resolve_absolute(uint8_t offset)
{
    uint16_t base_addr = bus.read(pc) | bus.read(pc + 1) << 8;
    pc += 2;
    uint16_t effective_addr = (base_addr + offset) & 0xFFFF;
    page_crossed = is_page_crossed(base_addr, effective_addr);
    return effective_addr;
}

uint16_t CPU::resolve_indirect()
{
    uint16_t addr = bus.read(pc) | bus.read(pc + 1) << 8;
    pc += 2;
    return read_wrapped_page(addr);
}

uint16_t CPU::resolve_preindex_indirect()
{
    uint16_t zeropage_addr = (bus.read(pc++) + x) & 0xFF;
    return read_wrapped_page(zeropage_addr);
}

uint16_t CPU::resolve_postindex_indirect()
{
    uint16_t base_addr = read_wrapped_page(bus.read(pc++));
    uint16_t effective_addr = (base_addr + y) & 0xFFFF;
    page_crossed = is_page_crossed(base_addr, effective_addr);
    return effective_addr;
}

uint16_t CPU::resolve_relative()
{
    return resolve_immediate();
}

uint8_t CPU::get_operand() const
{
    return addr_mode == AddressMode::ACCUMULATOR ? a : bus.read(operand_addr);
}

void CPU::store(uint8_t data)
{
    if (addr_mode == AddressMode::ACCUMULATOR)
    {
        a = data;
    }
    else
    {
        bus.write(operand_addr, data);
    }
}

void CPU::brk()
{
    uint16_t return_addr = ++pc;
    push_stack(return_addr >> 8);
    push_stack(return_addr & 0xFF);
    push_stack(p | Flag::BREAK);
    p |= Flag::INTERRUPT;
    pc = bus.read(IRQ_VECTOR_ADDRESS) | bus.read(IRQ_VECTOR_ADDRESS + 1) << 8;
}

void CPU::ora()
{
    a |= get_operand();
    set_nz_flags(a);
}

void CPU::and_()
{
    a &= get_operand();
    set_nz_flags(a);
}

void CPU::eor()
{
    a ^= get_operand();
    set_nz_flags(a);
}

void CPU::asl()
{
    uint8_t operand = get_operand();
    uint8_t result = operand << 1 & 0xFF;
    set_flag(Flag::CARRY, operand >> 7);
    set_nz_flags(result);
    store(result);
}

void CPU::lsr()
{
    uint8_t operand = get_operand();
    uint8_t result = operand >> 1;
    set_flag(Flag::CARRY, operand & 1);
    set_nz_flags(result);
    store(result);
}

void CPU::rol()
{
    uint8_t operand = get_operand();
    uint8_t result = (operand << 1 | p & Flag::CARRY) & 0xFF;
    set_flag(Flag::CARRY, operand >> 7);
    set_nz_flags(result);
    store(result);
}

void CPU::ror()
{
    uint8_t operand = get_operand();
    uint8_t result = (operand >> 1 | (p & Flag::CARRY) << 7) & 0xFF;
    set_flag(Flag::CARRY, operand & 1);
    set_nz_flags(result);
    store(result);
}

void CPU::bit()
{
    uint8_t operand = get_operand();
    set_flag(Flag::ZERO, (a & operand) == 0 ? 1 : 0);
    set_flag(Flag::OVERFLOW_, (operand >> 6) & 1);
    set_flag(Flag::NEGATIVE, operand >> 7);
}

void CPU::php()
{
    push_stack(p | Flag::BREAK);
}

void CPU::plp()
{
    p = pop_stack() & ~Flag::BREAK | Flag::UNUSED;
}

void CPU::pha()
{
    push_stack(a);
}

void CPU::pla()
{
    a = pop_stack();
    set_nz_flags(a);
}

void CPU::clc()
{
    p &= ~Flag::CARRY;
}

void CPU::sec()
{
    p |= Flag::CARRY;
}

void CPU::cli()
{
    p &= ~Flag::INTERRUPT;
}

void CPU::sei()
{
    p |= Flag::INTERRUPT;
}

void CPU::clv()
{
    p &= ~Flag::OVERFLOW_;
}

void CPU::cld()
{
    p &= ~Flag::DECIMAL;
}

void CPU::sed()
{
    p |= Flag::DECIMAL;
}

void CPU::rti()
{
    p = pop_stack() | Flag::UNUSED;
    pc = pop_stack() | pop_stack() << 8;
}

void CPU::rts()
{
    pc = (pop_stack() | pop_stack() << 8) + 1;
}

void CPU::jmp()
{
    pc = operand_addr;
}

void CPU::adc_(uint8_t operand)
{
    uint16_t result = a + operand + (p & Flag::CARRY);
    set_flag(Flag::CARRY, result > 0xFF ? 1 : 0);
    set_flag(Flag::OVERFLOW_, ((a ^ result) & (operand ^ result) & 0x80) >> 7 ? 1 : 0);
    result &= 0xFF;
    set_nz_flags(result);
    a = result;
}

void CPU::adc()
{
    adc_(get_operand());
}

void CPU::sbc()
{
    adc_(get_operand() ^ 0xFF);
}

void CPU::sta()
{
    store(a);
}

void CPU::stx()
{
    store(x);
}

void CPU::sty()
{
    store(y);
}

void CPU::inx()
{
    x = (x + 1) & 0xFF;
    set_nz_flags(x);
}

void CPU::dex()
{
    x = (x - 1) & 0xFF;
    set_nz_flags(x);
}

void CPU::iny()
{
    y = (y + 1) & 0xFF;
    set_nz_flags(y);
}

void CPU::dey()
{
    y = (y - 1) & 0xFF;
    set_nz_flags(y);
}

void CPU::inc()
{
    uint8_t result = (get_operand() + 1) & 0xFF;
    set_nz_flags(result);
    store(result);
}

void CPU::dec()
{
    uint8_t result = (get_operand() - 1) & 0xFF;
    set_nz_flags(result);
    store(result);
}

void CPU::txa()
{
    a = x;
    set_nz_flags(a);
}

void CPU::tya()
{
    a = y;
    set_nz_flags(a);
}

void CPU::txs()
{
    sp = x;
}

void CPU::tay()
{
    y = a;
    set_nz_flags(y);
}

void CPU::tax()
{
    x = a;
    set_nz_flags(x);
}

void CPU::tsx()
{
    x = sp;
    set_nz_flags(x);
}

void CPU::lda()
{
    a = get_operand();
    set_nz_flags(a);
}

void CPU::ldx()
{
    x = get_operand();
    set_nz_flags(x);
}

void CPU::ldy()
{
    y = get_operand();
    set_nz_flags(y);
}

void CPU::cpx()
{
    uint8_t operand = get_operand();
    uint8_t result = x - operand;
    set_flag(Flag::CARRY, x >= operand ? 1 : 0);
    set_nz_flags(result);
}

void CPU::cpy()
{
    uint8_t operand = get_operand();
    uint8_t result = y - operand;
    set_flag(Flag::CARRY, y >= operand ? 1 : 0);
    set_nz_flags(result);
}

void CPU::cmp()
{
    uint8_t operand = get_operand();
    uint8_t result = a - operand;
    set_flag(Flag::CARRY, a >= operand ? 1 : 0);
    set_nz_flags(result);
}

void CPU::branch(bool condition)
{
    if (!condition)
    {
        return;
    }
    ++curr_cycle;
    uint8_t operand = get_operand();
    int8_t offset = (operand & 0x80) == 0x80 ? -(0x100 - operand) : operand;
    uint16_t addr = pc + offset;
    page_crossed = is_page_crossed(pc, addr);
    pc = addr;
}

void CPU::bpl()
{
    branch((p & Flag::NEGATIVE) == 0);
}

void CPU::bmi()
{
    branch((p & Flag::NEGATIVE) != 0);
}

void CPU::bvc()
{
    branch((p & Flag::OVERFLOW_) == 0);
}

void CPU::bvs()
{
    branch((p & Flag::OVERFLOW_) != 0);
}

void CPU::bcc()
{
    branch((p & Flag::CARRY) == 0);
}

void CPU::bcs()
{
    branch((p & Flag::CARRY) != 0);
}

void CPU::bne()
{
    branch((p & Flag::ZERO) == 0);
}

void CPU::beq()
{
    branch((p & Flag::ZERO) != 0);
}

void CPU::jsr()
{
    uint16_t return_addr = pc - 1;
    push_stack(return_addr >> 8);
    push_stack(return_addr & 0xFF);
    pc = operand_addr;
}

void CPU::nop()
{
}
