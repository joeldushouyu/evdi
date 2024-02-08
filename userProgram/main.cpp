#include <fstream>
#include <iostream>
#include <string>

#include "Card.h"
#include "Buffer.h"

//TODO: load from file in the future
#define _FullHDAreaLimit 1920 * 1080
#define _4KAreaLimit 3840 * 2160
#define _60Hz 60



// another test
#define _30HZ 30
std::string readEdidInfo(const std::string &fileName)
{
	std::ifstream in(fileName, std::ios_base::binary);
	//https://stackoverflow.com/questions/18398167/how-to-copy-a-txt-file-to-a-char-array-in-c
	std::string contents((std::istreambuf_iterator<char>(in)),
			     std::istreambuf_iterator<char>());
	return contents;
}
int main()
{
	std::cout << "Hello, Unix!" << std::endl;

	Card testGraphicCard;
	std::string edidString = readEdidInfo("4K60HzTest.edid");

	testGraphicCard.connect(edidString.data(), edidString.size(),
				_FullHDAreaLimit, _FullHDAreaLimit * _60Hz);

	while (true) {
		// possible correct way
		testGraphicCard.handle_events(int((1 / _30HZ) * 1000)); // in unit of ms
		//testGraphicCard.handle_events(1000);
	}

	testGraphicCard.disconnect();
	testGraphicCard.close();
	return 0;
}
