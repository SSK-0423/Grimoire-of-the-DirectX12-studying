#pragma once
#include <string>
#include <cassert>
#include <Windows.h>

// �t�@�C��������g���q���擾����
// @param path �Ώۂ̃p�X������
// @return �g���q
std::string GetExtension(const std::string& path);
///�t�@�C��������g���q���擾����(���C�h������)
///@param path �Ώۂ̃p�X������
///@return �g���q
std::wstring
GetExtension(const std::wstring& path);

///�e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������
///@param path �Ώۂ̃p�X������
///@param splitter ��؂蕶��
///@return �����O��̕�����y�A
std::pair<std::string, std::string>
SplitFileName(const std::string& path, const char splitter = '*');

///string(�}���`�o�C�g������)����wstring(���C�h������)�𓾂�
///@param str �}���`�o�C�g������
///@return �ϊ����ꂽ���C�h������
std::wstring
GetWideStringFromString(const std::string& str);