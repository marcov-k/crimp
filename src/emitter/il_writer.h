#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

inline constexpr std::size_t NOT_VARIADIC{ SIZE_MAX };

enum class QBEType
{
	None,
	W,
	L,
};

enum class OpCode
{
	Add,
};

struct ILParam
{
	QBEType type{ QBEType::None };
	std::string_view name{};
};

struct ILArg
{
	QBEType type{ QBEType::None };
	std::string value{};
};

class ILWriter
{
public: // Expect names without sigils | Return names with sigils
	void BeginFunction(std::string_view linkName, QBEType returnType, const std::vector<ILParam>& params);
	void EndFunction();

	std::string NewTemp();
	std::string NewLabel(std::string_view hint);
	static std::string ParamOperand(std::string_view name);

	std::string Alloc(std::string_view hint, std::size_t size, std::size_t alignment);
	std::string Load(QBEType type, std::string_view slot);
	void Store(QBEType type, std::string_view value, std::string_view slot);

	std::string Binary(OpCode opCode, QBEType type, std::string_view lhs, std::string_view rhs);
	
	std::string Call(QBEType returnType, std::string_view callee, const std::vector<ILArg>& args, std::size_t fixedArgCount);
	void Ret(std::string_view operand);
	void Ret();

	void Comment(std::string_view text);

	std::string InternString(std::string content);

	std::string Finish(); // can only be called once

private:
	bool _inFunction{ false };
	std::string _linkName{};
	QBEType _returnType{ QBEType::None };
	std::string _paramText{};
	std::size_t _tempCount{ 0 };
	std::size_t _slotCount{ 0 };
	std::size_t _labelCount{ 0 };
	std::vector<std::string> _entryBuffer{};
	std::vector<std::string> _bodyBuffer{};
	std::vector<std::string> _dataBuffer{};
	bool _terminated{ false };
	
	std::size_t _stringCount{ 0 };
	std::unordered_map<std::string, std::string> _contentMap{};
	std::vector<std::string> _finishedFuncs{};

	bool _finished{ false };

	void Emit(std::string_view line);

	void ResetFunctionData();
};