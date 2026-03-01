#ifndef JSON_H
#define JSON_H

#include <stddef.h>

typedef enum {
    JSON_UNDEFINED,
    JSON_NULL,
    JSON_BOOLEAN,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
} JsonType;

typedef struct Json Json;

#define JSON_TABLE_SIZE 17

typedef struct {
    Json* buckets[JSON_TABLE_SIZE];
} JsonTable;

struct Json {
    JsonType type;
    union {
        bool boolean;
        double number;
        const char* string;
        Json* array;
        JsonTable* table;
    };
    Json* next; // linked list for JSON objects and arrays
    const char* key;
};

#define JSON_ARENA_SIZE 1024

typedef struct JsonArena JsonArena;
struct JsonArena {
    alignas(max_align_t) char data[JSON_ARENA_SIZE];
    size_t size;
    JsonArena* next;
};

typedef enum {
    JSON_TOKEN_ERROR,
    JSON_TOKEN_EOF,
    JSON_TOKEN_LEFT_CURLY,
    JSON_TOKEN_RIGHT_CURLY,
    JSON_TOKEN_LEFT_BRACKET,
    JSON_TOKEN_RIGHT_BRACKET,
    JSON_TOKEN_COLON,
    JSON_TOKEN_COMMA,
    JSON_TOKEN_NULL,
    JSON_TOKEN_TRUE,
    JSON_TOKEN_FALSE,
    JSON_TOKEN_NUMBER,
    JSON_TOKEN_STRING,
} JsonToken;

typedef struct {
    const char* text;
    JsonArena* arena;
    const char* begin;
    const char* end;
    JsonToken token;
} JsonParser;

void jsonParserInit(JsonParser* parser, const char* text);
void jsonParserFree(JsonParser* parser);

Json* jsonParse(JsonParser* parser);

void jsonPrint(Json* json);

#endif // JSON_H
