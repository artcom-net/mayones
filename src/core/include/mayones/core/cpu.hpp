#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <variant>

#include "mayones/core/bus.hpp"

namespace mayones::core {

struct TraceEntry {
    std::uint8_t opcode;
    std::string mnemonic;
    std::variant<std::monostate, std::uint16_t, std::uint8_t> operand;
    std::uint8_t a;
    std::uint8_t x;
    std::uint8_t y;
    std::uint8_t p;
    std::uint8_t sp;
    std::uint16_t pc;
    std::size_t cycles;

    bool operator==(const TraceEntry& other) const = default;
};

class Cpu {
public:
    Cpu(CpuBus& bus) :
        a_{ 0 },
        x_{ 0 },
        y_{ 0 },
        p_{ 0 },
        sp_{ 0 },
        pc_{ 0 },
        addr_mode_{ AddressMode::UNKNOWN },
        operand_addr_{ 0 },
        curr_cycle_{ 0 },
        total_cycles_{ 0 },
        suspend_cycles_{ 0 },
        page_crossed_{ false },
        bus_{ bus }
    {
    }

    std::size_t reset();
    std::size_t reset(std::uint16_t pc);
    std::size_t tick();
    TraceEntry trace_tick();

private:
    enum Flag : std::uint8_t {
        CARRY = 1 << 0,
        ZERO = 1 << 1,
        INTERRUPT = 1 << 2,
        DECIMAL = 1 << 3,
        BREAK = 1 << 4,
        UNUSED = 1 << 5,
        OVERFLOW_ = 1 << 6,
        NEGATIVE = 1 << 7
    };

    enum class AddressMode : std::uint8_t {
        UNKNOWN,
        ACCUMULATOR,
        IMPLIED,
        IMMEDIATE,
        ABSOLUTE,
        ZEROPAGE,
        ABSOLUTE_X,
        ABSOLUTE_Y,
        ZEROPAGE_X,
        ZEROPAGE_Y,
        INDIRECT,
        X_INDIRECT,
        INDIRECT_Y,
        RELATIVE
    };

    struct Instruction {
        Instruction() :
            mnemonic{ "UNKNOWN" },
            addr_mode{ Cpu::AddressMode::UNKNOWN },
            cycles{ 0 },
            check_page_cross{ false },
            func{ nullptr }
        {
        }

        Instruction(std::string_view mnemonic,
                    AddressMode addr_mode,
                    std::uint8_t cycles,
                    bool check_page_cross,
                    void (Cpu::*func)()) :
            mnemonic{ mnemonic },
            addr_mode{ addr_mode },
            cycles{ cycles },
            check_page_cross{ check_page_cross },
            func{ func }
        {
        }

        std::string_view mnemonic;
        AddressMode addr_mode;
        std::uint8_t cycles;
        bool check_page_cross;
        void (Cpu::*func)();
    };

    static constexpr std::size_t INSTRUCTIONS_TABLE_SIZE{ 256 };
    static const std::array<const Instruction, INSTRUCTIONS_TABLE_SIZE> INSTRUCTIONS_TABLE;

    static constexpr std::uint16_t STACK_BASE_ADDRESS{ 0x0100 };
    static constexpr std::uint16_t NMI_VECTOR_ADDRESS{ 0xFFFA };
    static constexpr std::uint16_t RESET_VECTOR_ADDRESS{ 0xFFFC };
    static constexpr std::uint16_t IRQ_VECTOR_ADDRESS{ 0xFFFE };

    std::uint8_t a_;
    std::uint8_t x_;
    std::uint8_t y_;
    std::uint8_t p_;
    std::uint8_t sp_;
    std::uint16_t pc_;

    AddressMode addr_mode_;
    std::uint16_t operand_addr_;
    std::size_t curr_cycle_;
    std::size_t total_cycles_;
    std::size_t suspend_cycles_;
    bool page_crossed_;

    CpuBus& bus_;

    std::size_t reset_registers(std::uint16_t pc) noexcept;

    void push_stack(std::uint8_t data);
    std::uint8_t pop_stack();

    void set_flag(Flag flag, std::uint8_t value);
    void set_nz_flags(std::uint8_t data);

    bool is_page_crossed(std::uint16_t address1, std::uint16_t address2) const;
    std::uint16_t read_wrapped_page(std::uint16_t address) const;

    std::uint16_t resolve_immediate();
    std::uint16_t resolve_zeropage(std::uint8_t offset);
    std::uint16_t resolve_absolute(std::uint8_t offset);
    std::uint16_t resolve_indirect();
    std::uint16_t resolve_preindex_indirect();
    std::uint16_t resolve_postindex_indirect();
    std::uint16_t resolve_relative();

    std::uint8_t get_operand() const;
    void store(std::uint8_t data);

    void brk();

    void ora();
    void and_();
    void eor();

    void asl();
    void lsr();
    void rol();
    void ror();

    void bit();

    void php();
    void plp();
    void pha();
    void pla();

    void clc();
    void sec();
    void cli();
    void sei();
    void clv();
    void cld();
    void sed();

    void rti();
    void rts();

    void jmp();

    void adc_(std::uint8_t operand);
    void adc();
    void sbc();

    void sta();
    void stx();
    void sty();

    void inx();
    void dex();
    void iny();
    void dey();

    void inc();
    void dec();

    void txa();
    void tya();
    void txs();
    void tay();
    void tax();
    void tsx();

    void lda();
    void ldx();
    void ldy();

    void cpx();
    void cpy();
    void cmp();

    void branch(bool condition);
    void bpl();
    void bmi();
    void bvc();
    void bvs();
    void bcc();
    void bcs();
    void bne();
    void beq();

    void jsr();

    void nop();

    // illegal opcodes
    void kil();
    void slo();
    void anc();
    void rla();
    void sre();
    void alr();
    void rra();
    void arr();
    void sax();
    void ane();
    void sha();
    void tas();
    void shy();
    void shx();
    void lax();
    void lxa();
    void las();
    void dcp();
    void sbx();
    void isb();
};

} // namespace mayones::core
