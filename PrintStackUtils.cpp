#include "BfdInfo.h"
#include <string>

#include <iostream>

#include <execinfo.h>
#include <stdlib.h>
#include <unistd.h>

#define STACK_LEVEL 62

static std::string extractString(const std::string& line, unsigned char startCh, unsigned char endCh);
static void addressSymbolInfoToDescr(std::string& descr, const std::string& address, bool& gotFileLine, bool& gotName);
static void addSymbolDescr(std::string& descr, const std::string& symbolStr, bool& gotFileLine, bool& gotName);
static std::string lookupSymbol(std::string addressStr);

void printStack()
{
    void* buffer[STACK_LEVEL];
    char** strings;
    int num = backtrace(buffer, STACK_LEVEL);
    strings = backtrace_symbols(buffer, num);
    if (strings == nullptr)
    {
        std::cerr << "No stack info was found";
        std::cerr << std::endl;
        return;
    }
    for (int j = 0; j < num; j++)
    {
        std::string line = strings[j], descr;
        bool gotName = false;
        bool gotFileLine = false;
        auto mangledName = extractString(line, '(', '+');
        if (not mangledName.empty() && mangledName.at(0) != '+')
        {
            gotName = true;
            auto demangledName = demangleName(mangledName);
            descr += " ";
            descr += demangledName.empty() ? mangledName : demangledName;
        }
        auto offset = extractString(line, '+', ')');
        if (not offset.empty())
        {
            addressSymbolInfoToDescr(descr, offset, gotFileLine, gotName);
        }
        auto address = extractString(line, '[', ']');
        if (not address.empty())
        {
            addressSymbolInfoToDescr(descr, address, gotFileLine, gotName);
        }
        std::cerr << strings[j] << descr;
        std::cerr << std::endl;
    }
    free(strings);
}
std::string extractString(const std::string& line, unsigned char startCh, unsigned char endCh)
{
    auto posSt = line.find(startCh);
    if (posSt != std::string::npos)
    {
        auto posEnd = line.find(endCh);
        return line.substr(posSt + 1, posEnd - posSt - 1);
    }
    return {};
}
std::string lookupSymbol(std::string addressStr)
{
    size_t pos;
    auto address = std::stoull(addressStr, &pos, 16);
    static BfdInfo bfdInfo;
    return bfdInfo.lookupSymbol(reinterpret_cast<void*>(address));
}
void addressSymbolInfoToDescr(std::string& descr, const std::string& address, bool& gotFileLine, bool& gotName)
{
    std::string symbol = lookupSymbol(address);
    if (not symbol.empty())
    {
        addSymbolDescr(descr, symbol, gotFileLine, gotName);
    }
}
void addSymbolDescr(std::string& descr, const std::string& symbolStr, bool& gotFileLine, bool& gotName)
{
    if (not gotFileLine)
    {
        auto pos = symbolStr.find_first_of(':');
        auto endPos = symbolStr.find_first_of(' ');
        if (pos != std::string::npos && endPos != std::string::npos)
        {
            auto fileLine = symbolStr.substr(0, endPos);
            descr += " ";
            descr += fileLine;
            if (fileLine.find("?:") == std::string::npos)
            {
                gotFileLine = true;
            }
        }
    }
    if (not gotName)
    {
        auto pos = symbolStr.find_first_of(' ');
        if (pos != std::string::npos)
        {
            auto demangledName = demangleName(symbolStr.substr(pos + 1));
            if (not demangledName.empty())
            {
                descr += " ";
                descr += demangledName;
                gotName = true;
            }
        }
    }
}
