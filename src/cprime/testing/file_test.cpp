#include "file_test.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <print>
#include <cxxopts.hpp>
#include <cprime/support/prelude.hpp>
#include <cprime/support/contract.hpp>

namespace cprime::testing {

namespace {

auto must_read_file(const std::filesystem::path& path) -> std::string
{
    std::ifstream is{path, std::ios::binary};
    CPRIME_ASSERT(!is.fail(), "Failed to open file.");

    // Возможно, что std::istreambuf_iterator может и не устанавливать флаги
    // ошибок в is. Конкретно в данном случае это не критично.
    std::string buffer{std::istreambuf_iterator<char>{is}, {}};
    CPRIME_ASSERT(!is.bad(), "Failed to read file.");

    return buffer;
}

auto must_write_file(
    const std::filesystem::path& path,
    std::string_view data) -> void
{
    std::ofstream os{path, std::ios::binary};
    CPRIME_ASSERT(!os.fail(), "Failed to open file.");
    os.write(data.data(), static_cast<std::streamsize>(data.size()));
    CPRIME_ASSERT(!os.bad(), "Failed to write file.");
    os.close();
    CPRIME_ASSERT(!os.fail(), "Failed to close file.");
}

// Диагностику пишем в stderr: во-первых, это очищает stdout от ненужной
// информации, а, во-вторых, буферизация в stderr обычно отключена, что
// освобождает нас от необходимости везде делать flush.
std::FILE* const kOutputStream = stderr;

template<typename... Args>
auto console_print(
    std::format_string<Args...> fmt = "",
    Args&&... args) -> void
{
    std::print(kOutputStream, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
auto console_println(
    std::format_string<Args...> fmt = "",
    Args&&... args) -> void
{
    std::println(kOutputStream, fmt, std::forward<Args>(args)...);
}

// TODO: сделать настраиваемым через аргументы конструктора FileTest.
constexpr std::string_view kOutputFileExtension = ".out";

auto get_output_file_path(const std::filesystem::path& path)
    -> std::filesystem::path
{
    std::filesystem::path result = path;
    result += kOutputFileExtension;
    return result;
}

} // namespace

auto FileTest::run(int argc, char** argv) -> int
{
    cxxopts::Options option_spec{
        std::string{exe_name_},
        "FileTest runner"};

    // clang-format off
    option_spec.add_options()
        ("u,update", "Automatically update test files")
        ("p,process", "Process single file and print the result", cxxopts::value<std::string>())
        ("project", "Path to project root", cxxopts::value<std::string>())
        ("h,help", "Print this message");
    // clang-format on
    option_spec.parse_positional({"project"});
    option_spec.positional_help("<path>");

    // TODO: Текущее help-сообщение сделано плохо. Нужно исправить.
    auto run_print_help = [&](int exit_code) {
        console_print("{}", option_spec.help());
        return exit_code;
    };

    auto options = [&] {
        try {
            return option_spec.parse(argc, argv);
        } catch (const cxxopts::exceptions::parsing&) {
            std::exit(run_print_help(1));
        }
    }();

    if (options.count("help") > 0) {
        return run_print_help(0);
    }

    bool has_update = options.count("update") > 0;
    bool has_process = options.count("process") > 0;
    bool has_project = options.count("project") > 0;

    // Опции --update и --process несовместимы.
    if (has_update && has_process) {
        return run_print_help(1);
    }
    if (has_process) {
        // При --process путь к файлу, который нужно обработать, передаётся
        // вместе с самим --process. Значение <project> не нужно.
        if (has_project) {
            return run_print_help(1);
        }
        return run_process_single_file(options["process"].as<std::string>());
    }
    // Если путь не был передан к программе, то это ошибка.
    if (!has_project) {
        return run_print_help(1);
    }
    if (has_update) {
        return run_update(options["project"].as<std::string>());
    }
    return run_check(options["project"].as<std::string>());
}

FileTest::FileTest(
    std::string_view test_name,
    const std::filesystem::path& testdata_dir,
    std::string_view exe_name)
    : test_name_{test_name}
    , testdata_dir_{testdata_dir}
    , exe_name_{exe_name}
{
}

auto FileTest::run_check(const std::filesystem::path& project_root) -> int
{
    console_println("FileTest: {}\nChecking", test_name_);

    CPRIME_ASSERT(std::filesystem::is_directory(project_root));

    i32 test_count = 0;
    i32 test_pass_count = 0;
    i32 test_fail_count = 0;
    std::filesystem::path resolved_testdata_dir = project_root / testdata_dir_;
    for (const auto& entry :
        std::filesystem::recursive_directory_iterator{resolved_testdata_dir})
    //
    {
        if (!entry.is_regular_file() ||
            entry.path().extension() == kOutputFileExtension)
        //
        {
            continue;
        }

        ++test_count;

        // Сначала читаем файлы теста, чтобы сразу сообщить об ошибках чтения
        // этих файлов.
        std::string input = must_read_file(entry.path());
        std::string expected_output =
            must_read_file(get_output_file_path(entry.path()));

        std::string actual_output = process(input);

        bool pass = actual_output == expected_output;
        if (pass) {
            ++test_pass_count;
        } else {
            ++test_fail_count;
        }
        std::filesystem::path pretty_path =
            entry.path().lexically_proximate(project_root);
        char status_char = pass ? '.' : '!';
        console_println("{} {}", status_char, pretty_path.string());
    }

    console_println();
    console_println("pass/fail/all: {}/{}/{}",
        test_pass_count,
        test_fail_count,
        test_count);

    return (test_fail_count == 0) ? 0 : 1;
}

auto FileTest::run_update(const std::filesystem::path& project_root) -> int
{
    console_println("FileTest: {}\nUpdating", test_name_);

    CPRIME_ASSERT(std::filesystem::is_directory(project_root));

    std::filesystem::path resolved_testdata_dir = project_root / testdata_dir_;
    for (const auto& entry :
        std::filesystem::recursive_directory_iterator{resolved_testdata_dir})
    //
    {
        if (!entry.is_regular_file() ||
            entry.path().extension() == kOutputFileExtension)
        //
        {
            continue;
        }

        std::string input = must_read_file(entry.path());
        std::string processed = process(input);
        must_write_file(get_output_file_path(entry.path()), processed);

        std::filesystem::path pretty_path =
            entry.path().lexically_proximate(project_root);
        console_println("{}", pretty_path.string());
    }

    return 0;
}

auto FileTest::run_process_single_file(const std::filesystem::path& path) -> int
{
    CPRIME_ASSERT(std::filesystem::is_regular_file(path));

    console_println("FileTest: {}\nProcess {}", test_name_, path.string());

    std::string input = must_read_file(path);
    std::string processed = process(input);

    // Выводим в stdout, чтобы можно было использовать pipes для перенаправления
    // текстового потока.
    std::print(stdout, "{}", processed);

    return 0;
}

} // namespace cprime::testing
