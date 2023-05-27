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
