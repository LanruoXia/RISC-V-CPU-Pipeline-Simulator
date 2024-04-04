#include "simulator/memory_manager.h"
#include <cstdint>

MemoryManager::MemoryManager() {
  for (uint32_t i = 0; i < 1024; ++i) {
    memory[i] = nullptr;
  }
  access_latency = 10; //?
}

MemoryManager::~MemoryManager() {
  for (uint32_t i = 0; i < 1024; ++i) {
    if (memory[i] != nullptr) {
      for (uint32_t j = 0; j < 1024; ++j) {
        if (memory[i][j] != nullptr) {
          delete[] memory[i][j];
          memory[i][j] = nullptr;
        }
      }
      delete[] memory[i];
      memory[i] = nullptr;
    }
  }
}

uint32_t MemoryManager::get_first_entry_id(uint32_t addr) {
  return (addr >> 22) & 0x3FF;
}

uint32_t MemoryManager::get_second_entry_id(uint32_t addr) {
  return (addr >> 12) & 0x3FF;
}

uint32_t MemoryManager::get_page_offset(uint32_t addr) { return addr & 0xFFF; }

bool MemoryManager::add_page(uint32_t addr) {
  uint32_t i = get_first_entry_id(addr);
  uint32_t j = get_second_entry_id(addr);
  if (memory[i] == nullptr) {
    memory[i] = new uint8_t *[1024]; //why uint8_t?
    memset(memory[i], 0, sizeof(uint8_t *) * 1024);
  }
  if (memory[i][j] == nullptr) {
    memory[i][j] = new uint8_t[4096];
    memset(memory[i][j], 0, 4096);
  } else {
    // page already exists
    return false;
  }
  return true;
}

bool MemoryManager::is_addr_exist(uint32_t addr) {
  uint32_t i = get_first_entry_id(addr);
  uint32_t j = get_second_entry_id(addr);

  if (memory[i] && memory[i][j])
    return true;
  return false;
}

bool MemoryManager::is_page_exist(uint32_t addr) { return is_addr_exist(addr); }

uint8_t MemoryManager::get_byte(uint32_t addr, uint32_t *cycles) {//what is cycles?
  if (!is_addr_exist(addr))
    throw std::runtime_error("Memory address not found");

  if (cycles != nullptr)
    *cycles = access_latency;

  return memory[get_first_entry_id(addr)][get_second_entry_id(addr)]
               [get_page_offset(addr)];
}

bool MemoryManager::set_byte(uint32_t addr, uint8_t val, uint32_t *cycles) {
  if (!is_addr_exist(addr))
    throw std::runtime_error("Memory address not found");

  if (cycles != nullptr)
    *cycles = access_latency;

  memory[get_first_entry_id(addr)][get_second_entry_id(addr)]
        [get_page_offset(addr)] = val;
  return true;
}

uint16_t MemoryManager::get_short(uint32_t addr, uint32_t *cycles) {
  if (!is_addr_exist(addr))
    throw std::runtime_error("Memory address not found");

  return get_byte(addr, cycles) | get_byte(addr + 1) << 8;
}

bool MemoryManager::set_short(uint32_t addr, uint16_t val, uint32_t *cycles) {
  if (!is_addr_exist(addr))
    throw std::runtime_error("Memory address not found");

  set_byte(addr, val & 0xFF, cycles);
  set_byte(addr + 1, (val >> 8) & 0xFF);
  return true;
}

uint32_t MemoryManager::get_int(uint32_t addr, uint32_t *cycles) {
  if (!is_addr_exist(addr))
    throw std::runtime_error("Memory address not found");

  return get_short(addr, cycles) | get_short(addr + 2) << 16;
}

bool MemoryManager::set_int(uint32_t addr, uint32_t val, uint32_t *cycles) {
  if (!is_addr_exist(addr))
    throw std::runtime_error("Memory address not found");

  set_short(addr, val & 0xFFFF, cycles);
  set_short(addr + 2, (val >> 16) & 0xFFFF);
  return true;
}
