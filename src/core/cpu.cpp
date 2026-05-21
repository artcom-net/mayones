#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <variant>

#include "mayones/core/cpu.hpp"

namespace mayones::core {

// bool TraceEntry::operator==(const TraceEntry& other) const noexcept
// {
//     return opcode == other.opcode && mnemonic == other.mnemonic && operand == other.operand &&
//            a == other.a && x == other.x && y == other.y && p == other.p && sp == other.sp &&
//            pc == other.pc && cycles == other.cycles;
// }

const std::array<const Cpu::Instruction, Cpu::INSTRUCTIONS_TABLE_SIZE> Cpu::INSTRUCTIONS_TABLE = {
#include "cpu_instructions.inc"
};

std::size_t Cpu::reset_registers(std::uint16_t pc) noexcept
{
    constexpr std::size_t RESET_CYCLES{ 7 };
    a_ = 0x00;
    x_ = 0x00;
    y_ = 0x00;
    sp_ = 0xFD;
    p_ = Flag::INTERRUPT | Flag::UNUSED;
    pc_ = pc;
    total_cycles_ = RESET_CYCLES;
    return total_cycles_;
}

std::size_t Cpu::reset()
{
    std::uint16_t pc = bus_.read(RESET_VECTOR_ADDRESS) | (bus_.read(RESET_VECTOR_ADDRESS + 1) << 8);
    return reset_registers(pc);
}

std::size_t Cpu::reset(std::uint16_t pc)
{
    return reset_registers(pc);
}

std::size_t Cpu::tick()
{
    if (suspend_cycles_ > 0)
    {
        --suspend_cycles_;
        ++total_cycles_;
        return 1;
    }

    curr_cycle_ = 0;
    std::uint8_t opcode{ bus_.read(pc_++) };
    const Instruction& instruction = INSTRUCTIONS_TABLE[opcode];
    addr_mode_ = instruction.addr_mode;

    switch (instruction.addr_mode)
    {
        case AddressMode::ACCUMULATOR:
        case AddressMode::IMPLIED:
            break;
        case AddressMode::IMMEDIATE:
            operand_addr_ = resolve_immediate();
            break;
        case AddressMode::ABSOLUTE:
            operand_addr_ = resolve_absolute(0);
            break;
        case AddressMode::ZEROPAGE:
            operand_addr_ = resolve_zeropage(0);
            break;
        case AddressMode::ABSOLUTE_X:
            operand_addr_ = resolve_absolute(x_);
            break;
        case AddressMode::ABSOLUTE_Y:
            operand_addr_ = resolve_absolute(y_);
            break;
        case AddressMode::ZEROPAGE_X:
            operand_addr_ = resolve_zeropage(x_);
            break;
        case AddressMode::ZEROPAGE_Y:
            operand_addr_ = resolve_zeropage(y_);
            break;
        case AddressMode::INDIRECT:
            operand_addr_ = resolve_indirect();
            break;
        case AddressMode::X_INDIRECT:
            operand_addr_ = resolve_preindex_indirect();
            break;
        case AddressMode::INDIRECT_Y:
            operand_addr_ = resolve_postindex_indirect();
            break;
        case AddressMode::RELATIVE:
            operand_addr_ = resolve_relative();
            break;
        case AddressMode::UNKNOWN:
        default:
            std::unreachable();
    }
    (this->*instruction.func)();
    if (instruction.check_page_cross && page_crossed_)
    {
        ++curr_cycle_;
        page_crossed_ = false;
    }
    curr_cycle_ += instruction.cycles;
    total_cycles_ += curr_cycle_;
    return curr_cycle_;
}

TraceEntry Cpu::trace_tick()
{
    std::uint8_t trace_a{ a_ };
    std::uint8_t trace_x{ x_ };
    std::uint8_t trace_y{ y_ };
    std::uint8_t trace_p{ p_ };
    std::uint8_t trace_sp{ sp_ };
    std::uint16_t trace_pc{ pc_ };
    std::size_t trace_cycles{ total_cycles_ };
    std::variant<std::monostate, std::uint16_t, std::uint8_t> operand{};

    std::uint16_t tmp_pc{ trace_pc };

    std::uint8_t opcode = bus_.read(tmp_pc++);
    const Instruction& instruction{ INSTRUCTIONS_TABLE[opcode] };

    switch (instruction.addr_mode)
    {
        case AddressMode::ACCUMULATOR:
        case AddressMode::IMPLIED:
            break;
        case AddressMode::IMMEDIATE:
        case AddressMode::ZEROPAGE:
        case AddressMode::ZEROPAGE_X:
        case AddressMode::ZEROPAGE_Y:
        case AddressMode::X_INDIRECT:
        case AddressMode::INDIRECT_Y:
        case AddressMode::RELATIVE:
            operand = bus_.read(tmp_pc);
            break;
        case AddressMode::ABSOLUTE:
        case AddressMode::ABSOLUTE_X:
        case AddressMode::ABSOLUTE_Y:
        case AddressMode::INDIRECT:
            operand = static_cast<std::uint16_t>(bus_.read(tmp_pc) | bus_.read(tmp_pc + 1) << 8);
            break;
        case AddressMode::UNKNOWN:
        default:
            std::unreachable();
    }

    tick();

    return { .opcode = opcode,
             .mnemonic = std::string{ instruction.mnemonic },
             .operand = operand,
             .a = trace_a,
             .x = trace_x,
             .y = trace_y,
             .p = trace_p,
             .sp = trace_sp,
             .pc = trace_pc,
             .cycles = trace_cycles };
}

inline void Cpu::push_stack(std::uint8_t data)
{
    bus_.write(STACK_BASE_ADDRESS | sp_--, data);
}

inline std::uint8_t Cpu::pop_stack()
{
    return bus_.read(STACK_BASE_ADDRESS | ++sp_);
}

inline void Cpu::set_flag(Flag flag, std::uint8_t value)
{
    if (value)
    {
        p_ |= flag;
    }
    else
    {
        p_ &= ~flag;
    }
}

inline void Cpu::set_nz_flags(std::uint8_t data)
{
    set_flag(Flag::ZERO, data == 0 ? 1 : 0);
    set_flag(Flag::NEGATIVE, (data >> 7) & 1);
}

inline bool Cpu::is_page_crossed(std::uint16_t address1, std::uint16_t address2) const
{
    return (address1 & 0xFF00) != (address2 & 0xFF00);
}

inline std::uint16_t Cpu::read_wrapped_page(uint16_t address) const
{
    uint16_t pointer = bus_.read(address);
    if (is_page_crossed(address, address + 1))
    {
        pointer |= bus_.read(address & 0xFF00) << 8;
    }
    else
    {
        pointer |= bus_.read(address + 1) << 8;
    }
    return pointer;
}

inline std::uint16_t Cpu::resolve_immediate()
{
    return pc_++;
}

inline std::uint16_t Cpu::resolve_zeropage(std::uint8_t offset)
{
    return (bus_.read(pc_++) + offset) & 0xFF;
}

inline std::uint16_t Cpu::resolve_absolute(std::uint8_t offset)
{
    std::uint16_t base_addr = bus_.read(pc_) | bus_.read(pc_ + 1) << 8;
    pc_ += 2;
    std::uint16_t effective_addr = (base_addr + offset) & 0xFFFF;
    page_crossed_ = is_page_crossed(base_addr, effective_addr);
    return effective_addr;
}

inline uint16_t Cpu::resolve_indirect()
{
    std::uint16_t addr = bus_.read(pc_) | bus_.read(pc_ + 1) << 8;
    pc_ += 2;
    return read_wrapped_page(addr);
}

inline std::uint16_t Cpu::resolve_preindex_indirect()
{
    std::uint16_t zeropage_addr = (bus_.read(pc_++) + x_) & 0xFF;
    return read_wrapped_page(zeropage_addr);
}

inline std::uint16_t Cpu::resolve_postindex_indirect()
{
    std::uint16_t base_addr = read_wrapped_page(bus_.read(pc_++));
    std::uint16_t effective_addr = (base_addr + y_) & 0xFFFF;
    page_crossed_ = is_page_crossed(base_addr, effective_addr);
    return effective_addr;
}

inline std::uint16_t Cpu::resolve_relative()
{
    return resolve_immediate();
}

inline std::uint8_t Cpu::get_operand() const
{
    return addr_mode_ == AddressMode::ACCUMULATOR ? a_ : bus_.read(operand_addr_);
}

inline void Cpu::store(std::uint8_t data)
{
    if (addr_mode_ == AddressMode::ACCUMULATOR)
    {
        a_ = data;
    }
    else
    {
        bus_.write(operand_addr_, data);
    }
}

void Cpu::brk()
{
    std::uint16_t return_addr = ++pc_;
    push_stack(return_addr >> 8);
    push_stack(return_addr & 0xFF);
    push_stack(p_ | Flag::BREAK);
    p_ |= Flag::INTERRUPT;
    pc_ = bus_.read(IRQ_VECTOR_ADDRESS) | bus_.read(IRQ_VECTOR_ADDRESS + 1) << 8;
}

void Cpu::ora()
{
    a_ |= get_operand();
    set_nz_flags(a_);
}

void Cpu::and_()
{
    a_ &= get_operand();
    set_nz_flags(a_);
}

void Cpu::eor()
{
    a_ ^= get_operand();
    set_nz_flags(a_);
}

void Cpu::asl()
{
    std::uint8_t operand = get_operand();
    std::uint8_t result = operand << 1 & 0xFF;
    set_flag(Flag::CARRY, operand >> 7);
    set_nz_flags(result);
    store(result);
}

void Cpu::lsr()
{
    std::uint8_t operand = get_operand();
    std::uint8_t result = operand >> 1;
    set_flag(Flag::CARRY, operand & 1);
    set_nz_flags(result);
    store(result);
}

void Cpu::rol()
{
    std::uint8_t operand = get_operand();
    std::uint8_t result = (operand << 1 | p_ & Flag::CARRY) & 0xFF;
    set_flag(Flag::CARRY, operand >> 7);
    set_nz_flags(result);
    store(result);
}

void Cpu::ror()
{
    std::uint8_t operand = get_operand();
    std::uint8_t result = (operand >> 1 | (p_ & Flag::CARRY) << 7) & 0xFF;
    set_flag(Flag::CARRY, operand & 1);
    set_nz_flags(result);
    store(result);
}

void Cpu::bit()
{
    std::uint8_t operand = get_operand();
    set_flag(Flag::ZERO, (a_ & operand) == 0 ? 1 : 0);
    set_flag(Flag::OVERFLOW_, (operand >> 6) & 1);
    set_flag(Flag::NEGATIVE, operand >> 7);
}

void Cpu::php()
{
    push_stack(p_ | Flag::BREAK);
}

void Cpu::plp()
{
    p_ = pop_stack() & ~Flag::BREAK | Flag::UNUSED;
}

void Cpu::pha()
{
    push_stack(a_);
}

void Cpu::pla()
{
    a_ = pop_stack();
    set_nz_flags(a_);
}

void Cpu::clc()
{
    p_ &= ~Flag::CARRY;
}

void Cpu::sec()
{
    p_ |= Flag::CARRY;
}

void Cpu::cli()
{
    p_ &= ~Flag::INTERRUPT;
}

void Cpu::sei()
{
    p_ |= Flag::INTERRUPT;
}

void Cpu::clv()
{
    p_ &= ~Flag::OVERFLOW_;
}

void Cpu::cld()
{
    p_ &= ~Flag::DECIMAL;
}

void Cpu::sed()
{
    p_ |= Flag::DECIMAL;
}

void Cpu::rti()
{
    p_ = pop_stack() | Flag::UNUSED;
    pc_ = pop_stack() | pop_stack() << 8;
}

void Cpu::rts()
{
    pc_ = (pop_stack() | pop_stack() << 8) + 1;
}

void Cpu::jmp()
{
    pc_ = operand_addr_;
}

void Cpu::adc_(std::uint8_t operand)
{
    std::uint16_t result = a_ + operand + (p_ & Flag::CARRY);
    set_flag(Flag::CARRY, result > 0xFF ? 1 : 0);
    set_flag(Flag::OVERFLOW_, ((a_ ^ result) & (operand ^ result) & 0x80) >> 7 ? 1 : 0);
    result &= 0xFF;
    set_nz_flags(result);
    a_ = result;
}

void Cpu::adc()
{
    adc_(get_operand());
}

void Cpu::sbc()
{
    adc_(get_operand() ^ 0xFF);
}

void Cpu::sta()
{
    store(a_);
}

void Cpu::stx()
{
    store(x_);
}

void Cpu::sty()
{
    store(y_);
}

void Cpu::inx()
{
    x_ = (x_ + 1) & 0xFF;
    set_nz_flags(x_);
}

void Cpu::dex()
{
    x_ = (x_ - 1) & 0xFF;
    set_nz_flags(x_);
}

void Cpu::iny()
{
    y_ = (y_ + 1) & 0xFF;
    set_nz_flags(y_);
}

void Cpu::dey()
{
    y_ = (y_ - 1) & 0xFF;
    set_nz_flags(y_);
}

void Cpu::inc()
{
    std::uint8_t result = (get_operand() + 1) & 0xFF;
    set_nz_flags(result);
    store(result);
}

void Cpu::dec()
{
    std::uint8_t result = (get_operand() - 1) & 0xFF;
    set_nz_flags(result);
    store(result);
}

void Cpu::txa()
{
    a_ = x_;
    set_nz_flags(a_);
}

void Cpu::tya()
{
    a_ = y_;
    set_nz_flags(a_);
}

void Cpu::txs()
{
    sp_ = x_;
}

void Cpu::tay()
{
    y_ = a_;
    set_nz_flags(y_);
}

void Cpu::tax()
{
    x_ = a_;
    set_nz_flags(x_);
}

void Cpu::tsx()
{
    x_ = sp_;
    set_nz_flags(x_);
}

void Cpu::lda()
{
    a_ = get_operand();
    set_nz_flags(a_);
}

void Cpu::ldx()
{
    x_ = get_operand();
    set_nz_flags(x_);
}

void Cpu::ldy()
{
    y_ = get_operand();
    set_nz_flags(y_);
}

void Cpu::cpx()
{
    std::uint8_t operand = get_operand();
    std::uint8_t result = x_ - operand;
    set_flag(Flag::CARRY, x_ >= operand ? 1 : 0);
    set_nz_flags(result);
}

void Cpu::cpy()
{
    std::uint8_t operand = get_operand();
    std::uint8_t result = y_ - operand;
    set_flag(Flag::CARRY, y_ >= operand ? 1 : 0);
    set_nz_flags(result);
}

void Cpu::cmp()
{
    std::uint8_t operand = get_operand();
    std::uint8_t result = a_ - operand;
    set_flag(Flag::CARRY, a_ >= operand ? 1 : 0);
    set_nz_flags(result);
}

void Cpu::branch(bool condition)
{
    if (!condition)
    {
        return;
    }
    ++curr_cycle_;
    std::uint8_t operand = get_operand();
    std::int8_t offset = (operand & 0x80) == 0x80 ? -(0x100 - operand) : operand;
    std::uint16_t addr = pc_ + offset;
    page_crossed_ = is_page_crossed(pc_, addr);
    pc_ = addr;
}

void Cpu::bpl()
{
    branch((p_ & Flag::NEGATIVE) == 0);
}

void Cpu::bmi()
{
    branch((p_ & Flag::NEGATIVE) != 0);
}

void Cpu::bvc()
{
    branch((p_ & Flag::OVERFLOW_) == 0);
}

void Cpu::bvs()
{
    branch((p_ & Flag::OVERFLOW_) != 0);
}

void Cpu::bcc()
{
    branch((p_ & Flag::CARRY) == 0);
}

void Cpu::bcs()
{
    branch((p_ & Flag::CARRY) != 0);
}

void Cpu::bne()
{
    branch((p_ & Flag::ZERO) == 0);
}

void Cpu::beq()
{
    branch((p_ & Flag::ZERO) != 0);
}

void Cpu::jsr()
{
    std::uint16_t return_addr = pc_ - 1;
    push_stack(return_addr >> 8);
    push_stack(return_addr & 0xFF);
    pc_ = operand_addr_;
}

void Cpu::nop()
{
}

void Cpu::kil()
{
    throw std::runtime_error("KIL instruction encountered");
}

void Cpu::slo()
{
    asl();
    ora();
}

void Cpu::anc()
{
    a_ &= get_operand();
    set_nz_flags(a_);
    set_flag(Flag::CARRY, a_ >> 7);
}

void Cpu::rla()
{
    rol();
    and_();
}

void Cpu::sre()
{
    lsr();
    eor();
}

void Cpu::alr()
{
    and_();
    lsr();
}

void Cpu::rra()
{
    ror();
    adc();
}

void Cpu::arr()
{
    // https://www.nesdev.org/wiki/Programming_with_unofficial_opcodes
    //
    // Similar to AND #i then ROR A, except sets the flags differently. N and Z are normal, but C is
    // bit 6 and V is bit 6 xor bit 5
    std::uint8_t tmp = a_ & get_operand();
    a_ = (tmp >> 1 | (tmp & Flag::CARRY) << 7) & 0xFF;
    set_flag(Flag::CARRY, (a_ >> 6) & 1);
    set_flag(Flag::OVERFLOW_, ((a_ >> 6) & 1) ^ ((a_ >> 5) & 1));
    set_nz_flags(a_);
}

void Cpu::sax()
{
    store(a_ & x_);
}

void Cpu::ane()
{
    a_ = ((a_ | 0xEE) & x_ & get_operand());
    set_nz_flags(a_);
}

void Cpu::sha()
{
    store(a_ & x_ & ((operand_addr_ >> 8) + 1));
}

void Cpu::tas()
{
    sp_ = a_ & x_;
    store(sp_ & ((operand_addr_ >> 8) + 1));
}

void Cpu::shy()
{
    store(y_ & ((operand_addr_ >> 8) + 1));
}

void Cpu::shx()
{
    store(x_ & ((operand_addr_ >> 8) + 1));
}

void Cpu::lax()
{
    a_ = get_operand();
    x_ = a_;
    set_nz_flags(a_);
}

void Cpu::lxa()
{
    a_ = (a_ | 0xEE) & get_operand();
    x_ = a_;
    set_nz_flags(a_);
}

void Cpu::las()
{
    a_ = sp_ & get_operand();
    x_ = a_;
    sp_ = a_;
    set_nz_flags(a_);
}

void Cpu::dcp()
{
    dec();
    cmp();
}

void Cpu::sbx()
{
    std::uint8_t operand = get_operand();
    std::uint8_t x_and_a = x_ & a_;
    x_ = x_and_a - operand;
    set_flag(Flag::CARRY, x_and_a >= operand ? 1 : 0);
    set_nz_flags(x_);
}

void Cpu::isb()
{
    inc();
    sbc();
}

} // namespace mayones::core
