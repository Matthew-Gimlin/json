# JSON

A JSON parsing library written in C.

## Usage

```c
Json* root = jsonParse(&parser);
Json* fruits = jsonSelect(root, "/fruits");
JSON_FOR_EACH(fruit, fruits) {
    printf("%s is %g calories\n",
        jsonSelect(fruit, "/name")->string,
        jsonSelect(fruit, "/calories")->number);
}
```
