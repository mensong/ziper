#include "pch.h"
#include "zip.h"
#include <zipper.h>
#include <unzipper.h>

using namespace zipper;


class ZipImp
    : public IZip
    , public Zipper
{
public:
    ZipImp()
        : Zipper()
    {

    }

	bool open(const char* zipname, const char* password = NULL, IZip::openFlags flags = IZip::openFlags::Overwrite) override
	{
        return Zipper::open(zipname, (password ? password : ""), (Zipper::openFlags)flags);
	}

    virtual bool add(const char* sourceFile, const char* nameInZip, 
        IZip::zipFlags flags = IZip::zipFlags::Better, const std::tm* timestamp = NULL) override
    {
		try
		{
			bool res = false;
			if (timestamp)
				res = Zipper::add(sourceFile, *timestamp, nameInZip, (Zipper::zipFlags)flags);
			else
				res = Zipper::add(sourceFile, nameInZip, (Zipper::zipFlags)flags);
			return res;
		}
		catch (std::exception& ex)
		{
			return false;
		}
    }

    virtual bool addFolder(const char* folderPath, const char* folderInZip = NULL, 
        IZip::zipFlags flags = IZip::zipFlags::Better) override
    {
		try
		{
			bool res = Zipper::addFolder(folderPath, (folderInZip ? folderInZip : ""), (Zipper::zipFlags)flags);
			return res;
		}
		catch (std::exception& ex)
		{
			return false;
		}
    }

    void close() override
    {
        Zipper::close();
    }

    bool reopen(IZip::openFlags flags) override
    {
		try
		{
			Zipper::close();
			Zipper::reopen((Zipper::openFlags)flags);
            return true;
		}
		catch (std::exception& ex)
		{
			return false;
		}
    }

};

class ZipEntryInfoImp
    : public ZipEntryInfo
{
public:
    ZipEntryInfoImp(const ZipEntry& rawEntry)
        : m_rawEntry(rawEntry)
    {

    }

    bool valid() const override
    {
        return m_rawEntry.valid();
    }

    const char* name() const override
    {
        return m_rawEntry.name.c_str();
    }

    const char* timestamp() const override
    {
        return m_rawEntry.timestamp.c_str();
    }

    unsigned long long int compressedSize() const override
    {
        return m_rawEntry.compressedSize;
    }

    unsigned long long int uncompressedSize() const override
    {
        return m_rawEntry.uncompressedSize;
    }

    unsigned long dosdate() const override
    {
        return m_rawEntry.dosdate;
    }

    tm_s unixdate() const override
    {
        ZipEntry::tm_s tm = m_rawEntry.unixdate;
        tm_s resTm;
		resTm.tm_year = tm.tm_year;
		resTm.tm_mon = tm.tm_mon;
		resTm.tm_mday = tm.tm_mday;
		resTm.tm_hour = tm.tm_hour;
		resTm.tm_min = tm.tm_min;
		resTm.tm_sec = tm.tm_sec;
        return resTm;
    }

private:
    ZipEntry m_rawEntry;
};

class UnzipImp
    : public IUnzip
    , public Unzipper
{
public:
    UnzipImp()
        : Unzipper()
    {
        
    }

	bool open(const char* zipname, const char* password = NULL) override
	{
        bool res = Unzipper::open(zipname, (password ? password : ""));

        if (res)
        {
			std::vector<ZipEntry> rawEntries = this->entries();
			for (size_t i = 0; i < rawEntries.size(); ++i)
			{
				m_cacheZipEntryInfo.insert(std::make_pair(
					rawEntries[i].name.c_str(), ZipEntryInfoImp(rawEntries[i])
				));
			}
        }

        return res;
	}

    bool extractAll(const char* targetFolder) override
    {
        try
        {
			return Unzipper::extract(targetFolder);
        }
        catch (std::exception& ex)
        {
            return false;
        }
    }

    bool extractAEntry(const char* nameInZip, const char* targetPath) override
    {
		try
		{
            return Unzipper::extractEntry(nameInZip, targetPath);
		}
		catch (std::exception& ex)
		{
			return false;
		}
    }

    unsigned char* extractToMemory(const char* nameInZip) override
    {
		try
		{
			std::vector<unsigned char> vecData;
			if (!Unzipper::extractEntryToMemory(nameInZip, vecData) || vecData.size() == 0)
				return NULL;
			unsigned char* pResData = new unsigned char[vecData.size()];
			memcpy_s(pResData, vecData.size(), &vecData[0], vecData.size());
			return pResData;
		}
		catch (std::exception& ex)
		{
			return NULL;
		}
    }

    const ZipEntryInfo* getZipEntryInfo(const char* nameInZip) override
    {
         auto itFinder = m_cacheZipEntryInfo.find(nameInZip);
         if (itFinder == m_cacheZipEntryInfo.end())
             return NULL;
         return &(itFinder->second);
    }

    int getZipEntryCount() override
    {
        return (int)m_cacheZipEntryInfo.size();
    }


    const ZipEntryInfo* getZipEntryInfo(int idx) override
    {
        if (idx >= m_cacheZipEntryInfo.size())
            return NULL;
        auto it = m_cacheZipEntryInfo.begin();
        for (int i = 0; i < idx; i++)
        {
            ++it;
        }
        return &it->second;
    }


    void close() override
    {
        m_cacheZipEntryInfo.clear();
        Unzipper::close();
    }

private:
    std::map<std::string, ZipEntryInfoImp> m_cacheZipEntryInfo;
};

ZIP_API IZip* CreateZip()
{
	ZipImp* imp = new ZipImp();
	return imp;
}

ZIP_API void ReleaseZip(IZip* z)
{
    delete z;
}

ZIP_API IUnzip* CreateUnzip()
{
	UnzipImp* imp = new UnzipImp();
	return imp;
}

ZIP_API void ReleaseUnzip(IUnzip* uz)
{
    delete uz;
}

ZIP_API void ReleaseZBuffer(void* zbuff)
{
    delete[] zbuff;
}
