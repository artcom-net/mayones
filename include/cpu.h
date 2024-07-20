#pragma once

#include <cstdint>
#include <string>

#include "bus.h"


class CPU
{
public:
    CPU(CPUMemoryBus& bus);

    CPU(const CPU& cpu) = delete;
    CPU& operator=(const CPU& cpu) = delete;
    CPU(CPU&& cpu) = delete;
    CPU& operator=(CPU&& cpu) = delete;

    void reset();
    size_t step();

private:
    enum Flag : uint8_t
    {
        CARRY = 1 << 0,
        ZERO = 1 << 1,
        INTERRUPT = 1 << 2,
        DECIMAL = 1 << 3,
        BREAK = 1 << 4,
        UNUSED = 1 << 5,
        OVERFLOW_ = 1 << 6,
        NEGATIVE = 1 << 7
    };
    enum class AddressMode
    {
        NONE,
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

    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t p;
    uint8_t sp;
    uint16_t pc;

    AddressMode addr_mode;
    uint16_t operand_addr;
    size_t curr_cycle;
    size_t total_cycles;
    bool page_crossed;

    CPUMemoryBus& bus;

    struct Instruction
    {
        std::string mnemonic;
        AddressMode addr_mode;
        uint8_t cycles;
        bool check_page_cross;
        void (CPU::* func)();

        Instruction();
        Instruction(const std::string& mnemonic, AddressMode addr_mode, uint8_t cycles, bool check_page_cross, void (CPU::* func)());
    };

    static const uint16_t STACK_BASE_ADDRESS;
    static const uint16_t NMI_VECTOR_ADDRESS;
    static const uint16_t RESET_VECTOR_ADDRESS;
    static const uint16_t IRQ_VECTOR_ADDRESS;

    static const CPU::Instruction INSTRUCTIONS[0xFF];

    void execute(const Instruction& instruction);

    void push_stack(uint8_t data);
    uint8_t pop_stack();

    void set_flag(Flag flag, uint8_t value);
    void set_nz_flags(uint8_t data);

    bool is_page_crossed(uint16_t address1, uint16_t address2) const;
    uint16_t read_wrapped_page(uint16_t address) const;

    uint16_t resolve_immediate();
    uint16_t resolve_zeropage(uint8_t offset);
    uint16_t resolve_absolute(uint8_t offset);
    uint16_t resolve_indirect();
    uint16_t resolve_preindex_indirect();
    uint16_t resolve_postindex_indirect();
    uint16_t resolve_relative();

    uint8_t get_operand() const;
    void store(uint8_t data);

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

    void adc_(uint8_t operand);
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
};
