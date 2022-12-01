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


#include"util.h"

#include <ctime>


using namespace std;

// Sigint, sigterm will call the exit handler.
function<void()> exit_handler_;

bool got_hupped_ {};

void exitHandler(int signum)
{
    got_hupped_ = signum == SIGHUP;
    if (exit_handler_) exit_handler_();
}

bool gotHupped()
{
    return got_hupped_;
}


void doNothing(int signum)
{
}

struct sigaction old_int, old_hup, old_term, old_chld, old_usr1, old_usr2;


bool signalsInstalled()
{
    return exit_handler_ != NULL;
}

int char2int(char input)
{
    if(input >= '0' && input <= '9')
        return input - '0';
    if(input >= 'A' && input <= 'F')
        return input - 'A' + 10;
    if(input >= 'a' && input <= 'f')
        return input - 'a' + 10;
    return -1;
}

bool isHexChar(uchar c)
{
    return char2int(c) != -1;
}

// The byte 0x13 i converted into the integer value 13.
uchar bcd2bin(uchar c)
{
    return (c&15)+(c>>4)*10;
}

// The byte 0x13 is converted into the integer value 31.
uchar revbcd2bin(uchar c)
{
    return (c&15)*10+(c>>4);
}

uchar reverse(uchar c)
{
    return ((c&15)<<4) | (c>>4);
}


bool isHexString(const char* txt, bool *invalid, bool strict)
{
    *invalid = false;
    // An empty string is not an hex string.
    if (*txt == 0) return false;

    const char *i = txt;
    int n = 0;
    for (;;)
    {
        char c = *i++;
        if (!strict && c == '#') continue; // Ignore hashes if not strict
        if (!strict && c == ' ') continue; // Ignore hashes if not strict
        if (!strict && c == '|') continue; // Ignore hashes if not strict
        if (!strict && c == '_') continue; // Ignore underlines if not strict
        if (c == 0) break;
        n++;
        if (char2int(c) == -1) return false;
    }
    // An empty string is not an hex string.
    if (n == 0) return false;
    if (n%2 == 1) *invalid = true;

    return true;
}

bool isHexStringFlex(const char* txt, bool *invalid)
{
    return isHexString(txt, invalid, false);
}

bool isHexStringFlex(const std::string &txt, bool *invalid)
{
    return isHexString(txt.c_str(), invalid, false);
}

bool isHexStringStrict(const char* txt, bool *invalid)
{
    return isHexString(txt, invalid, true);
}

bool isHexStringStrict(const std::string &txt, bool *invalid)
{
    return isHexString(txt.c_str(), invalid, true);
}

bool hex2bin(const char* src, vector<uchar> *target)
{
    if (!src) return false;
    while(*src && src[1]) {
        if (*src == ' ' || *src == '#' || *src == '|' || *src == '_') {
            // Ignore space and hashes and pipes and underlines.
            src++;
        } else {
            int hi = char2int(*src);
            int lo = char2int(src[1]);
            if (hi<0 || lo<0) return false;
            target->push_back(hi*16 + lo);
            src += 2;
        }
    }
    return true;
}

bool hex2bin(string &src, vector<uchar> *target)
{
    return hex2bin(src.c_str(), target);
}

bool hex2bin(vector<uchar> &src, vector<uchar> *target)
{
    if (src.size() % 2 == 1) return false;
    for (size_t i=0; i<src.size(); i+=2) {
        if (src[i] != ' ') {
            int hi = char2int(src[i]);
            int lo = char2int(src[i+1]);
            if (hi<0 || lo<0) return false;
            target->push_back(hi*16 + lo);
        }
    }
    return true;
}

char const hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A','B','C','D','E','F'};


std::string bin2hex(const uchar* target, int len) {
    std::string str;
    for (size_t i = 0; i < len; ++i) {
        const char ch = target[i];
        str.append(&hex[(ch  & 0xF0) >> 4], 1);
        str.append(&hex[ch & 0xF], 1);
    }
    return str;
}



std::string bin2hex(const vector<uchar> &target) {
    std::string str;
    for (size_t i = 0; i < target.size(); ++i) {
        const char ch = target[i];
        str.append(&hex[(ch  & 0xF0) >> 4], 1);
        str.append(&hex[ch & 0xF], 1);
    }
    return str;
}

std::string bin2hex(vector<uchar>::iterator data, vector<uchar>::iterator end, int len) {
    std::string str;
    while (data != end && len-- > 0) {
        const char ch = *data;
        data++;
        str.append(&hex[(ch  & 0xF0) >> 4], 1);
        str.append(&hex[ch & 0xF], 1);
    }
    return str;
}

std::string bin2hex(vector<uchar> &data, int offset, int len) {
    std::string str;
    vector<uchar>::iterator i = data.begin();
    i += offset;
    while (i != data.end() && len-- > 0) {
        const char ch = *i;
        i++;
        str.append(&hex[(ch  & 0xF0) >> 4], 1);
        str.append(&hex[ch & 0xF], 1);
    }
    return str;
}

std::string safeString(vector<uchar> &target) {
    std::string str;
    for (size_t i = 0; i < target.size(); ++i) {
        const char ch = target[i];
        if (ch >= 32 && ch < 127 && ch != '<' && ch != '>') {
            str += ch;
        } else {
            str += '<';
            str.append(&hex[(ch  & 0xF0) >> 4], 1);
            str.append(&hex[ch & 0xF], 1);
            str += '>';
        }
    }
    return str;
}

void xorit(uchar *srca, uchar *srcb, uchar *dest, int len)
{
    for (int i=0; i<len; ++i) { dest[i] = srca[i]^srcb[i]; }
}

void shiftLeft(uchar *srca, uchar *srcb, int len)
{
    uchar overflow = 0;

    for (int i = len-1; i >= 0; i--)
    {
        srcb[i] = srca[i] << 1;
        srcb[i] |= overflow;
        overflow = (srca[i] & 0x80) >> 7;
    }
    return;
}

string format3fdot3f(double v)
{
    string r;
    strprintf(r, "%3.3f", v);
    return r;
}


bool is_ascii_alnum(char c)
{
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c >= '0' && c <= '9') return true;
    if (c == '_') return true;
    return false;
}

bool is_ascii(char c)
{
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    return false;
}

bool isValidAlias(string alias)
{
    if (alias.length() == 0) return false;

    if (!is_ascii(alias[0])) return false;

    for (char c : alias)
    {
        if (!is_ascii_alnum(c)) return false;
    }

    return true;
}

bool isValidMatchExpression(string me, bool non_compliant)
{
    // Examples of valid match expressions:
    //  12345678
    //  *
    //  123*
    // !12345677
    //  2222222*
    // !22222222

    // A match expression cannot be empty.
    if (me.length() == 0) return false;

    // An me can be negated with an exclamation mark first.
    if (me.front() == '!') me.erase(0, 1);

    // A match expression cannot be only a negation mark.
    if (me.length() == 0) return false;

    int count = 0;
    if (non_compliant)
    {
        // Some non-compliant meters have full hex in the id,
        // but according to the standard there should only be bcd here...
        while (me.length() > 0 &&
               ((me.front() >= '0' && me.front() <= '9') ||
                (me.front() >= 'a' && me.front() <= 'f')))
        {
            me.erase(0,1);
            count++;
        }
    }
    else
    {
        // But compliant meters use only a bcd subset.
        while (me.length() > 0 &&
               (me.front() >= '0' && me.front() <= '9'))
        {
            me.erase(0,1);
            count++;
        }
    }

    bool wildcard_used = false;
    // An expression can end with a *
    if (me.length() > 0 && me.front() == '*')
    {
        me.erase(0,1);
        wildcard_used = true;
    }

    // Now we should have eaten the whole expression.
    if (me.length() > 0) return false;

    // Check the length of the matching bcd/hex
    // If no wildcard is used, then the match expression must be exactly 8 digits.
    if (!wildcard_used) return count == 8;

    // If wildcard is used, then the match expressions must be 7 or less digits,
    // even zero is allowed which means a single *, which matches any bcd/hex id.
    return count <= 7;
}

bool isValidMatchExpressions(string mes, bool non_compliant)
{
    vector<string> v = splitMatchExpressions(mes);

    for (string me : v)
    {
        if (!isValidMatchExpression(me, non_compliant)) return false;
    }
    return true;
}

bool isValidId(string id, bool accept_non_compliant)
{

    for (size_t i=0; i<id.length(); ++i)
    {
        if (id[i] >= '0' && id[i] <= '9') continue;
        if (accept_non_compliant)
        {
            if (id[i] >= 'a' && id[i] <= 'f') continue;
            if (id[i] >= 'A' && id[i] <= 'F') continue;
        }
        return false;
    }
    return true;
}

bool doesIdMatchExpression(string id, string match)
{
    if (id.length() == 0) return false;

    // Here we assume that the match expression has been
    // verified to be valid.
    bool can_match = true;

    // Now match bcd/hex until end of id, or '*' in match.
    while (id.length() > 0 && match.length() > 0 && match.front() != '*')
    {
        if (id.front() != match.front())
        {
            // We hit a difference, it cannot match.
            can_match = false;
            break;
        }
        id.erase(0,1);
        match.erase(0,1);
    }

    bool wildcard_used = false;
    if (match.length() && match.front() == '*')
    {
        wildcard_used = true;
        match.erase(0,1);
    }

    if (can_match)
    {
        // Ok, now the match expression should be empty.
        // If wildcard is true, then the id can still have digits,
        // otherwise it must also be empty.
        if (wildcard_used)
        {
            can_match = match.length() == 0;
        }
        else
        {
            can_match = match.length() == 0 && id.length() == 0;
        }
    }

    return can_match;
}

bool hasWildCard(string &mes)
{
    return mes.find('*') != string::npos;
}

bool doesIdsMatchExpressions(vector<string> &ids, vector<string>& mes, bool *used_wildcard)
{
    bool match = false;
    for (string &id : ids)
    {
        if (doesIdMatchExpressions(id, mes, used_wildcard))
        {
            match = true;
        }
        // Go through all ids even though there is an early match.
        // This way we can see if theres an exact match later.
    }
    return match;
}

bool doesIdMatchExpressions(string id, vector<string>& mes, bool *used_wildcard)
{
    bool found_match = false;
    bool found_negative_match = false;
    bool exact_match = false;
    *used_wildcard = false;

    // Goes through all possible match expressions.
    // If no expression matches, neither positive nor negative,
    // then the result is false. (ie no match)

    // If more than one positive match is found, and no negative,
    // then the result is true.

    // If more than one negative match is found, irrespective
    // if there is any positive matches or not, then the result is false.

    // If a positive match is found, using a wildcard not any exact match,
    // then *used_wildcard is set to true.

    for (string me : mes)
    {
        bool has_wildcard = hasWildCard(me);
        bool is_negative_rule = (me.length() > 0 && me.front() == '!');
        if (is_negative_rule)
        {
            me.erase(0, 1);
        }

        bool m = doesIdMatchExpression(id, me);

        if (is_negative_rule)
        {
            if (m) found_negative_match = true;
        }
        else
        {
            if (m)
            {
                found_match = true;
                if (!has_wildcard)
                {
                    exact_match = true;
                }
            }
        }
    }

    if (found_negative_match)
    {
        return false;
    }
    if (found_match)
    {
        if (exact_match)
        {
            *used_wildcard = false;
        }
        else
        {
            *used_wildcard = true;
        }
        return true;
    }
    return false;
}

string toIdsCommaSeparated(std::vector<std::string> &ids)
{
    string cs;
    for (string& s: ids)
    {
        cs += s;
        cs += ",";
    }
    if (cs.length() > 0) cs.pop_back();
    return cs;
}

bool isFrequency(std::string& fq)
{
    int len = fq.length();
    if (len == 0) return false;
    if (fq[len-1] != 'M') return false;
    len--;
    for (int i=0; i<len; ++i) {
        if (!isdigit(fq[i]) && fq[i] != '.') return false;
    }
    return true;
}

bool isNumber(std::string& fq)
{
    int len = fq.length();
    if (len == 0) return false;
    for (int i=0; i<len; ++i) {
        if (!isdigit(fq[i])) return false;
    }
    return true;
}

vector<string> splitMatchExpressions(string& mes)
{
    vector<string> r;
    bool eof, err;
    vector<uchar> v (mes.begin(), mes.end());
    auto i = v.begin();

    for (;;) {
        auto id = eatTo(v, i, ',', 16, &eof, &err);
        if (err) break;
        trimWhitespace(&id);
        if (id == "ANYID") id = "*";
        r.push_back(id);
        if (eof) break;
    }
    return r;
}

void incrementIV(uchar *iv, size_t len) {
    uchar *p = iv+len-1;
    while (p >= iv) {
        int pp = *p;
        (*p)++;
        if (pp+1 <= 255) {
            // Nice, no overflow. We are done here!
            break;
        }
        // Move left add add one.
        p--;
    }
}


string eatTo(vector<uchar> &v, vector<uchar>::iterator &i, int c, size_t max, bool *eof, bool *err)
{
    string s;

    *eof = false;
    *err = false;
    while (max > 0 && i != v.end() && (c == -1 || *i != c))
    {
        s += *i;
        i++;
        max--;
    }
    if (c != -1 && i != v.end() && *i != c)
    {
        *err = true;
    }
    if (i != v.end())
    {
        i++;
    }
    if (i == v.end()) {
        *eof = true;
    }
    return s;
}

void padWithZeroesTo(vector<uchar> *content, size_t len, vector<uchar> *full_content)
{
    if (content->size() < len) {
        //warning("Padded with zeroes.", (int)len);
        size_t old_size = content->size();
        content->resize(len);
        for(size_t i = old_size; i < len; ++i) {
            (*content)[i] = 0;
        }
        full_content->insert(full_content->end(), content->begin()+old_size, content->end());
    }
}

static string space = "                                                   ";
string padLeft(string input, int width)
{
    int w = width-input.size();
    if (w < 0) return input;
    assert(w < (int)space.length());
    return space.substr(0, w)+input;
}

int parseTime(string time) {
    int mul = 1;
    if (time.back() == 'h') {
        time.pop_back();
        mul = 3600;
    }
    if (time.back() == 'm') {
        time.pop_back();
        mul = 60;
    }
    if (time.back() == 's') {
        time.pop_back();
        mul = 1;
    }
    int n = atoi(time.c_str());
    return n*mul;
}

#define CRC16_EN_13757 0x3D65

uint16_t crc16_EN13757_per_byte(uint16_t crc, uchar b)
{
    unsigned char i;

    for (i = 0; i < 8; i++) {

        if (((crc & 0x8000) >> 8) ^ (b & 0x80)){
            crc = (crc << 1)  ^ CRC16_EN_13757;
        }else{
            crc = (crc << 1);
        }

        b <<= 1;
    }

    return crc;
}

uint16_t crc16_EN13757(uchar *data, size_t len)
{
    uint16_t crc = 0x0000;

    assert(len == 0 || data != NULL);

    for (size_t i=0; i<len; ++i)
    {
        crc = crc16_EN13757_per_byte(crc, data[i]);
    }

    return (~crc);
}

#define CRC16_INIT_VALUE 0xFFFF
#define CRC16_GOOD_VALUE 0x0F47
#define CRC16_POLYNOM    0x8408

uint16_t crc16_CCITT(uchar *data, uint16_t length)
{
    uint16_t initVal = CRC16_INIT_VALUE;
    uint16_t crc = initVal;
    while(length--)
    {
        int bits = 8;
        uchar byte = *data++;
        while(bits--)
        {
            if((byte & 1) ^ (crc & 1))
            {
                crc = (crc >> 1) ^ CRC16_POLYNOM;
            }
            else
                crc >>= 1;
            byte >>= 1;
        }
    }
    return crc;
}

bool crc16_CCITT_check(uchar *data, uint16_t length)
{
    uint16_t crc = ~crc16_CCITT(data, length);
    return crc == CRC16_GOOD_VALUE;
}


uchar *safeButUnsafeVectorPtr(std::vector<uchar> &v)
{
    if (v.size() == 0) return NULL;
    return &v[0];
}
