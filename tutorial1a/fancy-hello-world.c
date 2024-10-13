#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fancy-hello-world.h"

const int MAX_NAME_SIZE = 53;

void hello_string(char* name, char* output){
    char copy_name[MAX_NAME_SIZE];
    strcpy(copy_name, name);

    strcat(output, "Hello World, hello ");
    strcat(output, copy_name);
    printf("%s\n", output);
}

int main(){
    char name[MAX_NAME_SIZE], output[2*MAX_NAME_SIZE];
    fgets(name, MAX_NAME_SIZE, stdin);

    hello_string(name, output);
    return 0;
}