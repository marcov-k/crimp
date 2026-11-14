#include "ast_dump_tests.h"

#include "ast.h"
#include "ast_dump.h"
#include "../common/arena.h"
#include "../common/span.h"
#include "../diagnostics/source_map.h"
#include "../lexer/tokens.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

static_assert(std::is_trivially_destructible_v<AstExpr>);
static_assert(std::is_trivially_destructible_v<AstStmt>);
static_assert(std::is_trivially_destructible_v<AstDecl>);
static_assert(std::is_trivially_destructible_v<AstProgram>);

namespace
{
	int g_failures{ 0 };

	[[noreturn]] void SetupError(const char* what, std::string_view detail)
	{
		std::fprintf(stderr, "test setup error: %s: '%.*s'\n", what,
			static_cast<int>(detail.size()), detail.data());
		std::abort();
	}

	Span Cover(const Span& a, const Span& b)
	{
		Span s{};
		s.file = a.file;
		s.begin = std::min(a.begin, b.begin);
		s.end = std::max(a.end, b.end);
		return s;
	}

	// TreeBuilder: owns a source file and an arena, and creates nodes whose
	// spans point at real text. Spans are looked up with At(), which is
	// stateless, so the order in which function arguments are evaluated
	// can't affect the result.
	class TreeBuilder
	{
	public:
		explicit TreeBuilder(std::string_view source)
			: _id(_sources.Add("test.crimp", std::string{ source }))
			, _text(_sources.File(_id).text)
		{
		}

		// Finds `context` in the source, then `needle` inside that context.
		// With no needle, the span covers the whole context. Pick contexts
		// that occur exactly once in the test source.
		Span At(std::string_view context, std::string_view needle = {}) const
		{
			std::size_t c{ _text.find(context) };
			if (c == std::string_view::npos) SetupError("context not found", context);

			std::size_t begin{ c };
			std::size_t length{ context.size() };
			if (!needle.empty())
			{
				std::size_t n{ context.find(needle) };
				if (n == std::string_view::npos) SetupError("needle not found in context", needle);
				begin = c + n;
				length = needle.size();
			}

			Span s{};
			s.file = _id;
			s.begin = begin;
			s.end = begin + length;
			return s;
		}

		// Expressions
		AstExpr* Int(Span s) { return Leaf(ExprKind::IntLiteral, s); }
		AstExpr* Str(Span s) { return Leaf(ExprKind::StringLiteral, s); }
		AstExpr* ErrorExpr(Span s) { return Leaf(ExprKind::Error, s); }

		AstExpr* Name(Span s)
		{
			AstExpr* e{ Leaf(ExprKind::Name, s) };
			e->name = s;
			return e;
		}

		AstExpr* Binary(TokenKind op, AstExpr* lhs, AstExpr* rhs)
		{
			AstExpr* e{ _arena.Create<AstExpr>() };
			e->kind = ExprKind::Binary;
			e->span = Cover(lhs->span, rhs->span);
			e->op = op;
			e->lhs = lhs;
			e->rhs = rhs;
			return e;
		}

		AstExpr* Call(Span name, std::vector<AstExpr*> args)
		{
			AstExpr* e{ _arena.Create<AstExpr>() };
			e->kind = ExprKind::Call;
			e->name = name;
			e->span = args.empty() ? name : Cover(name, args.back()->span);
			e->args = ToList(args);
			return e;
		}

		// Statements (spans derive from their children)
		AstStmt* Return(AstExpr* value)
		{
			AstStmt* s{ _arena.Create<AstStmt>() };
			s->kind = StmtKind::Return;
			s->span = value ? value->span : Span{};
			s->expr = value;
			return s;
		}

		AstStmt* ExprStmt(AstExpr* value)
		{
			AstStmt* s{ _arena.Create<AstStmt>() };
			s->kind = StmtKind::ExprStmt;
			s->span = value ? value->span : Span{};
			s->expr = value;
			return s;
		}

		AstStmt* Var(Span type, Span name, AstExpr* init)
		{
			AstStmt* s{ _arena.Create<AstStmt>() };
			s->kind = StmtKind::VarDecl;
			s->span = init ? Cover(type, init->span) : Cover(type, name);
			s->typeRef = type;
			s->name = name;
			s->expr = init;
			return s;
		}

		AstStmt* Block(std::vector<AstStmt*> stmts, Span emptySpan = {})
		{
			AstStmt* s{ _arena.Create<AstStmt>() };
			s->kind = StmtKind::Block;
			s->span = stmts.empty() ? emptySpan : Cover(stmts.front()->span, stmts.back()->span);
			s->stmts = ToList(stmts);
			return s;
		}

		// Declarations
		AstDecl* Param(Span type, Span name)
		{
			AstDecl* d{ _arena.Create<AstDecl>() };
			d->kind = DeclKind::Param;
			d->span = Cover(type, name);
			d->typeRef = type;
			d->name = name;
			return d;
		}

		AstDecl* Function(Span type, Span name, std::vector<AstDecl*> params, AstStmt* body)
		{
			AstDecl* d{ _arena.Create<AstDecl>() };
			d->kind = DeclKind::Function;
			d->span = body ? Cover(type, body->span) : Cover(type, name);
			d->typeRef = type;
			d->name = name;
			d->params = ToList(params);
			d->body = body;
			return d;
		}

		AstProgram* Program(std::vector<AstDecl*> decls)
		{
			AstProgram* p{ _arena.Create<AstProgram>() };
			p->fileId = _id;
			p->body = ToList(decls);
			return p;
		}

		std::string Dump(const AstProgram* program) const
		{
			return DumpAstToString(_sources, *program);
		}

	private:
		AstExpr* Leaf(ExprKind kind, Span s)
		{
			AstExpr* e{ _arena.Create<AstExpr>() };
			e->kind = kind;
			e->span = s;
			return e;
		}

		template<typename T>
		List<T*> ToList(const std::vector<T*>& items)
		{
			List<T*> list{};
			list.data = _arena.CopyArray<T*>(items); // null for an empty vector
			list.count = items.size();
			return list;
		}

		// Declaration order matters: _id and _text are initialized from _sources.
		SourceMap _sources{};
		FileId _id{};
		std::string_view _text{};
		Arena _arena{};
	};

	// Comparison helpers
	std::string StripCarriageReturns(std::string_view s)
	{
		std::string out;
		out.reserve(s.size());
		for (char c : s)
		{
			if (c != '\r') out.push_back(c);
		}
		return out;
	}

	std::vector<std::string_view> SplitLines(std::string_view s)
	{
		std::vector<std::string_view> lines;
		while (!s.empty())
		{
			std::size_t nl{ s.find('\n') };
			if (nl == std::string_view::npos)
			{
				lines.push_back(s);
				break;
			}
			lines.push_back(s.substr(0, nl));
			s.remove_prefix(nl + 1);
		}
		return lines;
	}

	bool HasTrailingSpace(std::string_view s)
	{
		for (std::size_t i{ 0 }; i < s.size(); ++i)
		{
			if (s[i] == ' ' && (i + 1 == s.size() || s[i + 1] == '\n')) return true;
		}
		return false;
	}

	void PrintLine(const char* label, std::string_view line)
	{
		// The bars make trailing spaces visible.
		std::printf("        %s |%.*s|\n", label, static_cast<int>(line.size()), line.data());
	}

	void ReportFirstDifference(std::string_view expected, std::string_view actual)
	{
		std::vector<std::string_view> e{ SplitLines(expected) };
		std::vector<std::string_view> a{ SplitLines(actual) };
		std::size_t count{ std::max(e.size(), a.size()) };

		for (std::size_t i{ 0 }; i < count; ++i)
		{
			std::string_view le{ i < e.size() ? e[i] : std::string_view{ "<missing line>" } };
			std::string_view la{ i < a.size() ? a[i] : std::string_view{ "<missing line>" } };
			if (le != la)
			{
				std::printf("      first difference at line %zu:\n", i + 1);
				PrintLine("expected", le);
				PrintLine("actual  ", la);
				return;
			}
		}
		std::printf("      lines are identical; the outputs differ only in the trailing newline\n");
	}

	void Check(const char* name, const std::string& actual, std::string_view expectedRaw)
	{
		// Raw string literals may carry CRLF if the source file does.
		std::string expected{ StripCarriageReturns(expectedRaw) };

		bool ok{ true };
		if (actual != expected)
		{
			ok = false;
			std::printf("  FAIL  %s\n", name);
			ReportFirstDifference(expected, actual);
		}
		if (HasTrailingSpace(actual))
		{
			if (ok) std::printf("  FAIL  %s\n", name);
			ok = false;
			std::printf("      output contains a trailing space\n");
		}

		if (ok) std::printf("  pass  %s\n", name);
		else ++g_failures;
	}

	// Tests
	constexpr std::string_view kAddSource = R"SRC(int add(int a, int b)
{
	return a + b;
}

int main()
{
	int r = add(2, 3);
	print("%d\n", r);
	return 0;
}
)SRC";

	void TestAddProgram()
	{
		TreeBuilder t{ kAddSource };

		AstExpr* sum{ t.Binary(TokenKind::Plus,
			t.Name(t.At("a + b", "a")),
			t.Name(t.At("a + b", "b"))) };
		AstStmt* addBody{ t.Block({ t.Return(sum) }) };
		AstDecl* add{ t.Function(
			t.At("int add", "int"), t.At("int add", "add"),
			{ t.Param(t.At("int a,", "int"), t.At("int a,", "a")),
			  t.Param(t.At("int b)", "int"), t.At("int b)", "b")) },
			addBody) };

		AstExpr* addCall{ t.Call(t.At("add(2, 3)", "add"),
			{ t.Int(t.At("add(2, 3)", "2")), t.Int(t.At("add(2, 3)", "3")) }) };
		AstExpr* printCall{ t.Call(t.At("print(", "print"),
			{ t.Str(t.At(R"X("%d\n")X")), t.Name(t.At(", r)", "r")) }) };
		AstStmt* mainBody{ t.Block({
			t.Var(t.At("int r", "int"), t.At("int r", "r"), addCall),
			t.ExprStmt(printCall),
			t.Return(t.Int(t.At("return 0;", "0"))) }) };
		AstDecl* mainFn{ t.Function(
			t.At("int main", "int"), t.At("int main", "main"),{}, mainBody) };

		Check("add program", t.Dump(t.Program({ add, mainFn })),
			R"EXP((function int add ((param int a) (param int b))
  (block
    (return (+ (name a) (name b)))))
(function int main ()
  (block
    (var int r (call add (int 2) (int 3)))
    (expr (call print (string "%d\n") (name r)))
    (return (int 0))))
)EXP");
	}

	void TestEmptyFunction()
	{
		TreeBuilder t{ "int e() {}\n" };

		AstDecl* fn{ t.Function(t.At("int e", "int"), t.At("int e", "e"),{},
			t.Block({}, t.At("{}"))) };

		Check("empty function", t.Dump(t.Program({ fn })),
			R"EXP((function int e ()
  (block))
)EXP");
	}

	void TestCalls()
	{
		TreeBuilder t{ "int c() { f(); g(1, x); }\n" };

		AstStmt* body{ t.Block({
			t.ExprStmt(t.Call(t.At("f()", "f"),{})),
			t.ExprStmt(t.Call(t.At("g(1, x)", "g"),
				{ t.Int(t.At("g(1, x)", "1")), t.Name(t.At("g(1, x)", "x")) })) }) };
		AstDecl* fn{ t.Function(t.At("int c", "int"), t.At("int c", "c"),{}, body) };

		Check("calls with zero and several arguments", t.Dump(t.Program({ fn })),
			R"EXP((function int c ()
  (block
    (expr (call f))
    (expr (call g (int 1) (name x)))))
)EXP");
	}

	void TestErrorsAndNulls()
	{
		TreeBuilder t{ "int bad() { return ?; }\nint nobody();\n" };

		AstStmt* body{ t.Block({
			t.Return(t.ErrorExpr(t.At("?"))),
			t.Return(nullptr) }) };
		AstDecl* bad{ t.Function(t.At("int bad", "int"), t.At("int bad", "bad"),{}, body) };
		AstDecl* nobody{ t.Function(t.At("int nobody", "int"), t.At("int nobody", "nobody"),{}, nullptr) };

		Check("error and null nodes", t.Dump(t.Program({ bad, nobody })),
			R"EXP((function int bad ()
  (block
    (return (error))
    (return (null))))
(function int nobody ()
  (null))
)EXP");
	}

	void TestStringAndVar()
	{
		TreeBuilder t{ R"SRC(int v() { string msg = "a\tb"; }
)SRC" };

		AstStmt* body{ t.Block({
			t.Var(t.At("string msg", "string"), t.At("string msg", "msg"),
				t.Str(t.At(R"X("a\tb")X"))) }) };
		AstDecl* fn{ t.Function(t.At("int v", "int"), t.At("int v", "v"),{}, body) };

		Check("variable with string initializer", t.Dump(t.Program({ fn })),
			R"EXP((function int v ()
  (block
    (var string msg (string "a\tb"))))
)EXP");
	}

	void TestNestedBinary()
	{
		TreeBuilder t{ "int sum() { return a + b + c; }\n" };

		AstExpr* ab{ t.Binary(TokenKind::Plus,
			t.Name(t.At("a + b + c", "a")),
			t.Name(t.At("a + b + c", "b"))) };
		AstExpr* abc{ t.Binary(TokenKind::Plus, ab, t.Name(t.At("a + b + c", "c"))) };
		AstStmt* body{ t.Block({ t.Return(abc) }) };
		AstDecl* fn{ t.Function(t.At("int sum", "int"), t.At("int sum", "sum"),{}, body) };

		Check("left-nested binary expression", t.Dump(t.Program({ fn })),
			R"EXP((function int sum ()
  (block
    (return (+ (+ (name a) (name b)) (name c)))))
)EXP");
	}
}

int RunAstDumpTests()
{
	std::printf("AST dump tests\n");

	TestAddProgram();
	TestEmptyFunction();
	TestCalls();
	TestErrorsAndNulls();
	TestStringAndVar();
	TestNestedBinary();

	if (g_failures == 0) std::printf("all AST dump tests passed\n");
	else std::printf("%d AST dump test(s) FAILED\n", g_failures);

	return g_failures == 0 ? 0 : 1;
}