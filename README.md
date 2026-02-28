# JSON

A JSON parsing library written in C.

## Usage

To parse JSON text, first initialize a JSON parser.

```c
JsonParser parser;
jsonParserInit(&parser, text);
```

Then, parse the text and operate on the parsed JSON tree.

```c
Json* root = jsonParse(&parser);
Json* fruits = jsonSelect(root, "/fruits");
JSON_FOR_EACH(fruit, fruits) {
    printf("%s is %g calories\n",
        jsonSelect(fruit, "/name")->string,
        jsonSelect(fruit, "/calories")->number);
}
```

Finally, free the memory allocated by the parser.

```c
jsonParserFree(&parser);
```
