#include "driver.h"

#include "../ast/ast.h"
#include "../ast/ast_dump.h"
#include "../checker/checker.h"
#include "../checker/symbols_dump.h"
#include "../common/arena.h"
#include "../common/span.h"
#include "../common/utils.h"
#include "../diagnostics/diagnostics.h"
#include "../diagnostics/source_map.h"
#include "../emitter/emitter.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../platform/file_io.h"
#include "../platform/process.h"
#include "../platform/startup.h"
#include "../platform/utf8_wide.h"

#include <cstddef>
#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	struct Tools
	{
		std::filesystem::path qbe;
		std::filesystem::path cc;
		std::wstring target{ L"amd64_win" };
	};

	void PrintNewDiagnostics(const SourceMap& sources, const Diagnostics& diag, std::size_t& printedCount)
	{
		for (; printedCount < diag.All().size(); ++printedCount)
		{
			std::string d{ Render(sources, diag.All()[printedCount]) };
			std::fwrite(d.data(), 1, d.size(), stderr);
		}
	}

	void DumpTokens(const SourceMap& source, FileId fileId, const std::vector<Token>& tokens)
	{
		const SourceFile& file{ source.File(fileId) };
		std::string_view text{ file.text };

		for (const Token& token : tokens)
		{
			LineCol lc{ source.Locate(fileId, token.span.begin) };
			std::string_view lexeme{ text.substr(token.span.begin, token.span.end - token.span.begin) };
			std::printf("%zu:%zu %s \"%s\"\n", lc.line, lc.col, GetTokenName(token.type).data(), EscapeString(lexeme).c_str());
		}
	}

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

	bool EmitSSA(const std::string& ssa, std::filesystem::path sourcePath)
	{
		std::filesystem::path outPath{ sourcePath.replace_extension(".ssa") };

		std::string err{};
		if (!WriteFile(outPath, ssa, err))
		{
			std::fprintf(stderr, "%s\n", err.c_str());
			return false;
		}

		std::fprintf(stderr, "\nemitted SSA at: %s\n", PathToUTF8(outPath).c_str());
		return true;
	}

	bool FindTools(Tools& tools, bool verbose)
	{
		tools.qbe = ResolveTool(L"CRIMP_QBE", L"qbe");
		if (tools.qbe.empty())
		{
			std::fprintf(stderr, "error: could not find QBE (set CRIMP_QBE or add it to PATH)\n");
			return false;
		}
		if (verbose)
		{
			std::printf("using QBE found at: %s\n", PathToUTF8(tools.qbe).c_str());
		}

		tools.cc = ResolveTool(L"CRIMP_CC", L"gcc");
		if (tools.cc.empty())
		{
			std::fprintf(stderr, "error: could not find GCC (set CRIMP_CC or add it to PATH)\n");
			return false;
		}
		if (verbose)
		{
			std::printf("using GCC found at: %s\n", PathToUTF8(tools.cc).c_str());
		}

		return true;
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
}

int Compile(const std::filesystem::path& sourcePath, const CompileFlags& flags)
{
	std::fprintf(stderr, "\nCompiling Crimp source code...\n");

	std::string text, err;

	if (!ReadFile(sourcePath, text, err))
	{
		std::fprintf(stderr, "error: %s\n", err.c_str());
		return 1;
	}

	Diagnostics diag;
	std::size_t diagsPrinted{ 0 };

	SourceMap sources;
	FileId id{ sources.Add(PathToUTF8(sourcePath), std::move(text)) };

	std::vector<Token> tokens{ Lex(sources, id, diag) };
	if (flags.dumpTokens)
	{
		std::printf("\n------Token Dump------\n\n");
		DumpTokens(sources, id, tokens);
		std::printf("----------------------\n");
	}

	Arena arena{};

	AstProgram* program{ Parse(id, tokens, diag, arena) };
	if (flags.dumpAst)
	{
		std::printf("\n-------AST Dump-------\n\n");
		DumpAst(sources, program);
		std::printf("----------------------\n");
	}

	PrintNewDiagnostics(sources, diag, diagsPrinted);
	if (diag.HasErrors()) return 1;

	Analyze(program, sources, diag, arena);
	if (flags.dumpSymbols)
	{
		std::printf("\n-----Symbol Dump------\n\n");
		DumpSymbols(program);
		std::printf("----------------------\n");
	}

	PrintNewDiagnostics(sources, diag, diagsPrinted);
	if (diag.HasErrors()) return 1;

	std::string ssa{ EmitIL(sources, program) };
	if (flags.emitSSA)
	{
		if (!EmitSSA(ssa, sourcePath)) return 1;
		return 0;
	}

	Tools tools;
	if (!FindTools(tools, flags.verbose)) return 1;

	std::filesystem::path exePath{ GetExecutablePath(sourcePath) };

	err.clear();
	if (!BuildExecutable(ssa, exePath, tools, flags.keepTemps, err))
	{
		std::fprintf(stderr, "%s\n", err.c_str());
		return 1;
	}
	std::fprintf(stderr, "\nbuilt %s\n", PathToUTF8(exePath).c_str());

	return 0;
}