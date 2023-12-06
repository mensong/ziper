// TestZipper.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <zipper.h>
#include <unzipper.h>

int main()
{
	zipper::Zipper z("E:\\1.zip", "123456");
	z.add(".\\");
	z.close();

	zipper::Unzipper uz("E:\\1.zip", "123456");
	uz.extract("E:\\mensong\\");
	uz.close();

	return 0;
}