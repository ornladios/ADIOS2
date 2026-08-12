/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adiosDynamicBinder.h"
#include "adiosLog.h"

#include <algorithm> // for copy
#include <iostream>  // for operator<<, stringstream, bas...
#include <iterator>  // for ostream_iterator
#include <sstream>   // for stringstream
#include <stdexcept> // for runtime_error
#include <vector>    // for vector

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace adios2
{
namespace helper
{

namespace
{
#ifdef _WIN32
// Convert a UTF-8 path to a native path usable with the wide Windows APIs:
// forward slashes become backslashes, duplicate slashes are collapsed, and
// the whole path is quoted if it contains a space.
std::wstring ConvertToOutputPath(const std::string &path)
{
    std::string native = path;
    for (auto &ch : native)
    {
        if (ch == '/')
        {
            ch = '\\';
        }
    }
    for (std::string::size_type pos = 1; (pos = native.find("\\\\", pos)) != std::string::npos;)
    {
        native.erase(pos, 1);
    }
    if (native.find(' ') != std::string::npos && native.front() != '"')
    {
        native = "\"" + native + "\"";
    }
    const int wideLength =
        MultiByteToWideChar(CP_UTF8, 0, native.c_str(), static_cast<int>(native.size()), nullptr, 0);
    std::wstring wide(wideLength, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, native.c_str(), static_cast<int>(native.size()), wide.data(),
                        wideLength);
    return wide;
}
#endif
} // end anonymous namespace

struct DynamicBinder::Impl
{
#ifdef _WIN32
    HMODULE m_LibraryHandle = nullptr;
#else
    void *m_LibraryHandle = nullptr;
#endif
};

DynamicBinder::DynamicBinder(std::string libName) : DynamicBinder(libName, "") {}

DynamicBinder::DynamicBinder(std::string libName, std::string libPath) : m_Impl(new Impl)
{
    std::vector<std::string> libPrefixes;
    libPrefixes.emplace_back("");
    libPrefixes.emplace_back("lib");
#ifdef __CYGWIN__
    libPrefixes.emplace_back("cyg");
#endif

    std::vector<std::string> libSuffixes;
    libSuffixes.emplace_back("");
#ifdef __APPLE__
    libSuffixes.emplace_back(".dylib");
    libSuffixes.emplace_back(".so");
#endif
#ifdef __hpux
    libSuffixes.emplace_back(".sl");
#endif
#ifdef __unix__
    libSuffixes.emplace_back(".so");
#endif
#ifdef _WIN32
    libSuffixes.emplace_back(".dll");
#endif

    std::vector<std::string> searchedLibs;
    std::string fileName;

    // Test the various combinations of library names
    for (const std::string &prefix : libPrefixes)
    {
        for (const std::string &suffix : libSuffixes)
        {
            if (!libPath.empty())
            {
                fileName = libPath + "/" + prefix + libName + suffix;
            }
            else
            {
                fileName = prefix + libName + suffix;
            }
#ifdef _WIN32
            // Slashes in fileName is correct for unix-like systems;
            // ConvertToOutputPath() will change slashes for a Windows system
            m_Impl->m_LibraryHandle = LoadLibraryExW(ConvertToOutputPath(fileName).c_str(), nullptr, 0);
#else
            m_Impl->m_LibraryHandle = dlopen(fileName.c_str(), RTLD_LAZY);
#endif
            searchedLibs.push_back(fileName);
            if (m_Impl->m_LibraryHandle)
            {
                break;
            }
        }
        if (m_Impl->m_LibraryHandle)
        {
            break;
        }
    }
    if (!m_Impl->m_LibraryHandle)
    {
        std::stringstream errString;
        errString << "Unable to locate the " << libName << " library; searched for ";
        std::copy(searchedLibs.begin(), searchedLibs.end(),
                  std::ostream_iterator<std::string>(errString, " "));

        helper::Throw<std::runtime_error>("Helper", "adiosDynamicBinder", "DynamicBinder",
                                          errString.str());
    }
}

DynamicBinder::~DynamicBinder()
{
    if (m_Impl->m_LibraryHandle)
    {
#ifdef _WIN32
        FreeLibrary(m_Impl->m_LibraryHandle);
#else
        dlclose(m_Impl->m_LibraryHandle);
#endif
    }
}

DynamicBinder::VoidSymbolPointer DynamicBinder::GetSymbol(std::string symbolName)
{
#ifdef _WIN32
    return reinterpret_cast<VoidSymbolPointer>(GetProcAddress(m_Impl->m_LibraryHandle, symbolName.c_str()));
#else
    // Hack to cast pointer-to-data to pointer-to-function, valid per POSIX dlsym() semantics.
    union
    {
        void *pvoid;
        VoidSymbolPointer psym;
    } result;
    result.pvoid = dlsym(m_Impl->m_LibraryHandle, symbolName.c_str());
    return result.psym;
#endif
}

} // end namespace helper
} // end namespace adios2
