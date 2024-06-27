#[derive(Debug)]
pub struct Mapper0 {
    prg_rom: Vec<u8>,
    chr_rom: Vec<u8>,
}

impl Mapper0 {
    pub fn new(prg_rom: Vec<u8>, chr_rom: Vec<u8>) -> Self {
        Self { prg_rom, chr_rom }
    }

    pub fn read(&self, address: u16) -> u8 {
        match address {
            // this actual for 1 bank roms (mirrored) but not for 2 banks
            0x8000..=0xFFFF => self.prg_rom[(address & 0x3FFF) as usize],
            0x0000..=0x1FFF => self.chr_rom[address as usize],
            _ => panic!("invalid address {:#X}", address),
        }
    }
}
