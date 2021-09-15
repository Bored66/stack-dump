#include <windows.h>
#include <iostream>

#if defined(__MINGW32__)
#include "BfdInfo.h"
#include <cxxabi.h>
#endif
#include <DbgHelp.h>
#define STACK_LEVEL 62

#if _WIN64
using PLATFORMDWORD = DWORD64;
const char* nameFormat = "%s [0x%I64x] %s\n";
#warning We are 64-bit platform
#else
using PLATFORMDWORD = DWORD;
const char* nameFormat = "%s [0x%lx] %s\n";
#warning We are NOT 64-bit platform
#endif
static void printLine(HANDLE hProcess, PLATFORMDWORD address, bool showErrors = false);
static void getModuleName(HANDLE hProcess, PLATFORMDWORD address, char* moduleBuff, bool showErrors = false);
static void printSymbol(HANDLE hProcess, PLATFORMDWORD address, bool showErrors = false);
static std::string getLastErrorMsgString(const char *prefix) noexcept;
const char *defaultDelimiter = ":";

void printStack()
{
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    if (SymInitialize(process, nullptr, TRUE) == FALSE)
    {
        std::cerr << getLastErrorMsgString("SymInitialize.");
        return;
    }
    SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_DEBUG);

    CONTEXT context = {};
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);

    STACKFRAME frame = {};
#if _WIN64
    PLATFORMDWORD machine = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset = context.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
#else
    PLATFORMDWORD machine = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = context.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrStack.Mode = AddrModeFlat;
#endif
#ifdef __MINGW32__
    BfdInfo bfdu;
#endif
    // using 64 ending for SymFunctionTableAccess/SymGetModuleBase does not change anything on 64 bit
    while (StackWalk(
        machine, process, thread, &frame, &context, nullptr, SymFunctionTableAccess, SymGetModuleBase, nullptr))
    {
        char moduleBuff[MAX_PATH];
        getModuleName(process, frame.AddrPC.Offset, moduleBuff);
#if defined(__MINGW32__)
        bfdu.init(moduleBuff);
        std::string result = bfdu.getSymbolByAddr(&frame.AddrPC.Offset);
        if (not result.empty())
        {
            printf(nameFormat, prefix, frame.AddrPC.Offset, result.c_str());
            if (result.find("?") == std::string::npos)
            {
                continue;
            }
        }
#endif
        printSymbol(process, frame.AddrPC.Offset);
        printLine(process, frame.AddrPC.Offset);
    }
    SymCleanup(process);
}
void getModuleName(HANDLE hProcess, PLATFORMDWORD address, char* moduleBuff, bool showErrors)
{
    PLATFORMDWORD moduleBase = SymGetModuleBase(hProcess, address);
    if (moduleBase && GetModuleFileNameA((HINSTANCE)moduleBase, moduleBuff, MAX_PATH))
    {
        if (showErrors)
        {
            printf("%s:  Module name: %s\n", prefix, moduleBuff);
        }
    }
    else if (showErrors)
    {
        std::cerr << getLastErrorMsgString("GetModuleFileNameA");
    }
}
void printSymbol(HANDLE hProcess, PLATFORMDWORD address, bool showErrors)
{
    constexpr size_t maxNameLength = 254;
    PLATFORMDWORD offset = 0;
    char symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + maxNameLength + 1];
    PIMAGEHLP_SYMBOL symbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
    symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL) + maxNameLength + 1;
    symbol->MaxNameLength = maxNameLength;
    if (SymGetSymFromAddr(hProcess, address, &offset, symbol))
    {
        printf(nameFormat, prefix, symbol->Address, symbol->Name);
    }
    else if (showErrors)
    {
        std::cerr << getLastErrorMsgString("SymGetSymFromAddr");
    }
}
#if _WIN64
void printLine(HANDLE hProcess, PLATFORMDWORD address, bool showErrors)
{
    DWORD dwDisplacement = 0;
    IMAGEHLP_LINE64 line;

    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    if (SymGetLineFromAddr64(hProcess, address, &dwDisplacement, &line))
    {
        if (line.FileName)
        {
            std::cout << prefix << line.FileName 
                                << defaultDelimiter
                                << line.LineNumber << "\n";
            std::cout << std::flush;
        }
    }
    else if (showErrors)
    {
        std::cerr << getLastErrorMsgString("SymFromAddr");
    }
}
#else
void printLine(HANDLE hProcess, PLATFORMDWORD address, bool showErrors)
{
    IMAGEHLP_LINE line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
    DWORD offset_ln = 0;
    if (SymGetLineFromAddr(hProcess, address, &offset_ln, &line))
    {
            std::cout << prefix << line.FileName 
                                << defaultDelimiter
                                << line.LineNumber << "\n";
            std::cout << std::flush;
    }
    else if (showErrors)
    {
        std::cerr << getLastErrorMsgString("SymGetLineFromAddr");
    }
}
#endif
std::string getLastErrorMessage(const char* prefix)
{
    TCHAR msgBuff[256];
    DWORD dw = GetLastError();

    int result = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                               nullptr,
                               dw,
                               MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                               msgBuff,
                               sizeof(msgBuff),
                               nullptr);

    return std::string(msgBuff, msgBuff + result);
}
std::string getLastErrorMsgString(const char* prefix) noexcept
{
    auto msg = getLastErrorMessage(prefix);
    std::string result(prefix);
    result += defaultDelimiter;
    result += msg;
    return result;
}
