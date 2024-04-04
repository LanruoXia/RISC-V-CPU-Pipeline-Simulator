#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <cstdint>

#include <elfio/elfio.hpp>

class MemoryManager {
public:
  MemoryManager();
  ~MemoryManager();

  bool add_page(uint32_t addr);
  bool is_page_exist(uint32_t addr);
  bool set_byte(uint32_t addr, uint8_t val, uint32_t* cycles = nullptr);
  uint8_t get_byte(uint32_t addr, uint32_t* cycles = nullptr);
  bool set_short(uint32_t addr, uint16_t val, uint32_t* cycles = nullptr);
  uint16_t get_short(uint32_t addr, uint32_t* cycles = nullptr);
  bool set_short(uint32_t addr, uint32_t* cycles = nullptr);
  uint32_t get_int(uint32_t addr, uint32_t* cycles = nullptr);
  bool set_int(uint32_t addr, uint32_t val, uint32_t* cycles = nullptr);


private:
  uint32_t get_first_entry_id(uint32_t addr);
  uint32_t get_second_entry_id(uint32_t addr);
  uint32_t get_page_offset(uint32_t addr);
  bool is_addr_exist(uint32_t addr);

  uint8_t** memory[1024];
  uint32_t access_latency;
};

#endif
