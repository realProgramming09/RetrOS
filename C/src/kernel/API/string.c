#include "kernel/str.h"
#include "kernel/memory.h"
#include "kernel/sys.h"

typedef struct String{
    int length; //String's length
    int capacity; //String's max capacity (if it's treated as buffer)
    char* data; //String's chars
}String;

int strlen(const char* str){
    int length = 0;
    while(str[length] != 0) length++; //Just counts the chars before a '\0'
    return length;
}

String* new(const char* str){
    String* s = genericAlloc(sizeof(String)); //Allocate the String* on RAM

    //Allocate on RAM the actual char*
    s->length = strlen(str);
    s->capacity = s->length > 0 ? s->length*2 : 1;
    s->data = genericAlloc(s->capacity);
    if(!s->data){
        genericFree(s);
        return NULL;
    }

    //Setup the string as all 0s
    for(int i = 0; i < s->capacity; i++) s->data[i] = 0;
    for(int i = 0; i < s->length; i++) s->data[i] = str[i];

    return s;
}
String* newBuffer(int capacity){
    String* s = genericAlloc(sizeof(String)); //Allocate the String* on RAM

    //Allocate on RAM the actual char*
    s->length = 0;
    s->capacity = capacity;
    s->data = genericAlloc(s->capacity);
    if(!s->data){
        genericFree(s);
        return NULL;
    }

    //Setup the buffer as all 0s
    for(int i = 0; i < s->capacity; i++) s->data[i] = 0;

    return s;
}
String* copyString(String* s){
    String* t = genericAlloc(sizeof(String)); //Allocate on RAM the new String*
    if(!t) return NULL;

    //Allocate on RAM the actual char*
    t->length = s->length;
    t->capacity = s->capacity;
    t->data = genericAlloc(t->capacity);
    if(!t->data){
        genericFree(t);
        return NULL;
    }

    //Copy the s->data into our new string
    for(int i = 0; i < t->length; i++) t->data[i] = s->data[i];
    for(int i = t->length; i < t->capacity; i++) t->data[i] = 0; 

    return t;
}

void setChar(String* s, char c, int index){
    if(index >= s->length || !s) return;
    s->data[index] = c; //Sets the char at index
    
}
char charAt(String* s, int index){
    if(index >= s->capacity || !s) return 0;
    return s->data[index]; //Returns char at index
}

int compareStrings(String* s1, String* s2){
    if(!s1 || !s2) return -1;
    if(s1->length != s2->length) return 1; //If length is different, we're sure they're different
    for(int i = 0; i < s1->length; i++){
        if(s1->data[i] != s2->data[i]) return 1; //Check the chars 1 by 1
    }
    return 0;
}
String* concatAndCopyStrings(String* s1, String* s2){
    String* s3 = newBuffer(s1->length + s2->length); //Allocate a new String*
    s3->length = s1->length + s2->length;

    //Concat the two strings in the third
    for(int i = 0; i < s1->length; i++) s3->data[i] = s1->data[i];
    for(int i = 0; i < s2->length; i++) s3->data[s1->length + i] = s2->data[i];

    return s3;
}
int strLength(const String* s){
    return !s ? -1 : s->length;
}
int strCapacity(const String* s){
    return !s ? -1 : s->length;
}
char* strPointer(const String* s){
    if(!s) return NULL;
    return s->data;
}
void unloadString(String* s){
    if(!s) return;
    genericFree(s->data);
    genericFree(s);
    s = NULL; //NO use-after-free
}
void clearString(String* s){
    if(!s) return;
    for(int i = 0; i < s->capacity; i++) s->data[i] = 0; //Sets all chars to 0
    s->length = 0;
}
void strncpy(const char* s, char* buf, size_t size){
    int length = strlen(s);
    if(length >= size) length = size;

    //Copies one char* into the other without overflowing
    for(int i = 0; i < length; i++) buf[i] = s[i];
    for(int i = length; i < size; i++) buf[i] = 0;
}
int compareImmediate(String* s1, const char* s2, size_t size){
    if(!s1 || !s2) return -1;
    if(!size) return s1->length;
    
    //Checks for overflow
    if(strlen(s2) < size) size = strlen(s2);
    if(size != s1->length) return 1;

    //Basically strcmp()
    int length = size > s1->length ? s1->length : size;
    for(int i = 0; i < length; i++){
        if(s1->data[i] != s2[i]) return 1;
    }
    return 0;
}
void concatStrings(String* s1, String* s2){
    int totalLength = s1->length + s2->length;
    if(totalLength > s1->capacity){ //Checks to see if we need to realloc
        char* temp= genericRealloc(s1->data, totalLength);
        s1->data = temp;
        s1->capacity = totalLength;
    }

    //Appends the second string onto the first
    for(int i = 0; i < s2->length; i++){
        s1->data[i + s1->length] = s2->data[i];
    }
    s1->length = totalLength;

}
void append(String* s, char c){
    if(s->length+1 > s->capacity){ //Checks to see if we need to realloc
        char* temp= genericRealloc(s->data, s->capacity+1);
        if(!temp) return;

        s->data = temp;
        s->capacity++;
        
    }
    s->data[s->length++] = c; //Sets the last char and increments length
}
StringArray* split(String* s, char c){
    if(!s || !c) return NULL;

    //Allocate on RAM the array
    StringArray* array = genericAlloc(sizeof(StringArray*));
    array->data = genericAlloc(sizeof(String*) * 8); //Let's start with 8 string*

    String* temp = newBuffer(s->capacity); //Allocate a temporary buffer

    int last = 0, capacity = 8;
    for(int i = 0; i < s->length; i++){
        if(s->data[i] == c){ //We found the splitting char
            if(last >= capacity){ //Checks to see if we need to realloc
                array->data = genericRealloc(array->data, sizeof(String*)*capacity*2);
                capacity *= 2;
            }
            array->data[last++] = copyString(temp); //Or else copies the buffer into the last string
            clearString(temp);
        }
        else append(temp, charAt(s, i)); //Appends the current char onto the buffer
    }

    //Does everything in the for loop once more to clear the buffer and not miss the last string
    if(last >= capacity){
        array->data = genericRealloc(array->data, sizeof(String*)*capacity*2);
        capacity *= 2;
    }
    array->data[last++] = copyString(temp);
    array->length = last;
    unloadString(temp);
    return array;
}
void unloadArray(StringArray* a){
    for(int i = 0; i < a->length; i++){
        unloadString(a->data[i]);
    }
    genericFree(a->data);
    genericFree(a);
    a = NULL;
}
StringArray* splitQuotes(String* s, char c){
    
    /*THE EXACT SAME AS split(), BUT TREATS EVERYTHING IN DOUBLE QUOTES ("") AS A SINGLE TOKEN AND DOES NOT SPLIT THAT.*/

    if(!s || !c) return NULL;


    StringArray* array = genericAlloc(sizeof(StringArray*));
    array->data = genericAlloc(sizeof(String*) * 8);

    String* temp = newBuffer(s->capacity);  

    int last = 0, capacity = 8;
    for(int i = 0, quotes = 0; i < s->length; i++){
        if(s->data[i] == '"') quotes = !quotes;
        else if(s->data[i] == c && !quotes){
            if(last >= capacity){
                array->data = genericRealloc(array->data, sizeof(String*)*capacity*2);
                capacity *= 2;
                 
            }
            array->data[last++] = copyString(temp);
            clearString(temp);
        }
        else append(temp, charAt(s, i));
    }
    if(last >= capacity){
        array->data = genericRealloc(array->data, sizeof(String*)*capacity*2);
        capacity *= 2;
    }
    array->data[last++] = copyString(temp);
    array->length = last;
    unloadString(temp);
    return array;
}
void copyImmediate(String*s, char* str, size_t size){
    if(!s || !str || size < 1) return;
    strncpy(s->data, str, size); //It's just a wrapper for strncpy()
    
}
int strcmp(const char* s1, const char* s2){
    if(!s1 || !s2) return -2;

    //Compares the two strings
    int length = strlen(s1);
    for(int i = 0; i < length; i++){
        if(s1[i] > s2[i]) return 1;
        else if(s1[i] < s2[i]) return -1;
    }
    return 0;
} 
int strncmp(const char* s1, const char* s2, size_t size){
    if(!s1 || !s2 || size < 1) return -2;

    //Compares the two strings without overflowing
    int length = strlen(s1);
    for(int i = 0; i < length && i < size; i++){
        if(s1[i] > s2[i]) return 1;
        else if(s1[i] < s2[i]) return -1;
    }
    return 0;
}
void strcat(char* s1, const char* s2){
    if(!s1 || !s2) return;
    
    int length1 = strlen(s1);
    int length2 = strlen(s2);
    for(int i = 0; i < length2; i++){
        s1[length1+i] = s2[i];
    } 
}
void strncat(char* s1, const char* s2, size_t size){
    if(!s1 || !s2 || size < 1) return;

    int length1 = strlen(s1);
    int length2 = strlen(s2);
    for(int i = 0; i < length2 && i+length1 < size; i++){
        s1[length1+i] = s2[i];
    } 
}