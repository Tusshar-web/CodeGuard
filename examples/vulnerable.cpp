#include <bits/stdc++.h>
#include <cstring>
#include <iostream>
#include <cstring>
#include <cstdio>

// Legacy function declaration for static-analysis demonstration.
// Never use gets() in real programs.
extern "C" char *gets(char *);

using namespace std;

int main()
{
    char destination[10];
    char source[] = "This is a very long string";

    strcpy(destination, source);

    char input[20];
    gets(input);

    char buffer[8];
    sprintf(buffer, "%s", "Hello World");

    char safeDestination[6];
    char safeSource[] = "Hello";

    strcpy(safeDestination, safeSource);

    char strncpyBuffer[10];

    strncpy(strncpyBuffer, "HelloWorld", 10);
    
    return 0;
}