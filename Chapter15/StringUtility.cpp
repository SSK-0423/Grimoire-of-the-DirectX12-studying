#pragma once
#include "StringUtility.h"

// ファイル名から拡張子を取得する
// @param path 対象のパス文字列
// @return 拡張子
std::string GetExtension(const std::string& path) {
	size_t idx = path.rfind('.');
	return path.substr(idx + 1, path.length() - idx - 1);
}
///ファイル名から拡張子を取得する(ワイド文字版)
///@param path 対象のパス文字列
///@return 拡張子
std::wstring
GetExtension(const std::wstring& path) {
	int idx = path.rfind(L'.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

///テクスチャのパスをセパレータ文字で分離する
///@param path 対象のパス文字列
///@param splitter 区切り文字
///@return 分離前後の文字列ペア
std::pair<std::string, std::string>
SplitFileName(const std::string& path, const char splitter) {
	int idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);
	return ret;
}

///string(マルチバイト文字列)からwstring(ワイド文字列)を得る
///@param str マルチバイト文字列
///@return 変換されたワイド文字列
std::wstring
GetWideStringFromString(const std::string& str) {
	//呼び出し1回目(文字列数を得る)
	auto num1 = MultiByteToWideChar(CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), -1, nullptr, 0);

	std::wstring wstr;//stringのwchar_t版
	wstr.resize(num1);//得られた文字列数でリサイズ

	//呼び出し2回目(確保済みのwstrに変換文字列をコピー)
	auto num2 = MultiByteToWideChar(CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), -1, &wstr[0], num1);

	assert(num1 == num2);//一応チェック
	return wstr;
}