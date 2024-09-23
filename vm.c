// FileName: vm.c
// Date Created: 1/29/23
// Authors: Carson Breitbart, Marco Diaz
// Description: This program implements a virtual machine (VM) known as the
// P-machine (PM/0).
#include <stdio.h>
#include <stdlib.h>
#define ARRAY_SIZE 500

// Struct that holds instructions
typedef struct {
  int OP, L, M;
} Instruction; // Instruction Register

// Global variables initialize PM/0 Registers
int SP = ARRAY_SIZE; // Stack pointer

int BP = ARRAY_SIZE - 1; // Base pointer

int count = 0;

int PC = 0; // Program counter

int pas[ARRAY_SIZE]; // pas array containing IR and stack

int EOP = 1; // End of Program flag

// Function prototypes
void print(char IROP[], int IRL, int IRM, int PC, int BP, int SP);
int base(int BP, int L);

// Main function
int main(int argc, char *argv[]) {

  // Opening file name provided by parameter
  FILE *fptr = fopen(argv[1], "r");

  // If file name is null or cannot be opened
  if (fptr == NULL) {
    printf("Error in opening file... \n");
    exit(0);
  }

  // Initializing the array (pas) with 0's
  for (int i = 0; i < ARRAY_SIZE;
       ++i) { // Initializing the array (pas) with 0's
    pas[i] = 0;
  }

  // Prints intital header
  printf("\t\t\t\t  PC   BP   SP   stack\n");
  printf("Initial values: %3d %6d %4d\n\n", PC, BP, SP);

  Instruction IR; // Creating struct variable IR

  while (EOP != 0) {
    count = 0;

    for (int count = 0; fscanf(fptr, "%d%d%d", &IR.OP, &IR.L, &IR.M) == 3;
         ++count) {

      if (count == (PC / 3)) {
        pas[PC] = IR.OP;
        pas[PC + 1] = IR.L;
        pas[PC + 2] = IR.M;
        PC = PC + 3;
        fseek(fptr, 0,
              SEEK_SET); // Setting file pointer back to beginning of the file
        break;
      }
    }

    // Switch case which executes operation based on IR.OP
    switch (IR.OP) {
    case 1: // LIT
      SP--;
      pas[SP] = IR.M;
      print("LIT", IR.L, IR.M, PC, BP, SP);
      break;
    case 2:           // RTN
      switch (IR.M) { // Switch case which reads the M value.
      case 0:
        SP = BP + 1;
        BP = pas[SP - 2];
        PC = pas[SP - 3];
        print("RTN", IR.L, IR.M, PC, BP, SP);
        printf("Output result is: %d", pas[SP]);
        break;
      case 1: // ADD
        pas[SP + 1] = pas[SP + 1] + pas[SP];
        SP++;
        print("ADD", IR.L, IR.M, PC, BP, SP);
        break;
      case 2: // SUB
        pas[SP + 1] = pas[SP + 1] - pas[SP];
        SP++;
        print("SUB", IR.L, IR.M, PC, BP, SP);
        break;
      case 3: // MUL
        pas[SP + 1] = pas[SP + 1] * pas[SP];
        SP++;
        print("MUL", IR.L, IR.M, PC, BP, SP);
        break;
      case 4: // DIV
        pas[SP + 1] = pas[SP + 1] / pas[SP];
        SP++;
        print("DIV", IR.L, IR.M, PC, BP, SP);
        break;
      case 5: // EQL
        pas[SP + 1] = pas[SP + 1] == pas[SP];
        SP++;
        print("EQL", IR.L, IR.M, PC, BP, SP);
        break;
      case 6: // NEQ
        pas[SP + 1] = pas[SP + 1] != pas[SP];
        SP++;
        print("NEQ", IR.L, IR.M, PC, BP, SP);
        break;
      case 7: // LSS
        pas[SP + 1] = pas[SP + 1] < pas[SP];
        SP++;
        print("LSS", IR.L, IR.M, PC, BP, SP);
        break;
      case 8: // LEQ
        pas[SP + 1] = pas[SP + 1] <= pas[SP];
        SP++;
        print("LEQ", IR.L, IR.M, PC, BP, SP);
        break;
      case 9: // GTR
        pas[SP + 1] = pas[SP + 1] > pas[SP];
        SP++;
        print("GTR", IR.L, IR.M, PC, BP, SP);
        break;
      case 10: // GEQ
        pas[SP + 1] = pas[SP + 1] >= pas[SP];
        SP++;
        print("GEQ", IR.L, IR.M, PC, BP, SP);
        break;
      case 11: // ODD
        pas[SP] = pas[SP] % 2;
        break;
      } // End of M value switch case
      break;
    case 3: // LOD
      SP -= 1;
      pas[SP] = pas[base(BP, IR.L) - IR.M];
      print("LOD", IR.L, IR.M, PC, BP, SP);
      break;
    case 4: // STO
      pas[base(BP, IR.L) - IR.M] = pas[SP];
      SP++;
      print("STO", IR.L, IR.M, PC, BP, SP);
      break;
    case 5:                         // CAL
      pas[SP - 1] = base(BP, IR.L); // Static Link (SL)
      pas[SP - 2] = BP;             // Dynamic Link (DL)
      pas[SP - 3] = PC;             // Return address (RA)
      BP = SP - 1;
      PC = IR.M;
      print("CAL", IR.L, IR.M, PC, BP, SP);
      break;
    case 6: // INC
      SP -= IR.M;
      print("INC", IR.L, IR.M, PC, BP, SP);
      break;
    case 7: // JMP
      PC = IR.M;
      print("JMP", IR.L, IR.M, PC, BP, SP);
      break;
    case 8: // JPC
      if (pas[SP] == 0)
        PC = IR.M;
      SP++;
      print("JPC", IR.L, IR.M, PC, BP, SP);
      break;
    case 9:
      if (IR.M == 1) { //SYS
        putc(pas[SP], stdout);
        SP++;
        print("SYS", IR.L, IR.M, PC, BP, SP);
      }
      if (IR.M == 2) { //SIN
        SP--;
        printf("Please Enter an Integer: ");
        scanf("%d", &pas[SP]);
        print("SIN", IR.L, IR.M, PC, BP, SP);
      }
      if (IR.M == 3) { //SOU
        EOP = 0;
        print("EOP", IR.L, IR.M, PC, BP, SP);
      }
      break;
    }
  }

  return 0;
}

// End of Main

// Print helper function
void print(char IROP[], int IRL, int IRM, int PC, int BP, int SP) {

  printf("\n\t%s %d    %-2d   %-2d   %-3d  %-2d", IROP, IRL, IRM, PC, BP, SP);

  // Stack + Activation Record print
  if (SP != 500) {

    printf(" ");

    for (int i = 499; i >= SP; i--) {
      printf(" %d", pas[i]);
      if (BP != 499 && SP < BP) {
        if (i == BP + 1)
          printf(" |");
      }
    }
  }

  printf("\n");
}

// Finds a variable in a different Activation Record some L levels down
int base(int BP, int L) {
  int arb = BP; // arb = activation record base
  while (L > 0) // find base L levels down
  {
    arb = pas[arb];
    L--;
  }
  return arb;
}