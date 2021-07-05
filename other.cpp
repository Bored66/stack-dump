#include <iostream>

using namespace std;

void printStack();

void other()
{
    printStack();
}

void another(const char*)
{
    cout << "dump stack:" << endl;
    other();
}
