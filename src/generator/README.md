# General

## Definitions

This generator can read YML and also JSON files.

### Enumeration

The following properties are allowed.

| name   | required | description                                                |
|--------|----------|------------------------------------------------------------|
| name   | yes      | the name of the enumeration                                |
| type   | yes      | describes the underlying datatype                          |
| values | yes      | list of enum entries                                       |
| autoId | no       | uses standard auto enumeration without declaring the value |

If it is required to set individual values have a look at following example.

Example:

```json
{
  "Enums": [
    {
      "name": "Status",
      "type": "uint32_t",
      "values": [
        "Booting",
        "Starting",
        "Finished"
      ],
      "autoId": true
    },
    {
      "name": "ErrorStates",
      "type": "uint32_t",
      "values": [
        {"Info": 1},
        {"Warn": 4}
      ],
      "autoId": false
    }
  ]
}
```

This example will generate this C++ code

```c++
enum class Status : uint32_t
{
    Booting,
    Starting,
    Finished,
};

enum class ErrorStates : uint32_t
{
    Info = 1,
    Warn = 4,
};
```

### Structures

The following properties are allowed.

| name             | required | description                                                        |
|------------------|----------|--------------------------------------------------------------------|
| name             | yes      | the name of the structs                                            |
| parameter        | yes      | properties which is a list, each variable will be a type of object |
| parameter - name | yes      | name of the struct variable                                        |
| parameter - type | yes      | datatype of struct variable                                        |

```json
{
  "Structs": [
    {
      "name": "Temperature",
      "parameter": [
        {"value": "float"},
        {"raw": "uint32_t"}
      ]
    }
  ]
}
```

This example will generate this C++ code

```c++
struct Temperature
{
    float value;
    uint32_t raw;
};
```

### Groups

The following properties are allowed.

| name          | required | description                                                                                                                                                                                         |
|---------------|----------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| name          | yes      | the name of the structs                                                                                                                                                                             |
| persistence   | yes      | the persistence can be described with following<br/>**None - no persistence used<br/>Cyclic - group will be persisted in cyclic manner<br/>OnWrite - will only be persisted if it will be changed** |
| baseId        | yes      | describes the base id for all datapoints below                                                                                                                                                      |
| version       | yes      | describes the version where it was created or updated. It is defined as string **\<MAJOR>.\<MINOR>.\<BUILD>**                                                                                       |
| allowUpgrade  | no       | describes if serialized data can be override group values given the version is higher                                                                                                               |


```json
{
  "Groups": [
    {
      "name": "DefaultGroup",
      "persistence": "None",
      "baseId": "0x4000",
      "version": "1.0.1"
    }
  ]
}
```

### Data Points

The following properties are allowed.

| name         | required | description                                                                                                    |
|--------------|----------|----------------------------------------------------------------------------------------------------------------|
| name         | yes      | the name of the datapoint                                                                                      |
| group        | yes      | name of the group where the datapoint will be registered                                                       |
| id           | yes      | id of the datapoint. **NOTE**: that's the id without the offset of the group itself.                           |
| type         | yes      | defines the datatype for normal values or this can also be a self defined struct                               |
| default      | no       | defines the initial value of the datapoint, structures can also be defaulted                                   |
| access       | yes      | describes the access to the datapoint:**<br/>READWRITE<br/>READONLY<br/>WRITEONLY**                            |
| namespace    | no       | each datapoint can be moved in separate namespaces otherwise the datapoint will be visible without a namespace |
| version      | yes      | describes the version where it was created or updated. It is defined as string **\<MAJOR>.\<MINOR>.\<BUILD>**  |
| arraySize    | no       | describes if the defined type is a std::array, it will define the size of the std::array with defined type     |
| allowUpgrade | no       | describes if serialized data can be override DP value given the version is higher                              |
| description  | no       | description of the datapoint can be written in this field                                                      |

```json
{
  "Datapoints": [
    {
      "name": "test",
      "group": "DefaultGroup",
      "id": 1,
      "type": "int32_t",
      "default": 4211,
      "access": "READWRITE",
      "namespace": "Testify",
      "version": "1.1.1"
    },
    {
      "name": "test3",
      "group": "DefaultGroup",
      "id": 3,
      "type": "int32_t",
      "access": "READWRITE",
      "version": "1.0.1"
    },
    {
      "name": "test3",
      "group": "DefaultGroup",
      "id": 3,
      "type": "int32_t",
      "access": "READWRITE",
      "version": "1.0.1",
      "arraySize": 42
    }
  ]
}

```
