#pragma once
#ifndef _AFX
#include <windows.h>
#endif
#include <string>
#include <stdint.h>

#ifdef ZIP_EXPORTS
#define ZIP_API extern "C" __declspec(dllexport)
#else
#define ZIP_API extern "C" __declspec(dllimport)
#endif

class IZip
{
public:

    // -------------------------------------------------------------------------
        //! \brief Archive flags.
        // -------------------------------------------------------------------------
    enum openFlags
    {
        //! \brief Minizip options/params: -o  Overwrite existing file.zip
        Overwrite = 0x01,
        //! \brief Minizip options/params: -a  Append to existing file.zip
        Append = 0x02,
        //! \brief Minizip options/params: -j  Exclude path. store only the file name.
        //TODO NoPaths = 0x20,
    };

    // -------------------------------------------------------------------------
    //! \brief Compression options for files.
    // -------------------------------------------------------------------------
    enum zipFlags
    {
        //! \brief Minizip options/params: -0  Store only
        Store = 0x00,
        //! \brief Minizip options/params: -1  Compress faster
        Faster = 0x01,
        //! \brief Minizip options/params: -5  Compress faster
        Medium = 0x05,
        //! \brief Minizip options/params: -9  Compress better
        Better = 0x09,
        //! \brief ???
        SaveHierarchy = 0x40
    };

public:
	virtual ~IZip() {}

	virtual bool open(const char* zipname, const char* password = NULL,
		IZip::openFlags flags = IZip::openFlags::Overwrite) = 0;

	virtual bool add(const char* sourceFile, const char* nameInZip,
		IZip::zipFlags flags = IZip::zipFlags::Better, const std::tm* timestamp = NULL) = 0;

	virtual bool addFolder(const char* folderPath, const char* folderInZip = NULL,
		IZip::zipFlags flags = IZip::zipFlags::Better) = 0;

	virtual void close() = 0;

	virtual bool reopen(IZip::openFlags flags) = 0;

};

class ZipEntryInfo
{
public:
	typedef struct
	{
		unsigned int tm_sec;
		unsigned int tm_min;
		unsigned int tm_hour;
		unsigned int tm_mday;
		unsigned int tm_mon;
		unsigned int tm_year;
	} tm_s;

public:
	virtual ~ZipEntryInfo() {}

	virtual bool valid() const = 0;

	virtual const char* name() const = 0;
	virtual const char* timestamp() const = 0;
	virtual unsigned long long int compressedSize() const = 0;
	virtual unsigned long long int uncompressedSize() const = 0;
	virtual unsigned long dosdate() const = 0;
	virtual tm_s unixdate() const = 0;
};

class IUnzip
{
public:
	virtual ~IUnzip() {}

	virtual bool open(const char* zipname, const char* password = NULL) = 0;

	virtual bool extractAll(const char* targetFolder) = 0;

	virtual bool extractAEntry(const char* nameInZip, const char* targetPath) = 0;

	virtual unsigned char* extractToMemory(const char* nameInZip) = 0;

	virtual const ZipEntryInfo* getZipEntryInfo(const char* nameInZip) = 0;

	virtual int getZipEntryCount() = 0;
	virtual const ZipEntryInfo* getZipEntryInfo(int idx) = 0;

	virtual void close() = 0;
};


ZIP_API IZip* CreateZip();
ZIP_API void ReleaseZip(IZip* z);
ZIP_API IUnzip* CreateUnzip();
ZIP_API void ReleaseUnzip(IUnzip* uz);
ZIP_API void ReleaseZBuffer(void* zbuff);


class zip
{
public:
#define DEF_PROC(name) \
	decltype(::name)* name

#define SET_PROC(hDll, name) \
	this->name = (decltype(::name)*)::GetProcAddress(hDll, #name)

public:
	zip()
	{
		hDll = LoadLibraryFromCurrentDir("zip.dll");
		if (!hDll)
			return;

		SET_PROC(hDll, CreateZip);
		SET_PROC(hDll, ReleaseZip);
		SET_PROC(hDll, CreateUnzip);
		SET_PROC(hDll, ReleaseUnzip);
		SET_PROC(hDll, ReleaseZBuffer);
	}


	DEF_PROC(CreateZip);
	DEF_PROC(ReleaseZip);
	DEF_PROC(CreateUnzip);
	DEF_PROC(ReleaseUnzip);
	DEF_PROC(ReleaseZBuffer);


public:
	static zip& Ins()
	{
		static zip s_ins;
		return s_ins;
	}

	static HMODULE LoadLibraryFromCurrentDir(const char* dllName)
	{
		char selfPath[MAX_PATH];
		MEMORY_BASIC_INFORMATION mbi;
		HMODULE hModule = ((::VirtualQuery(LoadLibraryFromCurrentDir, &mbi, sizeof(mbi)) != 0) ? 
			(HMODULE)mbi.AllocationBase : NULL);
		::GetModuleFileNameA(hModule, selfPath, MAX_PATH);
		std::string moduleDir(selfPath);
		size_t idx = moduleDir.find_last_of('\\');
		moduleDir = moduleDir.substr(0, idx);
		std::string modulePath = moduleDir + "\\" + dllName;
		char curDir[MAX_PATH];
		::GetCurrentDirectoryA(MAX_PATH, curDir);
		::SetCurrentDirectoryA(moduleDir.c_str());
		HMODULE hDll = LoadLibraryA(modulePath.c_str());
		::SetCurrentDirectoryA(curDir);
		if (!hDll)
		{
			DWORD err = ::GetLastError();
			char buf[10];
			sprintf_s(buf, "%u", err);
			::MessageBoxA(NULL, ("找不到" + modulePath + "模块:" + buf).c_str(),
				"找不到模块", MB_OK | MB_ICONERROR);
		}
		return hDll;
	}
	~zip()
	{
		if (hDll)
		{
			FreeLibrary(hDll);
			hDll = NULL;
		}
	}

private:
	HMODULE hDll;
};

