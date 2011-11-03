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


#include <string>
#include <istream>
#include <iostream>

#include <htmlcxx/html/ParserDom.h>

#include <hcxselect.h>

using namespace std;


// Pogram entry point
int main(int argc, char **argv)
{
	if (argc < 2) {
		cerr << "Usage: " << *argv << " <selector>" << endl;
		return 1;
	}

	// Read HTML source from stdin
	string source((istreambuf_iterator<char>(cin)), istreambuf_iterator<char>());

	htmlcxx::HTML::ParserDom parser;
	tree<htmlcxx::HTML::Node> dom = parser.parseTree(source);

	hcxselect::Selector s(dom);
	try {
		s = s.select(argv[1]);
	} catch (hcxselect::ParseException &ex) {
		cerr << "Parse error: '" << argv[1] << "': " << ex.what() << endl;
		cerr << "              ";
		for (int i = 1; i < ex.position(); i++) {
			cerr << " ";
		}
		cerr << "^" << endl;
		return 1;
	} catch (...) {
		cerr << "Error parsing '" << argv[1] << "'" << endl;
		return 1;
	}

	for (hcxselect::Selector::const_iterator it = s.begin(); it != s.end(); ++it) {
		cout << source.substr((*it)->data.offset(), (*it)->data.length());
	}
	return 0;
}
