MOVC,R0,#0
MOVC,R1,#1
MOVC,R2,#2
MOVC,R3,#3
ADD,R4,R0,R3
SUB,R5,R3,R4
MUL,R6,R4,R4
STORE,R6,R0,#4
JUMP,R6,#4031
MOVC,R8,#32
LOAD,R11,R5,#4
STORE,R11,R0,#8
MUL,R9,R1,R2
MUL,R11,R5,R6
MUL,R11,R9,R9
ADD,R8,R11,R3
LOAD,R11,R5,#4
ADD,R12,R11,R8
HALT,
ADD,R13,R13,R13