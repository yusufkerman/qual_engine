#pragma once

/**
 * @file path_utils.h
 * @brief UTF-8 uyumlu, çok baytlı (multibyte) ve Türkçe karakter destekli dosya yolu dönüştürücüleri.
 * Windows platformunda UTF-8 bayt dizilerini kayıpsız olarak wchar_t / std::filesystem::path'e dönüştürür.
 */

#include <filesystem>
#include <string>
#include <string_view>
#include <fstream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace qual {

    /**
     * @brief UTF-8 ile kodlanmış bir std::string'i işletim sistemi için güvenli std::filesystem::path'e çevirir.
     * Windows'ta CP_UTF8 -> wchar_t dönüşümü yaparak Türkçe ve yabancı karakterlerin bozulmasını (mojibake) önler.
     */
    inline std::filesystem::path path_from_utf8(std::string_view utf8_str)
    {
        if (utf8_str.empty())
            return {};

#if defined(_WIN32)
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), nullptr, 0);
        if (size_needed <= 0)
        {
            return std::filesystem::path(reinterpret_cast<const char8_t*>(utf8_str.data()),
                                         reinterpret_cast<const char8_t*>(utf8_str.data() + utf8_str.size()));
        }
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), &wstr[0], size_needed);
        return std::filesystem::path(wstr);
#else
        return std::filesystem::path(utf8_str);
#endif
    }

    /**
     * @brief std::filesystem::path nesnesini UTF-8 kodlu std::string'e çevirir.
     */
    inline std::string path_to_utf8(const std::filesystem::path& p)
    {
#if defined(_WIN32)
        const std::wstring& wstr = p.native();
        if (wstr.empty())
            return {};
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        if (size_needed <= 0)
            return p.string();
        std::string str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &str[0], size_needed, nullptr, nullptr);
        return str;
#else
        return p.string();
#endif
    }

    /**
     * @brief UTF-8 destekli güvenli dosya okuma akışı (std::ifstream) açar.
     */
    inline std::ifstream open_ifstream(const std::string& utf8_path, std::ios_base::openmode mode = std::ios_base::in)
    {
        return std::ifstream(path_from_utf8(utf8_path), mode);
    }

    /**
     * @brief UTF-8 destekli güvenli dosya yazma akışı (std::ofstream) açar.
     */
    inline std::ofstream open_ofstream(const std::string& utf8_path, std::ios_base::openmode mode = std::ios_base::out)
    {
        return std::ofstream(path_from_utf8(utf8_path), mode);
    }

} // namespace qual
