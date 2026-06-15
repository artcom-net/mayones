#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <variant>

#include "mayones/core/bus.hpp"

namespace mayones::core {

class Cpu {
public:
    struct TraceEntry {
        std::uint8_t opcode{};
        std::string mnemonic;
        std::variant<std::monostate, std::uint16_t, std::uint8_t> operand;
        std::uint8_t a{};
        std::uint8_t x{};
        std::uint8_t y{};
        std::uint8_t p{};
        std::uint8_t sp{};
        std::uint16_t pc{};
        std::size_t cycles{};

        bool operator==(const TraceEntry& other) const = default;
    };

    explicit Cpu(CpuBus& bus);

    void reset();
    void reset(std::uint16_t pc);

    void tick();
    TraceEntry trace_tick();

    void trigger_nmi();

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

    struct CoreContext {
        std::uint16_t pc{};
        std::uint8_t a{};
        std::uint8_t x{};
        std::uint8_t y{};
        std::uint8_t flags{};
        std::uint8_t sp{};
    };

    struct Instruction {
        std::string_view mnemonic;
        AddressMode addr_mode;
        std::uint8_t cycles;
        bool check_page_cross;
        void (Cpu::*func)();
    };

    struct ExecutionContext {
        const Instruction* instruction_ptr{};
        std::uint16_t operand_address{};
        std::uint16_t tmp_operand_address{};
        std::uint16_t result_u16{};
        std::uint8_t result_u8{};
        std::uint8_t operand{};
        std::uint8_t total_cycles_left{};
        std::uint8_t address_mode_cycles_left{};
    };

    struct DmaContext {
        std::uint16_t address{};
        std::uint16_t cycles_left{};
        std::uint8_t halt_cycles{};
        std::uint8_t data{};
    };

    static constexpr std::uint16_t STACK_BASE_ADDRESS{ 0x0100 };
    static constexpr std::uint16_t NMI_VECTOR_ADDRESS{ 0xFFFA };
    static constexpr std::uint16_t RESET_VECTOR_ADDRESS{ 0xFFFC };
    static constexpr std::uint16_t IRQ_VECTOR_ADDRESS{ 0xFFFE };
    static constexpr std::size_t INSTRUCTIONS_TABLE_SIZE{ 256 };
    static constexpr std::size_t DMA_CYCLES{ 512 };
    static constexpr std::array<std::uint8_t, 13> ADDRESS_MODE_CYCLE_TABLE{ 0, 0, 1, 2, 1, 3, 3,
                                                                            2, 2, 4, 4, 5, 1 };

    static const std::array<const Instruction, 256> INSTRUCTION_TABLE;
    static const Instruction NMI_INSTRUCTION;

    CoreContext core_ctx_{};
    ExecutionContext exec_ctx_{};
    DmaContext dma_ctx_{};
    CpuBus& bus_;
    std::size_t total_cycles_{};
    bool nmi_pending_{};

    void reset_registers(std::uint16_t pc);
    void push_stack(std::uint8_t data);
    std::uint8_t pop_stack();
    void set_flag(Flag flag, std::uint8_t value);
    void set_nz_flags(std::uint8_t data);

    std::uint16_t read_wrapped_page(std::uint16_t address, std::uint16_t pointer);
    void resolve_indexed_zeropage_address(std::uint8_t index);
    void resolve_indexed_absolute_address(std::uint8_t index);

    void tick_dma();
    void tick_nmi();

    std::uint8_t read_operand();
    void store(std::uint8_t data);

    void brk();

    void ora();
    void and_();
    void eor();

    void asl();
    void asl_a();

    void lsr();
    void lsr_a();

    void rol();
    void rol_a();

    void ror();
    void ror_a();

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
