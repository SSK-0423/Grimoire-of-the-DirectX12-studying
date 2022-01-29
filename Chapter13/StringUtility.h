#pragma once
#include <string>
#include <cassert>
#include <Windows.h>

// ファイル名から拡張子を取得する
// @param path 対象のパス文字列
// @return 拡張子
std::string GetExtension(const std::string& path);
///ファイル名から拡張子を取得する(ワイド文字版)
///@param path 対象のパス文字列
///@return 拡張子
std::wstring
GetExtension(const std::wstring& path);

///テクスチャのパスをセパレータ文字で分離する
///@param path 対象のパス文字列
///@param splitter 区切り文字
///@return 分離前後の文字列ペア
std::pair<std::string, std::string>
SplitFileName(const std::string& path, const char splitter = '*');

///string(マルチバイト文字列)からwstring(ワイド文字列)を得る
///@param str マルチバイト文字列
///@return 変換されたワイド文字列
std::wstring
GetWideStringFromString(const std::string& str);