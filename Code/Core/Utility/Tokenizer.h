#pragma once
// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
// 
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files(the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and / or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ********************************************************************

#include "Core/String/String.h"
#include "Core/String/StringUtil.h"
#include "Core/Utility/StdVector.h"

namespace lf {

// **********************************
// Tokenizer is a class designed to ease parsing text into a bunch of tokens.
// It uses a set of rules to parse the text.
// 
// typename T = Enum declared like DECLARE_ENUM
// **********************************
template<typename T>
class Tokenizer
{
public:
    struct RuleType
    {
        RuleType() :
            text(),
            caseSensitive(false),
            exactMatch(true),
            type(static_cast<T>(0))
        {}

        RuleType(const String& inText, typename T::t_EnumType inType, bool inCaseSensitive, bool inExactMatch = true) :
            text(inText),
            caseSensitive(inCaseSensitive),
            exactMatch(inExactMatch),
            type(inType)
        {}

        String text;
        bool caseSensitive;
        bool exactMatch;
        typename T::t_EnumType type;
    };

    struct TokenType
    {
        TokenType() :
            text(),
            type(static_cast<T>(0)),
            value(0)
        {}

        String GetString() const
        {
            return String(t::GetString(static_cast<size_t>(type))) + " " + text;
        }


        String text;
        typename T::t_EnumType type;
        size_t value;
    };

    typedef TVector<RuleType> RuleListType;
    typedef TVector<TokenType> TokenListType;
    static const SizeT TT_TEXT = 0; // Reserve 0 for TT_TEXT
    static const SizeT TT_RETURN_LINE_FEED = 1;
    static const SizeT TT_LINE_FEED = 2;

    Tokenizer() : mReserve(256) {}

    void Tokenize(const String& text, const RuleListType& rules, TokenListType& tokens)
    {
        String lowerBuffer;
        String buffer;
        buffer.Reserve(mReserve);
        lowerBuffer.Reserve(mReserve);
        for (size_t i = 0; i < text.Size(); ++i)
        {
            buffer.Append(text[i]);
            lowerBuffer.Append(StrToLower(text[i]));

            for (size_t j = 0; j < rules.size(); ++j)
            {
                const RuleType& rule = rules[j];
                // Token Match and next char is white space
                if (CompareToken(buffer, lowerBuffer, rule.text, !rule.caseSensitive))
                {
                    if (rule.exactMatch)
                    {
                        bool nextCharIsSpaceOrEnd = false;
                        if (i + 1 >= text.Size())
                        {
                            nextCharIsSpaceOrEnd = true;
                        }
                        else
                        {
                            nextCharIsSpaceOrEnd = text[i + 1] == ' ' || text[i + 1] == '\t';
                        }

                        bool isMatch = buffer == rule.text;

                        if (nextCharIsSpaceOrEnd)
                        {
                            String substr;
                            if (buffer.Size() > rule.text.Size())
                            {
                                buffer.SubString(buffer.Size() - rule.text.Size(), substr);
                            }
                            else
                            {
                                substr = buffer;
                            }
                            isMatch = substr == rule.text;
                        }

                        if (!nextCharIsSpaceOrEnd || !isMatch)
                        {
                            if (static_cast<SizeT>(rule.type) == TT_LINE_FEED || static_cast<SizeT>(rule.type) == TT_RETURN_LINE_FEED)
                            {
                                if (buffer.Size() > rule.text.Size())
                                {
                                    TokenType textToken;
                                    buffer.SubString(0, buffer.Size() - rule.text.Size(), textToken.text);
                                    textToken.type = static_cast<typename T::t_EnumType>(TT_TEXT);
                                    tokens.push_back(textToken);
                                }

                                TokenType token;
                                token.type = rule.type;
                                buffer.SubString(buffer.Size() - rule.text.Size(), token.text);
                                tokens.push_back(token);
                                buffer.Resize(0);
                                lowerBuffer.Resize(0);
                                break;
                            }

                            continue;
                        }

                    }

                    if (buffer.Size() > rule.text.Size())
                    {
                        TokenType textToken;
                        buffer.SubString(0, buffer.Size() - rule.text.Size(), textToken.text);
                        textToken.type = static_cast<typename T::t_EnumType>(TT_TEXT);
                        tokens.push_back(textToken);
                    }
                    TokenType token;
                    buffer.SubString(buffer.Size() - rule.text.Size(), token.text);
                    token.type = rule.type;
                    tokens.push_back(token);
                    buffer.Resize(0);
                    lowerBuffer.Resize(0);
                    break;
                }
            }

            if (i == (text.Size() - 1) && !buffer.Empty())
            {
                TokenType textToken;
                textToken.text = std::move(buffer);
                textToken.type = static_cast<typename T::t_EnumType>(TT_TEXT);
                tokens.push_back(textToken);
                buffer.Reserve(mReserve);
                lowerBuffer.Resize(0);
            }
        }
    }

    void SetReserve(size_t value) { mReserve = value; }
    size_t GetReserve() const { return mReserve; }

private:

    bool CompareToken(const String& buffer, const String& lowerBuffer, const String& token, bool toLower)
    {
        if (buffer.Size() < token.Size())
        {
            return false;
        }
        if (toLower)
        {
            // String lowerBuffer = buffer;
            // ToLower(lowerBuffer);
            return Valid(lowerBuffer.FindLast(token));
        }
        return Valid(buffer.FindLast(token));
    }

    size_t mReserve;
};

} // namespace lf