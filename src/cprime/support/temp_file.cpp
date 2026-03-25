#include "temp_file.hpp"

#include <cassert>
#include <fstream>
#include <random>
#include <system_error>

namespace cprime::support {

TempFile::~TempFile()
{
    remove();
}

TempFile::TempFile()
    : path_{create_temporary_file()}
{
}

auto TempFile::path() const -> const std::filesystem::path&
{
    return path_;
}

auto TempFile::with_content(std::string_view content) -> TempFile
{
    TempFile file{};
    std::ofstream output{file.path(), std::ios::binary};
    output << content;
    return file;
}

auto TempFile::remove() noexcept -> void
{
    if (path_.empty()) {
        return;
    }

    // Передаём error_code, чтобы избежать выбрасывания исключений.
    std::error_code error_code;
    std::filesystem::remove(path_, error_code);
}

auto TempFile::create_temporary_file() -> std::filesystem::path
{
    constexpr i32 kMaxRetryCount = 5;

    thread_local std::mt19937 random_generator{std::random_device{}()};
    thread_local std::uniform_int_distribution<u64> distribution{};

    auto temp_dir = std::filesystem::temp_directory_path();

    for (i32 i = 0; i < kMaxRetryCount; ++i) {
        auto random_suffix = std::to_string(distribution(random_generator));
        auto temp_file_path = temp_dir / ("tmp-" + random_suffix);
        std::ofstream file{
            temp_file_path,
            std::ios::binary | std::ios::noreplace};
        if (file.is_open()) {
            return temp_file_path;
        }
    }

    assert(false && "Failed to create temporary file.");
}

} // namespace cprime::support
