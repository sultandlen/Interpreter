#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

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
  ENDOFFILE
} TokenType;

typedef struct {
  TokenType type;
  char lexeme[32];
} Token;

void raiseError(char* message) {
  printf("Lexical ERR! %s\n", message);
  exit(1);
}

char skipWhitespace(char ch) {
  while (isspace(ch)) {
    ch = fgetc(fp);
  }
  return ch;
}

char skipComment(char ch) {
  if (ch == '/') {
    ch = fgetc(fp);
    char nextc;
    if (ch == '*') {
      do {
        if (nextc == EOF) {
          raiseError("Comment cannot terminated!");
        }
        ch = nextc;
        nextc = fgetc(fp);
      } while (!(ch == '*' && nextc == '/'));
      return fgetc(fp);
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

bool isOperator (char ch) {
  if ('+' == ch || '-' == ch) {
    return true;
  }
  if (':' == ch) {
    char nextCh = fgetc(fp);
    if ('=' == nextCh) {
      return true;
    }
    raiseError("Invalid operator, assignment operator must be used like ':='");
  }
  return false;
}


Token getNextToken() {
  Token token;
  char ch = fgetc(fp);


  //SKIP WHITESPACE and COMMENT
  while(isspace(ch) || ch == '/') {
    ch = skipComment(ch);
    ch = skipWhitespace(ch);
    if (ch == EOF) {
      token.type = ENDOFFILE;
      token.lexeme[0] = '\0';
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
      ch = fgetc(fp);
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


  //OPERATOR


  //STRING


  //ENDOFLINE
  if (ch == ';') {
    token.type = ENDOFLINE;
    strcpy(token.lexeme, "");
    return token;
  }


  return token;
}

int main(int argc, char *argv[]) {
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
  char c = fgetc(fp);
  while (c != EOF){
    ungetc(c, fp);
    token = getNextToken();
    printf("%s ", token.lexeme);
    printf("%d\n", token.type);
    c = fgetc(fp);
  }
}
