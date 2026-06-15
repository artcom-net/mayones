#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <variant>

#include "mayones/core/cpu.hpp"

namespace {

bool is_page_crossed(std::uint16_t address1, std::uint16_t address2)
{
    return (address1 & 0xFF00) != (address2 & 0xFF00);
}

} // namespace

namespace mayones::core {

const std::array<const Cpu::Instruction, Cpu::INSTRUCTIONS_TABLE_SIZE> Cpu::INSTRUCTION_TABLE = {
#include "cpu_instructions.inc"
};

const Cpu::Instruction Cpu::NMI_INSTRUCTION{ .mnemonic = "NMI",
                                             .addr_mode = Cpu::AddressMode::IMPLIED,
                                             .cycles = 7,
                                             .check_page_cross = false,
                                             .func = &Cpu::tick_nmi };

Cpu::Cpu(CpuBus& bus) :
    bus_{ bus }
{
}

void Cpu::reset_registers(std::uint16_t pc)
{
    core_ctx_.a = 0x00;
    core_ctx_.x = 0x00;
    core_ctx_.y = 0x00;
    core_ctx_.sp = 0xFD;
    core_ctx_.flags = Flag::INTERRUPT | Flag::UNUSED;
    core_ctx_.pc = pc;
    total_cycles_ = 7;
}

void Cpu::reset()
{
    reset_registers(bus_.read(RESET_VECTOR_ADDRESS) | (bus_.read(RESET_VECTOR_ADDRESS + 1) << 8));
}

void Cpu::reset(std::uint16_t pc)
{
    reset_registers(pc);
}

void Cpu::push_stack(std::uint8_t data)
{
    bus_.write(STACK_BASE_ADDRESS | core_ctx_.sp--, data);
}

std::uint8_t Cpu::pop_stack()
{
    return bus_.read(STACK_BASE_ADDRESS | ++core_ctx_.sp);
}

void Cpu::set_flag(Flag flag, std::uint8_t value)
{
    if (value)
    {
        core_ctx_.flags |= flag;
    }
    else
    {
        core_ctx_.flags &= ~flag;
    }
}

void Cpu::set_nz_flags(std::uint8_t data)
{
    set_flag(Flag::ZERO, data == 0 ? 1 : 0);
    set_flag(Flag::NEGATIVE, (data >> 7) & 1);
}

std::uint16_t Cpu::read_wrapped_page(std::uint16_t address, std::uint16_t pointer)
{
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

void Cpu::resolve_indexed_zeropage_address(std::uint8_t index)
{
    switch (exec_ctx_.address_mode_cycles_left)
    {
        case 2:
            exec_ctx_.operand_address = bus_.read(core_ctx_.pc++);
            break;
        case 1:
            exec_ctx_.operand_address = (exec_ctx_.operand_address + index) & 0x00FF;
            break;
        default:
            std::unreachable();
    }
}

void Cpu::resolve_indexed_absolute_address(std::uint8_t index)
{
    switch (exec_ctx_.address_mode_cycles_left)
    {
        case 3:
            exec_ctx_.operand_address = bus_.read(core_ctx_.pc++);
            break;
        case 2: {
            exec_ctx_.operand_address |= bus_.read(core_ctx_.pc++) << 8;
            bool page_cross =
              is_page_crossed(exec_ctx_.operand_address, exec_ctx_.operand_address + index);
            if (exec_ctx_.instruction_ptr->check_page_cross)
            {
                if (page_cross)
                {
                    ++exec_ctx_.total_cycles_left;
                }
                else
                {
                    --exec_ctx_.address_mode_cycles_left;
                }
            }
            exec_ctx_.operand_address += index;
            break;
        }
        case 1:
            // page cross penalty
            break;
        default:
            std::unreachable();
    }
}

void Cpu::tick_dma()
{
    switch (dma_ctx_.cycles_left & 1)
    {
        case 0:
            dma_ctx_.data = bus_.read(dma_ctx_.address++);
            break;
        case 1:
            bus_.write(0x2004, dma_ctx_.data);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::tick_nmi()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 6:
            push_stack(core_ctx_.pc >> 8);
            break;
        case 5:
            push_stack(core_ctx_.pc & 0xFF);
            break;
        case 4:
            push_stack(core_ctx_.flags & ~Flag::BREAK);
            break;
        case 3:
            core_ctx_.flags |= Flag::INTERRUPT;
            exec_ctx_.result_u16 = bus_.read(NMI_VECTOR_ADDRESS);
            break;
        case 2:
            exec_ctx_.result_u16 |= bus_.read(NMI_VECTOR_ADDRESS + 1) << 8;
            break;
        case 1:
            core_ctx_.pc = exec_ctx_.result_u16;
            break;
        default:
            std::unreachable();
    }
}

void Cpu::tick()
{
    // if (dma_ctx_.cycles_left > 0)
    // {
    //     if (dma_ctx_.halt_cycles > 0)
    //     {
    //         --dma_ctx_.halt_cycles;
    //         ++total_cycles_;
    //         return;
    //     }

    //     tick_dma();

    //     --dma_ctx_.cycles_left;
    //     ++total_cycles_;

    //     return;
    // }

    if (exec_ctx_.total_cycles_left == 0)
    {
        exec_ctx_ = ExecutionContext{};

        if (nmi_pending_)
        {
            nmi_pending_ = !nmi_pending_;
            exec_ctx_.instruction_ptr = &NMI_INSTRUCTION;
        }
        else
        {
            std::uint8_t opcode = bus_.read(core_ctx_.pc++);
            exec_ctx_.instruction_ptr = &INSTRUCTION_TABLE[opcode];
        }
        // minus 1 fetch opcode or dummy cycle for NMI
        exec_ctx_.total_cycles_left = exec_ctx_.instruction_ptr->cycles - 1;
        exec_ctx_.address_mode_cycles_left =
          ADDRESS_MODE_CYCLE_TABLE[std::to_underlying(exec_ctx_.instruction_ptr->addr_mode)];

        ++total_cycles_;

        return;
    }

    if (exec_ctx_.address_mode_cycles_left > 0)
    {
        switch (exec_ctx_.instruction_ptr->addr_mode)
        {
            case AddressMode::IMMEDIATE:
            case AddressMode::RELATIVE:
                exec_ctx_.operand_address = core_ctx_.pc++;
                break;
            case AddressMode::ABSOLUTE:
                switch (exec_ctx_.address_mode_cycles_left)
                {
                    case 2:
                        exec_ctx_.operand_address = bus_.read(core_ctx_.pc++);
                        break;
                    case 1:
                        exec_ctx_.operand_address |= bus_.read(core_ctx_.pc++) << 8;
                        break;
                    default:
                        std::unreachable();
                }
                break;
            case AddressMode::ZEROPAGE:
                exec_ctx_.operand_address = bus_.read(core_ctx_.pc++);
                break;
            case AddressMode::ABSOLUTE_X:
                resolve_indexed_absolute_address(core_ctx_.x);
                break;
            case AddressMode::ABSOLUTE_Y:
                resolve_indexed_absolute_address(core_ctx_.y);
                break;
            case AddressMode::ZEROPAGE_X:
                resolve_indexed_zeropage_address(core_ctx_.x);
                break;
            case AddressMode::ZEROPAGE_Y:
                resolve_indexed_zeropage_address(core_ctx_.y);
                break;
            case AddressMode::INDIRECT:
                switch (exec_ctx_.address_mode_cycles_left)
                {
                    case 4:
                        exec_ctx_.tmp_operand_address = bus_.read(core_ctx_.pc++);
                        break;
                    case 3:
                        exec_ctx_.tmp_operand_address |= bus_.read(core_ctx_.pc++) << 8;
                        break;
                    case 2: {
                        exec_ctx_.operand_address = bus_.read(exec_ctx_.tmp_operand_address);
                        break;
                    }
                    case 1:
                        exec_ctx_.operand_address = read_wrapped_page(exec_ctx_.tmp_operand_address,
                                                                      exec_ctx_.operand_address);
                        break;
                    default:
                        std::unreachable();
                }
                break;
            case AddressMode::X_INDIRECT:
                switch (exec_ctx_.address_mode_cycles_left)
                {
                    case 4:
                        exec_ctx_.tmp_operand_address =
                          bus_.read(core_ctx_.pc++); // IS NOT OPERAND ADDRESS, IS VALUE
                        break;
                    case 3:
                        exec_ctx_.tmp_operand_address =
                          (exec_ctx_.tmp_operand_address + core_ctx_.x) & 0xFF;
                        break;
                    case 2:
                        exec_ctx_.operand_address = bus_.read(exec_ctx_.tmp_operand_address);
                        break;
                    case 1:
                        exec_ctx_.operand_address = read_wrapped_page(exec_ctx_.tmp_operand_address,
                                                                      exec_ctx_.operand_address);
                        break;
                    default:
                        std::unreachable();
                }
                break;
            case AddressMode::INDIRECT_Y:
                switch (exec_ctx_.address_mode_cycles_left)
                {
                    case 5:
                        exec_ctx_.tmp_operand_address = bus_.read(core_ctx_.pc++);
                        break;
                    case 4:
                        exec_ctx_.operand_address = bus_.read(exec_ctx_.tmp_operand_address);
                        break;
                    case 3:
                        exec_ctx_.operand_address = read_wrapped_page(exec_ctx_.tmp_operand_address,
                                                                      exec_ctx_.operand_address);
                        break;
                    case 2: {
                        bool page_cross = is_page_crossed(exec_ctx_.operand_address,
                                                          exec_ctx_.operand_address + core_ctx_.y);
                        if (exec_ctx_.instruction_ptr->check_page_cross && page_cross)
                        {
                            ++exec_ctx_.total_cycles_left;
                        }
                        else
                        {
                            --exec_ctx_.address_mode_cycles_left;
                        }
                        exec_ctx_.operand_address += core_ctx_.y;
                        break;
                    }
                    case 1:
                        // page cross penalty
                        break;
                    default:
                        std::unreachable();
                }
                break;
            default:
                std::unreachable();
        }

        --exec_ctx_.total_cycles_left;
        --exec_ctx_.address_mode_cycles_left;
        ++total_cycles_;

        if (exec_ctx_.total_cycles_left == 0)
        {
            (this->*exec_ctx_.instruction_ptr->func)();
        }

        return;
    }

    (this->*exec_ctx_.instruction_ptr->func)();
    --exec_ctx_.total_cycles_left;
    ++total_cycles_;
}

Cpu::TraceEntry Cpu::trace_tick()
{
    while (exec_ctx_.total_cycles_left != 0)
    {
        tick();
    }

    std::uint8_t trace_a{ core_ctx_.a };
    std::uint8_t trace_x{ core_ctx_.x };
    std::uint8_t trace_y{ core_ctx_.y };
    std::uint8_t trace_p{ core_ctx_.flags };
    std::uint8_t trace_sp{ core_ctx_.sp };
    std::uint16_t trace_pc{ core_ctx_.pc };
    std::size_t trace_cycles{ total_cycles_ };
    std::variant<std::monostate, std::uint16_t, std::uint8_t> operand{};

    std::uint16_t tmp_pc{ trace_pc };
    std::uint8_t opcode = bus_.read(tmp_pc++);
    const Instruction* trace_instruction_ptr = &INSTRUCTION_TABLE[opcode];

    switch (trace_instruction_ptr->addr_mode)
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
        default:
            std::unreachable();
    }

    tick();

    return { .opcode = opcode,
             .mnemonic = std::string{ trace_instruction_ptr->mnemonic },
             .operand = operand,
             .a = trace_a,
             .x = trace_x,
             .y = trace_y,
             .p = trace_p,
             .sp = trace_sp,
             .pc = trace_pc,
             .cycles = trace_cycles };
}

void Cpu::trigger_nmi()
{
    nmi_pending_ = true;
}

std::uint8_t Cpu::read_operand()
{
    if (exec_ctx_.instruction_ptr->addr_mode == AddressMode::ACCUMULATOR)
    {
        return core_ctx_.a;
    }
    return bus_.read(exec_ctx_.operand_address);
}

void Cpu::store(std::uint8_t data)
{
    if (exec_ctx_.instruction_ptr->addr_mode == AddressMode::ACCUMULATOR)
    {
        core_ctx_.a = data;
    }
    else
    {
        bus_.write(exec_ctx_.operand_address, data);
        // if (exec_ctx_.operand_address == 0x4014)
        // {
        //     dma_ctx_ = { .address = static_cast<std::uint16_t>(data << 8),
        //                  .cycles_left = DMA_CYCLES,
        //                  .halt_cycles = static_cast<std::uint8_t>(1 + (total_cycles_ & 1)) };
        // }
        // else
        // {
        //     bus_.write(exec_ctx_.operand_address, data);
        // }
    }
}

void Cpu::brk()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 6:
            // Although BRK only uses 1 byte, its return address skips the following byte
            ++core_ctx_.pc;
            break;
        case 5:
            push_stack(core_ctx_.pc >> 8);
            break;
        case 4:
            push_stack(core_ctx_.pc & 0xFF);
            break;
        case 3:
            push_stack(core_ctx_.flags | Flag::BREAK);
            core_ctx_.flags |= Flag::INTERRUPT;
            break;
        case 2:
            core_ctx_.pc = bus_.read(IRQ_VECTOR_ADDRESS);
            break;
        case 1:
            core_ctx_.pc |= bus_.read(IRQ_VECTOR_ADDRESS + 1) << 8;
            break;
        default:
            std::unreachable();
    }
}

void Cpu::ora()
{
    core_ctx_.a |= read_operand();
    set_nz_flags(core_ctx_.a);
}

void Cpu::and_()
{
    core_ctx_.a &= read_operand();
    set_nz_flags(core_ctx_.a);
}

void Cpu::eor()
{
    core_ctx_.a ^= read_operand();
    set_nz_flags(core_ctx_.a);
}

void Cpu::asl()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
            exec_ctx_.operand = read_operand();
            break;
        case 2:
            exec_ctx_.result_u8 = (exec_ctx_.operand << 1) & 0xFF;
            store(exec_ctx_.operand);
            break;
        case 1:
            set_flag(Flag::CARRY, exec_ctx_.operand >> 7);
            set_nz_flags(exec_ctx_.result_u8);
            store(exec_ctx_.result_u8);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::asl_a()
{
    std::uint8_t result = (core_ctx_.a << 1) & 0xFF;
    set_flag(Flag::CARRY, core_ctx_.a >> 7);
    set_nz_flags(result);
    core_ctx_.a = result;
}

void Cpu::lsr()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
            exec_ctx_.operand = read_operand();
            break;
        case 2:
            exec_ctx_.result_u8 = exec_ctx_.operand >> 1;
            store(exec_ctx_.operand);
            break;
        case 1:
            set_flag(Flag::CARRY, exec_ctx_.operand & 1);
            set_nz_flags(exec_ctx_.result_u8);
            store(exec_ctx_.result_u8);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::lsr_a()
{
    exec_ctx_.result_u8 = core_ctx_.a >> 1;
    set_flag(Flag::CARRY, core_ctx_.a & 1);
    set_nz_flags(exec_ctx_.result_u8);
    core_ctx_.a = exec_ctx_.result_u8;
}

void Cpu::rol()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
            exec_ctx_.operand = read_operand();
            break;
        case 2:
            exec_ctx_.result_u8 = (exec_ctx_.operand << 1 | (core_ctx_.flags & Flag::CARRY)) & 0xFF;
            store(exec_ctx_.operand);
            break;
        case 1:
            set_flag(Flag::CARRY, exec_ctx_.operand >> 7);
            set_nz_flags(exec_ctx_.result_u8);
            store(exec_ctx_.result_u8);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::rol_a()
{
    exec_ctx_.result_u8 = (core_ctx_.a << 1 | (core_ctx_.flags & Flag::CARRY)) & 0xFF;
    set_flag(Flag::CARRY, core_ctx_.a >> 7);
    set_nz_flags(exec_ctx_.result_u8);
    core_ctx_.a = exec_ctx_.result_u8;
}

void Cpu::ror()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
            exec_ctx_.operand = read_operand();
            break;
        case 2:
            exec_ctx_.result_u8 =
              (exec_ctx_.operand >> 1 | (core_ctx_.flags & Flag::CARRY) << 7) & 0xFF;
            store(exec_ctx_.operand);
            break;
        case 1:
            set_flag(Flag::CARRY, exec_ctx_.operand & 1);
            set_nz_flags(exec_ctx_.result_u8);
            store(exec_ctx_.result_u8);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::ror_a()
{
    exec_ctx_.result_u8 = (core_ctx_.a >> 1 | (core_ctx_.flags & Flag::CARRY) << 7) & 0xFF;
    set_flag(Flag::CARRY, core_ctx_.a & 1);
    set_nz_flags(exec_ctx_.result_u8);
    core_ctx_.a = exec_ctx_.result_u8;
}

void Cpu::bit()
{
    exec_ctx_.operand = read_operand();
    set_flag(Flag::ZERO, (core_ctx_.a & exec_ctx_.operand) == 0 ? 1 : 0);
    set_flag(Flag::OVERFLOW_, (exec_ctx_.operand >> 6) & 1);
    set_flag(Flag::NEGATIVE, exec_ctx_.operand >> 7);
}

void Cpu::php()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 2:
            break;
        case 1:
            push_stack(core_ctx_.flags | Flag::BREAK);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::plp()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
        case 2:
            break;
        case 1:
            core_ctx_.flags = (pop_stack() & ~Flag::BREAK) | Flag::UNUSED;
            break;
        default:
            std::unreachable();
    }
}

void Cpu::pha()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
        case 2:
            break;
        case 1:
            push_stack(core_ctx_.a);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::pla()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
        case 2:
            break;
        case 1:
            core_ctx_.a = pop_stack();
            set_nz_flags(core_ctx_.a);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::clc()
{
    core_ctx_.flags &= ~Flag::CARRY;
}

void Cpu::sec()
{
    core_ctx_.flags |= Flag::CARRY;
}

void Cpu::cli()
{
    core_ctx_.flags &= ~Flag::INTERRUPT;
}

void Cpu::sei()
{
    core_ctx_.flags |= Flag::INTERRUPT;
}

void Cpu::clv()
{
    core_ctx_.flags &= ~Flag::OVERFLOW_;
}

void Cpu::cld()
{
    core_ctx_.flags &= ~Flag::DECIMAL;
}

void Cpu::sed()
{
    core_ctx_.flags |= Flag::DECIMAL;
}

void Cpu::rti()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 5:
            // dummy bus read
            break;
        case 4:
            core_ctx_.flags = pop_stack() | Flag::UNUSED;
            break;
        case 3:
            exec_ctx_.result_u16 = pop_stack();
            break;
        case 2:
            exec_ctx_.result_u16 |= pop_stack() << 8;
            break;
        case 1:
            core_ctx_.pc = exec_ctx_.result_u16;
            break;
        default:
            std::unreachable();
    }
}

void Cpu::rts()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 5:
        case 4:
            // dummy bus read
            break;
        case 3:
            exec_ctx_.result_u16 = pop_stack();
            break;
        case 2:
            exec_ctx_.result_u16 |= pop_stack() << 8;
            break;
        case 1:
            core_ctx_.pc = exec_ctx_.result_u16 + 1;
            break;
        default:
            std::unreachable();
    }
}

void Cpu::jmp()
{
    core_ctx_.pc = exec_ctx_.operand_address;
}

void Cpu::adc_(std::uint8_t operand)
{
    exec_ctx_.result_u16 = core_ctx_.a + operand + (core_ctx_.flags & Flag::CARRY);
    set_flag(Flag::CARRY, exec_ctx_.result_u16 > 0xFF ? 1 : 0);
    set_flag(Flag::OVERFLOW_,
             ((core_ctx_.a ^ exec_ctx_.result_u16) & (operand ^ exec_ctx_.result_u16) & 0x80) >> 7
               ? 1
               : 0);
    exec_ctx_.result_u16 &= 0xFF;
    set_nz_flags(exec_ctx_.result_u16);
    core_ctx_.a = exec_ctx_.result_u16;
}

void Cpu::adc()
{
    adc_(read_operand());
}

void Cpu::sbc()
{
    adc_(read_operand() ^ 0xFF);
}

void Cpu::sta()
{
    store(core_ctx_.a);
}

void Cpu::stx()
{
    store(core_ctx_.x);
}

void Cpu::sty()
{
    store(core_ctx_.y);
}

void Cpu::inx()
{
    core_ctx_.x = (core_ctx_.x + 1) & 0xFF;
    set_nz_flags(core_ctx_.x);
}

void Cpu::dex()
{
    core_ctx_.x = (core_ctx_.x - 1) & 0xFF;
    set_nz_flags(core_ctx_.x);
}

void Cpu::iny()
{
    core_ctx_.y = (core_ctx_.y + 1) & 0xFF;
    set_nz_flags(core_ctx_.y);
}

void Cpu::dey()
{
    core_ctx_.y = (core_ctx_.y - 1) & 0xFF;
    set_nz_flags(core_ctx_.y);
}

void Cpu::inc()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
            exec_ctx_.operand = read_operand();
            break;
        case 2:
            exec_ctx_.result_u8 = (exec_ctx_.operand + 1) & 0xFF;
            store(exec_ctx_.operand);
            break;
        case 1:
            set_nz_flags(exec_ctx_.result_u8);
            store(exec_ctx_.result_u8);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::dec()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
            exec_ctx_.operand = read_operand();
            break;
        case 2:
            exec_ctx_.result_u8 = (exec_ctx_.operand - 1) & 0xFF;
            store(exec_ctx_.operand);
            break;
        case 1:
            set_nz_flags(exec_ctx_.result_u8);
            store(exec_ctx_.result_u8);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::txa()
{
    core_ctx_.a = core_ctx_.x;
    set_nz_flags(core_ctx_.a);
}

void Cpu::tya()
{
    core_ctx_.a = core_ctx_.y;
    set_nz_flags(core_ctx_.a);
}

void Cpu::txs()
{
    core_ctx_.sp = core_ctx_.x;
}

void Cpu::tay()
{
    core_ctx_.y = core_ctx_.a;
    set_nz_flags(core_ctx_.y);
}

void Cpu::tax()
{
    core_ctx_.x = core_ctx_.a;
    set_nz_flags(core_ctx_.x);
}

void Cpu::tsx()
{
    core_ctx_.x = core_ctx_.sp;
    set_nz_flags(core_ctx_.x);
}

void Cpu::lda()
{
    core_ctx_.a = read_operand();
    set_nz_flags(core_ctx_.a);
}

void Cpu::ldx()
{
    core_ctx_.x = read_operand();
    set_nz_flags(core_ctx_.x);
}

void Cpu::ldy()
{
    core_ctx_.y = read_operand();
    set_nz_flags(core_ctx_.y);
}

void Cpu::cpx()
{
    exec_ctx_.operand = read_operand();
    exec_ctx_.result_u8 = core_ctx_.x - exec_ctx_.operand;
    set_flag(Flag::CARRY, core_ctx_.x >= exec_ctx_.operand ? 1 : 0);
    set_nz_flags(exec_ctx_.result_u8);
}

void Cpu::cpy()
{
    exec_ctx_.operand = read_operand();
    exec_ctx_.result_u8 = core_ctx_.y - exec_ctx_.operand;
    set_flag(Flag::CARRY, core_ctx_.y >= exec_ctx_.operand ? 1 : 0);
    set_nz_flags(exec_ctx_.result_u8);
}

void Cpu::cmp()
{
    exec_ctx_.operand = read_operand();
    exec_ctx_.result_u8 = core_ctx_.a - exec_ctx_.operand;
    set_flag(Flag::CARRY, core_ctx_.a >= exec_ctx_.operand ? 1 : 0);
    set_nz_flags(exec_ctx_.result_u8);
}

void Cpu::branch(bool condition)
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 0:
            if (condition)
            {
                ++exec_ctx_.total_cycles_left;

                std::uint8_t operand = read_operand();
                exec_ctx_.result_u16 = core_ctx_.pc + static_cast<int8_t>(operand);

                if (is_page_crossed(core_ctx_.pc, exec_ctx_.result_u16))
                {
                    ++exec_ctx_.total_cycles_left;
                }
            }
            break;
        case 1:
        case 2:
            core_ctx_.pc = exec_ctx_.result_u16;
            break;
        default:
            std::unreachable();
    }
}

void Cpu::bpl()
{
    branch((core_ctx_.flags & Flag::NEGATIVE) == 0);
}

void Cpu::bmi()
{
    branch((core_ctx_.flags & Flag::NEGATIVE) != 0);
}

void Cpu::bvc()
{
    branch((core_ctx_.flags & Flag::OVERFLOW_) == 0);
}

void Cpu::bvs()
{
    branch((core_ctx_.flags & Flag::OVERFLOW_) != 0);
}

void Cpu::bcc()
{
    branch((core_ctx_.flags & Flag::CARRY) == 0);
}

void Cpu::bcs()
{
    branch((core_ctx_.flags & Flag::CARRY) != 0);
}

void Cpu::bne()
{
    branch((core_ctx_.flags & Flag::ZERO) == 0);
}

void Cpu::beq()
{
    branch((core_ctx_.flags & Flag::ZERO) != 0);
}

void Cpu::jsr()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
            break;
        case 2:
            push_stack((core_ctx_.pc - 1) >> 8);
            break;
        case 1:
            push_stack((core_ctx_.pc - 1) & 0xFF);
            core_ctx_.pc = exec_ctx_.operand_address;
            break;
        default:
            std::unreachable();
    }
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
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
            exec_ctx_.operand = read_operand();
            break;
        case 2:
            store(exec_ctx_.operand);
            break;
        case 1:
            exec_ctx_.result_u8 = (exec_ctx_.operand << 1) & 0xFF;
            set_flag(Flag::CARRY, exec_ctx_.operand >> 7);
            core_ctx_.a |= exec_ctx_.result_u8;
            set_nz_flags(core_ctx_.a);
            store(exec_ctx_.result_u8);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::anc()
{
    core_ctx_.a &= read_operand();
    set_nz_flags(core_ctx_.a);
    set_flag(Flag::CARRY, core_ctx_.a >> 7);
}

void Cpu::rla()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
            exec_ctx_.operand = read_operand();
            break;
        case 2:
            store(exec_ctx_.operand);
            break;
        case 1:
            exec_ctx_.result_u8 = (exec_ctx_.operand << 1 | (core_ctx_.flags & Flag::CARRY)) & 0xFF;
            core_ctx_.a &= exec_ctx_.result_u8;
            set_flag(Flag::CARRY, exec_ctx_.operand >> 7);
            set_nz_flags(core_ctx_.a);
            store(exec_ctx_.result_u8);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::sre()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
            exec_ctx_.operand = read_operand();
            break;
        case 2:
            store(exec_ctx_.operand);
            break;
        case 1:
            exec_ctx_.result_u8 = exec_ctx_.operand >> 1;
            core_ctx_.a ^= exec_ctx_.result_u8;
            set_flag(Flag::CARRY, exec_ctx_.operand & 1);
            set_nz_flags(core_ctx_.a);
            store(exec_ctx_.result_u8);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::alr()
{
    exec_ctx_.result_u8 = core_ctx_.a & read_operand();
    core_ctx_.a = exec_ctx_.result_u8 >> 1;
    set_flag(Flag::CARRY, exec_ctx_.result_u8 & 1);
    set_nz_flags(core_ctx_.a);
}

void Cpu::rra()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
            exec_ctx_.operand = read_operand();
            break;
        case 2:
            store(exec_ctx_.operand);
            break;
        case 1:
            exec_ctx_.result_u8 =
              (exec_ctx_.operand >> 1 | (core_ctx_.flags & Flag::CARRY) << 7) & 0xFF;
            set_flag(Flag::CARRY, exec_ctx_.operand & 1);
            adc_(exec_ctx_.result_u8);
            store(exec_ctx_.result_u8);
            break;
        default:
            std::unreachable();
    }
}

void Cpu::arr()
{
    // https://www.nesdev.org/wiki/Programming_with_unofficial_opcodes
    //
    // Similar to AND #i then ROR A, except sets the flags differently. N and Z are normal, but C is
    // bit 6 and V is bit 6 xor bit 5
    exec_ctx_.result_u8 = core_ctx_.a & read_operand();
    core_ctx_.a = (exec_ctx_.result_u8 >> 1 | (core_ctx_.flags & Flag::CARRY) << 7) & 0xFF;
    set_flag(Flag::CARRY, (core_ctx_.a >> 6) & 1);
    set_flag(Flag::OVERFLOW_, ((core_ctx_.a >> 6) & 1) ^ ((core_ctx_.a >> 5) & 1));
    set_nz_flags(core_ctx_.a);
}

void Cpu::sax()
{
    store(core_ctx_.a & core_ctx_.x);
}

void Cpu::ane()
{
    core_ctx_.a = ((core_ctx_.a | 0xEE) & core_ctx_.x & read_operand());
    set_nz_flags(core_ctx_.a);
}

void Cpu::sha()
{
    store(core_ctx_.a & core_ctx_.x & ((exec_ctx_.operand_address >> 8) + 1));
}

void Cpu::tas()
{
    core_ctx_.sp = core_ctx_.a & core_ctx_.x;
    store(core_ctx_.sp & ((exec_ctx_.operand_address >> 8) + 1));
}

void Cpu::shy()
{
    store(core_ctx_.y & ((exec_ctx_.operand_address >> 8) + 1));
}

void Cpu::shx()
{
    store(core_ctx_.x & ((exec_ctx_.operand_address >> 8) + 1));
}

void Cpu::lax()
{
    core_ctx_.a = read_operand();
    core_ctx_.x = core_ctx_.a;
    set_nz_flags(core_ctx_.a);
}

void Cpu::lxa()
{
    core_ctx_.a = (core_ctx_.a | 0xEE) & read_operand();
    core_ctx_.x = core_ctx_.a;
    set_nz_flags(core_ctx_.a);
}

void Cpu::las()
{
    core_ctx_.a = core_ctx_.sp & read_operand();
    core_ctx_.x = core_ctx_.a;
    core_ctx_.sp = core_ctx_.a;
    set_nz_flags(core_ctx_.a);
}

void Cpu::dcp()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
            exec_ctx_.operand = read_operand();
            break;
        case 2:
            store(exec_ctx_.operand);
            break;
        case 1: {
            exec_ctx_.result_u8 = (exec_ctx_.operand - 1) & 0xFF;
            std::uint8_t tmp = core_ctx_.a - exec_ctx_.result_u8;
            set_flag(Flag::CARRY, core_ctx_.a >= exec_ctx_.result_u8 ? 1 : 0);
            set_nz_flags(tmp);
            store(exec_ctx_.result_u8);
            break;
        }
        default:
            std::unreachable();
    }
}

void Cpu::sbx()
{
    exec_ctx_.operand = read_operand();
    exec_ctx_.result_u8 = core_ctx_.x & core_ctx_.a;
    core_ctx_.x = exec_ctx_.result_u8 - exec_ctx_.operand;
    set_flag(Flag::CARRY, exec_ctx_.result_u8 >= exec_ctx_.operand ? 1 : 0);
    set_nz_flags(core_ctx_.x);
}

void Cpu::isb()
{
    switch (exec_ctx_.total_cycles_left)
    {
        case 3:
            exec_ctx_.operand = read_operand();
            break;
        case 2:
            store(exec_ctx_.operand);
            break;
        case 1:
            exec_ctx_.result_u8 = (exec_ctx_.operand + 1) & 0xFF;
            adc_(exec_ctx_.result_u8 ^ 0xFF);
            store(exec_ctx_.result_u8);
            break;
        default:
            std::unreachable();
    }
}

} // namespace mayones::core
