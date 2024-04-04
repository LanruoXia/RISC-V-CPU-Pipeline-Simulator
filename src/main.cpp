#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <elfio/elfio.hpp>

#include "simulator/memory_manager.h"
#include "simulator/simulator.h"

bool parse_params(int argc, char** argv);
void load_elf_to_memory(ELFIO::elfio* reader, MemoryManager* memory);

char* elf_file = nullptr; //?
uint32_t stack_base_addr = 0x80000000;
uint32_t stack_size = 0x400000;
MemoryManager memory;
Simulator simulator(&memory);

int main(int argc, char** argv) {
  // not necessary to implement, can delete
  if (!parse_params(argc, argv)) {
    exit(-1);
  }
  if(simulator.debug){
    printf("start loading: %s\n", elf_file);
  }
  // read elf file
  ELFIO::elfio reader;
  if (!reader.load(elf_file)) {
    fprintf(stderr, "Fail to load ELF file %s!\n", elf_file);
    return -1;
  }
  if(simulator.debug){
    printf("%s\n", "reader.load succeeded");
  }
  load_elf_to_memory(&reader, &memory);
  if(simulator.debug){
    printf("%s\n", "load_elf_to_memory succeeded");
  }
  // get entry point
  simulator.pc = reader.get_entry();
  uint32_t first_inst = memory.get_int(simulator.pc);//0100000110010111 auipc
  uint32_t second_inst = memory.get_int(simulator.pc + 4); //10001101100000011 000 00011 0010011 addi
  if(simulator.debug){
    printf("%s\n", "get_entry succeeded");
    printf("pc: %04x\n", simulator.pc);
    printf("first_inst: %04x\n", first_inst);
    printf("second_inst: %04x\n", second_inst);
  }
  // init stack
  simulator.init_stack(stack_base_addr, stack_size);
  if(simulator.debug){
   printf("%s\n", "init_stack succeeded");
  }
  simulator.simulate();

  return 0;
}

// not necessary to implement, can delete
bool parse_params(int argc, char** argv) {
  // Read Parameters implementation
  if(argc <= 1) {
    return false;
  } else if(argc == 2){
    elf_file = argv[1];
  }else{
     elf_file = argv[1];
     char* control = argv[2];
     if (!strcmp(control, "print_history")){
      simulator.print_history = true;
      std::cout << "History will be printed!" << std::endl;
     }else if(!strcmp(control, "debug")){
      simulator.debug = true;
      simulator.print_history = true;
      std::cout << "in debug mode" << std::endl;
     }
  }
  return true;
}

// load elf file to memory
void load_elf_to_memory(ELFIO::elfio* reader, MemoryManager* memory) {
  ELFIO::Elf_Half seg_num = reader->segments.size();
  for (int i = 0; i < seg_num; ++i) {
    const ELFIO::segment* pseg = reader->segments[i];

    uint32_t memory_size = pseg->get_memory_size();
    uint32_t offset = pseg->get_virtual_address();

    // check if the address space is larger than 32bit
    if (offset + memory_size > 0xFFFFFFFF) {
      fprintf(
          stderr,
          "ELF address space larger than 32bit! Seg %d has max addr of 0x%x\n",
          i, offset + memory_size);
      exit(-1);
    }

    uint32_t filesz = pseg->get_file_size();//最后一个segment储存的信息？
    uint32_t memsz = pseg->get_memory_size();
    uint32_t addr = (uint32_t)pseg->get_virtual_address();

    for (uint32_t p = addr; p < addr + memsz; ++p) {//++p?
      if (!memory->is_page_exist(p)) {
        memory->add_page(p);
      }

      if (p < addr + filesz) { //what is filesize?
        memory->set_byte(p, pseg->get_data()[p - addr]);
      } else {
        memory->set_byte(p, 0);
      }
    }
  }
}
