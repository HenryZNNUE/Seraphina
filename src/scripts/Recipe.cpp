#include <fstream>
#include <string>
#include <thread>

#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

void recipe()
{
	int threads = std::thread::hardware_concurrency();

	std::string path = "recipe.sh";
	std::ofstream r("recipe.sh");

	r << "function recipe {\n";
	r << "	cd ..\n";
	r << "	make -j" << threads << " build ARCH=vnni512\n";
	r << "	echo -e \"\\n\"\n";
	r << "	make -j" << threads << " build ARCH=vnni256\n";
	r << "	echo -e \"\\n\"\n";
	r << "	make -j" << threads << " build ARCH=avx512\n";
	r << "	echo -e \"\\n\"\n";
	r << "	make -j" << threads << " build ARCH=avxvnni\n";
	r << "	echo -e \"\\n\"\n";
	r << "	make -j" << threads << " build ARCH=avx2\n";
	r << "	return 0\n";
	r << "}\n\n";
	r << "recipe";

	r.close();
}

int main()
{
	recipe();
}