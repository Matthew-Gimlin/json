#include "json.h"
#include <string.h>

static inline void json_eat(json_lexer_t* lexer) {
    ++lexer->current;
}

static inline json_token_t json_token(
        json_lexer_t* lexer, json_token_type_e type) {
    return (json_token_t){
        .type = type,
        .start = lexer->start,
        .end = lexer->current,
    };
}

static inline void json_skip_whitespace(json_lexer_t* lexer) {
    while (*lexer->current) {
        switch (*lexer->current) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                json_eat(lexer);
                break;

            default:
                return;
        }
    }
}

static inline json_token_t json_string_token(json_lexer_t* lexer) {
    json_eat(lexer);
    while (*lexer->current) {
        switch (*lexer->current) {
            case '"':
                json_eat(lexer);
                return json_token(lexer, JSON_TOKEN_STRING);

            default:
                json_eat(lexer);
                break;
        }
    }
    return json_token(lexer, JSON_TOKEN_ERROR);
}

static inline json_token_t json_number_token(json_lexer_t* lexer) {
    json_eat(lexer);
    while (*lexer->current) {
        switch (*lexer->current) {
            case '-':
            case '+':
            case 'e':
            case 'E':
            case '.':
            case '0' ... '9':
                json_eat(lexer);
                break;
            
            default:
                return json_token(lexer, JSON_TOKEN_NUMBER);
        }
    }
    return json_token(lexer, JSON_TOKEN_NUMBER);
}

static inline json_token_t json_keyword_token(
        json_lexer_t* lexer, const char* keyword, size_t keyword_len,
        json_token_type_e type) {
    json_eat(lexer);
    if (strncmp(lexer->current, keyword, keyword_len) != 0) {
        return json_token(lexer, JSON_TOKEN_ERROR);
    }
    lexer->current += keyword_len;
    return json_token(lexer, type);
}

json_token_t json_next_token(json_lexer_t* lexer) {
    json_skip_whitespace(lexer);
    lexer->start = lexer->current;
    switch (*lexer->current) {
        case '\0':
            return json_token(lexer, JSON_TOKEN_EOF);
            
        case '{':
            json_eat(lexer);
            return json_token(lexer, JSON_TOKEN_LEFT_CURLY);

        case '}':
            json_eat(lexer);
            return json_token(lexer, JSON_TOKEN_RIGHT_CURLY);

        case '[':
            json_eat(lexer);
            return json_token(lexer, JSON_TOKEN_LEFT_BRACKET);

        case ']':
            json_eat(lexer);
            return json_token(lexer, JSON_TOKEN_RIGHT_BRACKET);

        case ':':
            json_eat(lexer);
            return json_token(lexer, JSON_TOKEN_COLON);

        case ',':
            json_eat(lexer);
            return json_token(lexer, JSON_TOKEN_COMMA);

        case '"':
            return json_string_token(lexer);

        case '-':
        case '0' ... '9':
            return json_number_token(lexer);

        case 't':
            return json_keyword_token(lexer, "rue", 3, JSON_TOKEN_TRUE);

        case 'f':
            return json_keyword_token(lexer, "alse", 4, JSON_TOKEN_FALSE);

        case 'n':
            return json_keyword_token(lexer, "ull", 3, JSON_TOKEN_NULL);
    }
    return json_token(lexer, JSON_TOKEN_ERROR);
}
