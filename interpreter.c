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

  char errMessage[26];
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
