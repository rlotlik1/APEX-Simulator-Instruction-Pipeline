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
	memset(cpu->ex_forward,0,sizeof(int) * 32);
	memset(cpu->wb_forward,0,sizeof(int) * 32);
	memset(cpu->mem_forward,0,sizeof(int) * 32);

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
	if(strcmp(stage->opcode, "JUMP") == 0) {
		printf("%s,R%d,#%d",stage->opcode,stage->rs1,stage->imm);
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
	//printf("F : %d, %d, %d, %d\n",cpu->stage_check[0][0] ,cpu->stage_check[0][1],cpu->stage_set[0][0],cpu->stage_set[1][0] );
	if (!cpu->stage_check[0][0] && !cpu->stage_check[0][1] && cpu->stage_set[0][0] && !cpu->halt && strcmp((&cpu->stage[MEM])->opcode, "JUMP") != 0 &&
	 (strcmp((&cpu->stage[MEM])->opcode, "BZ") != 0 || !cpu->zflag) && ( strcmp((&cpu->stage[MEM])->opcode, "BNZ") != 0 || !cpu->nzflag)) {  
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
     // struct CPU_Stage Apex;
     // cpu->stage[F]=Apex;
	}

	else if(!cpu->stage_set[0][0] && cpu->stage_set[1][0]) {
		cpu->stage_set[0][0] = 1;
        cpu->stage[DRF]=cpu->stage[F];
		if (strcmp((cpu->f),"display")==0) {
			print_stage_content("Fetch", stage);
		}
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
	// cpu->stage_set[1][0] = 1;

		}	
	}
	if(strcmp(stage->opcode,"ADD")==0 || strcmp(stage->opcode,"SUB")==0 || strcmp(stage->opcode, "MUL")== 0) {
		int temp1 = 0;
		int temp2 = 0;
	//	printf("STORE PRE rs1 : %d, %d, %d\n", cpu->ex_forward[stage->rs1] , cpu->mem_forward[stage->rs1] , cpu->regs_valid[stage->rs1]);
		if(cpu->ex_forward[stage->rs1] == 1 || cpu->mem_forward[stage->rs1] == 1 || cpu->regs_valid[stage->rs1] == 1 )
		{
			temp1 = 1;
		}
	//	printf("STORE PRE rs2 : %d, %d, %d\n", cpu->ex_forward[stage->rs2] , cpu->mem_forward[stage->rs2] , cpu->regs_valid[stage->rs2]);
		if(cpu->ex_forward[stage->rs2] == 1 || cpu->mem_forward[stage->rs2] == 1 || cpu->regs_valid[stage->rs2] == 1  )
		{
			temp2 = 1;
		}   
		if (temp1 && temp2)
		{
			cpu->stage_check[1][1]=0;
  			//cpu ->stage_set[1][0]=1;
		} 
	}

	if(strcmp(stage->opcode,"AND")==0) {
		int temp1 = 0;
		int temp2 = 0;
		if(cpu->ex_forward[stage->rs1] == 1 || cpu->mem_forward[stage->rs1] == 1 || cpu->regs_valid[stage->rs1] == 1 )
		{
			temp1 = 1;
		}
	//	printf("STORE PRE rs2 : %d, %d, %d\n", cpu->ex_forward[stage->rs2] , cpu->mem_forward[stage->rs2] , cpu->regs_valid[stage->rs2]);
		if(cpu->ex_forward[stage->rs2] == 1 || cpu->mem_forward[stage->rs2] == 1 || cpu->regs_valid[stage->rs2] == 1  )
		{
			temp2 = 1;
		}   
		if (temp1 && temp2)
		{
			cpu->stage_check[1][1]=0;
  			//cpu ->stage_set[1][0]=1;
		} 
	}

	if(strcmp(stage->opcode,"OR")==0) {
		int temp1 = 0;
		int temp2 = 0;
		if(cpu->ex_forward[stage->rs1] == 1 || cpu->mem_forward[stage->rs1] == 1 || cpu->regs_valid[stage->rs1] == 1 )
		{
			temp1 = 1;
		}
	//	printf("STORE PRE rs2 : %d, %d, %d\n", cpu->ex_forward[stage->rs2] , cpu->mem_forward[stage->rs2] , cpu->regs_valid[stage->rs2]);
		if(cpu->ex_forward[stage->rs2] == 1 || cpu->mem_forward[stage->rs2] == 1 || cpu->regs_valid[stage->rs2] == 1  )
		{
			temp2 = 1;
		}   
		if (temp1 && temp2)
		{
			cpu->stage_check[1][1]=0;
  			//cpu ->stage_set[1][0]=1;
		} 
	}

	if(strcmp(stage->opcode,"XOR")==0) {
		int temp1 = 0;
		int temp2 = 0;
		if(cpu->ex_forward[stage->rs1] == 1 || cpu->mem_forward[stage->rs1] == 1 || cpu->regs_valid[stage->rs1] == 1 )
		{
			temp1 = 1;
		}
	//	printf("STORE PRE rs2 : %d, %d, %d\n", cpu->ex_forward[stage->rs2] , cpu->mem_forward[stage->rs2] , cpu->regs_valid[stage->rs2]);
		if(cpu->ex_forward[stage->rs2] == 1 || cpu->mem_forward[stage->rs2] == 1 || cpu->regs_valid[stage->rs2] == 1  )
		{
			temp2 = 1;
		}   
		if (temp1 && temp2)
		{
			cpu->stage_check[1][1]=0;
  			//cpu ->stage_set[1][0]=1;
		} 
	}

	if(strcmp(stage->opcode,"LDR")== 0) {
		if(cpu->regs_valid[stage->rs1] && (cpu->regs_valid[stage->rs2]))
		{
			cpu->stage_check[1][1]=0;
		} 
	}

	if(strcmp(stage->opcode,"STORE")== 0) {
		int temp1 = 0;
		int temp2 = 0;
	//	printf("STORE PRE rs1 : %d, %d, %d\n", cpu->ex_forward[stage->rs1] , cpu->mem_forward[stage->rs1] , cpu->regs_valid[stage->rs1]);
		if(cpu->ex_forward[stage->rs1] == 1 || cpu->mem_forward[stage->rs1] == 1 || cpu->regs_valid[stage->rs1] == 1 )
		{
			temp1 = 1;
		}
	//	printf("STORE PRE rs2 : %d, %d, %d\n", cpu->ex_forward[stage->rs2] , cpu->mem_forward[stage->rs2] , cpu->regs_valid[stage->rs2]);
		if(cpu->ex_forward[stage->rs2] == 1 || cpu->mem_forward[stage->rs2] == 1 || cpu->regs_valid[stage->rs2] == 1  )
		{
			temp2 = 1;
		}   
		if (temp1 && temp2)
		{
			cpu->stage_check[1][1]=0;
  			//cpu ->stage_set[1][0]=1;
		} 
	}

	if(strcmp(stage->opcode,"LOAD")==0) {
		if(cpu->regs_valid[stage->rs1])
		{
			cpu->stage_check[1][1]=0;
		}
	}

	if(strcmp(stage->opcode,"JUMP")==0) {
		if(cpu->regs_valid[stage->rs1])
		{
			cpu->stage_check[1][1]=0;
		}
	}

	//printf("DRF : %d, %d, %d, %d\n",cpu->stage_check[1][0] ,cpu->stage_check[1][1] , cpu ->stage_set[1][0] ,cpu->halt );
	if(!cpu->stage_check[1][0] && !cpu->stage_check[1][1]  && !cpu->halt) {
		
    /* Read data from register file for store */
		if (strcmp(stage->opcode, "STORE") == 0) {

			if(cpu->ex_forward[stage->rs1] == 1 && strcmp((&cpu->stage[MEM])->opcode, "LOAD") != 0)
			{
	//			printf("1\n");
				stage->rs1_value = cpu->ex_data[stage->rs1];
			}
			else if(cpu->mem_forward[stage->rs1] == 1)
			{
	//			printf("2\n");
				stage->rs1_value = cpu->mem_data[stage->rs1];
			}
			else if(strcmp((&cpu->stage[MEM])->opcode, "LOAD") != 0 && cpu->regs_valid[stage->rs1] == 1)
			{
	//			printf("3\n");
				stage->rs1_value = cpu->regs[stage->rs1];
			}
			else {
	//			printf("4\n");
				cpu->stage_check[1][1]=1;
				cpu ->stage_set[1][0]=0;
			}

			if(cpu->ex_forward[stage->rs2] == 1 && strcmp((&cpu->stage[MEM])->opcode, "LOAD") != 0)
			{
	//			printf("5\n");
				stage->rs2_value = cpu->ex_data[stage->rs2];
			}
			else if(cpu->mem_forward[stage->rs2] == 1)
			{
	//			printf("6\n");
				stage->rs2_value = cpu->mem_data[stage->rs2];
			}
			else if(strcmp((&cpu->stage[MEM])->opcode, "LOAD") != 0 && cpu->regs_valid[stage->rs2] == 1)
			{
	//			printf("7\n");
				stage->rs2_value = cpu->regs[stage->rs2];
			}
			else{
	//			printf("8\n");
				cpu->stage_check[1][1]=1;
				cpu ->stage_set[1][0]=0;
			}
	//		printf("DRF STORE : %d, %d",cpu->stage_check[1][1],cpu ->stage_set[1][0]);
		}
		if (strcmp(stage->opcode, "LOAD") == 0) {
   // cpu->regs_valid[stage->rd]= 0; //busy

    if(cpu->ex_forward[stage->rs1] == 1)
    {
    	stage->rs1_value = cpu->ex_data[stage->rs1];
    }
    else if(cpu->mem_forward[stage->rs1] == 1)
    {
    	stage->rs1_value = cpu->mem_data[stage->rs1];
    }
    else if(cpu->regs_valid[stage->rs1] == 1)
    {
    	stage->rs1_value = cpu->regs[stage->rs1];
    }

    else {
    	cpu->stage_check[1][1]=1;
    	cpu->stage_set[1][0]=0;
    }  
    if (!cpu->stage_check[1][1])
    {
    	cpu->regs_valid[stage->rd]=0;
    }
}

if (strcmp(stage->opcode, "LDR") == 0) {
	cpu->regs_valid[stage->rd]= 0;
	if(cpu->ex_forward[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->ex_data[stage->rs1];
	}
	else if(cpu->mem_forward[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->mem_data[stage->rs1];
	}
	else if(cpu->regs_valid[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->regs[stage->rs1];
	}

	if(cpu->ex_forward[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->ex_data[stage->rs2];
	}
	else if(cpu->mem_forward[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->mem_data[stage->rs2];
	}
	else if(cpu->regs_valid[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->regs[stage->rs2];
	}

	else {
		cpu->stage_check[1][1]=1;
		cpu->stage_set[1][0]=0;
	}  
}



     /* No Register file read needed for MOVC */
if (strcmp(stage->opcode, "MOVC") == 0) {
	cpu->regs_valid[stage->rd] = 0;



}

if(strcmp(stage->opcode,"BNZ")==0 || strcmp(stage->opcode,"BZ")==0)
{
	if(strcmp((&cpu->stage[MEM])->opcode,"ADD")==0 || strcmp((&cpu->stage[MEM])->opcode,"SUB")==0 || strcmp((&cpu->stage[MEM])->opcode,"MUL")==0 || strcmp((&cpu->stage[WB])->opcode,"ADD")==0 || strcmp((&cpu->stage[WB])->opcode,"SUB")==0 || strcmp((&cpu->stage[WB])->opcode,"MUL")==0)
	{
		cpu->stage_check[1][1]=1;
		cpu->stage_set[1][0] = 0;

	}	
}

if(strcmp(stage->opcode,"XOR") == 0) {
	//cpu->regs_valid[stage->rd]=0;

	if(cpu->ex_forward[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->ex_data[stage->rs1];
	}
	else if(cpu->mem_forward[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->mem_data[stage->rs1];
	}
	else if(cpu->regs_valid[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->regs[stage->rs1];
	}

	if(cpu->ex_forward[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->ex_data[stage->rs2];
	}
	else if(cpu->mem_forward[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->mem_data[stage->rs2];
	}
	else if(cpu->regs_valid[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->regs[stage->rs2];
	}

	else{
		cpu->stage_check[1][1]=1;
		cpu ->stage_set[1][0]=0;

	}
	if (!cpu->stage_check[1][1])
    {
    	cpu->regs_valid[stage->rd]=0;
    }


}



if (strcmp(stage->opcode, "AND") == 0) {
	//cpu->regs_valid[stage->rd]=0;
	if(cpu->ex_forward[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->ex_data[stage->rs1];
	}
	else if(cpu->mem_forward[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->mem_data[stage->rs1];
	}
	else if(cpu->regs_valid[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->regs[stage->rs1];
	}

	if(cpu->ex_forward[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->ex_data[stage->rs2];
	}
	else if(cpu->mem_forward[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->mem_data[stage->rs2];
	}
	else if(cpu->regs_valid[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->regs[stage->rs2];
	}

	else{
		cpu->stage_check[1][1]=1;
		cpu ->stage_set[1][0]=0;

	}
	if (!cpu->stage_check[1][1])
    {
    	cpu->regs_valid[stage->rd]=0;
    }
}

if(strcmp(stage->opcode,"HALT")==0)
{
	cpu->halt=1;
}

if(strcmp(stage->opcode, "JUMP")==0)
{
	if(cpu->ex_forward[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->ex_data[stage->rs1];
	}
	else if(cpu->mem_forward[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->mem_data[stage->rs1];
	}

	else if(cpu->regs_valid[stage->rs1])
	{
		stage->rs1_value=cpu->regs[stage->rs1];
	}
	else{
		cpu->stage_check[1][1]=1;
		cpu -> stage_set[1][0]=0;

	}
	if (!cpu->stage_check[1][1])
    {
    	cpu->regs_valid[stage->rd]=0;
    }
}
if (strcmp(stage->opcode, "OR") == 0) {
	//cpu->regs_valid[stage->rd]=0;
	if(cpu->ex_forward[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->ex_data[stage->rs1];
	}
	else if(cpu->mem_forward[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->mem_data[stage->rs1];
	}
	else if(cpu->regs_valid[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->regs[stage->rs1];
	}

	if(cpu->ex_forward[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->ex_data[stage->rs2];
	}
	else if(cpu->mem_forward[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->mem_data[stage->rs2];
	}
	else if(cpu->regs_valid[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->regs[stage->rs2];
	}


	else{
		cpu->stage_check[1][1]=1;
		cpu -> stage_set[1][0]=0;
	}
	if (!cpu->stage_check[1][1])
    {
    	cpu->regs_valid[stage->rd]=0;
    }
}


if (strcmp(stage->opcode, "SUB") == 0 || strcmp(stage->opcode, "ADD") == 0 || strcmp(stage->opcode, "MUL") == 0) {
	

	if(cpu->ex_forward[stage->rs1] == 1 && strcmp((&cpu->stage[MEM])->opcode, "LOAD") != 0)
	{
		stage->rs1_value = cpu->ex_data[stage->rs1];
	}
	else if(cpu->mem_forward[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->mem_data[stage->rs1];
	}
	else if(strcmp((&cpu->stage[MEM])->opcode, "LOAD") != 0 && cpu->regs_valid[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->regs[stage->rs1];
	}
	else{
		cpu->stage_check[1][1]=1;
		cpu -> stage_set[1][0]=0;
	}

	if(cpu->ex_forward[stage->rs2] == 1 && strcmp((&cpu->stage[MEM])->opcode, "LOAD") != 0)
	{
		stage->rs2_value = cpu->ex_data[stage->rs2];
	}
	else if(cpu->mem_forward[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->mem_data[stage->rs2];
	}
	else if(strcmp((&cpu->stage[MEM])->opcode, "LOAD") != 0 && cpu->regs_valid[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->regs[stage->rs2];
	}
	else{
		cpu->stage_check[1][1]=1;
		cpu -> stage_set[1][0]=0;
	}
    if (!cpu->stage_check[1][1])
    {
    	cpu->regs_valid[stage->rd]=0;
    }

}

if (strcmp(stage->opcode, "XOR") == 0) {
	cpu->regs_valid[stage->rd]=0;
	if(cpu->ex_forward[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->ex_data[stage->rs1];
	}
	else if(cpu->mem_forward[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->mem_data[stage->rs1];
	}
	else if(cpu->regs_valid[stage->rs1] == 1)
	{
		stage->rs1_value = cpu->regs[stage->rs1];
	}

	if(cpu->ex_forward[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->ex_data[stage->rs2];
	}
	else if(cpu->mem_forward[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->mem_data[stage->rs2];
	}
	else if(cpu->regs_valid[stage->rs2] == 1)
	{
		stage->rs2_value = cpu->regs[stage->rs2];
	}

	else{
		cpu->stage_check[1][1]=1;
		cpu -> stage_set[1][0]=0;
	}
	if (!cpu->stage_check[1][1])
    {
    	cpu->regs_valid[stage->rd]=0;
    }
}




if (strcmp(stage->opcode, "LDR") == 0) {
	//cpu->regs_valid[stage->rd]=0;
	if((cpu->regs_valid[stage->rs1]) && (cpu->regs_valid[stage->rs2])) {
		stage->rs1_value=cpu->regs[stage->rs1];
		stage->rs2_value=cpu->regs[stage->rs2];  
	}

	else if(cpu->regs_forward[stage->rs1] && (cpu->regs_valid[stage->rs2]))
	{
		stage->rs1_value=cpu->regs_data[stage->rs1];
		stage->rs2_value=cpu->regs[stage->rs2];

	}

	else if(cpu->regs_valid[stage->rs1] && (cpu->regs_forward[stage->rs2]))
	{
		stage->rs1_value=cpu->regs[stage->rs1];
		stage->rs2_value=cpu->regs_data[stage->rs2];
	}

	else if(cpu->regs_valid[stage->rs2] && (cpu->regs_forward[stage->rs1]))
	{
		stage->rs2_value=cpu->regs[stage->rs2];
		stage->rs1_value=cpu->regs_data[stage->rs1];
	}

	else if(cpu->regs_forward[stage->rs1] && (cpu->regs_forward[stage->rs2]))
	{
		stage->rs1_value=cpu->regs_data[stage->rs1];
		stage->rs2_value=cpu->regs_data[stage->rs2];
	}

	else{
		cpu->stage_check[1][1]=1;
		cpu -> stage_set[1][0]=0;
	}
	if (!cpu->stage_check[1][1])
    {
    	cpu->regs_valid[stage->rd]=0;
    }
}

    /* Copy data from decode latch to execute latch*/
     //printf("DRF EX check : %d,%d\n",cpu->stage_check[1][1] , cpu->stage_set[2][0] );
if(!cpu->stage_check[1][1] && cpu->stage_set[2][0]) {
	cpu->stage_set[1][0]=1;
	cpu->stage[EX] = cpu->stage[DRF];
}  

    /*else if(!(&cpu->stage[EX])->busy){
    struct CPU_Stage Bubble;
    cpu->stage[EX]=Bubble;
    }*/
if (strcmp((cpu->f),"display")==0) {
	print_stage_content("Decode/RF", stage);
}
     //struct CPU_Stage Apex;
      //cpu->stage[DRF]=Apex;
}
else if(!cpu->stage_set[2][0] && !cpu->stage_set[1][0] ) {
	if(strcmp((cpu->f),"display")==0) {
		print_stage_content("Decode/RF", stage);
	}
	//cpu->stage_set[1][0]=1;
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
			cpu->ex_forward[stage->mem_address]=1;
			(cpu->ex_data[stage->mem_address])=stage->buffer;


		}
		if (strcmp(stage->opcode, "LDR") == 0) {
			stage->mem_address=(stage->rs1_value)+(stage->rs2_value);

			cpu->ex_forward[stage->mem_address]=1;
			(cpu->ex_data[stage->mem_address])=stage->buffer;

		}

		if(strcmp(stage->opcode, "JUMP") == 0)
		{
			cpu->pc=(stage->rs1_value)+(stage->imm);
			//cpu->ins_completed = (cpu->pc - 4000) /4;
			strcpy(cpu->stage[F].opcode,"NOP");
			strcpy(cpu->stage[DRF].opcode,"NOP");
			cpu->halt=0;
			//cpu->ex_forward[stage->rd]=1;

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
					//cpu->ins_completed = (cpu->pc - 4000) /4;
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
					//cpu->ins_completed = (cpu->pc - 4000) /4;
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
			(cpu->regs_data[stage->rd])=stage->buffer;
			cpu->ex_forward[stage->rd]=1;
			(cpu->ex_data[stage->rd])=stage->buffer;

		}

		if (strcmp(stage->opcode, "MUL") == 0) {
   //	stage->buffer=(stage->rs1_value)+(stage->rs2_value);
			cpu->stage_check[2][0]=1;
			cpu->stage_check[1][1]=1;
			cpu ->stage_set[1][0] = 0;
    //cpu->stage_check[0][1]=1;
		}
		if (strcmp(stage->opcode, "ADD") == 0) {
			stage->buffer=(stage->rs1_value)+(stage->rs2_value);
			(cpu->regs_forward[stage->rd])=1;
			(cpu->regs_data[stage->rd])=stage->buffer;
			cpu->ex_forward[stage->rd]=1;
			(cpu->ex_data[stage->rd])=stage->buffer;
		}

		if (strcmp(stage->opcode, "XOR") == 0) {
			stage->buffer=(stage->rs1_value)^(stage->rs2_value);
			(cpu->regs_forward[stage->rd])=1;
			(cpu->regs_data[stage->rd])=stage->buffer;
			cpu->ex_forward[stage->rd]=1;
			(cpu->ex_data[stage->rd])=stage->buffer;
		}

		if(strcmp(stage->opcode, "SUB") == 0) {
			stage->buffer=(stage->rs1_value)-(stage->rs2_value);
			(cpu->regs_forward[stage->rd])=1;
			(cpu->regs_data[stage->rd])=stage->buffer;
			cpu->ex_forward[stage->rd]=1;
			(cpu->ex_data[stage->rd])=stage->buffer;
		}

		if (strcmp(stage->opcode, "AND") == 0) {
			stage->buffer=(stage->rs1_value)&(stage->rs2_value);
			(cpu->regs_forward[stage->rd])=1;
			(cpu->regs_data[stage->rd])=stage->buffer;
			cpu->ex_forward[stage->rd]=1;
			(cpu->ex_data[stage->rd])=stage->buffer;


		}

		if (strcmp(stage->opcode, "OR") == 0) {
			stage->buffer=(stage->rs1_value)|(stage->rs2_value);
			(cpu->regs_forward[stage->rd])=1;
			(cpu->regs_data[stage->rd])=stage->buffer;
			cpu->ex_forward[stage->rd]=1;
			(cpu->ex_data[stage->rd])=stage->buffer;


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
	cpu->stage_check[2][0]=0;
	cpu->stage_set[2][0]  = 1;
	cpu->stage_check[1][1]=0;
    //cpu->stage_check[0][1]=0;
	cpu ->stage_set[1][0] = 1;
	stage->buffer=(stage->rs1_value)*(stage->rs2_value);
	cpu->ex_forward[stage->rd] = 1;
	cpu->ex_data[stage->rd] = stage->buffer;
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
		//cpu->ins_completed = (cpu->pc - 4000) /4;
	}	
	cpu->stage[MEM] = cpu->stage[EX];

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
		//cpu->ins_completed = (cpu->pc - 4000) /4;
	}	cpu->stage[MEM] = cpu->stage[EX];

	if (strcmp((cpu->f),"display")==0) {
		print_stage_content("Execute", stage);
	} 
	struct CPU_Stage Bubble;
	cpu->stage[EX]=Bubble;

}  
else {
	if (strcmp((cpu->f),"display")==0) {
		print_stage_content("Execute", stage);
	}
} 
if(strcmp(stage->opcode, "HALT") == 0) {
	cpu->stage[MEM] = cpu->stage[EX];	
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
	for(int i=0;i<16;i++) {
		cpu->mem_data[i]=cpu->ex_data[i];
		cpu->mem_forward[i]=cpu->ex_forward[i];
		cpu->ex_forward[i]=0;
	}
	CPU_Stage* stage = &cpu->stage[MEM];
	if (!cpu->stage_check[3][0] && !cpu->stage_check[3][1]) {
  /* Store */
		if (strcmp(stage->opcode, "STORE") == 0) {
			cpu->data_memory[stage->mem_address]=stage->rs1_value;  
		}
		if (strcmp(stage->opcode, "LDR") == 0) {
			stage->buffer=cpu->data_memory[stage->mem_address %4000];
			(cpu->mem_forward[stage->rd])=1;
			(cpu->mem_data[stage->rd])=stage->buffer;

		}
		if (strcmp(stage->opcode, "LOAD") == 0) {
			stage->buffer=cpu->data_memory[stage->mem_address %4000];
			(cpu->mem_forward[stage->rd])=1;
			(cpu->mem_data[stage->rd])=stage->buffer;

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
	for(int i = 0;i<16;i++){
		cpu->regs_valid[i]=1;
	}
	CPU_Stage* stage = &cpu->stage[WB];
	if (!cpu->stage_check[4][0] && !cpu->stage_check[4][1]) {
    /* Update register file */
		cpu->regs_valid[0]=1;
		if (strcmp(stage->opcode, "MOVC") == 0) {
			cpu->regs[stage->rd] = stage->buffer;
			cpu->regs_valid[stage->rd]=1;
			cpu->mem_forward[stage->rd] = 0;
			cpu->ins_completed = (stage->pc - 4000) /4;
		}
		if (strcmp(stage->opcode, "LOAD") == 0) {
			cpu->regs[stage->rd] = stage->buffer;
			cpu->regs_valid[stage->rd]=1;
			cpu->mem_forward[stage->rd] = 0;
			cpu->ins_completed = (stage->pc - 4000) /4;
		}
		if (strcmp(stage->opcode, "LDR") == 0) {
			cpu->regs[stage->rd] = stage->buffer;
			cpu->regs_valid[stage->rd]=1;
			cpu->mem_forward[stage->rd] = 0;
			cpu->ins_completed = (stage->pc - 4000) /4;
		}

		if(strcmp(stage->opcode,"HALT")==0)
		{
			cpu->halt++;
		}

		if (strcmp(stage->opcode, "STORE") == 0) {

			cpu->ins_completed = (stage->pc - 4000) /4;
		}
		if (strcmp(stage->opcode, "ADD") == 0 || strcmp(stage->opcode, "MUL") == 0) {
			cpu->regs[stage->rd]=stage->buffer;
			cpu->regs_valid[stage->rd]=1;
			cpu->wb_forward[stage->rd]=1;
			cpu->mem_forward[stage->rd] = 0;
			if((cpu->regs[stage->rd]) == 0)
			{
				cpu->zflag=1;
				cpu->nzflag=0;
			}
			else {
				cpu->zflag=0;
				cpu->nzflag=1;
			}
			cpu->ins_completed = (stage->pc - 4000) /4;
		}
		if (strcmp(stage->opcode, "XOR") == 0) {
			cpu->regs[stage->rd]=stage->buffer;
			cpu->regs_valid[stage->rd]=1;
			cpu->wb_forward[stage->rd]=1;
			cpu->mem_forward[stage->rd] = 0;
			cpu->ins_completed = (stage->pc - 4000) /4;
		}
		if(strcmp(stage->opcode, "SUB")== 0) {
			cpu->regs[stage->rd]=stage->buffer;
			cpu->regs_valid[stage->rd]=1;
			cpu->wb_forward[stage->rd]=1;
			cpu->mem_forward[stage->rd] = 0;
			if((cpu->regs[stage->rd]) == 0)
			{
				cpu->zflag=1;
				cpu->nzflag=0;
			}
			else {
				cpu->zflag=0;
				cpu->nzflag=1;
			}
			cpu->ins_completed = (stage->pc - 4000) /4;
		}

		if(strcmp(stage->opcode, "AND")== 0) {
			cpu->regs[stage->rd]=stage->buffer;
			cpu->regs_valid[stage->rd]=1;
			cpu->wb_forward[stage->rd]=1;
			cpu->mem_forward[stage->rd] = 0;
			cpu->ins_completed = (stage->pc - 4000) /4;
		}

		if(strcmp(stage->opcode, "MUL")== 0) {
			cpu->regs[stage->rd]=stage->buffer;
			cpu->mem_forward[stage->rd] = 0;
			cpu->ins_completed = (stage->pc - 4000) /4;
		}

		if(strcmp(stage->opcode, "OR")== 0) {
			cpu->regs[stage->rd]=stage->buffer;
			cpu->regs_valid[stage->rd]=1;
			cpu->wb_forward[stage->rd]=1;
			cpu->mem_forward[stage->rd] = 0;
			cpu->ins_completed = (stage->pc - 4000) /4;
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
   /* for (int i = 0; i < 16; ++i)
    {
    	cpu->regs_valid[i] = 1;
    }*/
	while (1) {
  //printf("INS completed %d  - %d  %d  %d %d\n",cpu->ins_completed ,cpu->code_memory_size ,cpu->clock, cpu->cycle , cpu->halt );
    /* All the instructions committed, so exit */
		if (cpu->ins_completed == cpu->code_memory_size || cpu->clock==cpu->cycle || cpu->halt >=4) {
			printf("%d",cpu->code_memory_size);
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
	for(int j=0;j<99;j++)
	{
		printf("\n MEM[%d]       |   %d        | ",j,cpu->data_memory[j]);

	}

	return 0;

}
