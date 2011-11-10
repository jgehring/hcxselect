hcxselect - A CSS selector engine for htmlcxx
=============================================

hcxselect is a small CSS selector engine for htmlcxx. It parses
CSS selector expressions and applies them to a set of trees of HTML
nodes parsed via htmlcxx.


# Usage

The "library" consists of a header and a source file and is currently
meant to be directly included in other software, as it provides no
installation options. Please be aware the flex is needed to generate
code for the lexical scanner. However, this needs to be done only once,
and the respective header file can be used directly.


# Compliance

hcxselect aims to comply with the Selectors Level 3 specification
<http://www.w3.org/TR/selectors/>. However, a couple of browser-specific
selectors are not implemented, as they don't make sense here:

	* pseudo-classes:
	  :link, :visited, :hover, :active, :focus, :target, :lang,
	  :enabled, :disabled, :checked, :indeterminate

	* pseudo-elements:
	  ::before, ::after

The following extra pseudo-classes are provided for convenience:

	* :text matches text (i.e., non-text and non-comment node)
	* :comment matches comment nodes


# License

hcxselect licensed under 3-clause BSD license. However, please be
aware that htmlcxx itself uses the GNU Lesser General Public License
(LGPL) and uses tree.hh, which in turn is available under the GNU
General Public License (GPL), version 2 or 3.
