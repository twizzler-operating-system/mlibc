
#include <ctype.h>
#include <wctype.h>

#include <bits/ensure.h>
#include <mlibc/charset.hpp>

// --------------------------------------------------------------------------------------
// char ctype functions.
// --------------------------------------------------------------------------------------

int isalpha(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_alpha(cp);
}

int isdigit(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_digit(cp);
}

int isxdigit(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_xdigit(cp);
}

int isalnum(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_alnum(cp);
}

int ispunct(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_punct(cp);
}

int isgraph(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_graph(cp);
}

int isblank(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_blank(cp);
}

int isspace(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_space(cp);
}

int isprint(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_print(cp);
}

int islower(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_lower(cp);
}

int isupper(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_upper(cp);
}

int iscntrl(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::generic_is_control(cp);
}

// --------------------------------------------------------------------------------------
// wchar_t ctype functions.
// --------------------------------------------------------------------------------------

int iswalpha(wint_t nc) {
	auto cc = mlibc::platform_wide_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_alpha(cp);
}

int iswdigit(wint_t nc) {
	auto cc = mlibc::platform_wide_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_digit(cp);
}

int iswxdigit(wint_t nc) {
	auto cc = mlibc::platform_wide_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_xdigit(cp);
}

int iswalnum(wint_t nc) {
	auto cc = mlibc::platform_wide_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_alnum(cp);
}

int iswpunct(wint_t nc) {
	auto cc = mlibc::platform_wide_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_punct(cp);
}

int iswgraph(wint_t nc) {
	auto cc = mlibc::platform_wide_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_graph(cp);
}

int iswblank(wint_t nc) {
	auto cc = mlibc::platform_wide_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_blank(cp);
}

int iswspace(wint_t nc) {
	auto cc = mlibc::platform_wide_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_space(cp);
}

int iswprint(wint_t nc) {
	auto cc = mlibc::platform_wide_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_print(cp);
}

int iswlower(wint_t nc) {
	auto cc = mlibc::platform_wide_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_lower(cp);
}

int iswupper(wint_t nc) {
	auto cc = mlibc::platform_wide_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::current_charset()->is_upper(cp);
}

int iswcntrl(wint_t nc) {
	auto cc = mlibc::platform_wide_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return 0;
	return mlibc::generic_is_control(cp);
}

// --------------------------------------------------------------------------------------
// iswctype functions.
// --------------------------------------------------------------------------------------

namespace {
	enum {
		ct_null,
		ct_alnum,
		ct_alpha,
		ct_blank,
		ct_cntrl,
		ct_digit,
		ct_graph,
		ct_lower,
		ct_print,
		ct_punct,
		ct_space,
		ct_upper,
		ct_xdigit,
		ct_count
	};
}

wctype_t wctype(const char *cs) {
	frg::string_view s{cs};
	if(s == "alnum") return ct_alnum;
	if(s == "alpha") return ct_alpha;
	if(s == "blank") return ct_blank;
	if(s == "cntrl") return ct_cntrl;
	if(s == "digit") return ct_digit;
	if(s == "graph") return ct_graph;
	if(s == "lower") return ct_lower;
	if(s == "print") return ct_print;
	if(s == "punct") return ct_punct;
	if(s == "space") return ct_space;
	if(s == "upper") return ct_upper;
	if(s == "xdigit") return ct_xdigit;
	mlibc::infoLogger() << "mlibc: wctype(\"" << cs << "\") is not supported" << frg::endlog;
	return ct_null;
}

int iswctype(wint_t, wctype_t) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

// --------------------------------------------------------------------------------------
// char conversion functions.
// --------------------------------------------------------------------------------------

int tolower(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return nc;
	return mlibc::current_charset()->to_lower(cp);
}

int toupper(int nc) {
	auto cc = mlibc::current_charcode();
	mlibc::codepoint cp;
	if(auto e = cc->promote(nc, cp); e != mlibc::charcode_error::null)
		return nc;
	return mlibc::current_charset()->to_upper(cp);
}

// --------------------------------------------------------------------------------------
// wchar_t conversion functions.
// --------------------------------------------------------------------------------------

wint_t towlower(wint_t) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

wint_t towupper(wint_t) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

