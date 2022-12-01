/*
 Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UTIL_H
#define UTIL_H

#include<signal.h>
//#include<stdint.h>
#include<string>
#include<functional>
#include<map>
#include<vector>


typedef unsigned char uchar;

#define call(A,B) ([&](){A->B();})
#define calll(A,B,T) ([&](T t){A->B(t);})

enum class TestBit
{
    Set,
    NotSet
};

uchar bcd2bin(uchar c);
bool hex2bin(const char* src, std::vector<uchar> *target);
bool hex2bin(std::string &src, std::vector<uchar> *target);
bool hex2bin(std::vector<uchar> &src, std::vector<uchar> *target);
std::string bin2hex(const uchar* target, int len);
std::string bin2hex(const std::vector<uchar> &target);
std::string bin2hex(std::vector<uchar>::iterator data, std::vector<uchar>::iterator end, int len);
std::string bin2hex(std::vector<uchar> &data, int offset, int len);
void strprintf(std::string &s, const char* fmt, ...);
void xorit(uchar *srca, uchar *srcb, uchar *dest, int len);
bool doesIdMatchExpressions(std::string id, std::vector<std::string>& match_rules, bool *used_wildcard);
std::vector<std::string> splitMatchExpressions(std::string& mes);
void incrementIV(uchar *iv, size_t len);
std::string eatTo(std::vector<uchar> &v, std::vector<uchar>::iterator &i, int c, size_t max, bool *eof, bool *err);
void trimWhitespace(std::string *s);
uchar *safeButUnsafeVectorPtr(std::vector<uchar> &v);

#ifndef FUZZING
#define FUZZING false
#endif

#endif
