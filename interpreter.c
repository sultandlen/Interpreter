#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_IDENT_LENGTH  30

FILE* fp;
int currentLine = 1;
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
  printf("ERR! Line %d:  %s\n", currentLine, message);
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
          raiseError("Comment cannot be terminated!");
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
  for (int i = 0; i < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); i++) {
    if (strcmp(KEYWORDS[i], str) == 0) {
      return true;
    }
  }
  return false;
}

char isOperator (char ch) {
  if (ch == '+' || ch == '-') { return ch; }
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
    unsigned long long value = 0;
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
    sprintf(token.lexeme, "%llu", value);
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
  if (line[1].type != KEYWORD || line[2].type != IDENTIFIER || line[3].type != NO_TYPE) {
    raiseError("Invalid variable initialization");
  }

  Variable variable;
  strcpy(variable.name, line[2].lexeme);
  variable.value = calloc(1, sizeof(char));
  variable.value = "";

  if (strcmp(line[1].lexeme, "int") == 0) {
    variable.type = INT;
  } else if (strcmp(line[1].lexeme, "text") == 0) {
    variable.type = TEXT;
  } else {
    char errMessage[50];
    sprintf(errMessage, "Unrecognized type: %s!", line[1].lexeme);
    raiseError(errMessage);
  }
  variables[variablesSize++] = variable;
}

Variable* getVariable(char *name) {
  for (int i = 0; i < variablesSize; i++) {
    if (strcmp(variables[i].name, name) == 0) {
      return &variables[i];
    }
  }
  char errMessage[50];
  sprintf(errMessage, "Variable not found: %s!", name);
  raiseError(errMessage);
}

void parseOutput(Token *line) {
  if (line[1].type != IDENTIFIER || line[2].type != NO_TYPE) {
    raiseError("Invalid output statement!");
  }
  Variable variable = *getVariable(line[1].lexeme);
  printf("%s\n", variable.value);
}

void parseInput(Token *line) {
  if (line[1].type != IDENTIFIER || line[3].type != IDENTIFIER || line[4].type != NO_TYPE) {
    raiseError("Invalid input!");
  }
  if (line[2].type != KEYWORD && strcmp(line[2].lexeme, "prompt") != 0) {
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
  if (line[1].type != IDENTIFIER || line[3].type != IDENTIFIER || line[4].type != NO_TYPE) {
    raiseError("Invalid read!");
  }
  if (line[2].type != KEYWORD && strcmp(line[2].lexeme, "from") != 0) {
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
  if (line[1].type != IDENTIFIER || line[3].type != IDENTIFIER || line[4].type != NO_TYPE) {
    raiseError("Invalid write!");
  }
  if (line[2].type != KEYWORD && strcmp(line[2].lexeme, "to") != 0) {
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

int sizeFunc(const char *string) {
  int i = 0;
  while (string[i] != '\0') {
    i++;
  }
  return i;
}

char* subsFunc(const char *string, int start, int end) {
  char *substring = calloc(end - start + 1, sizeof(char));
  int j = 0;
  for (int i = start; i < end; i++) {
    substring[j++] = string[i];
  }
  return substring;
}

int locateFunc(const char* bigText, const char* smallText, int start) {
  int bigLen = (int) strlen(bigText);
  int smallLen = (int) strlen(smallText);
  if (start < 0 || start >= bigLen) {
    return 0;
  }
  int i, j;
  for (i = start; i <= bigLen - smallLen; i++) {
    for (j = 0; j < smallLen; j++) {
      if (bigText[i + j] != smallText[j]) {
        break;
      }
    }
    if (j == smallLen) {
      return i;
    }
  }
  return 0;
}

char* insertFunc(char* myText, int location, const char* insertText) {
  int textLen = (int) strlen(myText);
  int insertLen = (int) strlen(insertText);
  if (location < 0 || location > textLen) { return myText; }
  int newLen = textLen + insertLen;
  char* newText = (char*)malloc((newLen + 1) * sizeof(char));
  if (newText == NULL) { return myText; }
  strncpy(newText, myText, location);
  strncpy(newText + location, insertText, insertLen);
  strncpy(newText + location + insertLen, myText + location, textLen - location);
  newText[newLen] = '\0';
  return newText;
}

char* overrideFunc(const char* myText, int location, const char* ovrText) {
  int textLen = (int) strlen(myText);
  int ovrLen = (int) strlen(ovrText);
  int newLen = location + ovrLen;
  if (newLen > textLen) { newLen = textLen; }
  char* newText = (char*)malloc((newLen + 1) * sizeof(char));
  if (newText == NULL) { return NULL; }
  strncpy(newText, myText, location);
  strncpy(newText + location, ovrText, newLen - location);
  newText[newLen] = '\0';
  return newText;
}

void parseFunctionAssignment(Token *line) {
  if(strcmp(line[2].lexeme, "size") == 0){
    if(line[4].type != IDENTIFIER || line[5].type != PARENTHESIS_CLOSE || line[6].type != NO_TYPE){
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
  } else if (strcmp(line[2].lexeme, "asString") == 0){
    if(line[4].type != IDENTIFIER || line[5].type != PARENTHESIS_CLOSE || line[6].type != NO_TYPE){
      raiseError("Invalid function assignment!");
    }
    Variable *variable = getVariable(line[4].lexeme);
    if(variable->type != INT){
      raiseError("Invalid function assignment!");
    }
    int number = (int) strtol(variable->value, NULL, 10);
    char *string = calloc(10, sizeof(char));
    sprintf(string, "%d", number);
    Variable *variable2 = getVariable(line[0].lexeme);
    if(variable2->type != TEXT){
      raiseError("Invalid function assignment!");
    }
    variable2->value = calloc(strlen(string) + 1, sizeof(char));
    strcpy(variable2->value, string);
  } else if (strcmp(line[2].lexeme, "asText") == 0){
    if(line[4].type != IDENTIFIER || line[5].type != PARENTHESIS_CLOSE || line[6].type != NO_TYPE){
      raiseError("Invalid function assignment!");
    }
    Variable *variable = getVariable(line[4].lexeme);
    if(variable->type != TEXT){
      raiseError("Invalid function assignment!");
    }
    int number = (int) strtol(variable->value, NULL, 10);
    char *string = calloc(10, sizeof(char));
    sprintf(string, "%d", number);
    Variable *variable2 = getVariable(line[0].lexeme);
    if(variable2->type != INT){
      raiseError("Invalid function assignment!");
    }
    variable2->value = calloc(strlen(string) + 1, sizeof(char));
    strcpy(variable2->value, string);
  } else if (strcmp(line[2].lexeme, "insert") == 0) {
    if(line[4].type != IDENTIFIER || line[5].type != COMMA || line[6].type != INT_CONST || line[7].type != COMMA || line[8].type != IDENTIFIER || line[9].type != PARENTHESIS_CLOSE || line[10].type != NO_TYPE){
      raiseError("Invalid function assignment!");
    }
    Variable *variable = getVariable(line[4].lexeme);
    if(variable->type != TEXT){
      raiseError("Invalid function assignment!");
    }
    char *myText = variable->value;
    int position = (int) strtol(line[6].lexeme, NULL, 10);
    Variable *variable2 = getVariable(line[8].lexeme);
    if(variable2->type != TEXT){
      raiseError("Invalid function assignment!");
    }
    char *insertText = variable2->value;
    char *newText = insertFunc(myText, position, insertText);
    Variable *variable3 = getVariable(line[0].lexeme);
    if(variable3->type != TEXT){
      raiseError("Invalid function assignment!");
    }
    variable3->value = calloc(strlen(newText) + 1, sizeof(char));
    strcpy(variable3->value, newText);
  } else if (strcmp(line[2].lexeme, "override") == 0) {
    if(line[4].type != IDENTIFIER || line[5].type != COMMA || line[6].type != INT_CONST || line[7].type != COMMA || line[8].type != IDENTIFIER|| line[9].type != PARENTHESIS_CLOSE || line[10].type != NO_TYPE){
      raiseError("Invalid function assignment!");
    }
    Variable *variable = getVariable(line[4].lexeme);
    if(variable->type != TEXT){
      raiseError("Invalid function assignment!");
    }
    char *myText = variable->value;
    int position = (int) strtol(line[6].lexeme, NULL, 10);
    Variable *variable2 = getVariable(line[8].lexeme);
    if(variable2->type != TEXT){
      raiseError("Invalid function assignment!");
    }
    char *overText = variable2->value;
    char *newText = overrideFunc(myText, position, overText);
    Variable *variable3 = getVariable(line[0].lexeme);
    if(variable3->type != TEXT){
      raiseError("Invalid function assignment!");
    }
    variable3->value = calloc(strlen(newText) + 1, sizeof(char));
    strcpy(variable3->value, newText);
  } else {
    raiseError("Invalid function assignment!");
  }
}

void parseArithmeticAssignment(Token *line) {
  if (line[0].type != IDENTIFIER || line[3].type != OPERATOR || line[5].type != NO_TYPE) {
    raiseError("Invalid arithmetic assignment!");
  }
  Variable *variable1 = getVariable(line[0].lexeme);
  if(variable1->type == INT) {
    int value1;
    int value2;
    if (line[2].type != INT_CONST && line[2].type != IDENTIFIER) {
      raiseError("Invalid  arithmetic assignment!");
    }
    if (line[4].type != INT_CONST && line[4].type != IDENTIFIER) {
      raiseError("Invalid arithmetic assignment!");
    }
    if (line[2].type == INT_CONST) {
      value1 = strtol(line[2].lexeme, NULL, 10);
    } else {
      Variable *variable2 = getVariable(line[2].lexeme);
      if (variable2->type != INT) {
        raiseError("Invalid arithmetic assignment!");
      }
      value1 = strtol(variable2->value, NULL, 10);
    }
    if (line[4].type == INT_CONST) {
      value2 = strtol(line[4].lexeme, NULL, 10);
    } else {
      Variable *variable2 = getVariable(line[4].lexeme);
      if (variable2->type != INT) {
        raiseError("Invalid arithmetic assignment!");
      }
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
      raiseError("Invalid arithmetic assignment!");
    }
  }

  if(variable1->type == TEXT) {
    char *value1;
    char *value2;
    if (line[2].type != STR_CONST && line[2].type != IDENTIFIER) {
      raiseError("Invalid arithmetic assignment!");
    }
    if (line[4].type != STR_CONST && line[4].type != IDENTIFIER) {
      raiseError("Invalid arithmetic assignment!");
    }
    if (line[2].type == STR_CONST) {
      value1 = line[2].lexeme;
    } else {
      Variable *variable2 = getVariable(line[2].lexeme);
      if (variable2->type != TEXT) {
        raiseError("Invalid arithmetic assignment!");
      }
      value1 = variable2->value;
    }
    if (line[4].type == STR_CONST) {
      value2 = line[4].lexeme;
    } else {
      Variable *variable2 = getVariable(line[4].lexeme);
      if (variable2->type != TEXT) {
        raiseError("Invalid arithmetic assignment!");
      }
      value2 = variable2->value;
    }
    if (strcmp(line[3].lexeme, "+") == 0) {
      variable1->value = calloc(strlen(value1) + strlen(value2) + 1, sizeof(char));
      strcpy(variable1->value, value1);
      strcat(variable1->value, value2);
    } else if (strcmp(line[3].lexeme, "-") == 0) {
      if (strlen(value1) < strlen(value2)) {
        raiseError("The subtrahend cannot be longer than the minuend!");
      }
      size_t resultLength = strlen(value1) - strlen(value2) + 1;
      variable1->value = calloc(resultLength, sizeof(char));
      char* found = strstr(value1, value2);
      if (found != NULL) {
        strncpy(variable1->value, value1, found - value1);
        strcat(variable1->value, found + strlen(value2));
      } else {
        strcpy(variable1->value, value1);
      }
    } else {
      raiseError("Invalid arithmetic assignment!");
    }
  }
}

void parseLine(Token *line) {
  //DECLARATION
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "new") == 0) {
    return parseDeclaration(line);
  }
  //COMMAND OUTPUT
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "output") == 0) {
    return parseOutput(line);
  }
  //COMMAND INPUT
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "input") == 0) {
    return parseInput(line);
  }
  //COMMAND READ
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "read") == 0) {
    return parseRead(line);
  }
  //COMMAND WRITE
  if (line[0].type == KEYWORD && strcmp(line[0].lexeme, "write") == 0) {
    return parseWrite(line);
  }
  //ASSIGNMENT
  if (line[1].type == OPERATOR && strcmp(line[1].lexeme, "=") == 0) {
    if(line[3].type == NO_TYPE) {
      return parseAssignment(line);
    } else if(line[2].type == KEYWORD && line[3].type == PARENTHESIS_OPEN){
      return parseFunctionAssignment(line);
    } else if(line[3].type == OPERATOR && line[5].type == NO_TYPE) {
      return parseArithmeticAssignment(line);
    } else {
      raiseError("Invalid assignment!");
    }
  }
  raiseError("Parsing error!");
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
      currentLine++;
    }
    c = (char) fgetc(fp);
  }
}