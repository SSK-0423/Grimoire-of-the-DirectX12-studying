#include <vector>
#include <functional>
#include <iostream>

using namespace std;

int main() {
	std::vector<std::function<void(void)>> commandlist;	// ƒRƒ}ƒ“ƒhƒŠƒXƒg‚ğ–Í‚µ‚½‚à‚Ì

	commandlist.push_back([]() { cout << "GPU Set RTV-‡@" << endl; });
	cout << "CPU Set–½—ß-‡A" << endl;

	commandlist.push_back([]() {cout << "GPU Clear RTV-‡B" << endl; });
	cout << "CPU Clear–½—ß-‡C" << endl;

	commandlist.push_back([]() {cout << "GPU Close-‡D" << endl; });
	cout << "CPU Close–½—ß-‡E" << endl;

	cout << endl;

	// ƒRƒ}ƒ“ƒhƒLƒ…[‚ÌExecuteCommand‚ğ–Í‚µ‚½ˆ—
	for (auto& cmd : commandlist) {
		cmd();
	}

	getchar();

	return 0;

}