#include <stdio.h>
#include <string.h>

void print_hello_string();

int main(void) {
    print_hello_string(); 
    return 0;
}

void print_hello_string() {
    #ifdef FRENCH
    printf("Bonjour le monde!\n");
    #endif

    #ifdef SPANISH
    printf("Hola, mundo!\n");
    #endif
    
    #ifdef ENGLISH
    printf("Hello world!\n");
    #endif

    #ifdef CHINESE
    printf("Nihao, shijie!\n");
    #endif    

    #ifdef DANISH
    printf("Hej Verden!\n");
    #endif
}