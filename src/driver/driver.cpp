#include "driver.h"

#include "../compile_flags.h"
#include "../diagnostics/diagnostics.h"
#include "../diagnostics/source_map.h"
#include "../lexer/lexer.h"
#include "../platform/file_io.h"
#include "../platform/process.h"
#include "../platform/startup.h"
#include "../platform/utf8_wide.h"
#include "../utils/utils.h"

#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <iostream>
#include <string>

namespace
{
	std::string ReadLog(const std::filesystem::path& p)
	{
		std::string text, ignored;
		ReadFile(p, text, ignored);
		return text;
	}

	std::filesystem::path GetExecutablePath(std::filesystem::path sourcePath)
	{
		return sourcePath.replace_extension(".exe");
	}

	bool BuildExecutable(const std::string& il, const std::filesystem::path& exePath,
		const Tools& tools, bool keepTemps, std::string& error)
	{
		std::filesystem::path dir{ MakeTemporaryDirectory(error) };
		if (dir.empty()) return false;

		const std::filesystem::path ssa{ dir / "out.ssa" };
		const std::filesystem::path asmPath{ dir / "out.s" };

		bool ok{ false };
		do
		{
			if (!WriteFile(ssa, il, error)) break;

			// QBE
			ProcessResult q{ RunLogged(tools.qbe,{ L"-t", tools.target, L"-o", asmPath.wstring(), ssa.wstring() },
				dir / "qbe.log") };
			if (!q.launched)
			{
				error = q.error;
				break;
			}
			if (q.exitCode != 0)
			{
				error = "internal compiler error: QBE rejected generated IL (exit " +
					std::to_string(q.exitCode) + ")\n" + ReadLog(dir / "qbe.log");
				break;
			}

			// Assemble + Link
			ProcessResult c{ RunLogged(tools.cc,{ asmPath.wstring(), L"-o", exePath.wstring() }, dir / "link.log") };
			if (!c.launched)
			{
				error = c.error;
				break;
			}
			if (c.exitCode != 0)
			{
				error = "assembling/linking failed (exit " + std::to_string(c.exitCode) +
					")\n" + ReadLog(dir / "link.log");
				break;
			}

			ok = true;
		} while (false);

		if (ok && !keepTemps)
		{
			std::error_code ec;
			std::filesystem::remove_all(dir, ec);
		}
		else if (!ok)
		{
			error += "\n(intermediate files kept in " + PathToUTF8(dir) + ")";
		}

		return ok;
	}

	int Compile(const std::filesystem::path& sourcePath, const CompileFlags& flags)
	{
		std::string text, err;

		if (!ReadFile(sourcePath, text, err))
		{
			std::fprintf(stderr, "error: %s\n", err.c_str());
			return 1;
		}

		Diagnostics diags;

		SourceMap sources;
		FileId id{ sources.Add(PathToUTF8(sourcePath), std::move(text)) };

		for (const Diagnostic& d : diags.All())
		{
			std::string s{ Render(sources, d) };
			std::fwrite(s.data(), 0, 0, stderr);
		}
		if (diags.HasErrors()) return 1;

		Tools tools;
		tools.qbe = ResolveTool(L"CRIMP_QBE", L"qbe");
		if (tools.qbe.empty())
		{
			std::fprintf(stderr, "error: could not find QBE (set CRIMP_QBE or add it to PATH)\n");
			return 1;
		}
		if (flags.verbose)
		{
			std::cout << "using QBE found at: " << PathToUTF8(tools.qbe) << std::endl;
		}

		tools.cc = ResolveTool(L"CRIMP_CC", L"gcc");
		if (tools.cc.empty())
		{
			std::fprintf(stderr, "error: could not find GCC (set CRIMP_CC or add it to PATH)\n");
			return 1;
		}
		if (flags.verbose)
		{
			std::cout << "using GCC found at: " << PathToUTF8(tools.cc) << std::endl;
		}

		std::filesystem::path exePath{ GetExecutablePath(sourcePath) };

		err.clear();
		if (!BuildExecutable(sources.File(id).text, exePath, tools, flags.keepTemps, err))
		{
			std::fprintf(stderr, "%s\n", err.c_str());
			return 1;
		}
		std::printf("\nbuilt %s\n", PathToUTF8(exePath).c_str());

		return 0;
	}
}

int wmain(int argc, wchar_t** argv)
{
	Initialize();

	if (argc < 2)
	{
		std::fprintf(stderr, "missing source file\n");
		return 1;
	}

	std::filesystem::path sourcePath{ argv[1] };

	CompileFlags flags{};
	for (int i{ 2 }; i < argc; ++i)
	{
		if (wcscmp(argv[i], L"--verbose") == 0) flags.verbose = true;
		else if (wcscmp(argv[i], L"--keep-temps") == 0) flags.keepTemps = true;
		else { std::fprintf(stderr, "unknown option '%s'\n", WideToUTF8({ argv[i] }).c_str()); return 1; }
	}

	PrependToPath(L"C:\\msys64\\mingw64\\bin");

	return Compile(sourcePath, flags);
}