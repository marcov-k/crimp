#include "driver.h"

#include "../platform/file_io.h"
#include "../platform/process.h"
#include "../platform/utf8_wide.h"

#include <cwchar>
#include <filesystem>
#include <iostream>
#include <string>

static std::string ReadLog(const std::filesystem::path& p)
{
	std::string text, ignored;
	ReadFile(p, text, ignored);
	return text;
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

int wmain(int argc, wchar_t** argv)
{
	CompileFlags flags{};
	if (argc > 1)
	{
		for (int i = 0; i < argc; ++i)
		{
			if (wcscmp(argv[i], L"--verbose") == 0) flags.verbose = true;
		}
	}

	const std::string il = R"IL(
data $fmt = { b "%d\n", b 0 }

function w $add(w %a, w %b)
{
@start
	%r =w add %a, %b
	ret %r
}

export function w $main()
{
@start
	%r =w call $add(w 2, w 3)
	%n =w call $printf(l $fmt, ..., w %r)
	ret 0
}
)IL";

	PrependToPath(L"C:\\msys64\\mingw64\\bin");

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

	std::string error;
	if (!BuildExecutable(il, std::filesystem::path{ L"out.exe" }, tools, true, error))
	{
		std::fprintf(stderr, "%s\n", error.c_str());
		return 1;
	}
	std::puts("\nbuilt out.exe");

	return 0;
}