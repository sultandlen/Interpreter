#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

FILE* fp;

typedef enum {
  IDENTIFIER,
  INT_CONST,
  OPERATOR,
  STR_CONST,
  KEYWORD,
  ENDOFLINE,
  NO_TYPE
} TokenType;

typedef struct {
  TokenType type;
  char lexeme[];
} Token;

void raiseError(char* message) {
  printf("Lexical ERR! %s\n", message);
  exit(1);
}

void skipComment(char ch) {
  if (ch == '/') {
    char c = fgetc(fp);
    char nextc;
    if (c == '*') {
      do {
        if (nextc == EOF) {
          raiseError("Comment cannot terminated!");
        }
        c = nextc;
        nextc = fgetc(fp);
      } while (!(c == '*' && nextc == '/'));
    } else {
      ungetc(c, fp);
    }
  }
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
    raiseError("':' should continue with '='");
  }
  return false;
}


Token getNextToken() {
  Token token;
  char ch = fgetc(fp);


  //SKIP WHITESPACE
  while (isspace(ch)) {
    ch = fgetc(fp);
    }

  //SKIP COMMENT


  //IDENTIFIER


  //INTEGER


  //OPERATOR


  //STRING


  //KEYWORD


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
    printf("%s\n", token.lexeme);
    printf("%d\n", token.type);
    c = fgetc(fp);
  }
}
