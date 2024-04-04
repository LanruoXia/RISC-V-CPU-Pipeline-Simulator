#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "simulator/memory_manager.h"
#include <cstdint>
#include <set>

const int REGNUM = 32;

enum Instruction {
  LUI = 0,
  AUIPC = 1,
  JAL = 2,
  JALR = 3,
  BEQ = 4,
  BNE = 5,
  BLT = 6,
  BGE = 7,
  BLTU = 8,
  BGEU = 9,
  LB = 10,
  LH = 11,
  LW = 12,
  LD = 13,
  LBU = 14,
  LHU = 15,
  SB = 16,
  SH = 17,
  SW = 18,
  SD = 19,
  ADDI = 20,
  SLTI = 21,
  SLTIU = 22,
  XORI = 23,
  ORI = 24,
  ANDI = 25,
  SLLI = 26,
  SRLI = 27,
  SRAI = 28,
  ADD = 29,
  SUB = 30,
  SLL = 31,
  SLT = 32,
  SLTU = 33,
  XOR = 34,
  SRL = 35,
  SRA = 36,
  OR = 37,
  AND = 38,
  ECALL = 39,
  UNKNOWN = -1, //?
};

// opcode field
const int OP_REG = 0x33;
const int OP_IMM = 0x13;
const int OP_LUI = 0x37;
const int OP_BRANCH = 0x63;
const int OP_STORE = 0x23;
const int OP_LOAD = 0x03;
const int OP_SYSTEM = 0x73;
const int OP_AUIPC = 0x17;
const int OP_JAL = 0x6F;
const int OP_JALR = 0x67;

class Simulator {
public:
  uint32_t pc;
  uint32_t reg[REGNUM];
  uint32_t stack_base;
  uint32_t max_stack_size;
  MemoryManager* memory;
  bool print_history = false; //history switch: if true, history can be printed
  bool debug = false; //used for entering debug mode
  std::set<uint32_t> dependence_table; //store rd that has not been written back yet.
  int control; // used for debug
  uint32_t inst_count; // instruction counter

  Simulator(MemoryManager* memory);
  ~Simulator();

  void init_stack(uint32_t base_addr, uint32_t max_size);
  void simulate();

private:
  // can change according your need
  struct FetchRegister {
    bool bubble;
    uint32_t stall;

    uint32_t pc;
    uint32_t inst;
    uint32_t len; //?
    uint32_t count;
  } f_reg, f_reg_new;

  // can change according your need
  struct DecodeRegister {
    bool bubble;
    uint32_t stall;
    uint32_t rs1, rs2;
    uint32_t pc;
    Instruction inst;
    int32_t op1;
    int32_t op2;
    uint32_t dest;
    int32_t imm;
    uint32_t count;
    bool write_reg;
  } d_reg, d_reg_new;

  // can change according your need
  struct ExecuteRegister {
    bool bubble;
    //uint32_t stall;

    uint32_t pc;
    Instruction inst;
    int32_t op1;
    int32_t op2;
    bool write_reg;
    uint32_t dest_reg;
    // output of execute stage
    int32_t out;
    bool write_mem;
    bool read_unsigned_mem;
    bool read_signed_mem;
    uint32_t mem_len;
    uint32_t count;
  } e_reg, e_reg_new;

  // can change according your need
  struct MemoryRegister {
    bool bubble;
    uint32_t stall;

    uint32_t pc;
    Instruction inst;
    int32_t op1;
    int32_t op2;
    int32_t out;
    bool write_reg;
    uint32_t dest_reg;
    uint32_t count;
  } m_reg, m_reg_new;

  // can change according your need
  struct History {
    uint32_t inst_count;
    uint32_t cycle_count;
    uint32_t stall_cycle_count;

    uint32_t data_hazard_count;
    uint32_t control_hazard_count;
    uint32_t mem_hazard_count;
  } history;

  void fetch();
  void decode();
  void excecute();
  void memory_access();
  void write_back();

  Instruction decode_R(uint32_t funct3, uint32_t funct7);
  Instruction decode_I(uint32_t opcode, uint32_t funct3, uint32_t imm_5_11);
  Instruction decode_S(uint32_t funct3);
  Instruction decode_B(uint32_t funct3);
  Instruction decode_UJ(uint32_t opcode);
  
  int32_t handle_system_call(int32_t op1, int32_t op2);
};

#endif
