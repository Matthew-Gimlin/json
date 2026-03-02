#include "json.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static JsonArena* jsonNewArena() {
    JsonArena* arena = malloc(sizeof(*arena));
    if (!arena) {
        return NULL;
    }
    arena->size = 0;
    arena->next = NULL;
    return arena;
}

static void jsonParserFreeArena(JsonArena* arena) {
    JsonArena* next;
    while (arena) {
        next = arena->next;
        free(arena);
        arena = next;
    }
}

static inline size_t alignUp(size_t n) {
    return (n + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1);
}

static void* jsonAlloc(JsonArena* arena, size_t n) {
    if (arena == NULL || n == 0 || n > JSON_ARENA_SIZE) {
        return NULL;
    }
    size_t i = alignUp(arena->size);
    while (i + n > JSON_ARENA_SIZE) {
        if (!arena->next) {
            if (!(arena->next = jsonNewArena())) {
                return NULL;
            }
        }
        arena = arena->next;
        i = alignUp(arena->size);
    }
    void* p = &arena->data[i];
    arena->size = i + n;
    return p;
}

void jsonParserInit(JsonParser* parser) {
    if (!(parser->arena = jsonNewArena())) {
        return;
    }
    parser->begin = NULL;
    parser->end = NULL;
    parser->token = JSON_TOKEN_ERROR;
}

void jsonParserFree(JsonParser* parser) {
    jsonParserFreeArena(parser->arena);
    parser->arena = NULL;
    parser->begin = NULL;
    parser->end = NULL;
}

static inline bool jsonIsSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static inline bool jsonIsDigit(char c) {
    return c >= '0' && c <= '9';
}

static void jsonSkipSpace(JsonParser* parser) {
    while (jsonIsSpace(*parser->end)) {
        parser->end++;
    }
}

static bool jsonNumberToken(JsonParser* parser) {
    if (*parser->end == '-') {
        parser->end++;
    }
    while (jsonIsDigit(*parser->end)) {
        parser->end++;
    }
    if (*parser->end == '.') {
        parser->end++;
        while (jsonIsDigit(*parser->end)) {
            parser->end++;
        }
    }
    if (*parser->end == 'e' || *parser->end == 'E') {
        parser->end++;
        if (*parser->end == '+' || *parser->end == '-') {
            parser->end++;
        }
        while (jsonIsDigit(*parser->end)) {
            parser->end++;
        }
    }
    parser->token = JSON_TOKEN_NUMBER;
    return true;
}

static bool jsonStringToken(JsonParser* parser) {
    bool isEscaped = false;
    do {
        parser->end++;
        if (*parser->end == '"' && !isEscaped) {
            parser->end++;
            parser->token = JSON_TOKEN_STRING;
            return true;
        } else if (*parser->end == '\\') {
            isEscaped = !isEscaped;
        } else {
            isEscaped = false;
        }
    } while (*parser->end);
    parser->token = JSON_TOKEN_ERROR;
    return false;
}

static bool jsonIsKeyword(JsonParser* parser, const char* keyword, size_t n,
        JsonToken token) {
    if (strncmp(parser->end, keyword, n) == 0) {
        parser->end += n;
        parser->token = token;
        return true;
    }
    parser->token = JSON_TOKEN_ERROR;
    return false;
}

static bool jsonNextToken(JsonParser* parser) {
    jsonSkipSpace(parser);
    parser->begin = parser->end;
    switch (*parser->end) {
        case '\0':
            parser->token = JSON_TOKEN_EOF;
            return true;
        case '{':
            parser->end++;
            parser->token = JSON_TOKEN_LEFT_CURLY;
            return true;
        case '}':
            parser->end++;
            parser->token = JSON_TOKEN_RIGHT_CURLY;
            return true;
        case '[':
            parser->end++;
            parser->token = JSON_TOKEN_LEFT_BRACKET;
            return true;
        case ']':
            parser->end++;
            parser->token = JSON_TOKEN_RIGHT_BRACKET;
            return true;
        case ':':
            parser->end++;
            parser->token = JSON_TOKEN_COLON;
            return true;
        case ',':
            parser->end++;
            parser->token = JSON_TOKEN_COMMA;
            return true;
        case 'n':
            return jsonIsKeyword(parser, "null", 4, JSON_TOKEN_NULL);
        case 't':
            return jsonIsKeyword(parser, "true", 4, JSON_TOKEN_TRUE);
        case 'f':
            return jsonIsKeyword(parser, "false", 5, JSON_TOKEN_FALSE);
        case '"':
            return jsonStringToken(parser);
        default:
            break;
    }
    if (*parser->end == '-' || jsonIsDigit(*parser->end)) {
        return jsonNumberToken(parser);
    }
    parser->token = JSON_TOKEN_ERROR;
    return false;
}

static const char* jsonStringDuplicate(JsonParser* parser) {
    size_t n = parser->end - parser->begin - 2; // ignore the quotes
    char* s = jsonAlloc(parser->arena, n + 1);
    memcpy(s, parser->begin + 1, n);
    s[n] = '\0';
    return s;
}

static Json* jsonNewNode(JsonParser* parser, JsonType type) {
    Json* node = jsonAlloc(parser->arena, sizeof(*node));
    if (!node) {
        return NULL;
    }
    node->type = type;
    node->next = NULL;
    node->key = NULL;
    return node;
}

static Json* jsonParseNode(JsonParser* parser);

static Json* jsonParseArray(JsonParser* parser) {
    Json* head = jsonParseNode(parser);
    if (!head) {
        return NULL;
    }
    Json* current = head;
    while (jsonNextToken(parser) && parser->token == JSON_TOKEN_COMMA) {
        jsonNextToken(parser);
        if (!(current->next = jsonParseNode(parser))) {
            return NULL;
        }
        current = current->next;
    }
    if (parser->token != JSON_TOKEN_RIGHT_BRACKET) {
        return NULL;
    }
    return head;
}

static uint32_t jsonHash(const char* key, size_t n) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < n; i++) {
        hash ^= (unsigned char)key[i];
        hash *= 16777619u;
    }
    return hash;
}

static JsonTable* jsonParseObject(JsonParser* parser) {
    JsonTable* table = jsonAlloc(parser->arena, sizeof(*table));
    if (!table) {
        return NULL;
    }
    memset(table->buckets, 0, sizeof(table->buckets));
    if (!table) {
        return NULL;
    }
    do {
        if (parser->token != JSON_TOKEN_STRING) {
            return NULL;
        }
        const char* key = jsonStringDuplicate(parser);
        if (!key) {
            return NULL;
        }
        uint32_t hash = jsonHash(key, strlen(key));
        if (!jsonNextToken(parser) || parser->token != JSON_TOKEN_COLON) {
            return NULL;
        }
        Json** value = &table->buckets[hash % JSON_TABLE_SIZE];
        while (*value) {
            value = &((*value)->next);
        }
        if (!jsonNextToken(parser) || !(*value = jsonParseNode(parser))) {
            return NULL;
        }
        (*value)->key = key;
        jsonNextToken(parser);
    } while (parser->token == JSON_TOKEN_COMMA && jsonNextToken(parser));
    if (parser->token != JSON_TOKEN_RIGHT_CURLY) {
        return NULL;
    }
    return table;
}

static Json* jsonParseNode(JsonParser* parser) {
    Json* node = NULL;
    switch (parser->token) {
        case JSON_TOKEN_NULL:
            return jsonNewNode(parser, JSON_NULL);
        case JSON_TOKEN_TRUE:
        case JSON_TOKEN_FALSE:
            node = jsonNewNode(parser, JSON_BOOLEAN);
            node->boolean = (parser->token == JSON_TOKEN_TRUE);
            return node;
        case JSON_TOKEN_NUMBER: {
            node = jsonNewNode(parser, JSON_NUMBER);
            char* end;
            node->number = strtod(parser->begin, &end);
            if (!end) {
                return NULL;
            }
            return node;
        }
        case JSON_TOKEN_STRING:
            node = jsonNewNode(parser, JSON_STRING);
            node->string = jsonStringDuplicate(parser);
            return node;
        case JSON_TOKEN_LEFT_BRACKET:
            node = jsonNewNode(parser, JSON_ARRAY);
            if (jsonNextToken(parser)
                    && parser->token == JSON_TOKEN_RIGHT_BRACKET) {
                return node;
            } else if (!(node->array = jsonParseArray(parser))) {
                return NULL;
            }
            return node;
        case JSON_TOKEN_LEFT_CURLY:
            node = jsonNewNode(parser, JSON_OBJECT);
            if (jsonNextToken(parser)
                    && parser->token == JSON_TOKEN_RIGHT_CURLY) {
                node->table = NULL;
                return node;
            }
            if (!(node->table = jsonParseObject(parser))) {
                return NULL;
            }
            return node;
        default:
            break;
    }
    return NULL;
}

Json* jsonParse(JsonParser* parser, const char* text) {
    parser->begin = text;
    parser->end = text;
    if (!jsonNextToken(parser)) {
        return NULL;
    }
    return jsonParseNode(parser);
}

Json* jsonParseFile(JsonParser* parser, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        return NULL;
    }
    long n = ftell(f);
    if (n < 0) {
        return NULL;
    }
    rewind(f);
    char* text = malloc(n + 1);
    if (!text) {
        return NULL;
    }
    if (fread(text, sizeof(*text), n, f) != (size_t)n) {
        fclose(f);
        free(text);
        return NULL;
    }
    fclose(f);
    text[n] = '\0';
    Json* json = jsonParse(parser, text);
    free(text);
    return json;
}

void jsonPrint(Json* json) {
    if (!json) {
        return;
    }
    switch (json->type) {
        case JSON_NULL:
            printf("null");
            break;
        case JSON_BOOLEAN:
            printf("%s", json->boolean ? "true" : "false");
            break;
        case JSON_NUMBER:
            printf("%g", json->number);
            break;
        case JSON_STRING:
            printf("\"%s\"", json->string);
            break;
        case JSON_ARRAY: {
            printf("[");
            Json* node = json->array;
            while (node) {
                jsonPrint(node);
                if (node->next) {
                    printf(",");
                }
                node = node->next;
            }
            printf("]");
            break;
        }
        case JSON_OBJECT: {
            printf("{");
            if (!json->table) {
                printf("}");
                break;
            }
            bool comma = false;
            for (int i = 0; i < JSON_TABLE_SIZE; i++) {
                Json* head = json->table->buckets[i];
                while (head) {
                    if (comma) {
                        printf(",");
                    }
                    printf("\"%s\":", head->key);
                    jsonPrint(head);
                    head = head->next;
                    comma = true;
                }
            }
            printf("}");
            break;
        }
        default:
            break;
    }
}

Json* jsonSelect(Json* root, const char* path) {
    if (!root || !path) {
        return root;
    }
    if (path[0] != '/') {
        return NULL;
    }
    Json* selected = root;
    while (*path) {
        path++;
        const char* end = strchr(path, '/');
        if (!end) {
            end = path + strlen(path);
        }
        if (end - path == 0) {
            break;
        } else if (selected->type != JSON_OBJECT) {
            return NULL;
        }
        uint32_t hash = jsonHash(path, end - path);
        Json* value = selected->table->buckets[hash % JSON_TABLE_SIZE];
        while (value && strncmp(value->key, path, end - path) != 0) {
            value = value->next;
        }
        if (!value) {
            return NULL;
        }
        selected = value;
        path = end;
    }
    return selected;
}
