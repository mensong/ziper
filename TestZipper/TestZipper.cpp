// TestZipper.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <zipper.h>
#include <unzipper.h>

int main()
{
	zipper::Zipper z("mensong.zip", "123456");
	z.add(".\\");
	z.add("D:\\ActiveTcl.rar", "custom\\ActiveTcl.rar");
	z.close();

	zipper::Unzipper uz("mensong.zip", "123456");
	uz.extract("mensong\\");
	uz.close();

	return 0;
}