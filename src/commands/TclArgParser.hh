#ifndef TCLARGPARSER_HH
#define TCLARGPARSER_HH

#include "CommandException.hh"
#include "TclObject.hh"
#include "optional.hh"
#include "span.hh"
#include "string_view.hh"
#include <functional>
#include <vector>

namespace openmsx {

namespace detail {
	template<typename T> struct GetArg;

	template<> struct GetArg<bool> {
		void operator()(Interpreter& interp, const TclObject& obj, bool& result) const {
			result = obj.getBoolean(interp);
		}
	};
	template<> struct GetArg<int> {
		void operator()(Interpreter& interp, const TclObject& obj, int& result) const {
			result = obj.getInt(interp);
		}
	};
	template<> struct GetArg<double> {
		void operator()(Interpreter& interp, const TclObject& obj, double& result) const {
			result = obj.getDouble(interp);
		}
	};
	template<> struct GetArg<string_view> {
		void operator()(Interpreter& /*interp*/, const TclObject& obj, string_view& result) const {
			result = obj.getString();
		}
	};
	template<> struct GetArg<std::string> {
		void operator()(Interpreter& /*interp*/, const TclObject& obj, std::string& result) const {
			result = obj.getString().str();
		}
	};
	template<> struct GetArg<TclObject> {
		void operator()(Interpreter& /*interp*/, const TclObject& obj, TclObject& result) const {
			result = obj;
		}
	};

	template<typename T> struct GetArg<optional<T>> {
		void operator()(Interpreter& interp, const TclObject& obj, optional<T>& result) const {
			T t;
			GetArg<T>{}(interp, obj, t);
			result = std::move(t);
		}
	};

	template<typename T> struct GetArg<std::vector<T>> {
		void operator()(Interpreter& interp, const TclObject& obj, std::vector<T>& result) const {
			result.emplace_back();
			GetArg<T>{}(interp, obj, result.back());
		}
	};
}

// A Tcl-argument-parser description is made out of ArgsInfo objects
struct ArgsInfo
{
	string_view name;
	std::function<unsigned(Interpreter&, span<const TclObject>)> func;
};

// Parse a flag.
inline ArgsInfo flagArg(string_view name, bool& flag)
{
	return {
		name,
		[&flag](Interpreter& /*interp*/, span<const TclObject> /*args*/) {
			flag = true;
			return 0;
		}
	};
}

// Parse a value (like a flag but with associated value).
template<typename T>
ArgsInfo valueArg(string_view name, T& value)
{
	return {
		name,
		[name, &value](Interpreter& interp, span<const TclObject> args) {
			if (args.empty()) {
				throw CommandException("missing argument for ", name);
			}
			detail::GetArg<T>{}(interp, args.front(), value);
			return 1;
		}
	};
}

// Parse the given 'inArgs' arguments.
// The recognized flags/values are described in 'table'.
// The result of this parser is the collection of non-flag arguments.
// See src/unittest/TclArgParser.cc for example usages.
std::vector<TclObject> parseTclArgs(Interpreter& interp, span<const TclObject> inArgs, span<const ArgsInfo> table);

} // namespace openmsx

#endif
