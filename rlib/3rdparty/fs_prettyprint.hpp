#ifndef RLIB_FS_STATUS_PRETTYPRINT
#define RLIB_FS_STATUS_PRETTYPRINT

#include <filesystem>
#include <iomanip>
#include <ostream>
#include <string>

#include <rlib/require/cxx17>

namespace rlib::prettyprint {

namespace impl {
inline std::string ftypeToString(const std::filesystem::file_type &t) {
    switch(t) {
        case std::filesystem::file_type::regular: return "regular file";
        case std::filesystem::file_type::directory: return "directory";
        case std::filesystem::file_type::symlink: return "symlink";
        case std::filesystem::file_type::block: return "block file";
        case std::filesystem::file_type::character: return "character file";
        case std::filesystem::file_type::fifo: return "fifo";
        case std::filesystem::file_type::socket: return "socket";
        case std::filesystem::file_type::unknown: return "unknown";
        case std::filesystem::file_type::none: return "none";
        case std::filesystem::file_type::not_found: return "not_found";
    }
    return "";
}

inline std::string fsizeToString(const size_t fsize) {
    if(fsize < 1024)
        return std::to_string(fsize);
    const auto KiB = (double)fsize / 1024.;
    if(KiB < 1024) return std::to_string(KiB) + "Ki";
    const auto MiB = KiB / 1024.;
    if(MiB < 1024) return std::to_string(MiB) + "Mi";
    const auto GiB = MiB / 1024.;
    if(GiB < 1024) return std::to_string(GiB) + "Gi";
    const auto TiB = GiB / 1024.;
    if(TiB < 1024) return std::to_string(TiB) + "Ti";
    const auto PiB = TiB / 1024.;
    return std::to_string(PiB) + "Pi";
}

template <typename T>
std::string try_show_file_size(const T &arg) {
    try {
        return fsizeToString(std::filesystem::file_size(arg));
    }
    catch(...) {
        return "-";
    }
}
}

template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>&
operator<< (std::basic_ostream<CharT, Traits> &os, const std::filesystem::perms& p) {
    return os 
        << ((p & std::filesystem::perms::owner_read) != std::filesystem::perms::none ? "r" : "-")
        << ((p & std::filesystem::perms::owner_write) != std::filesystem::perms::none ? "w" : "-")
        << ((p & std::filesystem::perms::owner_exec) != std::filesystem::perms::none ? "x" : "-")
        << ((p & std::filesystem::perms::group_read) != std::filesystem::perms::none ? "r" : "-")
        << ((p & std::filesystem::perms::group_write) != std::filesystem::perms::none ? "w" : "-")
        << ((p & std::filesystem::perms::group_exec) != std::filesystem::perms::none ? "x" : "-")
        << ((p & std::filesystem::perms::others_read) != std::filesystem::perms::none ? "r" : "-")
        << ((p & std::filesystem::perms::others_write) != std::filesystem::perms::none ? "w" : "-")
        << ((p & std::filesystem::perms::others_exec) != std::filesystem::perms::none ? "x" : "-")
    ;
}

template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>&
operator<< (std::basic_ostream<CharT, Traits> &os, const std::filesystem::file_type& t) {
    return os << impl::ftypeToString(t);
}

template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>&
operator<< (std::basic_ostream<CharT, Traits> &os, const std::filesystem::directory_entry& entry) {
#if RLIB_CXX_STD >= 2020
    auto lastWriteInSysClock = std::chrono::clock_cast<std::chrono::system_clock>(entry.last_write_time());
#else
#error A BUG NOT RESOLVED NOW. DO NOT USE THIS LIB.
    auto lastWriteInSysClock = entry.last_write_time();
#endif
    auto lastWrite = std::chrono::system_clock::to_time_t(lastWriteInSysClock);
    return os << entry.status().permissions() << ' ' << impl::try_show_file_size(entry) << ' ' << std::put_time(std::localtime(&lastWrite), "%F %T") << ' ' << entry.status().type();
}



} // end namespace rlib::prettyprint


#endif
