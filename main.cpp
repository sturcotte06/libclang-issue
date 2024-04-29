#include <chrono>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

#include "process.hpp"
#include "clang-c/Index.h"

namespace detail
{
    constexpr std::string_view clang_exec = "C:/Program Files/LLVM/bin/clang++.exe";

    template <typename container_t, typename projection_t>
    std::string join(std::string_view separator, const container_t& values, const projection_t& projection)
    {
        std::string string;
        for (const auto& value : values)
        {
            string += projection(value);
            string += separator;
        }

        if (string.size() >= separator.size())
        {
            string.resize(string.size() - separator.size());
        }

        return string;
    }

    template <typename func_t>
    auto invoke_with_timing(std::string_view name, const func_t& func)
    {
        const auto now = std::chrono::steady_clock::now();
        func();
        const auto then = std::chrono::steady_clock::now();
        const std::chrono::duration<float> duration = then - now;
        std::cout << name << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(duration) << std::endl;
    }

    auto run_command(const std::string& command, const std::string& input = std::string {})
    {
        const bool has_stdin = not input.empty();
        const auto reader_factory = [](std::stringstream& stream) {
            return [&](const char* bytes, std::size_t n) {
                stream.write(bytes, static_cast<std::streamsize>(n));
            };
        };

        std::stringstream _stdout;
        std::stringstream _stderr;

        TinyProcessLib::Process process {
            command,
            std::string(),
            reader_factory(_stdout),
            reader_factory(_stderr),
            has_stdin,
        };

        if (has_stdin)
        {
            process.write(input);
            process.close_stdin();
        }

        std::int32_t result = process.get_exit_status();
        return std::tuple {result, _stdout.str(), _stderr.str()};
    }
}

struct ast_parser
{
    CXIndex index;
    std::vector<const char*> args;

    ast_parser(int argc, const char** argv)
    {
        index = clang_createIndex(0, 0);

        args = {
            "-xc++",
            "-std=c++20",
            "-w",
            "-Wno-everything",
        };

        for (std::size_t i = 0; i < argc; ++i)
        {
            args.push_back(argv[i]);
        }
    }

    ~ast_parser() noexcept
    {
        clang_disposeIndex(index);
    }

    void parse(std::string_view path) const
    {
        constexpr auto flags =
            static_cast<CXTranslationUnit_Flags>(
                CXTranslationUnit_Incomplete |
                CXTranslationUnit_SkipFunctionBodies |
                CXTranslationUnit_SingleFileParse |
                CXTranslationUnit_KeepGoing);

        std::string source;
        if (!preprocess(path, source))
        {
            throw std::exception("cannot preprocess");
        }

        CXUnsavedFile unsaved_file {path.data(), source.data(), static_cast<unsigned long>(source.size())};
        CXTranslationUnit translation_unit = clang_parseTranslationUnit(
            index, path.data(), args.data(), static_cast<int>(args.size()), &unsaved_file, 1, flags);

        if (!translation_unit)
        {
            throw std::exception("bad translation unit");
        }

        clang_disposeTranslationUnit(translation_unit);
    }

    bool preprocess(std::string_view path, std::string& source) const
    {
        std::initializer_list<std::string_view> additional_args {
            "-E",
            "-P",
        };

        std::string command;
        command += std::string(detail::clang_exec) + " ";
        command += detail::join(" ", args, std::identity()) + " ";
        command += detail::join(" ", additional_args, std::identity()) + " ";
        command += path;

        auto [result, out, err] = detail::run_command(command);
        if (result != 0)
        {
            return false;
        }

        source = std::move(out);
        return true;
    }
};

int main(int argc, const char* argv[])
{
    detail::invoke_with_timing("libclang", [&] {
        ast_parser parser(argc - 1, argv + 1);
        parser.parse("file.hpp");
    });

    detail::invoke_with_timing("clang dump-ast", [&] {
        std::string command = std::format("{} -Xclang -ast-dump -fsyntax-only -xc++ -std=c++20 -w -Wno-everything file.hpp", detail::clang_exec);
        detail::run_command(command);
    });

    return 0;
}
