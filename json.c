#include "json.h"

static inline void json_skip_whitespace(const char** json) {
    while (**json) {
        switch (**json) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                ++*json;
                break;

            default:
                return;
        }
    }
}

static inline json_token_t json_string_token(const char** json) {
    json_token_t string = {JSON_TOKEN_STRING, *json, *json};
    while (*++string.end) {
        switch (*string.end) {
            case '"':
                *json = ++string.end;
                return string;
            
            default:
                break;
        }
    }
    string.type = JSON_TOKEN_ERROR;
    *json = string.end;
    return string;
}

static inline json_token_t json_number_token(const char** json) {
    json_token_t number = {JSON_TOKEN_NUMBER, *json, *json};
    while (*++number.end) {
        switch (*number.end) {
            case '-':
            case '.':
            case '0' ... '9':
                break;

            default:
                *json = number.end;
                return number;
        }
    }
}

static inline json_token_t json_keyword_token(const char** json, const char* keyword, json_token_type_e type) {
    json_token_t token = {type, *json, *json};
    while (*token.end && *keyword && *++token.end == *++keyword);
    if (*keyword == '\0') {
        *json = token.end;
        return token;
    }
    token.type = JSON_TOKEN_ERROR;
    return token;
}

json_token_t json_next_token(const char** json) {
    json_skip_whitespace(json);
    switch (**json) {
        case '\0':
            return (json_token_t){JSON_TOKEN_EOF, *json, *json};
            
        case '{':
            return (json_token_t){JSON_TOKEN_LEFT_CURLY, *json, ++*json};

        case '}':
            return (json_token_t){JSON_TOKEN_RIGHT_CURLY, *json, ++*json};

        case '[':
            return (json_token_t){JSON_TOKEN_LEFT_BRACKET, *json, ++*json};

        case ']':
            return (json_token_t){JSON_TOKEN_RIGHT_BRACKET, *json, ++*json};

        case ':':
            return (json_token_t){JSON_TOKEN_COLON, *json, ++*json};

        case ',':
            return (json_token_t){JSON_TOKEN_COMMA, *json, ++*json};

        case '"':
            return json_string_token(json);

        case '-':
        case '0' ... '9':
            return json_number_token(json);

        case 't':
            return json_keyword_token(json, "true", JSON_TOKEN_TRUE);

        case 'f':
            return json_keyword_token(json, "false", JSON_TOKEN_FALSE);

        case 'n':
            return json_keyword_token(json, "null", JSON_TOKEN_NULL);
    }
    return (json_token_t){JSON_TOKEN_ERROR, *json, *json};
}