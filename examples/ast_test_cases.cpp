#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>

extern "C" char *gets(char *);

// Helper function for interprocedural analysis test
void copyData(char* destination, const char* source)
{
    strcpy(destination, source);
}

int main(int argc, char* argv[])
{
    // STR001: Unsafe strcpy (direct)
    char badCopy[4];
    strcpy(badCopy, "Hello");

    // Safe strcpy
    char safeCopy[10];
    strcpy(safeCopy, "Hello");

    // STR002: Unsafe gets
    char input[20];
    gets(input);

    // STR003: Unsafe sprintf
    char badFormat[5];
    sprintf(badFormat, "%s", "Hello");

    // Safe sprintf
    char safeFormat[10];
    sprintf(safeFormat, "%s", "Hello");

    // STR004: Unsafe strncpy
    char badNcopy[10];
    strncpy(badNcopy, "HelloWorld", 10);

    // Safe strncpy
    char safeNcopy[10];
    strncpy(safeNcopy, "Hello", 5);

    // MEM001: Unsafe memcpy (10 bytes into 5-byte buffer)
    char badMem[5];
    char srcMem[20] = "Sample data";
    memcpy(badMem, srcMem, 10);

    // Safe memcpy (5 bytes into 10-byte buffer)
    char safeMem[10];
    memcpy(safeMem, srcMem, 5);

    // MEM002: Unsafe memmove (16 bytes into 8-byte buffer)
    char badMove[8];
    memmove(badMove, srcMem, 16);

    // Safe memmove
    char safeMove[20];
    memmove(safeMove, srcMem, 5);

    // MEM003: Potential use-after-free
    char* uafPtr = (char*)malloc(16);
    free(uafPtr);
    *uafPtr = 'X';

    // Safe pointer deallocation
    char* safePtr = (char*)malloc(16);
    free(safePtr);
    safePtr = nullptr;

    // INT001: Integer overflow in allocation
    int count = 100;
    char* dynamicBuf = (char*)malloc(count * sizeof(int));
    free(dynamicBuf);

    // Safe allocation with constant size
    char* constBuf = (char*)malloc(10 * sizeof(int));
    free(constBuf);

    // CMD001: Command injection via dynamic/variable argument
    char* dynamicCmd = argv[0];
    system(dynamicCmd);

    // Safe command execution with literal constant
    system("echo Hello");

    // PTR001: Null pointer dereference
    int* nullPtr = nullptr;
    *nullPtr = 42;

    // Safe pointer reassignment before dereference
    int* safeNullPtr = nullptr;
    int stackVal = 99;
    safeNullPtr = &stackVal;
    *safeNullPtr = 100;

    // Interprocedural vulnerability test (STR001)
    char limitedBuf[5];
    copyData(limitedBuf, "Hello World");

    return 0;
}
