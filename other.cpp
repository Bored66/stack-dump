#include <iostream>

using namespace std;

extern void printStack();

void other()
{
    printStack();
}

void another(const char*)
{
    cout << "dump stack:" << endl;
    other();
}
