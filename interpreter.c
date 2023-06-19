#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_IDENT_LENGTH  30

FILE* fp;
typedef enum {
  IDENTIFIER,
  INT_CONST,
  OPERATOR,
  STR_CONST,
  KEYWORD,
  ENDOFLINE,
  NO_TYPE,
  ENDOFFILE,
  PARENTHESIS_OPEN,
  PARENTHESIS_CLOSE,
  COMMA
} TokenType;

typedef enum {
  INT,
  TEXT
} DataType;

typedef struct {
  TokenType type;
  char* lexeme;
} Token;

typedef struct {
  char name[MAX_IDENT_LENGTH];
  char* value;
  DataType type;
} Variable;

Variable* variables;
size_t variablesSize = 0;

void raiseError(char* message) {
  printf("Lexical ERR! %s\n", message);
  exit(1);
}

char skipWhitespace(char ch) {
  while (isspace(ch)) {
    ch = (char) fgetc(fp);
  }
  return ch;
}

char skipComment(char ch) {
  if (ch == '/') {
    ch = (char) fgetc(fp);
    char nextc;
    if (ch == '*') {
      do {
        if (nextc == EOF) {
          raiseError("Comment cannot terminated!");
        }
        ch = nextc;
        nextc = (char) fgetc(fp);
      } while (!(ch == '*' && nextc == '/'));
      return (char) fgetc(fp);
    } else {
      raiseError("Unrecognized character: '/'");
    }
  }
  return ch;
}

bool isKeyword (char str[]) {
  const char* KEYWORDS[] = {"new", "int", "text", "size", "subs", "locate", "insert", "override", "read", "write",
                           "from", "to", "input", "output", "asText", "asString"};
  for (int i = 0; i < sizeof(KEYWORDS) /sizeof(KEYWORDS[0]); i++) {
    if (strcmp(KEYWORDS[i], str) == 0) {
      return true;
    }
  }
  return false;
}

char isOperator (char ch) {
  if (ch == '+' || ch == '-') {
    return ch;
  }
  if (ch == ':') {
    char nextCh = (char) fgetc(fp);
    if (nextCh == '=') {
      return '=';
    }
    raiseError("Invalid operator, assignment operator must be used like ':='");
  }
  return '\0';
}


Token getNextToken() {
  Token token;
  token.lexeme = calloc(MAX_IDENT_LENGTH, sizeof(char));
  char ch = (char) fgetc(fp);


  //SKIP WHITESPACE and COMMENT
  while(isspace(ch) || ch == '/') {
    ch = skipComment(ch);
    ch = skipWhitespace(ch);
    if (ch == EOF) {
      token.type = ENDOFFILE;
      token.lexeme[0] = '\0';
      return token;
    }
  }

  //IDENTIFIER
  if (isalpha(ch)) { // Starts with letter
    int j = 0;
    while ((isalnum(ch) || ch == '_')) {
      token.lexeme[j++] = ch;
      if(j > MAX_IDENT_LENGTH) {
        char errMessage[56];
        sprintf(errMessage, "Identifiers must be smaller or equal than %d characters!", MAX_IDENT_LENGTH);
        raiseError(errMessage);
      }
      ch = (char) fgetc(fp);
    }
    ungetc(ch, fp);
    token.lexeme[j] = '\0'; //null terminator, marks the end of a string
    token.type = IDENTIFIER;
    if(isKeyword(token.lexeme)){
      token.type = KEYWORD;
    }
    return token;
  }

  //INTEGER
  if (isdigit(ch)) {
    unsigned long value = 0;
    while (isdigit(ch)) {
      value = value * 10 + (ch - '0');
      if(value > 4294967295 ) {
        raiseError("Integer value is too big!");
      }
      ch = (char) fgetc(fp);
    }
    if (isalpha(ch) || ch == '_') {
      raiseError("Invalid identifier, identifiers cannot start with a number!");
    }
    ungetc(ch,fp);
    sprintf(token.lexeme, "%lu", value);
    token.type = INT_CONST;
    return token;
  }

  //OPERATOR
  char operator = isOperator(ch);
  if (operator != '\0') {
    token.type = OPERATOR;
    token.lexeme[0] = operator;
    token.lexeme[1] = '\0';
    return token;
  }

  //PARENTHESIS_OPEN
  if (ch == '(') {
    token.type = PARENTHESIS_OPEN;
    strcpy(token.lexeme, "(");
    return token;
  }

  //PARENTHESIS_CLOSE
  if (ch == ')') {
    token.type = PARENTHESIS_CLOSE;
    strcpy(token.lexeme, ")");
    return token;
  }

  //COMMA
  if (ch == ',') {
    token.type = COMMA;
    strcpy(token.lexeme, ",");
    return token;
  }

  //STRING CONSTANT
  if (ch == '"') {
    int j = 0;
    ch = (char) fgetc(fp);
    while (ch != '"') {
      if (ch == EOF) {
        raiseError("String cannot terminated!");
      }
      token.lexeme[j++] = ch;
      ch = (char) fgetc(fp);
    }
    token.lexeme[j] = '\0';
    token.type = STR_CONST;
    return token;
  }

  //ENDOFLINE
  if (ch == ';') {
    token.type = ENDOFLINE;
    strcpy(token.lexeme, "");
    return token;
  }

  char errMessage[50];
  sprintf(errMessage, "Unrecognized character: '%c'!", ch);
  raiseError(errMessage);
}

void parseDeclaration(Token *line) {
  if (line[1].type != KEYWORD) {
    raiseError("Invalid declaration!");
  }
  if (line[2].type != IDENTIFIER) {
    raiseError("Invalid declaration!");
  }
  if (line[3].type != NO_TYPE) {
    raiseError("Invalid declaration!");
  }
  Variable variable;
  strcpy(variable.name, line[2].lexeme);
  variable.value = calloc(1, sizeof(char));
  variable.value = "test";

  if (strcmp(line[1].lexeme, "int") == 0) {
    variable.type = INT;
    variables[variablesSize++] = variable;
  } else if (strcmp(line[1].lexeme, "text") == 0) {
    variable.type = TEXT;
    variables[variablesSize++] = variable;
  } else {
    raiseError("Invalid declaration!");
  }
}

Variable* getVariable(char *name) {
  for (int i = 0; i < variablesSize; i++) {
    if (strcmp(variables[i].name, name) == 0) {
      return &variables[i];
    }
  }
  raiseError("Variable not found!");
}

void parseOutput(Token *line) {
  if (line[1].type != IDENTIFIER) {
    raiseError("Invalid output!");
  }
  if (line[2].type != NO_TYPE) {
    raiseError("Invalid output!");
  }
  Variable variable = *getVariable(line[1].lexeme);
  printf("%s\n", variable.value);
}

void parseInput(Token *line) {
  if (line[1].type != IDENTIFIER) {
    raiseError("Invalid input!");
  }
  if (line[2].type != KEYWORD && strcmp(line[2].lexeme, "prompt") != 0) {
    raiseError("Invalid input!");
  }
  if (line[3].type != IDENTIFIER) {
    raiseError("Invalid input!");
  }
  if (line[4].type != NO_TYPE) {
    raiseError("Invalid input!");
  }
  Variable prompt = *getVariable(line[3].lexeme);
  printf("%s: ", prompt.value);
  Variable* variable = getVariable(line[1].lexeme);
  char buffer[100];
  fgets(buffer, 100, stdin);
  buffer[strcspn(buffer, "\n")] = 0;
  variable->value = calloc(strlen(buffer) + 1, sizeof(char));
  strcpy(variable->value, buffer);
}

void parseRead(Token *line) {
  if (line[1].type != IDENTIFIER) {
    raiseError("Invalid read!");
  }
  if (line[2].type != KEYWORD && strcmp(line[2].lexeme, "from") != 0) {
    raiseError("Invalid read!");
  }
  if (line[3].type != IDENTIFIER) {
    raiseError("Invalid read!");
  }
  if (line[4].type != NO_TYPE) {
    raiseError("Invalid read!");
  }
  Variable* variable = getVariable(line[1].lexeme);
  char *fileName = strcat(line[3].lexeme, ".txt");
  FILE *fp = fopen(fileName, "r");
  if (fp == NULL) {
    raiseError("File not found!");
  }
  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char *string = calloc(fsize + 1, sizeof(char));
  fread(string, fsize, 1, fp);
  fclose(fp);
  variable->value = string;
}

void parseWrite(Token *line) {
  if (line[1].type != IDENTIFIER) {
    raiseError("Invalid write!");
  }
  if (line[2].type != KEYWORD && strcmp(line[2].lexeme, "to") != 0) {
    raiseError("Invalid write!");
  }
  if (line[3].type != IDENTIFIER) {
    raiseError("Invalid write!");
  }
  if (line[4].type != NO_TYPE) {
    raiseError("Invalid write!");
  }
  Variable* variable = getVariable(line[1].lexeme);
  char *fileName = strcat(line[3].lexeme, ".txt");
  FILE *fp = fopen(fileName, "w");
  if (fp == NULL) {
    raiseError("File not found!");
  }
  fprintf(fp, "%s", variable->value);
  fclose(fp);
}

void parseAssignment(Token *line) {
  if (line[0].type != IDENTIFIER) {
    raiseError("Invalid assignment!");
  }
  Variable *variable = getVariable(line[0].lexeme);
  if (line[2].type == INT_CONST){
    if (variable->type != INT) {
      raiseError("Invalid assignment!");
    }
    variable->value = calloc(strlen(line[2].lexeme) + 1, sizeof(char));
    strcpy(variable->value, line[2].lexeme);
  } else if (line[2].type == STR_CONST){
    if (variable->type != TEXT) {
      raiseError("Invalid assignment!");
    }
    variable->value = calloc(strlen(line[2].lexeme) + 1, sizeof(char));
    strcpy(variable->value, line[2].lexeme);
  } else if (line[2].type == IDENTIFIER) {
    Variable *variable2 = getVariable(line[2].lexeme);
    if (variable->type != variable2->type) {
      raiseError("Invalid assignment!");
    }
    variable->value = calloc(strlen(variable2->value) + 1, sizeof(char));
    strcpy(variable->value, variable2->value);
  } else {
    raiseError("Invalid assignment!");
  }
}

int sizeFunc(char *string) {
  int i = 0;
  while (string[i] != '\0') {
    i++;
  }
  return i;
}

char* subsFunc(char *string, int start, int end) {
  char *substring = calloc(end - start + 1, sizeof(char));
  int j = 0;
  for (int i = start; i < end; i++) {
    substring[j++] = string[i];
  }
  return substring;
}

int locateFunc(const char* bigText, const char* smallText, int start) {
  int bigLen = strlen(bigText);
  int smallLen = strlen(smallText);
  if (start < 0 || start >= bigLen) {
    return 0;  // Invalid start position
  }
  int i, j;
  for (i = start; i <= bigLen - smallLen; i++) {
    for (j = 0; j < smallLen; j++) {
      if (bigText[i + j] != smallText[j]) {
        break;  // Mismatch, move to the next position in bigText
      }
    }
    if (j == smallLen) {
      return i;  // Found smallText at position i
    }
  }
  return 0;  // smallText not found
}

void parseFunctionAssignment(Token *line) {

  if(strcmp(line[2].lexeme, "size") == 0){
    if(line[4].type != IDENTIFIER){
      raiseError("Invalid function assignment!");
    }
    if(line[5].type != PARENTHESIS_CLOSE){
      raiseError("Invalid function assignment!");
    }
    if(line[6].type != NO_TYPE){
      raiseError("Invalid function assignment!");
    }
    Variable *variable = getVariable(line[4].lexeme);
    if(variable->type != TEXT){
      raiseError("Invalid function assignment!");
    }
    char *string = variable->value;
    char *size = calloc(10, sizeof(char));
    sprintf(size, "%d", sizeFunc(string));
    Variable *variable2 = getVariable(line[0].lexeme);
    if(variable2->type != INT){
      raiseError("Invalid function assignment!");
    }
    variable2->value = calloc(strlen(size) + 1, sizeof(char));
    strcpy(variable2->value, size);

  } else if (strcmp(line[2].lexeme, "subs") == 0) {
    if(line[4].type != IDENTIFIER || line[5].type != COMMA || line[6].type != INT_CONST || line[7].type != COMMA || line[8].type != INT_CONST || line[9].type != PARENTHESIS_CLOSE || line[10].type != NO_TYPE){
      raiseError("Invalid function assignment!");
    }
    Variable *variable = getVariable(line[4].lexeme);
    if(variable->type != TEXT){
      raiseError("Invalid function assignment!");
    }
    char *string = variable->value;
    int start = (int) strtol(line[6].lexeme, NULL, 10);
    int end = (int) strtol(line[8].lexeme, NULL, 10);
    char *substring = subsFunc(string, start, end);
    Variable *variable2 = getVariable(line[0].lexeme);
    if(variable2->type != TEXT){
      raiseError("Invalid function assignment!");
    }
    variable2->value = calloc(strlen(substring) + 1, sizeof(char));
    strcpy(variable2->value, substring);
  } else if (strcmp(line[2].lexeme, "locate") == 0) {
    if(line[4].type != IDENTIFIER || line[5].type != COMMA || line[6].type != IDENTIFIER || line[7].type != COMMA || line[8].type != INT_CONST || line[9].type != PARENTHESIS_CLOSE || line[10].type != NO_TYPE){
      raiseError("Invalid function assignment!");
    }
    Variable *variable = getVariable(line[4].lexeme);
    if(variable->type != TEXT){
      raiseError("Invalid function assignment!");
    }
    char *bigText = variable->value;
    Variable *variable2 = getVariable(line[6].lexeme);
    if(variable2->type != TEXT){
      raiseError("Invalid function assignment!");
    }
    char *smallText = variable2->value;
    int start = (int) strtol(line[8].lexeme, NULL, 10);
    int location = locateFunc(bigText, smallText, start);
    Variable *variable3 = getVariable(line[0].lexeme);
    if(variable3->type != INT){
      raiseError("Invalid function assignment!");
    }
    variable3->value = calloc(10, sizeof(char));
    sprintf(variable3->value, "%d", location);
  }
}

void parseArithmeticAssignment(Token *line) {
  if (line[0].type != IDENTIFIER) {
    raiseError("Invalid assignment!");
  }
  if (line[3].type != OPERATOR) {
    raiseError("Invalid assignment!");
  }
  if (line[5].type != NO_TYPE) {
    raiseError("Invalid assignment!");
  }
  Variable *variable1 = getVariable(line[0].lexeme);
  int value1;
  int value2;
  if(variable1->type == INT) {
    if (line[2].type != INT_CONST && line[2].type != IDENTIFIER) {
      raiseError("Invalid assignment!");
    }
    if (line[4].type != INT_CONST && line[4].type != IDENTIFIER) {
      raiseError("Invalid assignment!");
    }
    if (line[2].type == INT_CONST) {
      value1 = strtol(line[2].lexeme, NULL, 10);
    } else {
      Variable *variable2 = getVariable(line[2].lexeme);
      value1 = strtol(variable2->value, NULL, 10);
    }
    if (line[4].type == INT_CONST) {
      value2 = strtol(line[4].lexeme, NULL, 10);
    } else {
      Variable *variable2 = getVariable(line[4].lexeme);
      value2 = strtol(variable2->value, NULL, 10);
    }
    if (strcmp(line[3].lexeme, "+") == 0) {
      variable1->value = calloc(10, sizeof(char));
      sprintf(variable1->value, "%d", value1 + value2);
    } else if (strcmp(line[3].lexeme, "-") == 0) {
      variable1->value = calloc(10, sizeof(char));
      sprintf(variable1->value, "%d", value1 - value2);
      if (value1 - value2 < 0) {
        raiseError("The answer cannot be negative!");
      }
    } else {
      raiseError("Invalid assignment!");
    }
  }

}

void parseLine(Token *line) {
  // declaration
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "new") == 0) {
    parseDeclaration(line);
  }
  //COMMAND OUTPUT
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "output") == 0) {
    parseOutput(line);
  }
  //COMMAND INPUT
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "input") == 0) {
    parseInput(line);
  }
  //COMMAND READ
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "read") == 0) {
    parseRead(line);
  }
  //COMMAND WRITE
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "write") == 0) {
    parseWrite(line);
  }

  //ASSIGNMENT
  if (line[1].type == OPERATOR && strcmp(line[1].lexeme, "=") == 0) {
    if(line[3].type == NO_TYPE) {
      parseAssignment(line);
    } else if(line[2].type == KEYWORD && line[3].type == PARENTHESIS_OPEN){
      parseFunctionAssignment(line);
    } else if(line[3].type == OPERATOR && line[5].type == NO_TYPE) {
      parseArithmeticAssignment(line);
    } else {
      raiseError("Invalid assignment!");
    }
  }
}

int main(int argc, char *argv[]) {
  variables = calloc(10, sizeof(Variable));
  char* file = "myprog.tj";
  if(argc > 1) {
      file = argv[1];
  }

  fp = fopen(file, "r");

  if(fp == NULL) {
    printf("Cannot open file: %s\n", file);
    return 1;
  }

  Token token;
  char c = (char) fgetc(fp);
  Token* line = calloc(10, sizeof(Token));
  int i = 0;
  while (c != EOF){
    ungetc(c, fp);
    token = getNextToken();
    if (token.type != ENDOFLINE && token.type != ENDOFFILE) {
      line[i++] = token;
    } else if (token.type == ENDOFLINE) {
      line[i].type = NO_TYPE;
      parseLine(line);
      line = calloc(10, sizeof(Token));
      i = 0;
    }
    c = (char) fgetc(fp);
  }
}
