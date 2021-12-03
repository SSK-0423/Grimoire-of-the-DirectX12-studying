#include <vector>
#include <functional>
#include <iostream>

using namespace std;

int main() {
	std::vector<std::function<void(void)>> commandlist;	// �R�}���h���X�g��͂�������

	commandlist.push_back([]() { cout << "GPU Set RTV-�@" << endl; });
	cout << "CPU Set����-�A" << endl;

	commandlist.push_back([]() {cout << "GPU Clear RTV-�B" << endl; });
	cout << "CPU Clear����-�C" << endl;

	commandlist.push_back([]() {cout << "GPU Close-�D" << endl; });
	cout << "CPU Close����-�E" << endl;

	cout << endl;

	// �R�}���h�L���[��ExecuteCommand��͂�������
	for (auto& cmd : commandlist) {
		cmd();
	}

	getchar();

	return 0;

}