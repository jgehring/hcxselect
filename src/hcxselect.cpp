/*
 * hcxselect - A CSS selector engine for htmlcxx
 * Copyright (C) 2011 Jonas Gehring
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holders nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <stack>
#include <sstream>

#include "hcxselect.h"

extern "C" {
	#include "lexer.h"
}

//#define TRACE printf("%s: ", __FUNCTION__); printf
inline void TRACE(...) { }


namespace hcxselect
{

// Lexer for CSS selector grammar (wrapper for reentrant FLEX parser)
class Lexer
{
public:
	Lexer(const std::string &str)
		: pos(0)
	{
		yylex_init(&yy);
		yy_scan_string(str.c_str(), yy);
	}

	~Lexer()
	{
		yylex_destroy(yy);
	}

	inline int lex(std::string *text)
	{
		int token = yylex(yy);
		pos += yyget_leng(yy);
		if (token > 0) {
			*text = yyget_text(yy);
		}
		return token;
	}

	yyscan_t yy;
	int pos;
};

// Anonymous namespace for local helpers
namespace
{

typedef htmlcxx::HTML::Node HTMLNode;

// Trims whitespace from the beginning and end of a string
std::string trim(const std::string &str)
{
	int start = 0;
	int end = str.length()-1;
	const char *data = str.c_str();

	while (start <= end && isspace(data[start])) ++start;
	while (end >= start && isspace(data[end])) --end;

	if (start > 0 || end < (int)str.length()) {
		return str.substr(start, (end - start + 1));
	}
	return str;
}

// Wrapper for strcasecmp
inline int strcasecmp(const std::string &s1, const std::string &s2)
{
	return ::strcasecmp(s1.c_str(), s2.c_str());
}

// Checks for string prefix
inline bool starts_with(const std::string &str, const std::string &start)
{
	return !::strncasecmp(str.c_str(), start.c_str(), start.length());
}

// Checks for string suffix
inline bool ends_with(const std::string &str, const std::string &end)
{
	if (str.length() >= end.length()) {
		return !::strcasecmp(str.c_str() + str.length() - end.length(), end.c_str());
	}
	return false;
}

// Delete all elements of a container
template<typename T>
inline void delete_all(const T &v)
{
	for (typename T::const_iterator it = v.begin(); it != v.end(); ++it) {
		delete *it;
	}
}

namespace Selectors
{

// Abstract base class for selector functions
struct SelectorFn
{
	typedef tree<HTMLNode>::iterator NodeIt;

	virtual ~SelectorFn() { }
	virtual bool match(const NodeIt &it) const = 0;
};

// Universal selector (*)
struct Universal : SelectorFn
{
	bool match(const NodeIt &) const
	{
		return true;
	}
};

// Type selector (E)
struct Type : SelectorFn
{
	Type(const std::string &type) : type(type) { }

	bool match(const NodeIt &it) const
	{
		return (it->isTag() && !strcasecmp(it->tagName(), type));
	}

	std::string type;
};

// Attribute selector (E[foo])
struct Attribute : SelectorFn
{
	Attribute(const std::string &attr) : attr(attr) { }

	bool match(const NodeIt &it) const
	{
		it->parseAttributes();
		return it->attribute(attr).first;
	}

	std::string attr;
};

// Attribute value, with optional comparison operator (E[foo=bar])
struct AttributeValue : SelectorFn
{
	AttributeValue(const std::string &attr, const std::string &value, char c = '=')
		: attr(attr), value(value), c(c) { }

	bool match(const NodeIt &it) const
	{
		it->parseAttributes();
		std::string str(it->attribute(attr).second);
		switch (c) {
			case '=': return !strcasecmp(str, value);
			case '^': return starts_with(str, value);
			case '$': return ends_with(str, value);
			case '*': return (str.find(value) != std::string::npos);
			case '|': return !(strcasecmp(str, value) && !starts_with(str, value + "-"));
			case '~': {
				std::vector<std::string> tokens;
				std::istringstream iss(str);
				std::copy(std::istream_iterator<std::string>(iss),
						std::istream_iterator<std::string>(),
						std::back_inserter<std::vector<std::string> >(tokens));
				std::vector<std::string>::const_iterator it;
				for (it = tokens.begin(); it != tokens.end(); ++it) {
					if (!strcasecmp(*it, value)) {
						return true;
					}
				}
				return false;
			}
			default: break;
		}
		return true;
	}

	std::string attr;
	std::string value;
	char c;
};

// Pseudo class or element
struct Pseudo : SelectorFn
{
	Pseudo(const std::string &type) : type(type) { }

	bool matchs(const NodeIt &it, const std::string &type) const
	{
		if (type == "root") {
			return (it.node->parent->parent == NULL);
		} else if (type == "first-child") {
			NodeIt jt(it.node->parent->first_child);
			while (jt.node && !jt->isTag()) {
				jt = jt.node->next_sibling;
			}
			return (jt.node == it.node);
		} else if (type == "last-child") {
			NodeIt jt(it.node->parent->last_child);
			while (jt.node && !jt->isTag()) {
				jt = jt.node->prev_sibling;
			}
			return (jt.node == it.node);
		} else if (type == "first-of-type") {
			NodeIt jt(it.node->parent->first_child);
			while (jt.node && (!jt->isTag() || strcasecmp(jt->tagName(), it->tagName()))) {
				jt = jt.node->next_sibling;
			}
			return (jt.node == it.node);
		} else if (type == "last-of-type") {
			NodeIt jt(it.node->parent->last_child);
			while (jt.node && (!jt->isTag() || strcasecmp(jt->tagName(), it->tagName()))) {
				jt = jt.node->prev_sibling;
			}
			return (jt.node == it.node);
		} else if (type == "empty") {
			if (it->isTag()) {
				return (it.node->first_child == NULL);
			}
			return (it->isComment() || it->length() == 0);
		}
		return false;
	}

	bool match(const NodeIt &it) const
	{
		if (type == "only-child") {
			return matchs(it, "first-child") && matchs(it, "last-child");
		} else if (type == "only-of-type") {
			return matchs(it, "first-of-type") && matchs(it, "last-of-type");
		}
		return matchs(it, type);
	}

	std::string type;
};

// Negation (:not)
struct Negation : SelectorFn
{
	Negation(SelectorFn *fn) : fn(fn) { }
	~Negation() { delete fn; }

	bool match(const NodeIt &it) const
	{
		return !fn->match(it);
	}

	SelectorFn *fn;
};

// A simple selector sequence
struct SimpleSequence : SelectorFn
{
	SimpleSequence(const std::vector<SelectorFn *> &fns) : fns(fns) { }
	~SimpleSequence() { delete_all(fns); }

	bool match(const NodeIt &it) const
	{
		std::vector<SelectorFn *>::const_iterator ft(fns.begin());
		std::vector<SelectorFn *>::const_iterator end(fns.end());
		while (ft != end && (*ft)->match(it)) {
			++ft;
		}
		return (ft == end);
	}

	std::vector<SelectorFn *> fns;
};

// Combinator ( , >, ~, +)
struct Combinator : SelectorFn
{
	Combinator(SelectorFn *left, SelectorFn *right, char c) : left(left), right(right), c(c) { }
	~Combinator() { delete left; delete right; }

	bool match(const NodeIt &it) const
	{
		// First, check if the node matches the right side of the combinator
		if (!right->match(it)) {
			return false;
		}

		// Check all suitable neighbor nodes using the left selector
		NodeIt jt;
		switch (c) {
			case ' ': // Descendant
			case '*': // Greatchild or further descendant
				jt = it.node->parent;
				if (c == '*' && jt.node) {
					jt = jt.node->parent;
				}
				while (jt.node) {
					if (left->match(jt)) {
						return true;
					}
					jt = jt.node->parent;
				}
				return false;

			case '>': // Child
				jt = it.node->parent;
				return jt.node && left->match(jt);

			case '+': // Adjacent sibling
				jt = it.node->next_sibling;
				return jt.node && left->match(jt);

			case '~': // General sibling
				jt = it.node->next_sibling;
				while (jt.node) {
					if (left->match(jt)) {
						return true;
					}
					jt = jt.node->next_sibling;
				}
				return false;

			default: break;
		}

		return false;
	}

	SelectorFn *left, *right;
	char c;
};

} // namespace Selectors

using Selectors::SelectorFn;

SelectorFn *parseSelector(Lexer *l, int &token, std::string &s);

// Tries to parse a simple selector
SelectorFn *parseSimpleSequence(Lexer *l, int &token, std::string &s)
{
	std::vector<SelectorFn *> fns;

	// [ type_selector | universal ]
	TRACE("%d: %s\n", token, s.c_str());
	if (token == IDENT) {
		fns.push_back(new Selectors::Type(s));
		token = l->lex(&s);
	} else if (token == '*') {
		fns.push_back(new Selectors::Universal());
		token = l->lex(&s);
	}

	// [ HASH | class | attrib | pseudo | negation ]*
	bool lex = true;
	while (token) {
		switch (token) {
			case HASH:
				fns.push_back(new Selectors::AttributeValue("id", s.substr(1)));
				break;
			case '.':
				token = l->lex(&s);
				if (token != IDENT) throw ParseException(l->pos, "Identfier expected");
				fns.push_back(new Selectors::AttributeValue("class", s, '~'));
				break;
			case '[': {
				token = l->lex(&s);
				if (token == S) token = l->lex(&s);
				if (token != IDENT) throw ParseException(l->pos, "Identifier expected");
				std::string a = s;

				token = l->lex(&s);
				if (token == S) token = l->lex(&s);
				if (token == ']') {
					fns.push_back(new Selectors::Attribute(a));
					break;
				}

				int c = 0;
				switch (token) {
					case INCLUDES: c = '~'; break;
					case DASHMATCH: c = '|'; break;
					case PREFIXMATCH: c = '^'; break;
					case SUFFIXMATCH: c = '$'; break;
					case SUBSTRINGMATCH: c = '*'; break;
					case '=': c = '='; break;
					default: throw ParseException(l->pos, "Invalid character");
				}
				TRACE("got %d, %c\n", token, token);

				token = l->lex(&s);
				if (token == S) token = l->lex(&s);
				if (token != STRING && token != IDENT) throw ParseException(l->pos, "Token is neither string nor identifier"); 
				std::string v = (token == STRING ? s.substr(1, s.length()-2) : s);

				fns.push_back(new Selectors::AttributeValue(a, v, c));
				token = l->lex(&s);
				if (token == S) token = l->lex(&s);
				if (token != ']') throw ParseException(l->pos, "']' expected");
				break;
			}
			case ':': {
				token = l->lex(&s);
				if (token == ':') { token = l->lex(&s); s.insert(0, ":"); }
				if (token == IDENT) {
					fns.push_back(new Selectors::Pseudo(s));
				} else if (token == FUNCTION) {
					// TODO!
				} else {
					throw ParseException(l->pos, "Identifier or funtion expected");
				}
				break;
			}
			case NOT: {
				token = l->lex(&s);
				fns.push_back(new Selectors::Negation(parseSelector(l, token, s)));
				lex = false;
				break;
			}
			case ')': // For negations
				token = l->lex(&s);
				// Fallthrough

			default: goto finish;
		}

		if (lex) {
			token = l->lex(&s);
		}
		lex = true;
	}

finish:
	return new Selectors::SimpleSequence(fns);
}

// Recursive parsing function
SelectorFn *parseSelector(Lexer *l, int &token, std::string &s)
{
	if (token == S) token = l->lex(&s);
	SelectorFn *fn = parseSimpleSequence(l, token, s);

	while (token) {
		TRACE("%d: %s\n", token, s.c_str());
		bool space = false;
		if (token == S) {
			space = true;
			token = l->lex(&s);
		}
		TRACE("%d: %s\n", token, s.c_str());

		char c = -1;
		switch (token) {
			case S: c = ' '; break;
			case PLUS: c = '+'; break;
			case GREATER: c = '>'; break;
			case TILDE: c = '~'; break;
			case '*': c = '*'; break;

			case 0: return fn;
			default:
				if (space) {
					c = ' ';
				} else {
					return fn;
				}
				break;
		}

		if (c != ' ') {
			token = l->lex(&s);
			TRACE("%d: %s\n", token, s.c_str());
			if (token == S) token = l->lex(&s);
		}
		TRACE("%d: %s\n", token, s.c_str());
		SelectorFn *fn2 = parseSimpleSequence(l, token, s);
		fn = new Selectors::Combinator(fn, fn2, c);
	}

	return fn;
}

// Parses a CSS selector expression and returns a set of functions
std::vector<SelectorFn *> parse(const std::string &expr)
{
	std::vector<SelectorFn *> fns;
	int token;
	std::string s;

	Lexer l(trim(expr));
	while ((token = l.lex(&s))) {
		fns.push_back(parseSelector(&l, token, s));
		if (token != COMMA && token != 0) {
			throw ParseException(l.pos, "Comma expected");
		}
	}

	return fns;
}

// Matches a set of nodes against a selector
NodeVector match(const NodeVector &nodes, const SelectorFn *fn)
{
	std::stack<tree<HTMLNode>::iterator> stack;
	for (NodeVector::const_iterator it(nodes.begin()); it != nodes.end(); ++it) {
		stack.push(tree<HTMLNode>::iterator(*it));
	}

	// Depth-first traversal using a stack
	NodeVector result;
	while (!stack.empty()) {
		tree<HTMLNode>::iterator it = stack.top();
		stack.pop();

		// Match all selectors
		if (fn->match(it)) {
			result.push_back(it.node);
			continue;
		}

		// Inspect all child nodes of non-matching elements
		tree<HTMLNode>::sibling_iterator jt;
		for (jt = it.begin(); jt != it.end(); ++jt) {
			stack.push(jt);
		}
	}

	return result;
}

} // Anonymous namespace


// Applies a CSS selector expression to a set of nodes.
NodeVector select(const NodeVector &nodes, const std::string &expr)
{
	// Parse expression
	std::vector<SelectorFn *> fns = parse(expr);

	NodeVector result;
	std::vector<SelectorFn *>::const_iterator it;
	for (it = fns.begin(); it != fns.end(); ++it) {
		NodeVector v = match(nodes, *it);
		result.insert(result.end(), v.begin(), v.end());
	}

	delete_all(fns);
	return result;
}


/*!
 * Constructs an empty selection.
 */
Selector::Selector()
{
}

/*!
 * Constructs a selection containing a whole tree and optionally
 * applies a selector.
 */
Selector::Selector(const tree<HTMLNode> &tree, const std::string &expr)
{
	NodeVector v;
	::tree<HTMLNode>::sibling_iterator it;
	for (it = tree.begin(); it != tree.end(); ++it) {
		v.push_back(it.node);
	}

	if (!expr.empty()) {
		v = hcxselect::select(v, expr);
	}
	assign(v.begin(), v.end());
}

/*!
 * Constructs a selection from a set of nodes and optionally 
 * applies a selector.
 */
Selector::Selector(const NodeVector &nodes, const std::string &expr)
{
	if (!expr.empty()) {
		NodeVector v = hcxselect::select(nodes, expr);
		assign(v.begin(), v.end());
	} else {
		assign(nodes.begin(), nodes.end());
	}
}

/*!
 * Returns a new selection by selecting elements from this 
 * selection using the given selector expression.
 */
Selector Selector::select(const std::string &expr)
{
	return hcxselect::select(*this, expr);
}

} // namespace hcxselect
