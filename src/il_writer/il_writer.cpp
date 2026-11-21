#include "il_writer.h"

#include <cassert>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace
{
	std::string GetTypeSpelling(QBEType type)
	{
		switch (type)
		{
		case QBEType::None:
			return "";
		case QBEType::W:
			return "w";
		case QBEType::L:
			return "l";
		}

		return "";
	}

	std::string GetOpCodeSpelling(OpCode op)
	{
		switch (op)
		{
		case OpCode::Add:
			return "add";
		}

		return "";
	}

	std::string GetAlignmentSpelling(std::size_t alignment)
	{
		switch (alignment)
		{
		case 4:
			return "alloc4";
		case 8:
			return "alloc8";
		default:
			return "";
		}
	}

	std::string GetParamName(std::string_view name)
	{
		return std::format("%p.{}", name);
	}

	std::string FormatParam(const ILParam& param)
	{
		return std::format("{} {}", GetTypeSpelling(param.type), GetParamName(param.name));
	}

	std::string FormatArg(const ILArg& arg)
	{
		return std::format("{} {}", GetTypeSpelling(arg.type), arg.value);
	}
}

void ILWriter::BeginFunction(std::string_view linkName, QBEType returnType, const std::vector<ILParam>& params)
{
	assert(!_inFunction);

	ResetFunctionData();

	_inFunction = true;
	_linkName = linkName;
	_returnType = returnType;

	for (const ILParam& param : params)
	{
		assert(param.type != QBEType::None);
		if (!_paramText.empty()) _paramText.append(", ");
		_paramText.append(FormatParam(param));
	}
}

void ILWriter::EndFunction()
{
	assert(_inFunction);

	if (!_terminated)
	{
		if (_returnType == QBEType::None) _bodyBuffer.push_back("\tret");
		else _bodyBuffer.push_back("\thlt");
	}

	std::string func{ "export function " };
	if (_returnType != QBEType::None) func.append(std::format("{} ", GetTypeSpelling(_returnType)));
	func.append(std::format("${}({})\n{{\n@start\n", _linkName, _paramText));

	for (const std::string& entry : _entryBuffer)
	{
		func.append(std::format("{}\n", entry));
	}
	
	for (const std::string& line : _bodyBuffer)
	{
		func.append(std::format("{}\n", line));
	}

	func.append("}");

	_finishedFuncs.push_back(func);

	ResetFunctionData();
}

std::string ILWriter::NewTemp()
{
	return std::format("%t.{}", _tempCount++);
}

std::string ILWriter::NewLabel(std::string_view hint)
{
	return std::format("@{}.{}", hint, _labelCount++);
}

std::string ILWriter::ParamOperand(std::string_view name)
{
	return GetParamName(name);
}

std::string ILWriter::Alloc(std::string_view hint, std::size_t size, std::size_t alignment)
{
	assert(alignment == 4 || alignment == 8);

	std::string name{ std::format("%v.{}.{}", hint, _slotCount++) };

	_entryBuffer.push_back(std::format("\t{} =l {} {}", name, GetAlignmentSpelling(alignment), size));

	return name;
}

std::string ILWriter::Load(QBEType type, std::string_view slot)
{
	assert(type != QBEType::None);

	std::string temp{ NewTemp() };

	std::string typeLetter{ GetTypeSpelling(type) };
	Emit(std::format("{} ={} load{} {}", temp, typeLetter, typeLetter, slot));

	return temp;
}

void ILWriter::Store(QBEType type, std::string_view value, std::string_view slot)
{
	assert(type != QBEType::None);

	Emit(std::format("store{} {}, {}", GetTypeSpelling(type), value, slot));
}

std::string ILWriter::Binary(OpCode opCode, QBEType type, std::string_view lhs, std::string_view rhs)
{
	assert(type != QBEType::None);

	std::string temp{ NewTemp() };

	Emit(std::format("{} ={} {} {}, {}", temp, GetTypeSpelling(type), GetOpCodeSpelling(opCode), lhs, rhs));

	return temp;
}

std::string ILWriter::Call(QBEType returnType, std::string_view callee, const std::vector<ILArg>& args, std::size_t fixedArgCount)
{
	std::string argText{};
	if (fixedArgCount == NOT_VARIADIC)
	{
		for (std::size_t i{ 0 }; i < args.size(); ++i)
		{
			assert(args[i].type != QBEType::None);
			if (!argText.empty()) argText.append(", ");
			argText.append(FormatArg(args[i]));
		}
	}
	else
	{
		assert(fixedArgCount <= args.size());

		std::size_t i{ 0 };
		for (; i < fixedArgCount; ++i)
		{
			assert(args[i].type != QBEType::None);
			if (!argText.empty()) argText.append(", ");
			argText.append(FormatArg(args[i]));
		}

		if (!argText.empty()) argText.append(", ");
		argText.append("...");

		for (; i < args.size(); ++i)
		{
			assert(args[i].type != QBEType::None);
			if (!argText.empty()) argText.append(", ");
			argText.append(FormatArg(args[i]));
		}
	}

	if (returnType == QBEType::None)
	{
		Emit(std::format("call ${}({})", callee, argText));
		return "";
	}
	else
	{
		std::string temp{ NewTemp() };
		Emit(std::format("{} ={} call ${}({})", temp, GetTypeSpelling(returnType), callee, argText));
		return temp;
	}
}

void ILWriter::Ret(std::string_view operand)
{
	Emit(std::format("ret {}", operand));
	_terminated = true;
}

void ILWriter::Ret()
{
	Emit("ret");
	_terminated = true;
}

void ILWriter::Comment(std::string_view text)
{
	std::string outputText{ text };
	for (std::size_t i{ 0 }; i < outputText.size(); ++i)
	{
		if (outputText[i] == '\n' || outputText[i] == '\r') outputText[i] = ' ';
	}
	_bodyBuffer.push_back(std::format("# {}", outputText));
}

std::string ILWriter::InternString(std::string content)
{
	auto found{ _contentMap.find(content) };
	if (found != _contentMap.end())
	{
		return found->second;
	}

	std::string name{ std::format("$crimp_str.{}", _stringCount++) };
	_contentMap[content] = name;

	_dataBuffer.push_back(std::format("data {} = {{ b \"{}\", b 0 }}", name, content));

	return name;
}

std::string ILWriter::Finish()
{
	assert(!_finished && !_inFunction);

	std::string output{};

	for (const std::string& data : _dataBuffer)
	{
		output.append(std::format("{}\n", data));
	}
	if (!_dataBuffer.empty()) output.append("\n");

	for (std::size_t i{ 0 }; i < _finishedFuncs.size(); ++i)
	{
		if (i > 0) output.append("\n\n");
		output.append(_finishedFuncs[i]);
	}
	output.append("\n");

	_finished = true;
	return output;
}

void ILWriter::Emit(std::string_view line)
{
	assert(_inFunction);

	if (_terminated)
	{
		_bodyBuffer.push_back(NewLabel("dead"));
		_terminated = false;
	}

	_bodyBuffer.push_back(std::format("\t{}", line));
}

void ILWriter::ResetFunctionData()
{
	_inFunction = false;

	_linkName.clear();
	_returnType = QBEType::None;
	_paramText.clear();

	_terminated = false;

	_tempCount = 0;
	_slotCount = 0;
	_labelCount = 0;

	_entryBuffer.clear();
	_bodyBuffer.clear();
}