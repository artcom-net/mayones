#include <iostream>
#include <memory>
#include <string>

#include "bus.h"
#include "cartridge.h"
#include "cpu.h"

int main()
{
    std::string rom_path;
    std::cout << "Enter ROM path: ";
    std::getline(std::cin, rom_path);

    int cycles = 0;
    std::cout << "Enter CPU cycles: ";
    std::cin >> cycles;

    auto cartridge = std::make_shared<Cartridge>(rom_path);
    CPUMemoryBus bus;
    bus.connect_cartridge(cartridge);
    CPU cpu(bus);
    cpu.reset();
    while (cycles--)
    {
        cpu.step();
    }
    return 0;
}
