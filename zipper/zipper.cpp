#include "zipper.h"
#include "zip.h"
#include "ioapi_mem.h"
#include "defs.h"
#include "tools.h"
#include "CDirEntry.h"
#include "Timestamp.h"

#include <fstream>
#include <stdexcept>

namespace zipper {

struct Zipper::Impl
{
    Zipper* m_outer;
    zipFile m_zf;
    ourmemory_t m_zipmem;
    zlib_filefunc_def m_filefunc;

    Impl(Zipper* outer)
        : m_outer(outer), m_zipmem(), m_filefunc()
    {
        m_zf = NULL;
        m_zipmem.base = NULL;
        //m_filefunc = { 0 };
    }

    ~Impl()
    {
        close();
    }

    bool initFile(const std::string& filename, Zipper::openFlags flags)
    {
#ifdef USEWIN32IOAPI
        zlib_filefunc64_def ffunc = { 0 };
#endif

        int mode = 0;

        /* open the zip file for output */
        if (checkFileExists(filename))
        {
            mode = (flags & Zipper::openFlags::Overwrite) ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP;
        }
        else
        {
            mode = APPEND_STATUS_CREATE;
        }

#ifdef USEWIN32IOAPI
        fill_win32_filefunc64A(&ffunc);
        m_zf = zipOpen2_64(filename.c_str(), mode, NULL, &ffunc);
#else
        m_zf = zipOpen64(filename.c_str(), mode);
#endif

        return NULL != m_zf;
    }

    bool initWithStream(std::iostream& stream)
    {
        m_zipmem.grow = 1;

        stream.seekg(0, std::ios::end);
        std::streampos s = stream.tellg();
        if (s < 0)
        {
            return false;
        }
        size_t size = static_cast<size_t>(s);
        stream.seekg(0);

        if (size > 0)
        {
            if (m_zipmem.base != NULL)
            {
                free(m_zipmem.base);
            }
            m_zipmem.base = reinterpret_cast<char*>(malloc(size * sizeof(char)));
            stream.read(m_zipmem.base, std::streamsize(size));
        }

        fill_memory_filefunc(&m_filefunc, &m_zipmem);

        return initMemory(size > 0 ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP, m_filefunc);
    }

    bool initWithVector(std::vector<unsigned char>& buffer)
    {
        m_zipmem.grow = 1;

        if (!buffer.empty())
        {
            if (m_zipmem.base != NULL)
            {
                free(m_zipmem.base);
            }
            m_zipmem.base = reinterpret_cast<char*>(malloc(buffer.size() * sizeof(char)));
            memcpy(m_zipmem.base, reinterpret_cast<char*>(buffer.data()), buffer.size());
            m_zipmem.size = static_cast<uLong>(buffer.size());
        }

        fill_memory_filefunc(&m_filefunc, &m_zipmem);

        return initMemory(buffer.empty() ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP, m_filefunc);
    }

    bool initMemory(int mode, zlib_filefunc_def& filefunc)
    {
        m_zf = zipOpen3("__notused__", mode, 0, 0, &filefunc);
        return m_zf != NULL;
    }

    bool add(std::istream& input_stream, const std::tm& timestamp,
             const std::string& nameInZip, const std::string& password, int flags)
    {
        if (!m_zf)
            return false;

        int compressLevel = 5; // Zipper::zipFlags::Medium
        bool zip64;
        size_t size_buf = WRITEBUFFERSIZE;
        int err = ZIP_OK;
        unsigned long crcFile = 0;

        zip_fileinfo zi;
        zi.dosDate = 0; // if dos_date == 0, tmz_date is used
        zi.internal_fa = 0; // internal file attributes
        zi.external_fa = 0; // external file attributes
        zi.tmz_date.tm_sec = uInt(timestamp.tm_sec);
        zi.tmz_date.tm_min = uInt(timestamp.tm_min);
        zi.tmz_date.tm_hour = uInt(timestamp.tm_hour);
        zi.tmz_date.tm_mday = uInt(timestamp.tm_mday);
        zi.tmz_date.tm_mon = uInt(timestamp.tm_mon);
        zi.tmz_date.tm_year = uInt(timestamp.tm_year);

        size_t size_read;

        std::vector<char> buff;
        buff.resize(size_buf);

        //
        std::string normaledNameInZip = normalPath(nameInZip);

        if (normaledNameInZip.empty())
            return false;

        flags = flags & ~int(Zipper::zipFlags::SaveHierarchy);
        if (flags == Zipper::zipFlags::Store)
            compressLevel = 0;
        else if (flags == Zipper::zipFlags::Faster)
            compressLevel = 1;
        else if (flags == Zipper::zipFlags::Better)
            compressLevel = 9;

        zip64 = isLargeFile(input_stream);
        if (password.empty())
        {
            err = zipOpenNewFileInZip64(m_zf,
                                        normaledNameInZip.c_str(),
                                        &zi,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        NULL /* comment*/,
                                        (compressLevel != 0) ? Z_DEFLATED : 0,
                                        compressLevel,
                                        zip64);
        }
        else
        {
            getFileCrc(input_stream, buff, crcFile);
            err = zipOpenNewFileInZip3_64(m_zf,
                                          normaledNameInZip.c_str(),
                                          &zi,
                                          NULL,
                                          0,
                                          NULL,
                                          0,
                                          NULL /* comment*/,
                                          (compressLevel != 0) ? Z_DEFLATED : 0,
                                          compressLevel,
                                          0,
                                          /* -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, */
                                          -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
                                          password.c_str(),
                                          crcFile,
                                          zip64);
        }

        if (ZIP_OK == err)
        {
            do
            {
                err = ZIP_OK;
                input_stream.read(buff.data(), std::streamsize(buff.size()));
                size_read = static_cast<size_t>(input_stream.gcount());
                if (size_read < buff.size() && !input_stream.eof() && !input_stream.good())
                {
                    err = ZIP_ERRNO;
                }

                if (size_read > 0)
                {
                    err = zipWriteInFileInZip(this->m_zf, buff.data(), static_cast<unsigned int>(size_read));
                }
            } while ((err == ZIP_OK) && (size_read > 0));
        }
        else
        {
            throw EXCEPTION_CLASS(("Error adding '" + nameInZip + "' to zip").c_str());
        }

        if (ZIP_OK == err)
        {
            err = zipCloseFileInZip(this->m_zf);
        }

        return ZIP_OK == err;
    }

    void close()
    {
        if (m_zf != NULL)
        {
            zipClose(m_zf, NULL);
            m_zf = NULL;
        }

        if (m_zipmem.base && m_zipmem.limit > 0)
        {
            if (m_outer->m_vecbuffer)
            {
                m_outer->m_vecbuffer->resize(m_zipmem.limit);
                m_outer->m_vecbuffer->assign(m_zipmem.base, m_zipmem.base + m_zipmem.limit);
            }
            else if (m_outer->m_obuffer)
            {
                m_outer->m_obuffer->write(m_zipmem.base, std::streamsize(m_zipmem.limit));
            }
        }

        if (m_zipmem.base != NULL)
        {
            free(m_zipmem.base);
            m_zipmem.base = NULL;
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

 Zipper::Zipper()
    : m_obuffer(NULL)
    , m_vecbuffer(NULL)
    , m_open(false)
    , m_impl(new Impl(this))
{
}

Zipper::~Zipper()
{
    close();
    release();
}

bool Zipper::open(const std::string& zipname, const std::string& password, Zipper::openFlags flags)
{
    if (m_open)
        return false;

    m_zipname = zipname;
    m_password = password;

    if (!m_impl->initFile(zipname, flags))
    {
        m_zipname.clear();
        m_password.clear();
        m_open = false;
    }
    else
    {
        m_open = true;
    }
    return m_open;
}

bool Zipper::open(std::iostream& buffer, const std::string& password)
{
    if (m_open)
        return false;

    m_obuffer = &buffer;
    m_password = password;

    if (!m_impl->initWithStream(*m_obuffer))
    {
        m_obuffer = NULL;
        m_password.clear();
        m_open = false;
    }
    else
    {
        m_open = true;
    }
    return m_open;
}

bool Zipper::open(std::vector<unsigned char>& buffer, const std::string& password)
{
    if (m_open)
        return false;

    m_vecbuffer = &buffer;
    m_password = password;

    if (!m_impl->initWithVector(*m_vecbuffer))
    {
        m_vecbuffer = NULL;
        m_password.clear();
        m_open = false;
    }
    else
    {
        m_open = true;
    }
    return m_open;
}

void Zipper::release()
{
    if (m_impl)
    {
        delete m_impl;
        m_impl = NULL;
    }
}

bool Zipper::add(std::istream& source, const std::tm& timestamp, 
    const std::string& nameInZip, zipFlags flags/* = Zipper::zipFlags::Better*/)
{
    return m_impl->add(source, timestamp, nameInZip, m_password, flags);
}

bool Zipper::add(const std::string& sourceFile, const std::tm& timestamp, 
    const std::string& nameInZip, Zipper::zipFlags flags /*= Zipper::zipFlags::Better*/)
{
    std::ifstream input(sourceFile.c_str(), std::ios::binary);

    bool addRes = false;
    try
    {
        addRes = add(input, timestamp, nameInZip, flags);
        input.close();
    }
    catch (std::exception& ex)
    {
        input.close();
        throw EXCEPTION_CLASS(ex);
    }
    
    return addRes;
}

bool Zipper::add(std::istream& source, const std::string& nameInZip, zipFlags flags)
{
    Timestamp time;
    return m_impl->add(source, time.timestamp, nameInZip, m_password, flags);
}

bool Zipper::add(const std::string& sourceFile, const std::string& nameInZip, 
    Zipper::zipFlags flags /*= Zipper::zipFlags::Better*/)
{
    std::ifstream input(sourceFile.c_str(), std::ios::binary);

    bool addRes = false;
    try
    {
        Timestamp time(sourceFile);
        addRes = add(input, time.timestamp, nameInZip, flags);
        input.close();
    } 
    catch (std::exception& ex)
    {
        input.close();
        throw EXCEPTION_CLASS(ex);
    }

    return addRes;
}

bool Zipper::addFolder(const std::string& folderPath, 
    const std::string& folderInZip/* = std::string()*/, Zipper::zipFlags flags)
{
    if (isDirectory(folderPath))
    {
        // std::string folderName = fileNameFromPath(fileOrFolderPath);
        std::vector<std::string> files = filesFromDirectory(folderPath);
        for (int i = 0; i < files.size(); ++i)
        {
            const std::string& fileName = files[i];
            std::string fileNameInZip = fileName.substr(folderPath.size() + 1);
            if (!folderInZip.empty())
                fileNameInZip = folderInZip + CDirEntry::Separator + fileNameInZip;

            std::ifstream input(fileName.c_str(), std::ios::binary);            
            try
            {
                Timestamp time(fileName);
                add(input, time.timestamp, fileNameInZip, flags);
                input.close();
            } 
            catch (std::exception& ex)
            {
                input.close();
                throw EXCEPTION_CLASS(ex);
            }
        }
    }
    else
    {
        return false;

        /*
        Timestamp time(fileOrFolderPath);
        std::ifstream input(fileOrFolderPath.c_str(), std::ios::binary);
        std::string fullFileName;

        if (flags & Zipper::SaveHierarchy)
        {
            fullFileName = fileOrFolderPath;
        }
        else
        {
            fullFileName = fileNameFromPath(fileOrFolderPath);
        }

        try
        {
            add(input, time.timestamp, fullFileName, flags);
            input.close();
        } 
        catch (std::exception& ex)
        {
            input.close();
            throw EXCEPTION_CLASS(ex);
        }
        */

    }

    return true;
}

bool Zipper::reopen(Zipper::openFlags flags)
{
    if (!m_open)
    {
        if (m_vecbuffer)
        {
            if (!m_impl->initWithVector(*m_vecbuffer))
            {
                m_vecbuffer = NULL;
                m_open = false;
                return false;
            }
        }
        else if (m_obuffer)
        {
            if (!m_impl->initWithStream(*m_obuffer))
            {
                m_obuffer = NULL;
                m_open = false;
                return false;
            }
        }
        else if (!m_zipname.empty())
        {
            if (!m_impl->initFile(m_zipname, flags))
            {
                m_open = false;
                return false;
            }
        }
        else
        {
            m_open = false;
            return false;
        }

        m_open = true;
        return true;
    }
    else
    {
        return false;
    }
}

void Zipper::close()
{
    if (m_open)
    {
        m_impl->close();
        m_open = false;
    }
}

} // namespace zipper
