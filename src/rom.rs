use std::fs::{metadata, File};
use std::io::{self, Read};
use std::path;

use crate::mapper;

const KB: usize = 1024;

const HEADER_SIZE: usize = 16;
const TRAINER_SIZE: usize = 512;
const HEADER_TITLE: &[u8; 4] = b"NES\x1A";

const FLAG6_MIRRORING: u8 = 1 << 0;
const FLAG6_BATTERY: u8 = 1 << 1;
const FLAG6_TRAINER: u8 = 1 << 2;
const FLAG6_ALTER_NT_LAYOUT: u8 = 1 << 3;
const FLAG6_MAPPER_LOWER_BITS: u8 = 0xF0;

const FLAG7_VS_UNISYSTEM: u8 = 1 << 0;
const FLAG7_PLAYCHOICE_10: u8 = 1 << 1;
const FLAG7_ROM_FORMAT_BITS: u8 = 0x0C;
const FLAG7_MAPPER_UPPER_BITS: u8 = 0xF0;

const FLAG9_TV_SYSTEM: u8 = 1 << 0;
const FLAG9_RESERVED_BITS: u8 = 0xFE;

#[derive(Debug)]
enum RomFormat {
    Unknown,
    Ines,
    Nes20,
}

#[derive(Debug)]
enum Mirroring {
    Horizontal,
    Vertical,
}

#[derive(Debug)]
enum ConsoleType {
    Default,
    VsUnisystem,
    Playchoice10,
}

#[derive(Debug)]
enum TvSystem {
    NTSC,
    PAL,
}

#[derive(Debug)]
pub struct Cartridge {
    format: RomFormat,
    pub size: usize,
    mirroring: Mirroring,
    console_type: ConsoleType,
    tv_system: TvSystem,
    has_battery: bool,
    has_trainer: bool,
    has_alter_nt: bool,
    prg_rom_banks: u8,
    chr_rom_banks: u8,
    prg_ram_banks: u8,
    prg_rom_size: usize,
    chr_rom_size: usize,
    mapper: mapper::Mapper0,
}

impl Cartridge {
    pub fn read(&self, address: u16) -> u8 {
        self.mapper.read(address)
    }
}

pub fn read(rom_path: &str) -> Result<Cartridge, String> {
    let buffer = match read_file(rom_path) {
        Ok(buff) => buff,
        Err(err) => return Err(err.to_string()),
    };
    match get_rom_format(&buffer) {
        RomFormat::Ines => Ok(parse_ines(&buffer)?),
        RomFormat::Nes20 => Err("nes 2.0 roms not supported ".to_string()),
        RomFormat::Unknown => Err("unknown rom format".to_string()),
    }
}

fn is_ines_header(buffer: &[u8]) -> bool {
    if buffer.len() < HEADER_SIZE {
        false
    } else {
        &buffer[0..4] == HEADER_TITLE
    }
}

fn parse_ines(buffer: &[u8]) -> Result<Cartridge, String> {
    let mut iter = buffer.iter().skip(HEADER_TITLE.len());
    let prg_rom_banks = iter.next().unwrap();
    let chr_rom_banks = iter.next().unwrap();
    let prg_rom_size = KB * 16 * (*prg_rom_banks as usize);
    let chr_rom_size = KB * 8 * (*chr_rom_banks as usize);
    let mut total_size = HEADER_SIZE + prg_rom_size + chr_rom_size;

    let flags6 = iter.next().unwrap();
    let mirroring = match flags6 & FLAG6_MIRRORING {  
        0 => Mirroring::Horizontal,
        _ => Mirroring::Vertical
    };
    let has_battery = (flags6 & FLAG6_BATTERY) != 0;
    let has_trainer = (flags6 & FLAG6_TRAINER) != 0;
    let has_alter_nt_layout = (flags6 & FLAG6_ALTER_NT_LAYOUT) != 0;
    let mut mapper_id = (flags6 & FLAG6_MAPPER_LOWER_BITS) >> 4;

    let flags7 = iter.next().unwrap();
    let is_vs_unisystem = (flags7 & FLAG7_VS_UNISYSTEM) != 0;
    let is_playchoice10 = (flags7 & FLAG7_PLAYCHOICE_10) != 0;
    let console_type = 
        if is_vs_unisystem {
            ConsoleType::VsUnisystem
        } else if is_playchoice10 {
            ConsoleType::Playchoice10
        } else {
            ConsoleType::Default
        };
    mapper_id |= flags7 & FLAG7_MAPPER_UPPER_BITS;

    let prg_ram_banks = iter.next().unwrap();

    let flags9 = iter.next().unwrap();
    let tv_system = if flags9 & FLAG9_TV_SYSTEM == 0 {
        TvSystem::NTSC
    } else {
        TvSystem::PAL
    };
    if flags9 & FLAG9_RESERVED_BITS != 0 {
        return Err("reserved bits is not zero".to_string());
    }
    let mut iter = iter.skip(1);
    for _ in 0..5 {
        if *iter.next().unwrap() != 0 {
            return Err("invalid padding value".to_string());
        }
    }
    let iter = if has_trainer {
        total_size += TRAINER_SIZE;
        iter.skip(TRAINER_SIZE)
    } else {
        iter.skip(0)
    };

    if total_size != buffer.len() {
        return Err("invalid buffer size".to_string());
    }

    let prg_it = iter.clone().take(prg_rom_size).cloned();
    let chr_it = iter.clone().skip(prg_rom_size).cloned();
    let prg_rom: Vec<u8> = Vec::from_iter(prg_it);
    let chr_rom: Vec<u8> = Vec::from_iter(chr_it);

    let mapper = match mapper_id {
        0 => mapper::Mapper0::new(prg_rom, chr_rom),
        _ => return Err("unsupported mapper".to_string()),
    };
    Ok(Cartridge {
        format: RomFormat::Ines,
        size: total_size,
        mirroring: mirroring,
        console_type: console_type,
        tv_system: tv_system,
        has_battery: has_battery,
        has_trainer: has_trainer,
        has_alter_nt: has_alter_nt_layout,
        prg_rom_banks: *prg_rom_banks,
        chr_rom_banks: *chr_rom_banks,
        prg_ram_banks: *prg_ram_banks,
        prg_rom_size: prg_rom_size,
        chr_rom_size: chr_rom_size,
        mapper: mapper,
    })
}

fn parse_nes20(_buffer: &[u8]) -> Result<Cartridge, String> {
    panic!("NES 2.0 rom format is not implemented");
}

fn read_file(rom_path: &str) -> Result<Vec<u8>, io::Error> {
    let path = path::Path::new(rom_path);
    let mut file = File::open(path)?;
    let rom_size: usize = metadata(path)?.len() as usize;
    let mut buffer = vec![0; rom_size];
    file.read_exact(&mut buffer)?;
    Ok(buffer)
}

fn get_rom_format(buffer: &[u8]) -> RomFormat {
    if !is_ines_header(buffer) {
        return RomFormat::Unknown;
    }
    if (buffer[7] & FLAG7_ROM_FORMAT_BITS) == 0x08 {
        RomFormat::Nes20
    } else {
        RomFormat::Ines
    }
}
