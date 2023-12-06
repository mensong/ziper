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
    ZipImp(const char* zipname, const char* password, IZip::openFlags flags)
        : Zipper(std::string(zipname), std::string(password), (Zipper::openFlags)flags)
    {

    }

    virtual bool add(const char* sourceFile, const char* nameInZip, 
        IZip::zipFlags flags = IZip::zipFlags::Better, const std::tm* timestamp = NULL) override
    {
        //std::tm atimestamp = ;
        //Zipper::add(sourceFile, *timestamp, nameInZip,)
        return false;
    }
    virtual bool addFolder(const char* folderPath, const char* folderInZip = NULL, 
        IZip::zipFlags flags = IZip::zipFlags::Better) override
    {
        return false;
    }
};

class UnzipImp
    : public IUnzip
    , public Unzipper
{
public:
    UnzipImp(const char* zipname, const char* password)
        : Unzipper(std::string(zipname), std::string(password))
    {

    }
};

ZIP_API IZip* CreateZip(const char* zipname, const char* password, IZip::openFlags flags)
{
    ZipImp* imp = new ZipImp(zipname, password, flags);
    return imp;
}

ZIP_API void ReleaseZip(IZip* z)
{
    delete z;
}

ZIP_API IUnzip* CreateUnzip(const char* zipname, const char* password)
{
    UnzipImp* imp = new UnzipImp(zipname, password);
    return imp;
}

ZIP_API void ReleaseUnzip(IUnzip* uz)
{
    delete uz;
}
