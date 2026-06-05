#ifndef CPRIME_TESTING_FILETEST_H_
#define CPRIME_TESTING_FILETEST_H_

#include <filesystem>
#include <cprime/support/prelude.hpp>

namespace cprime::testing {

// Очень простой тестер. Читает входные файлы, преобразует их и сравнивает с
// эталоном. Есть возможность автоматически обновить эталонные файлы.
//
// Ошибки внутри `FileTest::process()` рекомендуется не обрабатывать, а сразу
// завершать программу.
class FileTest
{
public:
    auto run(int argc, char** argv) -> int;

protected:
    FileTest(const FileTest&) = delete;
    auto operator=(const FileTest&) = delete;

    FileTest(FileTest&&) = delete;
    auto operator=(FileTest&&) = delete;

    explicit FileTest(
        std::string_view test_name,
        const std::filesystem::path& testdata_dir,
        std::string_view exe_name);

    virtual auto process(std::string_view input) -> std::string = 0;

private:
    auto run_check(const std::filesystem::path& project_root) -> int;

    auto run_update(const std::filesystem::path& project_root) -> int;

    auto run_process_single_file(const std::filesystem::path& path) -> int;

private:
    std::string_view test_name_;
    std::filesystem::path testdata_dir_;
    std::string_view exe_name_;
};

} // namespace cprime::testing

#endif // CPRIME_TESTING_FILETEST_H_
