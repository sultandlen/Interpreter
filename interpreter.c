#include <string.h>
#include <stdbool.h>
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

