#ifndef CPRIME_SUPPORT_TEMPFILE_H_
#define CPRIME_SUPPORT_TEMPFILE_H_

#include <filesystem>
#include <cprime/support/prelude.hpp>

namespace cprime::support {

class TempFile final
{
public:
    ~TempFile();

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

    TempFile(TempFile&&) = default;
    TempFile& operator=(TempFile&&) = default;

    explicit TempFile();

    [[nodiscard]]
    auto path() const -> const std::filesystem::path&;

    static auto with_content(std::string_view content) -> TempFile;

private:
    auto remove() noexcept -> void;

    static auto create_temporary_file() -> std::filesystem::path;

private:
    std::filesystem::path path_;
};

} // namespace cprime::support

#endif // CPRIME_SUPPORT_TEMPFILE_H_
