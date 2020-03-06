/*
 *  cpu.c
 *  Contains APEX cpu pipeline implementation
 *
 *  Author :
 *  Gaurav Kothari (gkothar1@binghamton.edu)
 *  State University of New York, Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

/* Set this flag to 1 to enable debug messages */
#define ENABLE_DEBUG_MESSAGES 1

/*
 * This function creates and initializes APEX cpu.
 *
 * Note : You are free to edit this function according to your
 *        implementation
 */
APEX_CPU*
APEX_cpu_init(const char* filename)
{
  if (!filename) {
    return NULL;
  }

  APEX_CPU* cpu = malloc(sizeof(*cpu));
  if (!cpu) {
    return NULL;
  }

  /* Initialize PC, Registers and all pipeline stages */
  cpu->pc = 4000;
  cpu->halt=0;
  memset(cpu->regs, 0, sizeof(int) * 32);
  memset(cpu->regs_valid, 1, sizeof(int) * 32);
  memset(cpu->stage, 0, sizeof(CPU_Stage) * NUM_STAGES);
  memset(cpu->data_memory, 0, sizeof(int) * 4000);
  memset(cpu->stage_set,1,sizeof(int) * 5 * 2);
  memset(cpu->stage_check,0,sizeof(int) * 5 * 2);
  memset(cpu->regs_forward,0,sizeof(int) *32);

  /* Parse input file and create code memory */
  cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);

  if (!cpu->code_memory) {
    free(cpu);
    return NULL;
  }

  if (ENABLE_DEBUG_MESSAGES) {
    fprintf(stderr,"APEX_CPU : Initialized APEX CPU, loaded %d instructions\n",cpu->code_memory_size);
    fprintf(stderr, "APEX_CPU : Printing Code Memory\n");
    printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode", "rd", "rs1", "rs2", "imm");

  for (int i = 0; i < cpu->code_memory_size; ++i) {
  printf("%-9s %-9d %-9d %-9d %-9d\n",
      cpu->code_memory[i].opcode,
      cpu->code_memory[i].rd,
      cpu->code_memory[i].rs1,
      cpu->code_memory[i].rs2,
      cpu->code_memory[i].imm);
    }
  }

  /* Make all stages busy except Fetch stage, initally to start the pipeline */
  for (int i = 1; i < NUM_STAGES; ++i) {
    cpu->stage[i].busy = 1;
  }
  return cpu;
}

/*
 * This function de-allocates APEX cpu.
 *
 * Note : You are free to edit this function according to your
 *        implementation
 */
void
APEX_cpu_stop(APEX_CPU* cpu)
{
  free(cpu->code_memory);
  free(cpu);
}

/* Converts the PC(4000 series) into
 * array index for code memory
 *
 * Note : You are not supposed to edit this function
 *
 */
int
get_code_index(int pc)
{
  return (pc - 4000) / 4;
}

static void
print_instruction(CPU_Stage* stage)
{
  if (strcmp(stage->opcode, "STORE") == 0) {
    printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rs1, stage->rs2, stage->imm);
  }
  if (strcmp(stage->opcode, "MOVC") == 0) {
    printf("%s,R%d,#%d ", stage->opcode, stage->rd, stage->imm);
  }
  if(strcmp(stage->opcode, "HALT") == 0) {
    printf("%s",stage->opcode);
  }
  if (strcmp(stage->opcode, "ADD") == 0 || strcmp(stage->opcode, "SUB") == 0 || strcmp(stage->opcode, "MUL") == 0) {
    printf("%s,R%d,R%d,R%d",stage->opcode,stage->rd,stage->rs1,stage->rs2);
  }
  if (strcmp(stage->opcode, "AND") == 0){
    printf("%s,R%d,R%d,R%d",stage->opcode,stage->rd,stage->rs1,stage->rs2);
  }
  if(strcmp(stage->opcode, "OR") == 0) {
    printf("%s,R%d,R%d,R%d",stage->opcode,stage->rd,stage->rs1,stage->rs2);
  }
  if(strcmp(stage->opcode, "LOAD") == 0) {
     printf("%s,R%d,R%d,#%d",stage->opcode,stage->rd,stage->rs1,stage->imm);
  }
  if(strcmp(stage->opcode, "LDR") == 0) {
     printf("%s,R%d,R%d,R%d",stage->opcode,stage->rd,stage->rs1,stage->rs2);
  }

  if(strcmp(stage->opcode,"XOR") == 0) {
    printf("%s,R%d,R%d,R%d",stage->opcode,stage->rd,stage->rs1,stage->rs2);
  }
  if(strcmp(stage->opcode,"NOP") == 0) {
    printf("%s",stage->opcode);
  }

  if(strcmp(stage->opcode,"JUMP") == 0) {
   printf("%s,R%d,#%d",stage->opcode,stage->rs1,stage->imm);
  }

  if(strcmp(stage->opcode,"BZ") == 0) {
   printf("%s,#%d",stage->opcode,stage->imm);
  }
  if(strcmp(stage->opcode,"BNZ") == 0) {
   printf("%s,#%d",stage->opcode,stage->imm);
  }
}

/* Debug function which dumps the cpu stage
 * content
 *
 * Note : You are not supposed to edit this function
 *
 */
static void
print_stage_content(char* name, CPU_Stage* stage)
{
  printf("%-15s: pc(%d) ", name, stage->pc);
  print_instruction(stage);
  printf("\n");
}

/*
 *  Fetch Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 *         implementation
 */
int
fetch(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[F];
  if (!cpu->stage_check[0][0] && !cpu->stage_check[0][1] && cpu->stage_set[0][0] && !cpu->halt) {  
    /* Store current PC in fetch latch */
  stage->pc = cpu->pc;
   

    /* Index into code memory using this pc and copy all instruction fields into
     * fetch latch
     */
    APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
    strcpy(stage->opcode, current_ins->opcode);
    stage->rd = current_ins->rd;
    stage->rs1 = current_ins->rs1;
    stage->rs2 = current_ins->rs2;
    stage->imm = current_ins->imm;
    //stage->rd = current_ins->rd;

    /* Update PC for next instruction */
    cpu->pc += 4;

    /* Copy data from fetch latch to decode latch*/

    if(cpu ->stage_set[1][0])
    {
    cpu->stage[DRF] = cpu->stage[F];
    cpu->stage_set[0][0]=1;
    }
    else
    {
      cpu ->stage_set[0][0]=0;
    }
    if (strcmp((cpu->f),"display")==0) {
      print_stage_content("Fetch", stage);
    }
 if(cpu ->stage_set[1][0])
    {

      struct CPU_Stage Apex;
      cpu->stage[F]=Apex;
}
    }
   
    else if(!cpu->stage_set[0][0] && cpu->stage_set[1][0]) {
        cpu->stage_set[0][0] = 1;
        cpu->stage[DRF]=cpu->stage[F];
        if (strcmp((cpu->f),"display")==0) {
        print_stage_content("Fetch", stage);}
    } 
     else if(strcmp((cpu->f),"display")==0) {
      print_stage_content("Fetch", stage);
    }
  
    return 0;
}

/*
 *  Decode Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 *         implementation
 */
int
decode(APEX_CPU* cpu)
{
   CPU_Stage* stage = &cpu->stage[DRF];

if(strcmp(stage->opcode,"BNZ")==0 || strcmp(stage->opcode,"BZ")==0)
{
   if(strcmp((&cpu->stage[MEM])->opcode,"ADD")!=0 && strcmp((&cpu->stage[MEM])->opcode,"SUB")!=0 && strcmp((&cpu->stage[MEM])->opcode,"MUL")!=0 && strcmp((&cpu->stage[WB])->opcode,"ADD")!=0 && strcmp((&cpu->stage[WB])->opcode,"SUB")!=0 && strcmp((&cpu->stage[WB])->opcode,"MUL")!=0)
   {
	cpu->stage_check[1][1]=0;
	 cpu->stage_set[1][0] = 1;

   }	
}

if(strcmp(stage->opcode,"ADD")==0 || strcmp(stage->opcode,"SUB")==0 || strcmp(stage->opcode, "MUL") == 0) {
	//printf("%d: %d    , %d: %d\n",stage->rs1,cpu->regs_valid[stage->rs1],stage->rs2,cpu->regs_valid[stage->rs2]);
if(cpu->regs_valid[stage->rs1] && (cpu->regs_valid[stage->rs2]))
{
   cpu->stage_check[1][1]=0;
   cpu ->stage_set[1][0]=1;
}
}

if(strcmp(stage->opcode,"AND")==0) {
if(cpu->regs_valid[stage->rs1] && (cpu->regs_valid[stage->rs2]))
{
  cpu->stage_check[1][1]=0;
	cpu ->stage_set[1][0]=1;

}
}

if(strcmp(stage->opcode,"OR")==0) {
if(cpu->regs_valid[stage->rs1] && (cpu->regs_valid[stage->rs2]))
{
  cpu->stage_check[1][1]=0;
	cpu ->stage_set[1][0]=1;

}
}

if(strcmp(stage->opcode,"XOR")==0) {
if(cpu->regs_valid[stage->rs1] && (cpu->regs_valid[stage->rs2]))
{
  cpu->stage_check[1][1]=0;
	cpu ->stage_set[1][0]=1;

}
}

if(strcmp(stage->opcode,"LDR")== 0 || strcmp(stage->opcode,"STORE")== 0) {
if(cpu->regs_valid[stage->rs1] && (cpu->regs_valid[stage->rs2]))
{
  cpu->stage_check[1][1]=0;
	cpu ->stage_set[1][0]=1;

} 
}

if(strcmp(stage->opcode,"LOAD")==0) {
if(cpu->regs_valid[stage->rs1])
{
  cpu->stage_check[1][1]=0;
	cpu ->stage_set[1][0]=1;

}
}


if(!cpu->stage_check[1][0] && !cpu->stage_check[1][1] && !cpu->halt) {
    /* Read data from register file for store */
  if (strcmp(stage->opcode, "STORE") == 0) {

  if(cpu->regs_valid[stage->rs1] && (cpu->regs_valid[stage->rs2]) )
    {
     stage->rs1_value=cpu->regs[stage->rs1];
     stage->rs2_value=cpu->regs[stage->rs2];
    }
  else{
    cpu->stage_check[1][1]=1;
    cpu ->stage_set[1][0]=0;
    }
    }
   
  if (strcmp(stage->opcode, "LOAD") == 0) {
     //busy
    if(cpu->regs_valid[stage->rs1])
    {
       stage->rs1_value=cpu->regs[stage->rs1];
	cpu->regs_valid[stage->rd]= 0;
    }
    else {
      cpu->stage_check[1][1]=1;
      cpu->stage_set[1][0]=0;
    }  
  }

if(strcmp(stage->opcode,"BNZ")==0 || strcmp(stage->opcode,"BZ")==0)
{
   if(strcmp((&cpu->stage[MEM])->opcode,"ADD")==0 || strcmp((&cpu->stage[MEM])->opcode,"SUB")==0 || strcmp((&cpu->stage[MEM])->opcode,"MUL")==0 || strcmp((&cpu->stage[WB])->opcode,"ADD")==0 || strcmp((&cpu->stage[WB])->opcode,"SUB")==0 || strcmp((&cpu->stage[WB])->opcode,"MUL")==0)
   {
	cpu->stage_check[1][1]=1;
	cpu->stage_set[1][0] = 0;
	
   }	
}

     /* No Register file read needed for MOVC */
    if (strcmp(stage->opcode, "MOVC") == 0) {
    cpu->regs_valid[stage->rd] = 0;
    }
    if (strcmp(stage->opcode, "AND") == 0) {
    
    if((cpu->regs_valid[stage->rs1]) && (cpu->regs_valid[stage->rs2])) {
    stage->rs1_value=cpu->regs[stage->rs1];
    stage->rs2_value=cpu->regs[stage->rs2]; 
    cpu->regs_valid[stage->rd]=0; 
    }
    else{
     cpu->stage_check[1][1]=1;
     cpu -> stage_set[1][0]=0;
    }
    }

    if (strcmp(stage->opcode, "OR") == 0) {
    
    if((cpu->regs_valid[stage->rs1]) && (cpu->regs_valid[stage->rs2])) {
    stage->rs1_value=cpu->regs[stage->rs1];
    stage->rs2_value=cpu->regs[stage->rs2]; 
    cpu->regs_valid[stage->rd]=0; 
    }
    else{
    cpu->stage_check[1][1]=1;
    cpu -> stage_set[1][0]=0;
    }
    }

    if (strcmp(stage->opcode, "SUB") == 0 || strcmp(stage->opcode, "ADD") == 0 || strcmp(stage->opcode, "MUL") == 0) {
    
    if((cpu->regs_valid[stage->rs1]) && (cpu->regs_valid[stage->rs2])) {
    stage->rs1_value=cpu->regs[stage->rs1];
    stage->rs2_value=cpu->regs[stage->rs2];  
    cpu->regs_valid[stage->rd]=0;
    }
    else{
    cpu->stage_check[1][1]=1;
    cpu -> stage_set[1][0]=0;
    }
    }

    if (strcmp(stage->opcode, "XOR") == 0) {
    
    if((cpu->regs_valid[stage->rs1]) && (cpu->regs_valid[stage->rs2])) {
    stage->rs1_value=cpu->regs[stage->rs1];
    stage->rs2_value=cpu->regs[stage->rs2];
    cpu->regs_valid[stage->rd]=0;  
    }
    else{
    cpu->stage_check[1][1]=1;
    cpu -> stage_set[1][0]=0;
    }
    }

   if(strcmp(stage->opcode,"HALT")==0)
    {
    cpu->halt=1;
    }


    if(strcmp(stage->opcode, "JUMP")==0)
    {
      if(cpu->regs_valid[stage->rs1])
      {
      stage->rs1_value=cpu->regs[stage->rs1];
      }
      else{
	 cpu->stage_check[1][1]=1;
         cpu -> stage_set[1][0]=0;
	
      }
    }
    if (strcmp(stage->opcode, "LDR") == 0) {
    
    if((cpu->regs_valid[stage->rs1]) && (cpu->regs_valid[stage->rs2])) {
    stage->rs1_value=cpu->regs[stage->rs1];
    stage->rs2_value=cpu->regs[stage->rs2];
    cpu->regs_valid[stage->rd]=0;  
    }
    else{
    cpu->stage_check[1][1]=1;
    cpu -> stage_set[1][0]=0;
    }
    }

    /* Copy data from decode latch to execute latch*/
    if(!cpu->stage_check[1][1]) {
    cpu->stage_set[1][0]=1;
    cpu->stage[EX] = cpu->stage[DRF];
    }  
 
    else if(!cpu->stage_check[2][0]){
    struct CPU_Stage Bubble;
    cpu->stage[EX]=Bubble;
    }
    if (strcmp((cpu->f),"display")==0) {
    print_stage_content("Decode/RF", stage);
    }
     
    }
    else if(!cpu->stage_set[2][0] && !cpu->stage_set[1][0] && strcmp((cpu->f),"display")==0) {
    print_stage_content("Decode/RF", stage);
    cpu->stage_set[1][0]=1;
    }

    else if(strcmp((cpu->f),"display")==0) {
    print_stage_content("Decode/RF", stage);
     } 
     return 0;
}
  
/*
 *  Execute Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 *         implementation
 */
int
execute(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[EX];

  if (!cpu->stage_check[2][0] && !cpu->stage_check[2][1])
{
   /* Store */
  if (strcmp(stage->opcode, "STORE") == 0) {
    stage->mem_address=(stage->rs2_value)+(stage->imm);
  }
  if (strcmp(stage->opcode, "LOAD") == 0) {
    stage->mem_address=(stage->rs1_value)+(stage->imm);
  }
  if (strcmp(stage->opcode, "LDR") == 0) {
    stage->mem_address=(stage->rs1_value)+(stage->rs2_value);
  }
 
  if(strcmp(stage->opcode, "JUMP") == 0)
  {
	cpu->pc=(stage->rs1_value)+(stage->imm);
	cpu->ins_completed = (cpu->pc - 4000) /4;
  } 
  if(strcmp(stage->opcode, "BZ") == 0) {

  if(strcmp((&cpu->stage[WB])->opcode,"ADD")==0 || strcmp((&cpu->stage[WB])->opcode,"SUB")==0 || strcmp((&cpu->stage[WB])->opcode,"MUL")==0)
  {
 cpu->stage_check[2][0] = 1;
   cpu->stage_check[1][1]=1;
   (&cpu->stage[DRF])->stalled =1;
    (&cpu->stage[F])->stalled =1;
     
  }
  else {

  if(cpu->zflag == 1)
  {
  cpu->pc=(stage->pc)+(stage->imm);
  strcpy(cpu->stage[F].opcode,"NOP");
  strcpy(cpu->stage[DRF].opcode,"NOP");
  cpu->halt=0;
  cpu->ins_completed = (cpu->pc - 4000) /4;
  }
  
  }
}
  if(strcmp(stage->opcode, "BNZ") == 0) {

  if(strcmp((&cpu->stage[WB])->opcode,"ADD")==0 || strcmp((&cpu->stage[WB])->opcode,"SUB")==0 || strcmp((&cpu->stage[WB])->opcode,"MUL")==0)
  {
 cpu->stage_check[2][0] = 1;
   cpu->stage_check[1][1]=1;
   (&cpu->stage[DRF])->stalled =1;
    (&cpu->stage[F])->stalled =1;
  }
  else {

  if(cpu->zflag == 0)
  {
  cpu->pc=(stage->pc)+(stage->imm);
  strcpy(cpu->stage[F].opcode,"NOP");
  strcpy(cpu->stage[DRF].opcode,"NOP");
  cpu->halt=0;
  cpu->ins_completed = (cpu->pc - 4000) /4;
  }
  
  }
}

   if(strcmp(stage->opcode,"HALT")==0)
    {
    cpu->halt++;
    }

   /* MOVC */
  if (strcmp(stage->opcode, "MOVC") == 0) {
    stage->buffer=0+(stage->imm);
   }
  if (strcmp(stage->opcode, "ADD") == 0) {
    stage->buffer=(stage->rs1_value)+(stage->rs2_value);
    (cpu->regs_forward[stage->rd])=1;
  }
  if (strcmp(stage->opcode, "XOR") == 0) {
    stage->buffer=(stage->rs1_value)^(stage->rs2_value);
     (cpu->regs_forward[stage->rd])=1;

  }

  if(strcmp(stage->opcode, "SUB") == 0) {
     stage->buffer=(stage->rs1_value)-(stage->rs2_value);
      (cpu->regs_forward[stage->rd])=1;

  }

    if(strcmp(stage->opcode, "MUL") == 0) {
     cpu->stage_check[2][0]=1;
 cpu->stage_check[1][0]=1;

 cpu->stage_check[0][0]=1;

    // stage->buffer=(stage->rs1_value)*(stage->rs2_value);
     
      //(cpu->regs_forward[stage->rd])=1;

  }


  if (strcmp(stage->opcode, "AND") == 0) {
     stage->buffer=(stage->rs1_value)&(stage->rs2_value);
      (cpu->regs_forward[stage->rd])=1;

  }
 
  if (strcmp(stage->opcode, "OR") == 0) {
      stage->buffer=(stage->rs1_value)|(stage->rs2_value);
       (cpu->regs_forward[stage->rd])=1;

  }

  /* Copy data from Execute latch to Memory latch*/
  if(!cpu->stage_check[2][0]) {
    cpu->stage[MEM] = cpu->stage[EX];
    cpu->stage_set[2][0]=1; //free
  }
  else {
    struct CPU_Stage Bubble;
    cpu->stage[MEM]=Bubble;
    cpu->stage_set[2][0]=0;  
  }

  if (strcmp((cpu->f),"display")==0) {
    print_stage_content("Execute", stage);
  } 
    if(!cpu->stage_check[2][0]) {
    struct CPU_Stage Apex;
    cpu->stage[EX]=Apex;
  }
}
else if(strcmp(stage->opcode, "MUL") == 0 && cpu->stage_check[2][0]) {
	cpu->stage_check[2][0] = 0;
cpu->stage_check[1][0]=0;

 cpu->stage_check[0][0]=0;

	stage->buffer=(stage->rs1_value)*(stage->rs2_value);
     
      	(cpu->regs_forward[stage->rd])=1;
	cpu->stage[MEM] = cpu->stage[EX];
    	
  if (strcmp((cpu->f),"display")==0) {
    print_stage_content("Execute", stage);
  } 
 struct CPU_Stage Bubble;
    cpu->stage[EX]=Bubble;

}
else if(strcmp(stage->opcode, "BZ") == 0 && cpu->stage_check[2][0]) {
	cpu->stage_check[2][0] = 0;
cpu->stage_check[1][0]=0;

 cpu->stage_check[0][0]=0;

	if(cpu->zflag == 1)
  {
  cpu->pc=(stage->pc)+(stage->imm);
  strcpy(cpu->stage[F].opcode,"NOP");
  strcpy(cpu->stage[DRF].opcode,"NOP");
	cpu->ins_completed = (cpu->pc - 4000) /4;
  }	cpu->stage[MEM] = cpu->stage[EX];
    	
  if (strcmp((cpu->f),"display")==0) {
    print_stage_content("Execute", stage);
  } 
 struct CPU_Stage Bubble;
    cpu->stage[EX]=Bubble;

}

else if(strcmp(stage->opcode, "BNZ") == 0 && cpu->stage_check[2][0]) {
	cpu->stage_check[2][0] = 0;
cpu->stage_check[1][0]=0;

 cpu->stage_check[0][0]=0;

	if(cpu->zflag == 0)
  {
  cpu->pc=(stage->pc)+(stage->imm);
  strcpy(cpu->stage[F].opcode,"NOP");
  strcpy(cpu->stage[DRF].opcode,"NOP");
	cpu->ins_completed = (cpu->pc - 4000) /4;
  }	cpu->stage[MEM] = cpu->stage[EX];
    	
  if (strcmp((cpu->f),"display")==0) {
    print_stage_content("Execute", stage);
  } 
 struct CPU_Stage Bubble;
    cpu->stage[EX]=Bubble;

}     

   return 0;
   
}
 

/*
 *  Memory Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 *         implementation
 */
int
memory(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[MEM];
  if (!cpu->stage_check[3][0] && !cpu->stage_check[3][1]) {
  /* Store */
  if (strcmp(stage->opcode, "STORE") == 0) {
	//printf("STORE MEM : add: %d  value add %d\n",stage->mem_address,stage->rs1_value);
    cpu->data_memory[stage->mem_address]=stage->rs1_value;  
  }
  if (strcmp(stage->opcode, "LDR") == 0) {
    stage->buffer=cpu->data_memory[stage->mem_address];  
  }
  if (strcmp(stage->opcode, "LOAD") == 0) {
    stage->buffer=cpu->data_memory[stage->mem_address];  
  }

   if(strcmp(stage->opcode,"HALT")==0)
    {
    cpu->halt++;
    }

   /* Copy data from decode latch to execute latch*/

  cpu->stage[WB] = cpu->stage[MEM];
  cpu->stage_set[3][0]=1;
  if (strcmp((cpu->f),"display")==0) {
  print_stage_content("Memory", stage);
  }
      struct CPU_Stage Apex;
      cpu->stage[MEM]=Apex;
  }    
  
  return 0;
}

/*
 *  Writeback Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 *         implementation
 */
int writeback(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[WB];
  if (!cpu->stage_check[4][0] && !cpu->stage_check[4][1]) {
    /* Update register file */
    if (strcmp(stage->opcode, "MOVC") == 0) {
      cpu->regs[stage->rd] = stage->buffer;
      cpu->regs_valid[stage->rd]=1;
      cpu->ins_completed++;
    }
    if (strcmp(stage->opcode, "LOAD") == 0) {
      cpu->regs[stage->rd] = stage->buffer;
      cpu->regs_valid[stage->rd]=1;
      cpu->ins_completed++;
    }
    if (strcmp(stage->opcode, "LDR") == 0) {
      cpu->regs[stage->rd] = stage->buffer;
      cpu->regs_valid[stage->rd]=1;
      cpu->ins_completed++;
    }

     if(strcmp(stage->opcode,"HALT")==0)
    {
    cpu->halt++;
    }

    if (strcmp(stage->opcode, "STORE") == 0) {
      
      cpu->ins_completed++;
    }
    if (strcmp(stage->opcode, "ADD") == 0 || strcmp(stage->opcode, "MUL") == 0) {
      cpu->regs[stage->rd]=stage->buffer;
      cpu->regs_valid[stage->rd]=1;
      if((cpu->regs[stage->rd]) == 0)
      {
        cpu->zflag=1;
        cpu->nzflag=0;
      }
      else {
        cpu->zflag=0;
        cpu->nzflag=1;
      }
       cpu->ins_completed++;
    }
    if (strcmp(stage->opcode, "XOR") == 0) {
      cpu->regs[stage->rd]=stage->buffer;
      cpu->regs_valid[stage->rd]=1;
       cpu->ins_completed++;
    }
    if(strcmp(stage->opcode, "SUB")== 0) {
      cpu->regs[stage->rd]=stage->buffer;
      cpu->regs_valid[stage->rd]=1;
      if((cpu->regs[stage->rd]) == 0)
      {
        cpu->zflag=1;
        cpu->nzflag=0;
      }
      else {
        cpu->zflag=0;
        cpu->nzflag=1;
      }
       cpu->ins_completed++;
    }

    if(strcmp(stage->opcode, "AND")== 0) {
      cpu->regs[stage->rd]=stage->buffer;
      cpu->regs_valid[stage->rd]=1;
       cpu->ins_completed++;
    }

    if(strcmp(stage->opcode, "OR")== 0) {
      cpu->regs[stage->rd]=stage->buffer;
      cpu->regs_valid[stage->rd]=1;
       cpu->ins_completed++;
    }

    if (strcmp((cpu->f),"display")==0) {
      print_stage_content("Writeback", stage);
    }
    struct CPU_Stage Apex;
    cpu ->stage[WB]=Apex;
  }
    
  return 0;

}
  

/*
 *  APEX CPU simulation loop
 *
 *  Note : You are free to edit this function according to your
 *         implementation
 */
int
APEX_cpu_run(APEX_CPU* cpu)
{
  
  while (1) {
  
    /* All the instructions committed, so exit */
    if (cpu->ins_completed == cpu->code_memory_size || cpu->clock==cpu->cycle || cpu->halt >=4) {
      printf("%d cycle\n",cpu->cycle);
      printf("%d clock\n",cpu->clock);
      printf("%d\n",cpu->ins_completed);
      printf("%d\n",cpu->code_memory_size);
      printf("(apex) >> Simulation Complete");
      break;
    }

    if (strcmp((cpu->f),"display")==0) {
      printf("--------------------------------\n");
      printf("Clock Cycle #: %d\n", cpu->clock);
      printf("--------------------------------\n");
    }

    writeback(cpu);
    memory(cpu);
    execute(cpu);
    decode(cpu);
    fetch(cpu);
    cpu->clock++;
    }

printf(" =============== STATE OF ARCHITECTURAL REGISTER FILE ==========");
  for(int i=0;i<16;i++)
    {
    printf("\n  REGS[%d]     |      %d     |      Status=%s ",i,cpu->regs[i],(cpu->regs_valid[i]) ? "VALID" : "INVALID");
    }
  
   printf("\n============== STATE OF DATA MEMORY =============");
   for(int j=0;j<100;j++)
    {
    printf("\n MEM[%d]       |   %d        | ",j,cpu->data_memory[j]);

    }  
   return 0;

}
