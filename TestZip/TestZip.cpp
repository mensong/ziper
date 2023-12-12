// TestZip.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "..\zip\zip.h"

int main()
{
    IZip* z = zip::Ins().CreateZip();
	if (!z->open("mensong.zip", "123456", IZip::Overwrite))
	{
		zip::Ins().ReleaseZip(z);
		return 1;
	}
	z->addFolder("tmp//");
	z->add("..\\//zipper.sln", "sln1\\zipper.sln");
	z->reopen(IZip::Append);
	z->addFolder("tmp//", "tmp2\\/");
	z->add("..\\//zipper.sln", "sln2\\zipper.sln");
	z->close();
	zip::Ins().ReleaseZip(z);

	IUnzip* uz = zip::Ins().CreateUnzip();
	if (!uz->open("mensong.zip", "123456"))
	{
		zip::Ins().ReleaseUnzip(uz);
		return 1;
	}
	uz->extractAll("mensong");

	std::cout << "=========================" << std::endl;
	int nCount = uz->getZipEntryCount();
	for (int i = 0; i < nCount; i++)
	{
		const ZipEntryInfo* zi = uz->getZipEntryInfo(i);
		if (!zi)
			continue;
		
		std::cout << "name:" << zi->name() << std::endl;
		std::cout << "timestamp:" << zi->timestamp() << std::endl;
		std::cout << "compressedSize:" << zi->compressedSize() << std::endl;
		std::cout << "uncompressedSize:" << zi->uncompressedSize() << std::endl;
		std::cout << "dosdate:" << zi->dosdate() << std::endl;
		auto date = zi->unixdate();
		std::cout << "unixdate:" 
			<< date.tm_year << "/" << date.tm_mon << "/" << date.tm_mday 
			<< " " << date.tm_hour << ":" << date.tm_min << ":" << date.tm_sec << std::endl;

		std::cout << std::endl;
	}

	zip::Ins().ReleaseUnzip(uz);
}
