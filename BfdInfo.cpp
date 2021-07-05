#include "BfdInfo.h"
//#include "utils/Logger.h"
#include <string>
#include <sstream>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <link.h>
#endif
#if defined(__MINGW32__) || defined(__GLIBC__)
#include <cxxabi.h>
#endif

#ifndef bfd_get_section_flags
#define bfd_get_section_flags(bfd, ptr) ((void)bfd, (ptr)->flags)
#endif
#ifndef bfd_section_size
#define bfd_section_size(bfd, ptr) ((ptr)->size)
#endif
#ifndef bfd_section_vma
#define bfd_section_vma(bfd, ptr) ((void)bfd, (ptr)->vma)
#endif

BfdInfo::BfdInfo()
    : _bfd(nullptr)
    , _sec(nullptr)
    , _symbolTable(nullptr)
{
#ifdef _WIN32
    TCHAR moduleName[_MAX_PATH];
    GetModuleFileName(nullptr, moduleName, sizeof(moduleName));
    _moduleName = bpl::String::fromWstring(moduleName).toStdString(); // TODO: use bpl::String for _moduleName?
    init();
#else

#endif
}

void BfdInfo::init(const char* fileName)
{
    if (_moduleName == fileName)
    {
        return;
    }
    _moduleName = fileName;
    if (_symbolTable)
    {
        std::free(_symbolTable);
        _symbolTable = nullptr;
    }
    init();
}
#ifndef _WIN32
std::string BfdInfo::lookupSymbol(void* addrVal)
{
    FileMatch match = {addrVal, nullptr, nullptr};
    dl_iterate_phdr(BfdInfo::findMatchingFile, &match);

    // adjust the address in the global space of your binary to an
    // offset in the relevant library
    bfd_vma addr = bfd_vma(addrVal);
    addr -= bfd_vma(match.base);

    // lookup the symbol in a module
    const char* moduleName = match.file && strlen(match.file) ? match.file : "/proc/self/exe";
    if (_moduleName != moduleName)
    {
        init(moduleName);
    }
    return getSymbolByAddr(&addr);
}

int BfdInfo::findMatchingFile(dl_phdr_info* info, size_t, void* data)
{
    FileMatch* match = reinterpret_cast<FileMatch*>(data);

    for (unsigned int i = 0; i < info->dlpi_phnum; i++)
    {
        const ElfW(Phdr)& phdr = info->dlpi_phdr[i];

        if (phdr.p_type == PT_LOAD)
        {
            ElfW(Addr) vaddr = phdr.p_vaddr + info->dlpi_addr;
            ElfW(Addr) maddr = ElfW(Addr)(match->address);
            if ((maddr >= vaddr) && (maddr < vaddr + phdr.p_memsz))
            {
                match->file = info->dlpi_name;
                match->base = reinterpret_cast<void*>(info->dlpi_addr);
                return 1;
            }
        }
    }
    return 0;
}
#endif
asymbol** BfdInfo::readSymbolTable(bfd* bfd, const char* fileName)
{
    if (!(bfd_get_file_flags(bfd) & HAS_SYMS))
    {
        if (fileName[0] == '/') // we ignore win modules
        {
            //mgtLogError("Error bfd file \"%s\" flagged as having no symbols.\n", fileName);
        }
        return nullptr;
    }

    asymbol** syms;
    unsigned int size;

    long symcount = bfd_read_minisymbols(bfd, false, reinterpret_cast<void**>(&syms), &size);
    if (symcount == 0)
    {
        symcount = bfd_read_minisymbols(bfd, true, reinterpret_cast<void**>(&syms), &size);
    }
    if (symcount < 0)
    {
        //mgtLogError("Error bfd file \"%s\", found no symbols.\n", fileName);
        return nullptr;
    }

    return syms;
}

bfd* BfdInfo::openBfdFile(const char* fileName)
{
    bfd* abfd = bfd_openr(fileName, nullptr);
    if (!abfd)
    {
        //mgtLogError("Error opening bfd file \"%s\"\n", fileName);
        return {};
    }

    if (bfd_check_format(abfd, bfd_archive))
    {
        //mgtLogError("Cannot get addresses from archive \"%s\"\n", fileName);
        bfd_close(abfd);
        return {};
    }

    char** matching = nullptr;
    if (!bfd_check_format_matches(abfd, bfd_object, &matching))
    {
        //mgtLogError("Format does not match for archive \"%s\"\n", fileName);
        bfd_close(abfd);
        return {};
    }
    return abfd;
}

std::string BfdInfo::getSymbolByAddr(bfd_vma* addr)
{
    std::stringstream result;
    FileLineDesc desc(_symbolTable, *addr);

    bfd_map_over_sections(_bfd, findAddressInSection, reinterpret_cast<void*>(&desc));

    if (!desc.found)
    {
        result << std::hex << *addr;
    }
    else
    {
        const char* name = desc.functionname;
        if (name == nullptr || *name == '\0')
        {
            name = "??";
        }
#if defined(__MINGW32__)
        else
        {
            std::string demangleName(const std::string& mangledName);
            auto demangledName = demangleName(desc.functionname);
            name = demangledName.empty() ? "??" : demangledName.c_str();
        }
#endif
        if (desc.filename != nullptr)
        {
            char* lastDelimiter = strrchr(desc.filename, '/');
            if (lastDelimiter != nullptr)
            {
                desc.filename = lastDelimiter + 1;
            }
        }
        result << (desc.filename ? desc.filename : "??") << ":" << desc.line << " " << name;
    }
    return result.str();
}
void BfdInfo::findAddressInSection(bfd* bfd, asection* section, void* data)
{
    FileLineDesc* desc = reinterpret_cast<FileLineDesc*>(data);
    if (desc)
    {
        desc->findAddressInSection(bfd, section);
    }
}
void BfdInfo::init()
{
#ifdef _WIN32
    mgtTrace() << prefix << " Module name: " << _moduleName;
#endif
    bfd_init();
    if ((_bfd = openBfdFile(_moduleName.c_str())))
    {
        if ((_symbolTable = readSymbolTable(_bfd, _moduleName.c_str())))
        {
            return;
        }
    }
}

BfdInfo::~BfdInfo()
{
    if (_symbolTable)
    {
        std::free(_symbolTable);
    }
    bfd_close(_bfd);
}

void BfdInfo::FileLineDesc::findAddressInSection(bfd* bfd, asection* section)
{
    if (found)
    {
        return;
    }
    if ((bfd_get_section_flags(bfd, section) & SEC_ALLOC) == 0)
    {
        return;
    }
    bfd_vma vma = bfd_section_vma(bfd, section);
    if (pc < vma)
    {
        return;
    }
    bfd_size_type size = bfd_section_size(bfd, section);
    if (pc >= (vma + size))
    {
        return;
    }
    found = bfd_find_nearest_line(bfd,
                                  section,
                                  syms,
                                  (pc - vma),
                                  const_cast<const char**>(reinterpret_cast<char**>(&filename)),
                                  const_cast<const char**>(reinterpret_cast<char**>(&functionname)),
                                  &line);
}

std::string demangleName(const std::string& mangledName)
{
#if defined(__MINGW32__) || defined(__GLIBC__)
    int status;
    if (char* result = abi::__cxa_demangle(mangledName.c_str(), nullptr, nullptr, &status))
    {
        return {result};
    }
#endif
    return {};
}
