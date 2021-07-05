#pragma once

#include <string>

#if defined(__MINGW32__)
#define PACKAGE 1
#define PACKAGE_VERSION 1
#include <bfd.h>
#elif !defined PACKAGE && !defined PACKAGE_VERSION
#define PACKAGE 1
#define PACKAGE_VERSION 1
#include <bfd.h>
struct dl_phdr_info;
#elif !defined(MGT_OS_WINDOWS)
#include <bfd.h>
struct dl_phdr_info;
#endif

class BfdInfo
{
public:
    BfdInfo(void);
    ~BfdInfo(void);
    BfdInfo(const BfdInfo&) = delete;
    BfdInfo& operator=(const BfdInfo&) = delete;

    void init(const char* fileName);

    bfd* getBfd()
    {
        return _bfd;
    }
    asymbol** getSymbolTable()
    {
        return _symbolTable;
    }

#ifndef _WIN32
    std::string lookupSymbol(void* addrVal);
    static int findMatchingFile(struct dl_phdr_info* info, size_t, void* data);
#endif
    asymbol** readSymbolTable(bfd* bfd, const char* fileName);
    bfd* openBfdFile(const char* fileName);
    std::string getSymbolByAddr(bfd_vma* addr);

private:
    struct FileLineDesc
    {
        FileLineDesc(asymbol** syms, bfd_vma pc)
            : pc(pc)
            , found(false)
            , syms(syms)
        {
        }

        void findAddressInSection(bfd* bfd, asection* section);
        bfd_vma pc;
        char* filename;
        char* functionname;
        unsigned int line;
        bool found;
        asymbol** syms;
    };
    static void findAddressInSection(bfd* bfd, asection* section, void* data);
    void init();
    bfd* _bfd;
    asection* _sec;
    asymbol** _symbolTable;
    std::string _moduleName;
    struct FileMatch
    {
        void* address;
        const char* file;
        void* base;
    };
};

std::string demangleName(const std::string& mangledName);

#ifdef _WIN32
constexpr char prefix[] = "TRACE:*** ";
#endif
