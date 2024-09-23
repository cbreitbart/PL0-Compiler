// FileName: hw4compiler.c
// Date Created: 4/13/23
// Authors: Carson Breitbart, Marco Diaz
// Description: The compiler reads a program written in PL/0 and generates code for the Virtual Machine (VM) implemented in HW1. 

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOL_TABLE_SIZE 500
#define NUM_RESERVERD_WORDS 13
#define MAX_WORD_LENGTH 10
#define MAX_IDENTIFIER_LENGTH 11
#define MAX_CODE_LENGTH 1000
#define MAX_DIGIT_IN_NUM 5
#define MAX_LEVEL 5

// Global tokens
typedef enum {
  skipsym = 1,
  identsym = 2,
  numbersym = 3,
  plussym = 4,
  minussym = 5,
  multsym = 6,
  slashsym = 7,
  oddsym = 8,
  eqsym = 9,
  neqsym = 10,
  lessym = 11,
  leqsym = 12,
  gtrsym = 13,
  geqsym = 14,
  lparentsym = 15,
  rparentsym = 16,
  commasym = 17,
  semicolonsym = 18,
  periodsym = 19,
  becomessym = 20,
  beginsym = 21,
  endsym = 22,
  ifsym = 23,
  thensym = 24,
  whilesym = 25,
  dosym = 26,
  callsym = 27,
  constsym = 28,
  varsym = 29,
  procsym = 30,
  writesym = 31,
  readsym = 32,
  // elsesym = 33
} token_type;

typedef struct { // Recomended data structure for the symbol
  int kind;      // const = 1, var = 2, proc = 3
  char name[10]; // name up to 11 chars
  int val;       // number (ASCII value)
  int level;     // L level
  int addr;      // M address
  int mark;      // to indicate unavailable or deleted
} symbol;

typedef struct {
  char *op;
  int l, m;
} instruction; // Instruction Register

// Function Prototypes
token_type get_token(FILE *fptr);

void error(char *message);
bool is_whitespace(char ch);
int is_reserved_word(char *str);
int add_symbol_to_table(int kind, char name[], int val, int level, int addr);
int SYMBOLTABLECHECK(char *name);
token_type reserved_word_token(char *str);
void CONST_DECLARATION(FILE *fptr);
int VAR_DECLARATION(FILE *fptr);
void CONDITION(FILE *fptr);
void STATEMENT(FILE *fptr);
void EXPRESSION(FILE *fptr);
void TERM(FILE *fptr);
void FACTOR(FILE *fptr);
void PROGRAM(FILE *fptr);
void BLOCK(FILE *fptr);
void read_file(FILE *fptr);
void print_table();
void emit(char *op, int l, int m);
void PROC_DECLARATION(FILE *fptr);

// Global Variables
symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];
int num_symbols = 0;
int token = 0;
char token_name[3000];
char reserved_words[NUM_RESERVERD_WORDS][MAX_WORD_LENGTH] = {
    "const", "var",  "procedure", "call", "begin", "end",   "if",
    "then", "while",     "do",   "read",  "write", "odd"};
int lexeme_count = 0;
int length_count = 0;
int digit_count = 0;
int comment_countL = 0;
int comment_countR = 0;
int level = 0;
int numVars = 0;
int count = 0;
FILE *fptr2;
instruction code[MAX_CODE_LENGTH];
int code_index = 0; // code index

// Main function
int main(int argc, char **argv) {

  if (argc < 1) { // If wrong number of arguments input
    printf("Error. Invalid Command Line Arguments.\n");
    printf("Format is: ./a.out <input filename>\n");
    return 1;
  }

  FILE *fptr1 = fopen(argv[1], "r");
  fptr2 = fopen("elf.txt", "w");

  // If file name is null or cannot be opened
  if (fptr2 == NULL) {
    printf("Error in opening file... \n");
    exit(0);
  }

  // Reads in the file
   read_file(fptr1);

  // Resets the file pointer
  fseek(fptr1, 0, SEEK_SET);

  // Hard coding in the first line of the assembly code
  symbol_table[num_symbols].kind = 3;
  strcpy(symbol_table[num_symbols].name, "main");
  symbol_table[num_symbols].val = 0;
  symbol_table[num_symbols].level = 0;
  symbol_table[num_symbols].addr = 3;
  num_symbols++;

  

  // Calls the "whole program" wrapper function
  PROGRAM(fptr1);
  // Prints the symbol table
  // print_table();

  // Close files
  fclose(fptr1);
  

  printf("No errors, program is syntactically correct.\n\n");
  
  printf("Assembly Code:\n\n");
  printf("Line\t OP\t L \tM\n");
  
  for(int i = 0; i<code_index; i++){ //Printing assembly code
    char *op = code[i].op;
    int l = code[i].l;
    int m = code[i].m;
      
    if(strcmp(op, "JMP") == 0 || strcmp(op, "JPC") == 0 ){ //Multiplying jumps by 3
      m *= 3;
    }
    printf("%d\t\t%s  %d  %d\n", i, op, l, m); //Printing assembly code
    
    if (strcmp(op, "LIT") == 0) // Printing instructions to elf.txt
    fprintf(fptr2, "1 %d %d\n", l, m);
  else if (strcmp(op, "OPR") == 0)
    fprintf(fptr2, "2 %d %d\n", l, m);
  else if (strcmp(op, "LOD") == 0)
    fprintf(fptr2, "3 %d %d\n", l, m);
  else if (strcmp(op, "STO") == 0)
    fprintf(fptr2, "4 %d %d\n", l, m);
  else if (strcmp(op, "CAL") == 0)
    fprintf(fptr2, "5 %d %d\n", l, m);
  else if (strcmp(op, "INC") == 0)
    fprintf(fptr2, "6 %d %d\n", l, m);
  else if (strcmp(op, "JMP") == 0)
    fprintf(fptr2, "7 %d %d\n", l, m);
  else if (strcmp(op, "JPC") == 0)
    fprintf(fptr2, "8 %d %d\n", l, m);
  else if (strcmp(op, "SYS") == 0)
    fprintf(fptr2, "9 %d %d\n", l, m);
  }
  
  fclose(fptr2);
  return 0;
} // End of Main function

// Start of functions

// Reads in a file
void read_file(FILE *fptr) {
  char ch;
  printf("Source program:\n");
  // Printing what is written in file
  // character by character using loop.
  do {
    ch = fgetc(fptr);
    if (ch != EOF)
      printf("%c", ch);
    // Checking if character is not EOF.
    // If it is EOF stop reading.
  } while (ch != EOF);
  printf("\n\n");
}

void print_table() {
  printf("\nKind | Name        | Value | Level | Address | Mark\n");
  printf("---------------------------------------------------\n");
  for (int i = 0; i < num_symbols; i++) {

    printf("%4d | %11s | %5d | %5d | %7d | %5d\n", symbol_table[i].kind,
           symbol_table[i].name, symbol_table[i].val, symbol_table[i].level,
           symbol_table[i].addr, symbol_table[i].mark);
  }
}

// Adds a symbol to the table
int add_symbol_to_table(int kind, char name[], int val, int level, int addr) {
  // If the symbol table is full, output an error
  if (num_symbols >= MAX_SYMBOL_TABLE_SIZE) {
    error("Too many symbols");
  }
  // Otherwise, fill the index of the symbol table
  symbol_table[num_symbols].kind = kind;
  strcpy(symbol_table[num_symbols].name, name);
  symbol_table[num_symbols].val = val;
  symbol_table[num_symbols].level = level;
  symbol_table[num_symbols].addr = addr;
  num_symbols++;
  return num_symbols - 1; // returns index of new symbol
}

// Prints the assembly code
void emit(char *op, int l, int m) {

  // If the program is too long, output an error
  if (code_index >= MAX_CODE_LENGTH) {
    printf("Error: program too long\n");
    exit(1);
  }
  // Otherwise, print out the op, l and m of the assembly code
  code[code_index].op = op;
  code[code_index].l = l;
  code[code_index].m = m;
  code_index++;

  // printf("%d\t\t%s  %d  %d\n", count, op, l, m);

  // If statement to send assembly code to elf.txt
  
  count++;
}

//"Whole program" wrapper function
void PROGRAM(FILE *fptr) {
  int level;
  // Get the next token
  get_token(fptr);
  // Call the block function
  BLOCK(fptr);
  // If the token is not periodsym (i.e. end of program), output an error
  if (token != periodsym) {
    error("Expected period symbol to terminate the program.");
  }
  // Otherwise, print out end for assembly code
  emit("SYS", 0, 3);
}

// Symbol table checker
int SYMBOLTABLECHECK(char *name) {
  int i;
  // For each symbol in the symbol table...
  for (i = num_symbols - 1; i >= 0; i--) {
    // If the symbol is in the symbol table, return its index
    if (strcmp(symbol_table[i].name, name) == 0 && symbol_table[i].mark == 0) {
      return i;
    }
  }
  // Otherwise, return -1
  return -1;
}
int tx;
// Block function
void BLOCK(FILE *fptr) {
  int dx = 3;
  int tx0 = add_symbol_to_table(
      3, "", 0, level, 0); // add dummy symbol for the current procedure
  tx = tx0;
  int cx0 = code_index;

   symbol_table[tx].addr=code_index;

  emit("JMP", 0, 0); // generate jmp 0, 0, the second 0 tentative

  if (level > MAX_LEVEL) {
    error("Error: Nested too deep");
  }
  do {
    // Call const function
    CONST_DECLARATION(fptr);
    // Declare numVars to be the return of var function
    numVars = VAR_DECLARATION(fptr);
    //procedure declaration
    PROC_DECLARATION(fptr);
    
  } while (token == constsym || token == varsym || token == procsym);

  code[symbol_table[tx0].addr].m = code_index;                      // the tentative jump address is fixed up
  symbol_table[tx0].addr = code_index; // the space for address for the above
                                       // jmp is now occupied by the new cx
  cx0 = code_index;

  emit("INC", 0, 3 + numVars);
  // Call statement function
  STATEMENT(fptr);
  
  emit("OPR", 0, 0); // return from procedure
  
  for(int i = 0; i < num_symbols; i++){
    if(symbol_table[i].level == level){
      symbol_table[i].mark = 1;
    }
  }
  level--;
  
}

// Checks if symbol is a const
void CONST_DECLARATION(FILE *fptr) {
  int num;
  // Check if token is 'const'
  if (token == constsym) {
    do {
      // Get the next token
      get_token(fptr);
      // Check if token is an identifier
      if (token != identsym) {
        error("Identifier expected after 'const'");
      }
      // Check if identifier is already in the symbol table
      int duplicate = SYMBOLTABLECHECK(token_name);
      if ( duplicate != -1 && symbol_table[duplicate].level == level) {
        error("Duplicate identifier");
      }
      // Save the identifier name
      char ident_name[MAX_IDENTIFIER_LENGTH];
      strcpy(ident_name, token_name); // Saving the name to ident_name
      // Get the next token
      get_token(fptr);
      // Check if token is '='
      if (token != eqsym) {
        error("Expected '=' after identifier in 'const'");
      }
      // Get the next token
      get_token(fptr);
      // Check if token is a number
      // Check if token is a number
      if (token != numbersym) {
        error("Number expected after '=' in 'const'");
      }

      num = atoi(token_name); // convert string to integer and assign to num

      // Add the constant to the symbol table
      int index = add_symbol_to_table(1, ident_name, num, level, 0);
      // Get the next token
      get_token(fptr);
    } while (token == commasym); // Continue if token is ','
    // Check if token is ';'
    if (token != semicolonsym) {
      error("Expected ';' after 'const' declaration");
    }
    // Get the next token
    get_token(fptr);
  }
}

// Checks if symbol is a variable
int VAR_DECLARATION(FILE *fptr) {


  // Reset numVars to 0
  numVars = 0;
  // If token is a variable...
  if (token == varsym) {
    do {
      // Increment numVars by 1
      numVars++;
      // Get the next token
      get_token(fptr);
      // If the token isn't an identifier, output an error
      if (token != identsym) {
        error("Identifier expected after 'var'");
      }
      // If the token is a duplicate, output an error
      int duplicate = SYMBOLTABLECHECK(token_name);
      if ( duplicate != -1 && symbol_table[duplicate].level == level) {
        error("Duplicate identifier");
      }
      // Add the symbol to the table
      add_symbol_to_table(2, token_name, 0, level, numVars + 2);
      // Get the next token
      get_token(fptr);
    } while (token == commasym); // Do while the token is a comma

    // If the token isn't a semicolon, output an error
    if (token != semicolonsym) {
      error("Expected ';' after 'var' declaration");
    }
    // Get the next token
    get_token(fptr);
  }

  // Return the number of variables
  return numVars;
}

void PROC_DECLARATION(FILE *fptr){
  // Proc-declaration addition
    while (token == procsym) {
      get_token(fptr);
      if (token != identsym)
        error("Identifer expected after 'procedure'.");
      add_symbol_to_table(3, token_name, 0, level, code_index);
      get_token(fptr);
      if (token != semicolonsym)
        error("Expected ';' after proc declaration.");
      get_token(fptr);
      level++;
      BLOCK(fptr);
      if (token != semicolonsym)
        error("Expected ';' after proc declaration");
      get_token(fptr);
    }
}

// Checks if the symbol is a statement
void STATEMENT(FILE *fptr) {
  int symIdx, jpcIdx, loopIdx;
  
  // If the token is an identifier...
  if (token == identsym) {
    // Check if token is in the symbol table 
    symIdx = SYMBOLTABLECHECK(token_name);
    
    // If not, output an error
    if (symIdx == -1) {
      error("Undeclared identifier");
    }
    // If symIdx is assigned to a constant or procedure, output an error
    if (symbol_table[symIdx].kind != 2) {
      error("Assignment to a constant or procedure is not allowed");
    }
    // Get the next token
    get_token(fptr);
    // If the token isn't becomessym, output an error
    if (token != becomessym) {
      error("Expected ':='");
    }
    // Get the next token
    get_token(fptr);
    // Call expression function
    EXPRESSION(fptr);
    // Call emit function
    emit("STO", level - symbol_table[symIdx].level, symbol_table[symIdx].addr);
    return;
  }

  if (token == callsym) {
    get_token(fptr);
    if (token != identsym)
      error("Expected Identifier after 'call' declaration");
    else {
      symIdx = SYMBOLTABLECHECK(token_name);
      if (symIdx == 0)
        error("Expected symbol");
      else if (symbol_table[symIdx].kind == 3) // 3 = procedure
        emit("CAL", level - symbol_table[symIdx].level,
             symbol_table[symIdx].addr);
      else
        error("Expected symbol");
      get_token(fptr);
    }
  }

  // If the token is beginsym...
  if (token == beginsym) {
    do {
      // Get the next token
      get_token(fptr);
      // Call the statement function
      STATEMENT(fptr);
    } while (token == semicolonsym); // Do while the token is semicolonsym
    // If the token isn't an endsym, output an error
    if (token != endsym) {
      error("Expected 'end'");
    }
    // Get the next token
    get_token(fptr);
    return;
  }
  // If the token is ifsym...
  if (token == ifsym) {
    // Get the next token
    get_token(fptr);
    // Call the condition function
    CONDITION(fptr);
    // Set jpcIdx to the code_index
    jpcIdx = code_index;
    // Call the emit function
    emit("JPC", 0, 0);
    // If the token isn't thensym, output an error
    if (token != thensym) {
      error("Expected 'then'");
    }
    // Get the next token
    get_token(fptr);
    // Call the statement function
    STATEMENT(fptr);
    // Set the m of the instruction register to the code_index at index of
    // jpcIdx
    code[jpcIdx].m = code_index;
    return;
  }
  // If the token is whilesym...
  if (token == whilesym) {
    // Get the next token
    get_token(fptr);
    // Set loopIdx to code_index
    loopIdx = code_index;
    // Call the condition function
    CONDITION(fptr);
    // Set jpcIdx to code_index
    jpcIdx = code_index;
    // Call the emit function
    emit("JPC", 0, 0);
    // If token isn't dosym, output an error
    if (token != dosym) {
      error("Expected 'do'");
    }
    // Get the next token
    get_token(fptr);
    // Call the statement function
    STATEMENT(fptr);
    // Call the emit function
    emit("JMP", 0, loopIdx);
    // Set the m of the instruction register to the code_index at index of
    // jpcIdx
    code[jpcIdx].m = code_index;
    return;
  }
  // If the token is readsym...
  if (token == readsym) {
    // Get the next token
    get_token(fptr);
    // If token isn't an identifier, output an error
    if (token != identsym) {
      error("Expected identifier after 'read'");
    }
    // Check if symIdx is in the symbol table
    symIdx = SYMBOLTABLECHECK(token_name);
    // If not, output an error
    if (symIdx == -1) {
      error("Undeclared identifier");
    }
    // If symIdx is assigned to a constant or procedure, output an error
    if (symbol_table[symIdx].kind != 2) {
      error("Assignment to a constant or procedure is not allowed");
    }
    // Get the next token
    get_token(fptr);
    // Call the emit function twice
    emit("SYS", 0, 2);
    emit("STO", level - symbol_table[symIdx].level, symbol_table[symIdx].addr);
    return;
  }
  // If the token is writesym...
  if (token == writesym) {
    // Get the next token
    get_token(fptr);
    // Call the expression function
    EXPRESSION(fptr);
    // Call the emit function
    emit("SYS", 0, 1);
    return;
  }
}

// Checks if the token is a condition
void CONDITION(FILE *fptr) {
  // If token is oddsym...
  if (token == oddsym) {
    // Get the next token
    get_token(fptr);
    // Call the expression function
    EXPRESSION(fptr);
    // Call the emit funcion
    emit("OPR", 0, 11);
  } else {
    // Call the expression function
    EXPRESSION(fptr);
    // If token is eqsym:
    //  1. Get the next token
    //  2. Call the expression function
    //  3. Call the emit function with appropriate values
    //  Else, output an error
    if (token == eqsym) {
      // Get next token
      get_token(fptr);
      EXPRESSION(fptr);
      emit("OPR", 0, 5);
    } else if (token == neqsym) {
      get_token(fptr);
      EXPRESSION(fptr);
      emit("OPR", 0, 6);
    } else if (token == lessym) {
      get_token(fptr);
      EXPRESSION(fptr);
      emit("OPR", 0, 7);
    } else if (token == leqsym) {
      get_token(fptr);
      EXPRESSION(fptr);
      emit("OPR", 0, 8);
    } else if (token == gtrsym) {
      get_token(fptr);
      EXPRESSION(fptr);
      emit("OPR", 0, 9);
    } else if (token == geqsym) {
      get_token(fptr);
      EXPRESSION(fptr);
      emit("OPR", 0, 10);
    } else {
      error("Expected relational operator");
    }
  }
}

// Checks if the token is an expression
void EXPRESSION(FILE *fptr) {
  // If token is minussym...
  if (token == minussym) {
    // Get the next token
    get_token(fptr);
    // Call the term function
    TERM(fptr);
    // Call the emit function
    emit("NEG", 0, 0);
    // While the token is either plussym or minussym...
    while (token == plussym || token == minussym) {
      // If the token is plussym:
      // 1. Get the next token
      // 2. Call the term function
      // 3. Call the emit function
      // Else, do the above steps for minussym
      if (token == plussym) {
        get_token(fptr);
        TERM(fptr);
        emit("OPR", 0, 1); // ADD
      } else {
        get_token(fptr);
        TERM(fptr);
        emit("OPR", 0, 2); // SUB
      }
    }
    // Else if the token is plussym, do the same as above
  } else {
    if (token == plussym) {
      get_token(fptr);
    }
    TERM(fptr);
    while (token == plussym || token == minussym) {
      if (token == plussym) {
        get_token(fptr);
        TERM(fptr);
        emit("OPR", 0, 1); // ADD
      } else {
        get_token(fptr);
        TERM(fptr);
        emit("OPR", 0, 2); // SUB
      }
    }
  }
}

// Check if the token is a term
void TERM(FILE *fptr) {
  // Call the factor function
  FACTOR(fptr);
  // While the token is either multsym or slashsym...
  while (token == multsym || token == slashsym) {
    // If the token is multsym:
    // 1. Get the next token
    // 2. Call the factor function
    // 3. Call the emit function
    // Else, do the above steps for slashsym
    if (token == multsym) {
      get_token(fptr);
      FACTOR(fptr);
      emit("OPR", 0, 3);
    } else if (token == slashsym) {
      get_token(fptr);
      FACTOR(fptr);
      emit("OPR", 0, 4);
    }
  }
}

// Checks if the token is a factor
void FACTOR(FILE *fptr) {
  if (token == identsym) {
    // Check if symbol exists in symbol table
    int symIdx = SYMBOLTABLECHECK(token_name);
    if (symIdx == -1) {
      error("Undeclared identifier");
    }
    // Load value from symbol table onto stack
    if (symbol_table[symIdx].kind == 1) { // constant
      emit("LIT", 0, symbol_table[symIdx].val);
    } else { // variable
      emit("LOD", level - symbol_table[symIdx].level,
           symbol_table[symIdx].addr);
    }
    get_token(fptr);
  } else if (token == numbersym) {
    // Push number value onto stack
    int num = atoi(token_name);
    emit("LIT", 0, num);
    get_token(fptr);
  } else if (token == lparentsym) {
    get_token(fptr);
    EXPRESSION(fptr);
    if (token != rparentsym) {
      error("Expected closing parenthesis");
    }
    get_token(fptr);
  } else {
    error("Unexpected symbol");
  }
}

// Outputs an error
void error(char *message) {
  printf("Error: %s\n", message);
  exit(1);
}

// Checks if ch is a whitespace character
bool is_whitespace(char ch) {
  if (ch == ' ' || ch == '\t' || ch == '\n') // Skip reading in white spaces
    return true;
  else
    return false;
}

// Checks if str is a valid reserved word
token_type reserved_word_token(char *str) {
  // Compare the input string with each of the reserved words and return the
  // corresponding token type if there is a match.
  if (strcmp(str, "const") == 0) {
    token = constsym;
    return constsym;
  } else if (strcmp(str, "var") == 0) {
    token = varsym;
    return varsym;
  } else if (strcmp(str, "procedure") == 0) {
    token = procsym;
    return procsym;
  } else if (strcmp(str, "call") == 0) {
    token = callsym;
    return callsym;
  } else if (strcmp(str, "begin") == 0) {
    token = beginsym;
    return beginsym;
  } else if (strcmp(str, "end") == 0) {
    token = endsym;
    return endsym;
  } else if (strcmp(str, "if") == 0) {
    token = ifsym;
    return ifsym;
  } else if (strcmp(str, "then") == 0) {
    token = thensym;
    return thensym;
  } /* else if (strcmp(str, "else") == 0) {
  //   token = elsesym;
  //   return elsesym;
  }*/
  else if (strcmp(str, "while") == 0) {
    token = whilesym;
    return whilesym;
  } else if (strcmp(str, "do") == 0) {
    token = dosym;
    return dosym;
  } else if (strcmp(str, "read") == 0) {
    token = readsym;
    return readsym;
  } else if (strcmp(str, "write") == 0) {
    token = writesym;
    return writesym;
  } else if (strcmp(str, "odd") == 0) {
    token = oddsym;
    return oddsym;
  }
}

int is_reserved_word(char *str) { // Function to check if a word is a reserved
                                  // word in the reserved_words[][] array.
  int i;
 
  for (i = 0; i < NUM_RESERVERD_WORDS; i++) {
    
    if (strcmp(str, reserved_words[i]) == 0) {
       
      return 1;
    }
  }
  return 0;
}

// Gets the token from the file
token_type get_token(FILE *fptr) {
  
  for (int i = 0; i < 100; i++) { //Clearing name from array
          token_name[i] = 0;
      }
        lexeme_count = 0;
  
  char ch = fgetc(fptr);

  // Switch statement to handle special symbols
  switch (ch) {
  case '+':
    token = plussym;
    return plussym;
  case '-':
    token = minussym;
    return minussym;
  case '*':
    ch = fgetc(fptr);
    if (ch == '/') {
      comment_countR++; // increase comment count for right bracket
      break;
    } else {
      ungetc(ch, fptr); // Put the character back to the input buffer
      token = multsym;
      return multsym;
    }
  case '/':
    ch = fgetc(fptr);
    if (ch == '*') {
      comment_countL++; // increase comment count for left bracket
      break;
    } else {
      ungetc(ch, fptr);
      token = slashsym;
      return slashsym;
    }
  case '(':
    token = lparentsym;
    return lparentsym;
  case ')':
    token = rparentsym;
    return rparentsym;
  case '=':
    token = eqsym;
    return eqsym;
  case ',':
    token = commasym;
    return commasym;
  case ';':
    token = semicolonsym;
    return semicolonsym;
  case '.':
    token = periodsym;
    return periodsym;
  case ':':
    ch = fgetc(fptr);
    if (ch == '=') { // If ':='
      token = becomessym;
      return becomessym;
    } else { // If only ':'
      ungetc(ch, fptr);
    }
  case '<':
    ch = fgetc(fptr);
    if (ch == '=') { // If '<='
      token = leqsym;
      return leqsym;
    } else if (ch == '>') { // If '<>'
      token = neqsym;
      return neqsym;
    } else {
      ungetc(ch, fptr);
      token = lessym;
      return lessym; // If only '<'
    }
  case '>':
    ch = fgetc(fptr);
    if (ch == '=') { // If '>='
      token = geqsym;
      return geqsym;
    } else {
      ungetc(ch, fptr);
      token = gtrsym;
      return gtrsym; // If only '>'
    }
  default:
    if (isalpha(ch)) { // If the character is an alphabetic letter, read in the
                       // whole identifier/word
      char buffer[MAX_IDENTIFIER_LENGTH + 1];
      int index = 0;
      buffer[index++] = ch;
      while (isalnum(ch = fgetc(fptr))) { // Keep reading characters if they are
                                          // alphabets or digits
        buffer[index++] = ch;
      }
      buffer[index] = '\0'; // Add the null terminator to the end of the buffer
      ungetc(
          ch,
          fptr); // Put the non-alphanumeric character back to the input buffer
      
      if (is_reserved_word(buffer)) { // If the buffer is a reserved word,
                                      // return the corresponding token type
        return reserved_word_token(buffer);
      } else { // If the buffer is not a reserved word, it is an
               // identifier/word, so save it to the lexeme table
        
        for (int i = 0; i < index; i++) {
          token_name[lexeme_count++] = buffer[i];
        }

        if (index > MAX_IDENTIFIER_LENGTH)
          error("Identifier too long"); // Error if the identifier/word is too
                                        // long
        else {
          token = identsym;
          return identsym; // Return the token type for an identifier/word
        }
      }
    } else if (isdigit(ch)) {
      // If is a digit 0-9, add the first digit to the lexeme table
      token_name[lexeme_count++] = ch;
      digit_count++;
      
      while (isdigit(ch = fgetc(fptr))) {
      token_name[digit_count] = ch;
      digit_count++;
      }
      
      // Check if the next character after the digits is an alphabetic letter
      if (isalpha(ch))
        error(
            "Error: Identifier cannot begin with digit"); // Error if it is, as
                                                          // identifiers cannot
                                                          // begin with digits
      digit_count = 0;
      ungetc(ch, fptr);
      token = numbersym;
      return numbersym; // Return the token type for a number
    } else if (ch == EOF) {
      break; // Break out of the loop if we have reached the end of the file
    } else if (is_whitespace(ch)) {
      return get_token(
          fptr); // Skip whitespace characters and get the next symbol
    } else {
      error("Error: Invalid Symbol");
    }
  }
}