#pragma once

 
//Enum to pass in datatypes
typedef enum{
    INT,
    CHAR,
    STRING, //String*
    FLOAT,
    STRING_IMMEDIATE, //char*
} DataType;



void print(DataType type, const void* data); //Prints something, taking the type as a parameter
void println(DataType type, const void* data); //print() that does '\n'
void printStatic(DataType type, const void* data); //print() that does not step forward. Useful for printing static text
