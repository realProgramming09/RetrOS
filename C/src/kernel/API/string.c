#include "kernel/str.h"
#include "kernel/mmu.h"
#include "kernel/sys.h"

typedef struct String{
    int length; //Lunghezza della stringa
    int capacity; //Capacità massima della stringa (se è un buffer)
    char* data; //I caratteri della stringa
}String;

int strlen(const char* str){
    int length = 0;
    while(str[length] != 0) length++;
    return length;
}

String* new(const char* str){
    String* s = genericAlloc(sizeof(String)); //Allocare sulla RAM la stringa

    //Allocare sulla RAM l'array di caratteri vero e proprio
    s->length = strlen(str);
    s->capacity = s->length > 0 ? s->length*2 : 1;
    s->data = genericAlloc(s->capacity);
    if(!s->data){
        genericFree(s);
        return NULL;
    }

    //Impostare la stringa
    for(int i = 0; i < s->capacity; i++) s->data[i] = 0;
    for(int i = 0; i < s->length; i++) s->data[i] = str[i];

    return s;
}
String* newBuffer(int capacity){
    String* s = genericAlloc(sizeof(String)); //Allocare sulla RAM la stringa

    //Allocare sulla RAM l'array di caratteri vero e proprio
    s->length = 0;
    s->capacity = capacity;
    s->data = genericAlloc(s->capacity);
    if(!s->data){
        genericFree(s);
        return NULL;
    }

    //Impostare la stringa
    for(int i = 0; i < s->capacity; i++) s->data[i] = 0;

    return s;
}
String* copyString(String* s){
    String* t = genericAlloc(sizeof(String)); //Allocare sulla RAM la nuova stringa
    if(!t) return NULL;

    //Allocare sulla RAM l'array di caratteri vero e proprio
    t->length = s->length;
    t->capacity = s->capacity;
    t->data = genericAlloc(t->capacity);
    if(!t->data){
        genericFree(t);
        return NULL;
    }

    //Impostare la stringa
    for(int i = 0; i < t->length; i++) t->data[i] = s->data[i];
    for(int i = t->length; i < t->capacity; i++) t->data[i] = 0; 

    return t;
}

void setChar(String* s, char c, int index){
    if(index >= s->capacity || !s) return;
    s->data[index] = c; //Impostare il carattere all'indice e aggiornare la lunghezza
    if(index >= s->length-1) s->length = index+1;
}
char charAt(String* s, int index){
    if(index >= s->capacity || !s) return 0;
    return s->data[index]; //Ritornare il carattere all'indice
}

int compareStrings(String* s1, String* s2){
    if(!s1 || !s2) return -1;
    if(s1->length != s2->length) return 1; //La lunghezza è diversa, non combaciano a prescindere
    for(int i = 0; i < s1->length; i++){
        if(s1->data[i] != s2->data[i]) return 1; //Vedere se le due stringhe sono uguali
    }
    return 0;
}
String* concatAndCopyStrings(String* s1, String* s2){
    String* s3 = newBuffer(s1->length + s2->length);
    s3->length = s1->length + s2->length;

    //Concatenare le due stringhe
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
    char* ptr = genericAlloc(s->length+1); //Allocare in RAM un puntatore di caratteri
    if(!ptr) return NULL;

    //Copiarci dentro la nostra stringa e nullterminiamola
    for(int i = 0; i < s->length; i++) ptr[i] = s->data[i];
    ptr[s->length] = '\0';
    return ptr;
}
void unloadString(String* s){
    if(!s) return;
    genericFree(s->data);
    genericFree(s);
    s = NULL; //NO use-after-free
}
void clearString(String* s){
    if(!s) return;
    for(int i = 0; i < s->capacity; i++) s->data[i] = 0;
    s->length = 0;
}
void strncpy(const char* s, char* buf, size_t size){
    int length = strlen(s);
    if(length >= size) length = size;

    for(int i = 0; i < length; i++) buf[i] = s[i];
    for(int i = length; i < size; i++) buf[i] = 0;
}
int compareImmediate(String* s1, const char* s2, size_t size){
    if(!s1 || !s2) return -1;
    if(!size) return s1->length;
    
    if(strlen(s2) < size) size = strlen(s2);

    
    if(size != s1->length) return 1;

    int length = size > s1->length ? s1->length : size;
    for(int i = 0; i < length; i++){
        if(s1->data[i] != s2[i]) return 1;
    }
    return 0;
}
void concatStrings(String* s1, String* s2){
    int totalLength = s1->length + s2->length;
    if(totalLength > s1->capacity){
        char* temp= genericRealloc(s1->data, totalLength);
        if(!temp) return;

        s1->data = temp;
        s1->capacity = totalLength;
        
    }

    for(int i = 0; i < s2->length; i++){
        s1->data[i + s1->length] = s2->data[i];
    }
    s1->length = totalLength;

}
void append(String* s, char c){
    if(s->length+1 > s->capacity){
        char* temp= genericRealloc(s->data, s->capacity+1);
        if(!temp) return;

        s->data = temp;
        s->capacity++;
        
    }
    s->data[s->length++] = c;
}
StringArray* split(String* s, char c){
    if(!s || !c) return NULL;


    StringArray* array = genericAlloc(sizeof(StringArray*));
    if(!array) return NULL;

    array->data = genericAlloc(sizeof(String*) * 8);
    if(!array->data){
        genericFree(array);
        return NULL;
    }

    String* temp = newBuffer(s->capacity); //Allocare spazio per un buffer temporaneo

    int last = 0, capacity = 8;
    for(int i = 0; i < s->length; i++){
        if(s->data[i] == c){
            if(last >= capacity){
                String** tempArray = genericRealloc(array->data, sizeof(String*)*capacity*2);
                if(!tempArray){
                    unloadArray(array);
                    return NULL;
                }
                capacity *= 2;
                array->data = tempArray;
            }
            array->data[last++] = copyString(temp);
            clearString(temp);
        }
        else append(temp, charAt(s, i));
    }
    if(last >= capacity){
        String** tempArray = genericRealloc(array->data, sizeof(String*)*capacity*2);
        if(!tempArray){
            unloadArray(array);
            return NULL;
        }
        capacity *= 2;
        array->data = tempArray;
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
    if(!s || !c) return NULL;


    StringArray* array = genericAlloc(sizeof(StringArray*));
    if(!array) return NULL;

    array->data = genericAlloc(sizeof(String*) * 8);
    if(!array->data){
        genericFree(array);
        return NULL;
    }

    String* temp = newBuffer(s->capacity); //Allocare spazio per un buffer temporaneo

    int last = 0, capacity = 8;
    for(int i = 0, quotes = 0; i < s->length; i++){
        if(s->data[i] == '"') quotes = !quotes;
        else if(s->data[i] == c && !quotes){
            if(last >= capacity){
                String** tempArray = genericRealloc(array->data, sizeof(String*)*capacity*2);
                if(!tempArray){
                    unloadArray(array);
                    return NULL;
                }
                capacity *= 2;
                array->data = tempArray;
            }
            array->data[last++] = copyString(temp);
            clearString(temp);
        }
        else append(temp, charAt(s, i));
    }
    if(last >= capacity){
        String** tempArray = genericRealloc(array->data, sizeof(String*)*capacity*2);
        if(!tempArray){
            unloadArray(array);
            return NULL;
        }
        capacity *= 2;
        array->data = tempArray;
    }
    array->data[last++] = copyString(temp);
    array->length = last;
    unloadString(temp);
    return array;
}
void copyImmediate(String*s, char* str, size_t size){
    if(!s || !str || size < 1) return;

    
    strncpy(s->data, str, size); //È uno strcpy, ma senza memory-leak
    
}
int strcmp(const char* s1, const char* s2){
    if(!s1 || !s2) return -2;

    int length = strlen(s1);
    for(int i = 0; i < length; i++){
        if(s1[i] > s2[i]) return 1;
        else if(s1[i] < s2[i]) return -1;
    }
    return 0;
}
 
int strncmp(const char* s1, const char* s2, size_t size){
    if(!s1 || !s2 || size < 1) return -2;

    int length = strlen(s1);
    for(int i = 0; i < length && i < size; i++){
        if(s1[i] > s2[i]) return 1;
        else if(s1[i] < s2[i]) return -1;
    }
    return 0;
}