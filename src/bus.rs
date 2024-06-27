use crate::rom;

const RAM_SIZE: usize = 2048;

#[derive(Debug)]
pub struct CpuBus {
    ram: [u8; RAM_SIZE],
    cartridge: rom::Cartridge,
}

impl CpuBus {
    pub fn new(cartridge: rom::Cartridge) -> Self {
        Self {
            ram: [0; RAM_SIZE],
            cartridge: cartridge,
        }
    }

    pub fn read(&self, address: u16) -> u8 {
        match address {
            0x0000..=0x1FFF => self.ram[(address & 0x07FF) as usize],
            // PPU registers
            0x2000..=0x3FFF => 0,
            // APU and I/O registers
            0x4000..=0x4017 => 0,
            // APU and I/O functionality that is normally disabled
            0x4018..=0x401F => 0,
            // PRG ROM, PRG RAM and mapper registers
            0x4020..=0xFFFF => self.cartridge.read(address),
            _ => panic!("invalid address: {address:#X}"),
        }
    }

    pub fn write(&mut self, address: u16, data: u8) {
        match address {
            0x0000..=0x1FFF => self.ram[address as usize & 0x07FF] = data,
            // PPU registers
            0x2000..=0x3FFF => (),
            // DMA
            0x4014 => (),
            // APU and I/O registers
            0x4000..=0x4017 => (),
            // APU and I/O functionality that is normally disabled
            0x4018..=0x401F => (),
            // PRG ROM, PRG RAM and mapper registers
            0x4020..=0xFFFF => (),
            _ => (),
        }
    }
}
