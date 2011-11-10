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


#include <iostream>
#include <iomanip>
#include <istream>
#include <sstream>
#include <string>

#include <htmlcxx/html/ParserDom.h>

#include <hcxselect.h>

using namespace std;


const char *rawsource = \
"<html>"
"  <ul>"
"    <li><bla></bla></li>"
"    <li n=\"2\"> </li>"
"  </ul>"
"  <p id=\"foobar\">This is a paragraph</p>"
"  <nonsense id=\"id1\">This is not real</nonsense>"
"  <p title=\"title\">"
"    A paragraph with a title"
"    <span class=\"class1\" lang=\"en-fr\">A span</span>"
"    <table><tr><td>"
"      <span class=\"sp\">Span in table</span>"
"    </td></tr></table>"
"  </p>"
"  <p title=\"t2\" lang=\"en-gb\">Another one</p>"
"  <span class=\"a bb c\">Multi-class span</span>"
"  <div class=\"one.word\">hooray"
"    <a class=\"13\" href=\"http://example.com\">ref</a>"
"  </div>"
"  <div class=\"span\">foobar</div>"
"  <table id=\"t\" class=\"\"><!-- A comment --></table>"
"</html>"
;

// Long selector strings at the end of the file
extern const char *s170, *s170a, *s170b, *s170c, *s170d;

// Test vectors from http://www.w3.org/Style/CSS/Test/CSS3/Selectors/current
struct tvec {
	const char *s;
	int n; // < 0: Syntax error expected
	const char *t;
} vectors[] = {
	{"li,nonsense", 3, "<li></li>,<li n=\"2\"></li>,<nonsense id=\"id1\"></nonsense>"}, // 1
	{"nonsense", 1, "<nonsense id=\"id1\"></nonsense>"}, // 2
	{"*", 19, "<html></html>,<ul></ul>,<li></li>,<bla></bla>,<li n=\"2\"></li>,<p id=\"foobar\"></p>,<nonsense id=\"id1\"></nonsense>,<p title=\"title\"></p>,<span class=\"class1\" lang=\"en-fr\"></span>,<table></table>,<tr></tr>,<td></td>,<span class=\"sp\"></span>,<p title=\"t2\" lang=\"en-gb\"></p>,<span class=\"a bb c\"></span>,<div class=\"one.word\"></div>,<a class=\"13\" href=\"http://example.com\"></a>,<div class=\"span\"></div>,<table id=\"t\" class=\"\"></table>"}, // 3
	{"*.class1", 1, "<span class=\"class1\" lang=\"en-fr\"></span>"}, // 3
	{"#foobar", 1, "<p id=\"foobar\"></p>"}, // 4
	{"p[title]", 2, "<p title=\"title\"></p>,<p title=\"t2\" lang=\"en-gb\"></p>"}, // 5
	{"p[title=\"t2\"]", 1, "<p title=\"t2\" lang=\"en-gb\"></p>"}, // 6
	{"p[title='t2']", 1, "<p title=\"t2\" lang=\"en-gb\"></p>"}, // 6
	{"span[class~=\"c\"]", 1, "<span class=\"a bb c\"></span>"}, // 7
	{"span[class~=\"b\"]", 0, ""},  // 7
	{"span[class~=\"a bb\"]", 0, ""}, // 7b
	{"p[lang|=\"en\"]", 1, "<p title=\"t2\" lang=\"en-gb\"></p>"}, // 8
	{"p[lang|=\"fr\"]", 0, ""}, // 8
	{"p[title^='ti']", 1, "<p title=\"title\"></p>"}, // 9
	{"p[id^=\"foo\"]", 1, "<p id=\"foobar\"></p>"}, // 9
	{"p[id$=\"bar\"]", 1, "<p id=\"foobar\"></p>"}, // 10
	{"p[id*=\"oob\"]", 1, "<p id=\"foobar\"></p>"}, // 11
	{".class1", 1, "<span class=\"class1\" lang=\"en-fr\"></span>"}, // 13
	{".cl", 0, ""}, // 13
	{".cl.ass1", 0, ""}, // 14
	{".a", 1, "<span class=\"a bb c\"></span>"}, // 14
	{".a.a", 1, "<span class=\"a bb c\"></span>"}, // 14
	{".a:not(.bb)", 0, ""}, // 14c
	{":not(.a).bb", 0, ""}, // 14c
	{"span.bb:not(.a):not(.a)", 0, ""}, // 14e
	{"#foo", 0, ""}, // 15
	{"#foo#id1", 0, ""}, // 15b
	{"#id1#id1", 1, "<nonsense id=\"id1\"></nonsense>"}, // 15b
	{"*:root", 1, "<html></html>"}, // 27
	{":root:first-child", 0, ""}, // 27b
	{":root:last-child", 0, ""}, // 27b
	{":root:only-child", 0, ""}, // 27b
	{":root:nth-child(1)", 0, ""}, // 27b
	{":root:nth-child(n)", 0, ""}, // 27b
	{":root:first-of-type", 0, ""}, // 27b
	{":root:last-of-type", 0, ""}, // 27b
	{":root:only-of-type", 0, ""}, // 27b
	{":root:nth-of-type(1)", 0, ""}, // 27b
	{":root:nth-of-type(n)", 0, ""}, // 27b
	{":root:nth-last-of-type(1)", 0, ""}, // 27b
	{":root:nth-last-of-type(n)", 0, ""}, // 27b
	{"* :root", 0, ""}, // 27c
	{"* html", 0, ""}, // 27c
	{"li:nth-child(odd)", 1, "<li></li>"}, // 28
	{"li:nth-child(even)", 1, "<li n=\"2\"></li>"}, // 28
	{"p:nth-child(4)", 1, "<p title=\"title\"></p>"}, // 28
	{"p:nth-child(20n+2)", 1, "<p id=\"foobar\"></p>"}, // 28
	{"p:nth-child(-4)", -1, ""}, // 28
	{"p:nth-child(2n-4)", -1, ""}, // 28
	{"p:nth-child(2n)", 2, "<p id=\"foobar\"></p>,<p title=\"title\"></p>"}, // 28
	{"a:nth-child(n+2)", 1, "<a class=\"13\" href=\"http://example.com\"></a>"}, // 28
	{"p:nth-last-child(5)", 1, "<p title=\"t2\" lang=\"en-gb\"></p>"}, // 29
	{"p:nth-last-child(4n+7)", 0, ""}, // 29
	{"p:nth-of-type(1)", 1, "<p id=\"foobar\"></p>"}, // 30
	{"p:nth-of-type(n)", 3, "<p id=\"foobar\"></p>,<p title=\"title\"></p>,<p title=\"t2\" lang=\"en-gb\"></p>"}, // 30
	{"p:nth-last-of-type(1)", 1, "<p title=\"t2\" lang=\"en-gb\"></p>"}, // 31
	{"p:nth-last-of-type(10n+20)", 0, ""}, // 31
	{"p > *:first-child", 1, "<span class=\"class1\" lang=\"en-fr\"></span>"}, // 32
	{"html > *:last-child", 1, "<table id=\"t\" class=\"\"></table>"}, // 33
	{"p:first-of-type", 1, "<p id=\"foobar\"></p>"}, // 34
	{"p:last-of-type", 1, "<p title=\"t2\" lang=\"en-gb\"></p>"}, // 35
	{"span:only-child", 1, "<span class=\"sp\"></span>"}, // 36
	{":only-of-type", 11, "<ul></ul>,<bla></bla>,<nonsense id=\"id1\"></nonsense>,<span class=\"class1\" lang=\"en-fr\"></span>,<table></table>,<tr></tr>,<td></td>,<span class=\"sp\"></span>,<span class=\"a bb c\"></span>,<a class=\"13\" href=\"http://example.com\"></a>,<table id=\"t\" class=\"\"></table>"}, // 37
	{"p span", 2, "<span class=\"class1\" lang=\"en-fr\"></span>,<span class=\"sp\"></span>"}, // 43
	{"p > span", 1, "<span class=\"class1\" lang=\"en-fr\"></span>"}, // 44
	{"p + span", 1, "<span class=\"a bb c\"></span>"}, // 45
	{"p ~ div", 2, "<div class=\"one.word\"></div>,<div class=\"span\"></div>"}, // 46
	{"p * span", 1, "<span class=\"sp\"></span>"}, // ?
	{"p:not([title^=\"t\"])", 1, "<p id=\"foobar\"></p>"}, // 54
	{"p:not([id$=\"bar\"])", 2, "<p title=\"title\"></p>,<p title=\"t2\" lang=\"en-gb\"></p>"}, // 55
	{"p:not([title*=\"tl\"])", 2, "<p id=\"foobar\"></p>,<p title=\"t2\" lang=\"en-gb\"></p>"}, // 56
	{"div:not(.span)", 1, "<div class=\"one.word\"></div>"}, // 57
	{"table:not(#t)", 1, "<table></table>"}, // 58
	{"a:not(:root)", 1, "<a class=\"13\" href=\"http://example.com\"></a>"}, // 72
	{"html:not(:root), test:not(:root)", 0, ""}, // 72b
	{"p:not(:nth-child(2n))", 1, "<p title=\"t2\" lang=\"en-gb\"></p>"}, // 73
	{"p:not(:nth-last-child(4n+7))", 3, "<p id=\"foobar\"></p>,<p title=\"title\"></p>,<p title=\"t2\" lang=\"en-gb\"></p>"}, // 74
	{"p:not(:nth-of-type(n))", 0, ""}, // 75
	{"p:not(:nth-last-of-type(10n+20))", 3, "<p id=\"foobar\"></p>,<p title=\"title\"></p>,<p title=\"t2\" lang=\"en-gb\"></p>"}, // 76
	{"p > *:not(:first-child)", 1, "<table></table>"}, // 77
	{"html > *:not(:last-child)", 8, "<ul></ul>,<p id=\"foobar\"></p>,<nonsense id=\"id1\"></nonsense>,<p title=\"title\"></p>,<p title=\"t2\" lang=\"en-gb\"></p>,<span class=\"a bb c\"></span>,<div class=\"one.word\"></div>,<div class=\"span\"></div>"}, // 78
	{"p:not(:first-of-type)", 2, "<p title=\"title\"></p>,<p title=\"t2\" lang=\"en-gb\"></p>"}, // 79
	{"p:not(:last-of-type)", 2, "<p id=\"foobar\"></p>,<p title=\"title\"></p>"}, // 80
	{"span:not(:only-child)", 2, "<span class=\"class1\" lang=\"en-fr\"></span>,<span class=\"a bb c\"></span>"}, // 81
	{"*:not(:only-of-type)", 8, "<html></html>,<li></li>,<li n=\"2\"></li>,<p id=\"foobar\"></p>,<p title=\"title\"></p>,<p title=\"t2\" lang=\"en-gb\"></p>,<div class=\"one.word\"></div>,<div class=\"span\"></div>"}, // 82
	{"p:not(:not(:first-of-type))", 1, "<p id=\"foobar\"></p>"}, // !83
	{"p > table td", 1, "<td></td>"},  // 86
	{"p + span ~ table", 1, "<table id=\"t\" class=\"\"></table>"}, // 87
	{"span + div a", 1, "<a class=\"13\" href=\"http://example.com\"></a>"}, // 88
	{"p td > span", 1, "<span class=\"sp\"></span>"}, // 89
	{"p ~ div + table", 1, "<table id=\"t\" class=\"\"></table>"}, // 90
	{"table:empty", 1, "<table id=\"t\" class=\"\"></table>"}, // 148, 150
	{"li:empty", 0, ""}, // 151, 152
	{".\\31 \\33", 1, "<a class=\"13\" href=\"http://example.com\"></a>"}, // 175c
	{"p.", -1, ""}, // 154
	{".13", -1, ""}, // 155, 175a
	{".\\13", 0, ""}, // 155a, 175b
	{".a\\ bb\\ c", 0, ""}, // 155b
	{".one.word", 0, ""}, // 155c
	{".one\\.word", 1, "<div class=\"one.word\"></div>"}, // 155d
	{"a & span, p", -1, ""}, // 156
	{"[*=t2]", -1, ""}, // 157
	{"[*|*=t2]", -1, ""}, // 158
	{s170, 3, "<span class=\"class1\" lang=\"en-fr\"></span>,<span class=\"sp\"></span>,<span class=\"a bb c\"></span>"}, // 170
	{s170a, 1, "<div class=\"span\"></div>"}, // 170a
	{s170b, 1, "<div class=\"span\"></div>"}, // 170b
	{s170c, 1, "<a class=\"13\" href=\"http://example.com\"></a>"}, // 170c
	{s170d, 1, "<a class=\"13\" href=\"http://example.com\"></a>"}, // 170d
	{"span::first-child", 0, ""}, // 177b
	{"span:not(:first-child)", 1, "<span class=\"a bb c\"></span>"}, // 178
	{".one\\.word A", 1, "<a class=\"13\" href=\"http://example.com\"></a>"}, // 181
	{".bb.", -1, ""}, // 183
	{"..bb", -1, ""}, // 183
	{".bb..c", -1, ""}, // 183
	{"table[class$=\"\"]", 0, ""}, // 184a
	{"table[class^=\"\"]", 0, ""}, // 184b
	{"table[class*=\"\"]", 0, ""}, // 184c
	{"table:not([class$=\"\"])", 2, "<table></table>,<table id=\"t\" class=\"\"></table>"}, // 184d
	{"table:not([class^=\"\"])", 2, "<table></table>,<table id=\"t\" class=\"\"></table>"}, // 184e
	{"table:not([class*=\"\"])", 2, "<table></table>,<table id=\"t\" class=\"\"></table>"}, // 184f
};


// Program entry point
int main(int argc, char **argv)
{
	// Parse HTML tree
	string source(rawsource);
	htmlcxx::HTML::ParserDom parser;
	tree<htmlcxx::HTML::Node> dom = parser.parseTree(source);
	cout << setfill('0');

	for (size_t i = 0; i < sizeof(vectors) / sizeof(tvec); i++) {
		stringstream ss;
		hcxselect::Selector s(dom);

		try {
			s = s.select(vectors[i].s);
		} catch (hcxselect::ParseException &ex) {
			if (vectors[i].n < 0) {
				goto pass;
			}
			cerr << endl;
			cerr << "Parse error: { " << vectors[i].s << " }: " << ex.what() << endl;
			cerr << "               ";
			for (int i = 1; i < ex.position(); i++) {
				cerr << " ";
			}
			cerr << "^" << endl;
			return 1;
		} catch (...) {
			cerr << endl;
			cerr << "Error parsing { " << vectors[i].s << " }" << endl;
			return 1;
		}

		if (vectors[i].n < 0) {
			cerr << endl;
			cerr << i << " { " << vectors[i].s << " } failed: " <<
				"Parse exception expected" << endl;
			return 1;
		}

		for (hcxselect::Selector::const_iterator it = s.begin(); it != s.end(); ++it) {
			if (it != s.begin()) ss << ",";
			ss << (*it)->data.text() << (*it)->data.closingText();
		}

		if (s.size() != (size_t)vectors[i].n) {
			cerr << endl;
			cerr << i << " { " << vectors[i].s << " } failed: " <<
				"Expected " << vectors[i].n << " results, got " <<
				s.size() << ":" << endl << ss.str() << endl;
			return 1;
		}

		if (ss.str() != vectors[i].t) {
			cerr << endl;
			cerr << i << " { " << vectors[i].s << " } failed: " <<
				"Expected " << vectors[i].t << ", got " << ss.str() << endl;
			return 1;
		}

pass:
		cout << setw(2) << hex << i;
		cout << (i > 0 && (i+1) % 16 == 0 ? "\n" : " ");
		cout << flush;
	}
	cout << endl;

	return 0;
}


// Long selector strings
const char *s170 = "span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span, span";
const char *s170a = "  .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span, .span";
const char *s170b = ".span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span.span";
const char *s170c = "a:not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span):not(.span)";
const char *s170d = "a:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child:first-child";
