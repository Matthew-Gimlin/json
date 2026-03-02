# JSON

A JSON parsing library written in C.

## Usage

Say the file `fruits.json` stores the following JSON.

```json
[
    {
        "fruit": "apple",
        "calories": 90
    },
    {
        "fruit": "banana",
        "calories": 100
    },
    {
        "fruit": "orange",
        "calories": 60
    }
]
```

To parse JSON, first initialize a JSON parser.

```c
JsonParser parser;
jsonParserInit(&parser);
```

Then, parse the file and operate on the resulting JSON tree.

```c
Json* root = jsonParseFile(&parser, "fruits.json");
JSON_FOR_EACH(fruit, root) {
    printf("%s is %g calories\n",
        jsonSelect(fruit, "/fruit")->string,
        jsonSelect(fruit, "/calories")->number);
}
```

Finally, free the memory allocated by the parser.

```c
jsonParserFree(&parser);
```

The sample code above produces the following output.

```
apple is 90 calories
banana is 100 calories
orange is 60 calories
```
