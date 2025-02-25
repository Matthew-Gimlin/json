#ifndef JSON_H
#define JSON_H

typedef enum {
    JSON_TOKEN_ERROR = 0,
    JSON_TOKEN_EOF,
    JSON_TOKEN_LEFT_CURLY,
    JSON_TOKEN_RIGHT_CURLY,
    JSON_TOKEN_LEFT_BRACKET,
    JSON_TOKEN_RIGHT_BRACKET,
    JSON_TOKEN_COLON,
    JSON_TOKEN_COMMA,
    JSON_TOKEN_STRING,
    JSON_TOKEN_NUMBER,
    JSON_TOKEN_TRUE,
    JSON_TOKEN_FALSE,
    JSON_TOKEN_NULL,
} json_token_type_e;

typedef struct {
    json_token_type_e type;
    const char* start;
    const char* end;
} json_token_t;

json_token_t json_next_token(const char** json);

#endif