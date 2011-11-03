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
#include <istream>
#include <sstream>
#include <string>

#include <htmlcxx/html/ParserDom.h>

#include <hcxselect.h>

using namespace std;


const char *rawsource = \
"<html>"
"  <ul>"
"    <li>A list element</li>"
"    <li>Another one</li>"
"  </ul>"
"  <p id=\"foobar\">This is a paragraph</p>"
"  <nonsense id=\"id1\">This is not real</nonsense>"
"  <p title=\"title\">"
"    A paragraph with a title"
"    <span class=\"class1\" lang=\"en-fr\">A span</span>"
"  </p>"
"  <p title=\"t2\" lang=\"en-gb\">Another one</p>"
"  <span class=\"a bb c\">Multi-class span</span>"
"  <a class=\"13\" href=\"http://example.com\">ref</a>"
"</html>"
;

struct tvec {
	const char *s;
	int n; // < 0: Syntax error expected
	const char *t;
} vectors[] = {
	{"li,nonsense", 3, "<li>Another one</li><li>A list element</li><nonsense id=\"id1\">This is not real</nonsense>"},
	{"nonsense", 1, "<nonsense id=\"id1\">This is not real</nonsense>"},
	{"*", 1, rawsource},
	{"*.class1", 1, "<span class=\"class1\" lang=\"en-fr\">A span</span>"},
	{"#foobar", 1, "<p id=\"foobar\">This is a paragraph</p>"},
	{"p[title]", 2, "<p title=\"t2\" lang=\"en-gb\">Another one</p><p title=\"title\">    A paragraph with a title    <span class=\"class1\" lang=\"en-fr\">A span</span>  </p>"},
	{"p[title=\"t2\"]", 1, "<p title=\"t2\" lang=\"en-gb\">Another one</p>"},
	{"p[title='t2']", 1, "<p title=\"t2\" lang=\"en-gb\">Another one</p>"},
	{"span[class~=\"c\"]", 1, "<span class=\"a bb c\">Multi-class span</span>"},
	{"span[class~=\"b\"]", 0, ""},
	{"span[class~=\"a bb\"]", 0, ""},
	{"p[lang|=\"en\"]", 1, "<p title=\"t2\" lang=\"en-gb\">Another one</p>"},
	{"p[lang|=\"fr\"]", 0, ""},
	{"p[title^='ti']", 1, "<p title=\"title\">    A paragraph with a title    <span class=\"class1\" lang=\"en-fr\">A span</span>  </p>"},
	{"span[class~=\"c\"]", 1, "<span class=\"a bb c\">Multi-class span</span>"},
	{"span[class~=\"b\"]", 0, ""},
	{"span[class~=\"a bb\"]", 0, ""},
	{"p[lang|=\"en\"]", 1, "<p title=\"t2\" lang=\"en-gb\">Another one</p>"},
	{"p[lang|=\"fr\"]", 0, ""},
	{"p[id^=\"foo\"]", 1, "<p id=\"foobar\">This is a paragraph</p>"},
	{"p[id$=\"bar\"]", 1, "<p id=\"foobar\">This is a paragraph</p>"},
	{"p[id*=\"oob\"]", 1, "<p id=\"foobar\">This is a paragraph</p>"},
	{".class1", 1, "<span class=\"class1\" lang=\"en-fr\">A span</span>"},
	{".cl", 0, ""},
	{".cl.ass1", 0, ""},
	{".a", 1, "<span class=\"a bb c\">Multi-class span</span>"},
	{".a.a", 1, "<span class=\"a bb c\">Multi-class span</span>"},
	{".a:not(.bb)", 0, ""},
	{":not(.a).bb", 0, ""},
	{"span.bb:not(.a):not(.a)", 0, ""},
	{"#foo", 0, ""},
	{"#foo#id1", 0, ""},
	{"#id1#id1", 1, "<nonsense id=\"id1\">This is not real</nonsense>"},
	{".13", -1, ""},
	{".\\13", 0, ""},
	{".\\31 \\33", 1, "<a class=\"13\" href=\"http://example.com\">ref</a>"},
};


// Program entry point
int main(int argc, char **argv)
{
	// Parse HTML tree
	string source(rawsource);
	htmlcxx::HTML::ParserDom parser;
	tree<htmlcxx::HTML::Node> dom = parser.parseTree(source);

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
			cerr << "Parse error: '" << vectors[i].s << "': " << ex.what() << endl;
			cerr << "              ";
			for (int i = 1; i < ex.position(); i++) {
				cerr << " ";
			}
			cerr << "^" << endl;
			return 1;
		} catch (...) {
			cerr << endl;
			cerr << "Error parsing '" << vectors[i].s << "'" << endl;
			return 1;
		}

		if (vectors[i].n < 0) {
			cerr << endl;
			cerr << i << " (" << vectors[i].s << ") failed: " <<
				"Parse exception expected" << endl;
			return 1;
		}

		for (hcxselect::Selector::const_iterator it = s.begin(); it != s.end(); ++it) {
			ss << source.substr((*it)->data.offset(), (*it)->data.length());
		}

		if (s.size() != (size_t)vectors[i].n) {
			cerr << endl;
			cerr << i << " (" << vectors[i].s << ") failed: " <<
				"Expected " << vectors[i].n << " results, got " <<
				s.size() << ":" << endl << ss.str() << endl;
			return 1;
		}

		if (ss.str() != vectors[i].t) {
			cerr << endl;
			cerr << i << " (" << vectors[i].s << ") failed: " <<
				"Expected " << vectors[i].t << ", got " << ss.str() << endl;
			return 1;
		}

pass:
		cout << i << (i > 0 && i % 20 == 0 ? "\n" : " ");
	}
	cout << endl;

	return 0;
}
