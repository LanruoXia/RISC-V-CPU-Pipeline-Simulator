#include "simulator/simulator.h"

// init memory, pc, register
Simulator::Simulator(MemoryManager* memory) {
  this->memory = memory; 
  memset(reg, 0, sizeof(uint32_t)*REGNUM);
} //自己实现 register归零

Simulator::~Simulator() {}

// init register, init memory (set to 0) according to stack base address and max
// size
void Simulator::init_stack(uint32_t base_addr, uint32_t max_size) {
  reg[2] = base_addr;
  // printf("%s\n", "x2 set");
  // printf("%d\n", base_addr);
  for (uint32_t p = base_addr; p > base_addr - max_size;  p--){
      if (!memory->is_page_exist(p)) {
        memory->add_page(p);
      }
      memory->set_byte(p, 0);
  }
  
} //register指向sp，检测是否存在，不存在就addpage

void Simulator::simulate() {
  // initialize pipeline registers
  if(debug){
    printf("%s\n", "start simulating");    
  }
  memset(&this->f_reg, 0, sizeof(this->f_reg));
  memset(&this->d_reg, 0, sizeof(this->d_reg));
  memset(&this->e_reg, 0, sizeof(this->e_reg));
  memset(&this->m_reg, 0, sizeof(this->m_reg));
  memset(&this->f_reg_new, 0, sizeof(this->f_reg_new));
  memset(&this->d_reg_new, 0, sizeof(this->d_reg_new));
  memset(&this->e_reg_new, 0, sizeof(this->e_reg_new));
  memset(&this->m_reg_new, 0, sizeof(this->m_reg_new));
  // insert Bubble to later pipeline stages when fetch the first instruction
  this->f_reg.bubble = true;
  this->d_reg.bubble = true;
  this->e_reg.bubble = true;
  this->m_reg.bubble = true;
  // initialize stall
  this->f_reg_new.stall= 0;
  this->d_reg_new.stall= 0;
  this->inst_count = 0;


  // main simulation loop
  while (true) {
    // set $zero to 0, some instruction might set this register to non-zero
    this->reg[0] = 0;
    // check stack overflow
    if(this->reg[2] < this->stack_base - this->max_stack_size){
      printf("%s at instruction 0x%8x!\n" , "stack overflow!", m_reg.inst);
      exit(-1);
    }
    this->inst_count ++;
    this->fetch();
    this->decode();
    this->excecute();
    this->memory_access();
    this->write_back();
    
    // check stalling
    if(d_reg_new.stall){
      // if decode stage is stalled, only update a bubble to execute stage 
      d_reg.bubble = true;
    }else{
      // copy old register values to new register values
      this->f_reg = this->f_reg_new;
      this->d_reg = this->d_reg_new;
    }
    this->e_reg = this->e_reg_new;
    this->m_reg = this->m_reg_new;
    // if(this->m_reg_new.count == 1){
    //   printf("before updating inst %d: %d\n", this->m_reg_new.count, this->m_reg_new.out);
    //   printf("after updating inst %d: %d\n", this->m_reg.count, this->m_reg.out);
    //   exit(-1);
    // }
    // if(this->d_reg.count == 2){
    //   printf("%s\n", "inst 2 continues");
    //   exit(-1);
    // }
    // if(this->d_reg_new.count == 7){
    //   printf("inst %d: %d\n", this->d_reg_new.count, this->d_reg_new.inst);
    // }
    memset(&this->f_reg_new, 0, sizeof(this->f_reg_new));
    memset(&this->d_reg_new, 0, sizeof(this->d_reg_new));
    memset(&this->e_reg_new, 0, sizeof(this->e_reg_new));
    memset(&this->m_reg_new, 0, sizeof(this->m_reg_new));
  }
}

// update pc and f_reg_new
void Simulator::fetch() {
  this->f_reg_new.bubble = false; //f_reg.bubble will be updated to false in next cycle
  this->f_reg_new.stall = 0;
  this->f_reg_new.len = 4;
  this->f_reg_new.pc = this->pc;
  this->f_reg_new.inst = this->memory->get_int(this->pc);
  this->f_reg_new.count = this->inst_count;
  if(debug){
    printf("fetch inst %d: %04x\n", this->f_reg_new.count, this->f_reg_new.inst);
  }
  // if(this->f_reg_new.count == 331){
  //   exit(-1);
  // }
  this->pc = pc + 4;

}

// decode instruction, deal with bubble and stall
// update pipeline register
void Simulator::decode() {
  if(this->f_reg.bubble){
    if(debug){
      printf("%s\n", "bubble at decode");
    }
    this->d_reg_new.bubble = true;
    return;
  }
  // get rs1 and rs2
  uint32_t opcode = this->f_reg.inst & 0x7F; // get bit 0 - 6;
  this->d_reg_new.rs1 = (this->f_reg.inst >> 15) & 0x1F;
  this->d_reg_new.rs2  = (this->f_reg.inst >> 20) & 0x1F;
  
  // check data hazard
  bool data_hazard = false;
  switch(opcode){
    case OP_REG: 
    case OP_BRANCH:
    case OP_STORE:
      if(this->dependence_table.find(this->d_reg_new.rs1) != this->dependence_table.end() 
      || this->dependence_table.find(this->d_reg_new.rs2) != this->dependence_table.end()){
        data_hazard = true;
      }
      break;
    case OP_IMM:
    case OP_LOAD:
    case OP_JALR:
      if(this->dependence_table.find(this->d_reg_new.rs1) != this->dependence_table.end()){
        data_hazard = true;
      }
      break;
    case OP_SYSTEM:
      if(this->dependence_table.find(10) != this->dependence_table.end() 
      || this->dependence_table.find(17) != this->dependence_table.end()){
        data_hazard = true;
      }
      break;
    case OP_LUI:
    case OP_AUIPC:
    case OP_JAL:
      data_hazard = false; // not using rs1 or rs2
      break;
    default:
      printf("%s\n", "error: check data hazard at unknown instruction");
      exit(-1);
      break;
  }
  if(data_hazard){
    if(debug){
      printf("inst %d stall at decode\n", this->f_reg.count);
    }
    this->pc -= 4;
    this->inst_count --;
    // set fetch and decode stage as stall
    this->d_reg_new.stall = true;
    return;
  }
  this->d_reg_new.stall = false;

  // start decoding
  if(debug){
    printf("decode inst %d: %04x\n", this->f_reg.count, this->f_reg.inst);
    printf("opcode of inst %d: %04x\n", this->f_reg.count, opcode);
  }
  // std::cin >> this->control;
  // if(control == 0){
  //   exit(-1);
  // }
  // commonly used fields for decoding
  uint32_t field_funct3 = (this->f_reg.inst >> 12) & 0x7; 
  uint32_t field_funct7 = (this->f_reg.inst >> 25) & 0x7F; 
  this->d_reg_new.dest = (this->f_reg.inst >> 7) & 0x1F;
  int32_t imm_5_11;
  switch (opcode) {
    // R-type
    case OP_REG: 
      // update pipeline register
      this->d_reg_new.write_reg = true;
      this->d_reg_new.op1 = this->reg[this->d_reg_new.rs1];
      this->d_reg_new.op2 = this->reg[this->d_reg_new.rs2];
      this->d_reg_new.imm = 0;
      // decode for instruction
      this->d_reg_new.inst = decode_R(field_funct3, field_funct7);
      break;
    case OP_IMM:
      this->d_reg_new.write_reg = true;
      this->d_reg_new.op1 = this->reg[this->d_reg_new.rs1];
      this->d_reg_new.op2 = int32_t(this->f_reg.inst) >> 20;
      this->d_reg_new.imm = int32_t(this->f_reg.inst) >> 20;
      imm_5_11 = this->d_reg_new.imm >> 5;
      // decode for instruction
      this->d_reg_new.inst = decode_I(opcode, field_funct3, imm_5_11);
      break;
    case OP_LUI:
      this->d_reg_new.write_reg = true;
      this->d_reg_new.op1 = 0;
      this->d_reg_new.op2 = 0;
      this->d_reg_new.imm = int32_t(this->f_reg.inst) >> 12;
      // decode for instruction
      this->d_reg_new.inst = decode_UJ(opcode);
      break;
    case OP_BRANCH:
      this->d_reg_new.write_reg = false;
      this->d_reg_new.op1 = this->reg[this->d_reg_new.rs1];
      this->d_reg_new.op2 = this->reg[this->d_reg_new.rs2];
      this->d_reg_new.imm = int32_t(((this->f_reg.inst >> 7) & 0x1E) | ((this->f_reg.inst >> 20) & 0x7E0) |
                             ((this->f_reg.inst << 4) & 0x800) | ((this->f_reg.inst >> 19) & 0x1000)) << 19 >> 19;
      // decode for instruction
      this->d_reg_new.inst = decode_B(field_funct3);
      break;
    case OP_STORE:
      this->d_reg_new.write_reg = false;
      this->d_reg_new.op1 = this->reg[this->d_reg_new.rs1];
      this->d_reg_new.op2 = this->reg[this->d_reg_new.rs2];
      this->d_reg_new.imm = int32_t(((this->f_reg.inst >> 7) & 0x1F) | ((this->f_reg.inst >> 20) & 0xFE0)) << 20 >> 20;
      // decode for instruction
      this->d_reg_new.inst = decode_S(field_funct3);
      break;
    case OP_LOAD:
      this->d_reg_new.write_reg = true;
      this->d_reg_new.op1 = this->reg[this->d_reg_new.rs1];
      this->d_reg_new.op2 = 0;
      this->d_reg_new.imm = int32_t(this->f_reg.inst) >> 20;
      imm_5_11 = this->d_reg_new.imm >> 5;
      // decode for instruction
      this->d_reg_new.inst = decode_I(opcode, field_funct3, imm_5_11);
      break;
    case OP_SYSTEM:
      this->d_reg_new.write_reg = true; // ?
      this->d_reg_new.op1 = this->reg[10]; // reg a0
      this->d_reg_new.op2 = this->reg[17]; // reg a7
      this->d_reg_new.dest = 10; //return value will be saved in a0
      this->d_reg_new.imm = int32_t(this->f_reg.inst) >> 20;
      imm_5_11 = this->d_reg_new.imm >> 5;
      // decode for instruction
      this->d_reg_new.inst = decode_I(opcode, field_funct3, imm_5_11);
      break;
    case OP_AUIPC:
      this->d_reg_new.write_reg = true;
      this->d_reg_new.op1 = 0;
      this->d_reg_new.op2 = 0;
      this->d_reg_new.imm = int32_t(this->f_reg.inst) >> 12;
      // decode for instruction
      this->d_reg_new.inst = decode_UJ(opcode);
      break;
    case OP_JAL:
      this->d_reg_new.write_reg = true;
      this->d_reg_new.op1 = 0;
      this->d_reg_new.op2 = 0;
      this->d_reg_new.imm = int32_t(((this->f_reg.inst >> 21) & 0x3FF) | ((this->f_reg.inst >> 10) & 0x400) |
                             ((this->f_reg.inst >> 1) & 0x7F800) | ((this->f_reg.inst >> 12) & 0x80000)) << 12 >> 11;
      // decode for instruction
      this->d_reg_new.inst = decode_UJ(opcode);
      break;
    case OP_JALR:
      this->d_reg_new.write_reg = true;
      this->d_reg_new.op1 = this->reg[this->d_reg_new.rs1];
      this->d_reg_new.op2 = 0;
      this->d_reg_new.imm = int32_t(this->f_reg.inst) >> 20;
      imm_5_11 = this->d_reg_new.imm >> 5;
      // decode for instruction
      this->d_reg_new.inst = decode_I(opcode, field_funct3, imm_5_11);    
      break;
    default:
      printf("%s\n", "error: decode unknown instruction");
      exit(-1);
      break;
  }
  // update pipeline register
  this->d_reg_new.bubble = false;
  this->d_reg_new.pc = this->f_reg.pc;
  this->d_reg_new.count = this->f_reg.count;

  // build dependence table for rd 
  if(this->d_reg_new.write_reg && this->d_reg_new.dest != 0){
    this->dependence_table.insert(this->d_reg_new.dest);
    if(debug){
     printf("insert rd at inst %d\n", this->d_reg_new.count);
     for(auto& i : this->dependence_table){
       printf("dependence table: %d\n", i);
     }      
    }

  }
  if(debug){
    printf("decode inst %d: op1: %d, op2 %d\n", this->d_reg_new.count,this->d_reg_new.op1, this->d_reg_new.op2);
    printf("decode result of inst %d: %d\n", this->d_reg_new.count, this->d_reg_new.inst);    
  }

  // if(this->d_reg_new.count == 7){
  //   exit(-1);
  // }
}

// execute instruction, deal with bubble and stall, check hazard and forward
// data update pipeline register
void Simulator::excecute() {
  // return;
  this->e_reg_new.bubble = this->d_reg.bubble;
  if(d_reg.bubble){
    if(debug){
     printf("%s\n", "bubble at execute");
    }
    // if(f_reg_new.count == 8){
    //   exit(-1);
    // }
    return;
  }
  if(debug){
    printf("test: execute inst %d\n", this->d_reg.count);
  }

  // elements used in execution operation
  int32_t op1 = this->d_reg.op1;
  int32_t op2 = this->d_reg.op2;
  int32_t imm = this->d_reg.imm;
  // printf("execute: inst %d, op1: %d,  op2:%d\n", this->d_reg.count, this->d_reg.op1, this->d_reg.op2);
  int32_t out;
  bool read_unsigned_mem = false;
  bool read_signed_mem = false;
  bool write_mem = false;
  uint32_t mem_len;
  bool branch = false;

  switch(this->d_reg.inst){
    case ADD:
    case ADDI:
      out = op1 + op2;
      break;
    case SUB:
      out = op1 - op2;
      break;
    case XOR:
    case XORI:
      out = op1 ^ op2;
      break;
    case OR:
    case ORI:
      out = op1 | op2;
      break;
    case AND:
    case ANDI:
      out = op1 & op2;
      break;
    case SLL:
      out = op1 << op2;
      break;
    case SLLI:
      out = op1 << (op2 & 0x1F);
      break;
    case SRL:
      out = uint32_t(op1) >> uint32_t(op2);
      break;
    case SRLI:
      out = uint32_t(op1) >> uint32_t(op2 & 0x1F);
      break;
    case SRA:
      out = op1 >> op2;
      break;
    case SRAI:
      out = op1 >> (op2 & 0x1F);
      break;
    case SLT:
    case SLTI:
      out = (op1 < op2) ? 1 : 0;
      break;
    case SLTU:
    case SLTIU:
      out = (uint32_t(op1) < uint32_t(op2)) ? 1 : 0;
      break;
    case LB:
      out = op1 + imm;
      read_signed_mem = true;
      mem_len = 1;
      break;
    case LH:
      out = op1 + imm;
      read_signed_mem = true;
      mem_len = 2;
      break;
    case LW:
      out = op1 + imm;
      read_signed_mem = true;
      mem_len = 4;
      break;
    case LBU:
      out = op1 + imm;
      read_unsigned_mem = true;
      mem_len = 1;
      break;
    case LHU:
      out = op1 + imm;
      read_unsigned_mem = true;
      mem_len = 2;
      break;
    case SB:
      out = op1 + imm;
      write_mem = true;
      mem_len = 1;
      op2 = op1 & 0xFF;
      break;
    case SH:
      out = op1 + imm;
      write_mem = true;
      mem_len = 2;
      op2 = op2 & 0xFFFF;
      break;
    case SW:
      out = op1 + imm;
      write_mem = true;
      mem_len = 4;
      break;
    case BEQ:
      if(op1 == op2){
        branch = true;
        this->pc = this->d_reg.pc + imm;
      }
      break;
    case BNE:
      if(op1 != op2){
        branch = true;
        this->pc = this->d_reg.pc + imm;
      }
      break;
    case BLT:
      if(op1 < op2){
        branch = true;
        this->pc = this->d_reg.pc + imm;
      }
      break;
    case BGE:
      if(op1 >= op2){
        branch = true;
        this->pc = this->d_reg.pc + imm;
      }
      break;
    case BLTU:
      if(uint32_t(op1) < uint32_t(op2)){
        branch = true;
        this->pc = this->d_reg.pc + imm;
      }
      break;
    case BGEU:
      if(uint32_t(op1) >= uint32_t(op2)){
        branch = true;
        this->pc = this->d_reg.pc + imm;
      }
      break;
    case JAL:
      branch = true;
      out = this->d_reg.pc + 4;
      this->pc = this->d_reg.pc + imm;
      break;
    case JALR:
      branch = true;
      out = this->d_reg.pc + 4;
      this->pc = op1 + imm;
      break;
    case LUI:
      out = imm << 12;
      break;
    case AUIPC:
      out = this->d_reg.pc + (imm << 12);
      break;
    case ECALL:
      out = this->handle_system_call(op1, op2);
      break;
    default:
      printf("error: execute unknown instruction at inst %d\n", this->d_reg.count);
      exit(-1);

  }
  // if(this->d_reg.count == 640){
  //   printf("%d\n", out);
  //   exit(-1);
  // }
  // deal with control hazard
  if(branch){
    //printf("branch at inst %d:\n", this->d_reg.count);
    // flush the current fetch stage and decode stage
    if(this->d_reg_new.write_reg){
      this->dependence_table.erase(d_reg_new.dest);
    }
    memset(&this->f_reg_new, 0, sizeof(this->f_reg_new));
    memset(&this->d_reg_new, 0, sizeof(this->d_reg_new));
    this->f_reg_new.bubble = true;
    this->d_reg_new.bubble = true;
    this->inst_count -= 2;
  }
  if(debug){
    printf("execute inst %d: %d\n", this->d_reg.count, out);
  }
  // update pipeline register
  this->e_reg_new.pc = this->d_reg.pc;
  this->e_reg_new.inst = this->d_reg.inst;
  this->e_reg_new.op1 = this->d_reg.op1;
  this->e_reg_new.op2 = this->d_reg.op2; // will be used in STORE
  this->e_reg_new.write_reg = this->d_reg.write_reg;
  this->e_reg_new.dest_reg = this->d_reg.dest;
  this->e_reg_new.out = out;
  this->e_reg_new.write_mem = write_mem;
  this->e_reg_new.read_unsigned_mem = read_unsigned_mem;
  this->e_reg_new.read_signed_mem = read_signed_mem;
  this->e_reg_new.mem_len = mem_len;
  this->e_reg_new.count = this->d_reg.count;
}

// memory access, deal with bubble and stall
void Simulator::memory_access() {
  if(debug){
    printf("test: memory access inst %d\n", this->e_reg.count);
  }
  if(this->e_reg.bubble){
    if(debug){
      printf("%s\n", "bubble at memory");
    }
    this->m_reg_new.bubble = true;
    return;
  }
  int32_t op1 = this->e_reg.op1;
  int32_t op2 = this->e_reg.op2;
  int32_t out = this->e_reg.out;
  uint32_t count = this->e_reg.count;


  if(this->e_reg.read_signed_mem){
    switch(this->e_reg.mem_len){
      case 1:
        out = int32_t(this->memory->get_byte(out));
        break;
      case 2:
        out = int32_t(this->memory->get_short(out));
        break;
      case 4:
        out = int32_t(this->memory->get_int(out));
        break;
      default:
        printf("error: reading invalid memory length for signed integer at inst %d", count);
        exit(-1);
    }
  }
  if(this->e_reg.read_unsigned_mem){
    switch(this->e_reg.mem_len){
      case 1:
        out = uint32_t(this->memory->get_byte(out));
        break;
      case 2:
        out = uint32_t(this->memory->get_short(out));
        break;
      default:
        printf("error: reading invalid memory length for unsigned integer at inst %d\n", count);
        exit(-1);
    }
  }
  if(debug){
    printf("finish memory access at inst %d: out: %d\n", this->e_reg.count, out);
  }

  if(this->e_reg.write_mem){
    bool set = true;
    // printf("write mem inst %d\n", this->e_reg.count);
    // printf("a10 %d\n", this->reg[11]);
    // printf("gp %x\n", this->reg[3]);
    switch(this->e_reg.mem_len){
      case 1:
        set = this->memory->set_byte(out, op2);
        break;
      case 2:
        set = this->memory->set_short(out, op2);
        break;
      case 4:
        set = this->memory->set_int(out, op2);
        break;
      default:
        printf("error: writing invalid memory length at inst %d\n", count);
        exit(-1);
    }
    if(!set){
      printf("error: failed to write memory at inst %d\n", count);
      exit(-1);
    }
  }
  // update pipeline register
  this->m_reg_new.bubble = this->e_reg.bubble;
  this->m_reg_new.pc = this->e_reg.pc;
  this->m_reg_new.inst = this->e_reg.inst;
  this->m_reg_new.op1 = op1;
  this->m_reg_new.op2 = op2;
  this->m_reg_new.out = out;
  this->m_reg_new.write_reg = this->e_reg.write_reg;
  this->m_reg_new.dest_reg = this->e_reg.dest_reg;
  this->m_reg_new.count = count;

}

// write result to register, deal with bubble and stall
// check for data hazard and forward data
// update pipeline register
void Simulator::write_back() {

  if(this->m_reg.bubble){
    if(debug){
      printf("%s\n", "bubble at write_back");
    }
    return;
  }
  if(!this->m_reg.write_reg || this->m_reg.dest_reg == 0){
    return;
  }
  this->reg[this->m_reg.dest_reg] =  this->m_reg.out;
  //printf("before: dependence table size %d\n", this->dependence_table.size());
  this->dependence_table.erase(this->m_reg.dest_reg);
  // if(this->m_reg.count == 1){
  //    printf("dependence table size %d\n", this->dependence_table.size());
  // }
}



// handle system according to system call number in reg a7
// exit program using exit(0);
// op1: reg a0 - parameter 
// op2: reg a7 - syscall number
int32_t Simulator::handle_system_call(int32_t op1, int32_t op2) {
  switch(op2){
    case 0:{ // print string 
      int32_t address = op1;
      char ch = this->memory->get_byte(address);
      while(ch != '\0'){
        printf("%c", ch);
        address ++;
        ch = this->memory->get_byte(address);
      }
      break;
    }
    case 1: // print char
      printf("%c", char(op1));
      break;
    case 2: // print number
      printf("%d", op1);
      break;
    case 3: // exit program
      if(debug){
        printf("%s\n", "program exits from system call 3");
      }
      exit(0);
    case 4: // read char
      scanf(" %c", (char*)&op1);
      break;
    case 5: // read number
      scanf(" %lld", &op1);
      break;
    default:
      printf("%s\n", "error: unknown system call");
      exit(-1);
  }
  return op1; 
}


Instruction Simulator::decode_R(uint32_t funct3, uint32_t funct7){
  switch(funct3){
    case 0x0:
      if(funct7 == 0x00){
        return ADD;
      }else{
        return SUB;
      }
    case 0x4:
      return XOR;
    case 0x6:
      return OR;
    case 0x7:
      return AND;
    case 0x1:
      return SLL;
    case 0x5:
      if(funct7 == 0x00){
        return SRL;
      }else{
        return SRA;
      }
    case 0x2:
      return SLT;
    case 0x3:
      return SLTU;
    default:
        printf("%s\n", "error: unknown R instruction");
        exit(-1);
  }
}
Instruction Simulator::decode_I(uint32_t opcode, uint32_t funct3, uint32_t imm_5_11){
  if(opcode == OP_IMM){
    switch(funct3){
      case 0x0:
        return ADDI;
      case 0x4:
        return XORI;
      case 0x6:
        return ORI;
      case 0x7:
        return ANDI;
      case 0x1:
        return SLLI;
      case 0x5:
        if(imm_5_11 == 0x00){
          return SRLI;
        }else{
          return SRAI;
        }
      case 0x2:
        return SLTI;
      case 0x3:
        return SLTIU;
      default:
        printf("%s\n", "error: unknown IMM instruction");
        exit(-1);
    }
  }else if(opcode == OP_LOAD){
    switch(funct3){
      case 0x0:
        return LB;
      case 0x1:
        return LH;
      case 0x2:
        return LW;
      case 0x4:
        return LBU;
      case 0x5:
        return LHU;
      default:
        printf("%s\n", "error: unknown LOAD instruction");
        exit(-1);
    }
  }else if(opcode == OP_SYSTEM){
    return ECALL;
  }else if(opcode == OP_JALR){
    return JALR;
  }else{
      printf("%s\n", "error: unknown I instruction");
      exit(-1);
  }
}
Instruction Simulator::decode_S(uint32_t funct3){
  switch(funct3){
    case 0x0:
      return SB;
    case 0x1:
      return SH;
    case 0x2:
      return SW;
    default:
      printf("%s\n", "error: unknown S instruction");
      exit(-1);
  }

}
Instruction Simulator::decode_B(uint32_t funct3){
  switch(funct3){
    case 0x0:
      return BEQ;
    case 0x1:
      return BNE;
    case 0x4:
      return BLT;
    case 0x5:
      return BGE;
    case 0x6:
      return BLTU;
    case 0x7:
      return BGEU;
    default:
      printf("%s\n", "error: unknown SB instruction");
      exit(-1);
  }

}
Instruction Simulator::decode_UJ(uint32_t opcode){
  switch(opcode){
    case OP_LUI:
      return LUI;
    case OP_AUIPC:
      return AUIPC;
    case OP_JAL:
      return JAL;
    default:
      printf("%s\n", "error: unknown UJ instruction");
      exit(-1);
  }

}
