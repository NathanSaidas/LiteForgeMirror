#include "Core/Test/Test.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Platform/File.h"
#include "Core/Utility/Time.h"
#include "Core/Utility/Log.h"

#include <algorithm>




namespace lf {


struct HlslSymbol
{
    enum Value : Int32
    {
        None,
        PreprocessIf,
        PreprocessElif,
        PreprocessElse,
        PreprocessEndIf,
        PreprocessInclude,
        PreprocessDefined,
        ForwardSlash,
        BackSlash,
        Return,
        NewLine,
        Asterix,
        LeftParenthesis,
        RightParenthesis,
        Quote,
        Comma,
        SemiColon,
        LogicalOr,
        LogicalAnd,
        LogicalNot,
        Int,
        UInt,
        Float,
        Float2,
        Float3,
        Float4,
        Float3x3,
        Float4x4,

        MAX_VALUE
    };

    HlslSymbol()
    : mSymbolText()
    , mSymbol(Value::None)
    , mDelimiters(nullptr)
    {}

    HlslSymbol(const String& text, Value symbol, const TVector<char>* delimiters)
        : mSymbolText(text)
        , mSymbol(symbol)
        , mDelimiters(delimiters)
    {}

    SizeT Size() const { return mSymbolText.Size(); }

    char operator[](Int32 index) const { return mSymbolText[index]; }

    bool IsDelimiter(char value) const
    {
        return mDelimiters != nullptr && std::find(mDelimiters->begin(), mDelimiters->end(), value) != mDelimiters->end();
    }

    bool HasDelimiter() const
    {
        return mDelimiters != nullptr && !mDelimiters->empty();
    }

    bool IsWhitespace() const
    {
        if (mSymbol != HlslSymbol::None)
        {
            return false;
        }

        for (char c : mSymbolText)
        {
            if (c != ' ' && c != '\t')
            {
                return false;
            }
        }
        return true;
    }

    String mSymbolText;
    Value  mSymbol;
    const TVector<char>* mDelimiters;
};



SizeT HlslParseFindLast(const String& buffer, const HlslSymbol& symbol)
{
    if (buffer.Size() < symbol.Size())
    {
        return INVALID;
    }

    Int32 aindex = static_cast<Int32>(buffer.Size()) - 1;
    Int32 bindex = static_cast<Int32>(symbol.Size()) - 1;
    while (bindex >= 0)
    {
        if (buffer[aindex] != symbol[bindex])
        {
            return INVALID;
        }
        --aindex;
        --bindex;
    }
    ++aindex;
    return aindex;
}

bool HlslParseMatch(const String& buffer, const HlslSymbol& symbol, char nextCharacter, String& outText, String& outSymbol)
{
    if (symbol.HasDelimiter() && !symbol.IsDelimiter(nextCharacter))
    {
        return false;
    }

    SizeT index = HlslParseFindLast(buffer, symbol);
    if (Invalid(index))
    {
        return false;
    }

    outText.Resize(0);
    buffer.SubString(0, index, outText);

    outSymbol.Resize(0);
    buffer.SubString(index, outSymbol);
    return true;
}

class HlslParsedFile
{
public:

    TVector<HlslSymbol> mSymbols;
    TVector<char> mDelimiters;

    const HlslSymbol& GetSymbol(HlslSymbol::Value symbol) const
    {
        auto iter = std::find_if(mSymbols.begin(), mSymbols.end(),
            [symbol](const HlslSymbol& value)
            {
                return value.mSymbol == symbol;
            });

        return *iter;
    }

    void ParseSymbols(const String& text, TVector<HlslSymbol>& outSymbols)
    {
        outSymbols.reserve(5000);
        String buffer;
        buffer.Reserve(ToKB(1));
        String outText;
        outText.Reserve(ToKB(1));
        String outSymbol;

        bool inQuote = false;

        auto lastSymbol = [&outSymbols]() { return outSymbols.empty() ? HlslSymbol::None : outSymbols.back().mSymbol; };

        for (SizeT i = 0; i < text.Size(); ++i)
        {
            buffer += text[i];

            const bool isLastChar = i == (text.Size() - 1);
            const char nextChar = isLastChar ? 0 : text[i + 1];

            for (const HlslSymbol& symbol : mSymbols)
            {
                if (HlslParseMatch(buffer, symbol, nextChar, outText, outSymbol))
                {
                    // Quote has special behavior:
                    if (symbol.mSymbol == HlslSymbol::Quote && lastSymbol() != HlslSymbol::BackSlash)
                    {
                        inQuote = !inQuote;

                        if (!outText.Empty())
                        {
                            outSymbols.push_back({ outText, HlslSymbol::None, nullptr });
                        }
                        outSymbols.push_back({ outSymbol, symbol.mSymbol, nullptr });
                        buffer.Resize(0);
                        break;
                    }

                    if (inQuote)
                    {
                        break;
                    }

                    if (!outText.Empty())
                    {
                        outSymbols.push_back({ outText, HlslSymbol::None, nullptr });
                    }

                    outSymbols.push_back({ outSymbol, symbol.mSymbol, nullptr });

                    buffer.Resize(0);
                    break;
                }
            }

            if (isLastChar && !buffer.Empty()) 
            {
                outSymbols.push_back({ buffer, HlslSymbol::None, nullptr});

                buffer.Resize(0);
            }
        }
    }

    void ParseReplaceIncludes(TVector<HlslSymbol>& outSymbols)
    {
        bool isLineComment = false;
        bool isBlockComment = false;

        auto processComment = [&isLineComment, &isBlockComment](HlslSymbol::Value current, HlslSymbol::Value previous)
        {
            switch (current)
            {
                case HlslSymbol::ForwardSlash:
                {
                    if ((!isLineComment || !isBlockComment) && previous == HlslSymbol::ForwardSlash)
                    {
                        isLineComment = true;
                    }
                    else if (isBlockComment && previous == HlslSymbol::Asterix)
                    {
                        isBlockComment = false;
                    }
                } break;
                case HlslSymbol::Asterix:
                {
                    if ((!isLineComment || !isBlockComment) && previous == HlslSymbol::ForwardSlash)
                    {
                        isBlockComment = true;
                    }
                } break;
                case HlslSymbol::NewLine:
                {
                    if (!isBlockComment && isLineComment)
                    {
                        isLineComment = false;
                    }
                } break;
            }
        };

        auto tryPeak = [&outSymbols](SizeT position) -> HlslSymbol*
        {
            if (position < outSymbols.size())
            {
                return &outSymbols[position];
            }
            return nullptr;
        };

        auto scanUntilText = [&outSymbols](SizeT& cursor) -> HlslSymbol*
        {
            HlslSymbol* result = nullptr;

            for (; cursor < outSymbols.size(); ++cursor)
            {
                HlslSymbol* symbol = &outSymbols[cursor];
                if (symbol->mSymbol == HlslSymbol::Quote)
                {
                    continue;
                }

                if (symbol->mSymbol != HlslSymbol::None)
                {
                    break;
                }

                if (!symbol->IsWhitespace())
                {
                    result = symbol;
                    break;
                }
            }

            if (result)
            {
                ++cursor;
                if (cursor >= outSymbols.size() || outSymbols[cursor].mSymbol != HlslSymbol::Quote)
                {
                    ReportBugMsg("Parse Error");
                    return nullptr;
                }
                ++cursor;
            }

            return result;
        };

        auto scanUntilNewLine = [&outSymbols, &processComment, &isLineComment, &isBlockComment](SizeT& cursor)
        {
            for (; cursor < outSymbols.size(); ++cursor)
            {
                // Comments:
                processComment(outSymbols[cursor].mSymbol, cursor > 0 ? outSymbols[cursor - 1].mSymbol : HlslSymbol::None);

                switch (outSymbols[cursor].mSymbol)
                {
                    case HlslSymbol::Return: break;
                    case HlslSymbol::NewLine: return true;
                    case HlslSymbol::None:
                    {
                        if (!outSymbols[cursor].IsWhitespace() && !isLineComment && !isBlockComment)
                        {
                            return false;
                        }
                    } break;
                    case HlslSymbol::ForwardSlash: break;
                    case HlslSymbol::Asterix:
                    {
                        if (!isLineComment && !isBlockComment)
                        {
                            return false;
                        }
                    } break;
                    default: return false;
                }
            }

            return true;
        };

        for (SizeT i = 0; i < outSymbols.size(); ++i)
        {
            // todo: Comment State:
            processComment(outSymbols[i].mSymbol, i > 0 ? outSymbols[i-1].mSymbol : HlslSymbol::None);

            if (!isLineComment && !isBlockComment)
            {
                if (outSymbols[i].mSymbol == HlslSymbol::PreprocessInclude)
                {
                    outSymbols[i].mSymbolText = "//" + outSymbols[i].mSymbolText;
                    ++i;
                    HlslSymbol* textSym = scanUntilText(i);
                    if (textSym != nullptr)
                    {

                        if (!scanUntilNewLine(i))
                        {
                            ReportBugMsg("Parse error after include");
                        }
                        else
                        {
                            outSymbols[i].mSymbolText += "^^^TODO: TEXT FROM INCLUDE ^^^\r\n";
                        }
                    }
                    else
                    {
                        ReportBugMsg("Parse Error");
                    }
                }
            }
        }
    }

    void Aggregate(const TVector<HlslSymbol>& symbols, String& outText)
    {
        for (const HlslSymbol& symbol : symbols)
        {
            outText += symbol.mSymbolText;
        }
    }
};

// Old: D3DCompileFromFile (5.1)
// New: IDxcCompiler::Compile
//
// Notes:
// TODO: ParseReplaceIncludes, implement the 'include text finder thingy' and then recursive parse symbols etc
// TODO: Implement a method to parse includes from symbols
// TODO: Implement a method to parse all defines (so we can gather a list of available defines)/
// TODO: Perf kinda sucks parsing...
// 
// -- 
// 1. Parse Symbols
// 2. Parse Conditions w/ Comments
// 3. Generate Text Symbols
// 4. Replace Include-Quote-Text-Quote (ignoring None) with proper include file (evaluated recursively, steps 1-4)
// 5. Replace symbols with text to generate 'file' 

// REAL TODO:
// function GetRawText(filepath) : string
// function GetParsedText(filepath) 
// function GetShaderBinary(api, text, defines) : binary
// 
REGISTER_TEST(GfxShaderFileTest, "AbstractEngine.Gfx")
{
    String tempDir = TestFramework::GetTempDirectory();
    String shaderFile = FileSystem::PathJoin(tempDir, "SampleFile.hlsl");
    String shaderText;
    if (File::ReadAllText(shaderFile, shaderText))
    {
        LF_DEBUG_BREAK;
    }

    const TVector<char> SingleCharDelimiters = { };
    const TVector<char> StandardSpaceDelimiters = { '\0', ' ', '\t', '\r', '\n' };
    const TVector<char> DefineDelimiters = { '\0', ' ', '\t', '\r', '\n', '(' };
    const TVector<char> TypeDelimiters = { '\0', ' ', '\t', '\r', '\n', ',', '(', ')'};

    HlslParsedFile file;
    file.mSymbols.push_back({ "#if", HlslSymbol::PreprocessIf, &StandardSpaceDelimiters });               // StandardSpaceDelimiters
    file.mSymbols.push_back({ "#elif", HlslSymbol::PreprocessElif, &StandardSpaceDelimiters });           // StandardSpaceDelimiters
    file.mSymbols.push_back({ "#else", HlslSymbol::PreprocessElse, &StandardSpaceDelimiters });           // StandardSpaceDelimiters
    file.mSymbols.push_back({ "#endif", HlslSymbol::PreprocessEndIf, &StandardSpaceDelimiters });         // StandardSpaceDelimiters
    file.mSymbols.push_back({ "#include", HlslSymbol::PreprocessInclude, &StandardSpaceDelimiters });     // StandardSpaceDelimiters
    file.mSymbols.push_back({ "defined", HlslSymbol::PreprocessDefined, &DefineDelimiters });      // StandardSpaceDelimiters + (
    file.mSymbols.push_back({ "/", HlslSymbol::ForwardSlash, &SingleCharDelimiters });                 // 
    file.mSymbols.push_back({ "\\", HlslSymbol::BackSlash, &SingleCharDelimiters });                   // 
    file.mSymbols.push_back({ "\r", HlslSymbol::Return, &SingleCharDelimiters });                      // 
    file.mSymbols.push_back({ "\n", HlslSymbol::NewLine, &SingleCharDelimiters });                      // 
    file.mSymbols.push_back({ "*", HlslSymbol::Asterix, &SingleCharDelimiters });                      // 
    file.mSymbols.push_back({ "(", HlslSymbol::LeftParenthesis, &SingleCharDelimiters });              // 
    file.mSymbols.push_back({ ")", HlslSymbol::RightParenthesis, &SingleCharDelimiters });             // 
    file.mSymbols.push_back({ "\"", HlslSymbol::Quote, &SingleCharDelimiters });                       // 
    file.mSymbols.push_back({ ",", HlslSymbol::Comma, &SingleCharDelimiters });                       // 
    file.mSymbols.push_back({ ";", HlslSymbol::SemiColon, &SingleCharDelimiters });                       // 
    file.mSymbols.push_back({ "!", HlslSymbol::LogicalNot, &SingleCharDelimiters });                       // 
    file.mSymbols.push_back({ "||", HlslSymbol::LogicalOr, &SingleCharDelimiters });                       // 
    file.mSymbols.push_back({ "&&", HlslSymbol::LogicalAnd, &SingleCharDelimiters });                       // 
    file.mSymbols.push_back({ "int", HlslSymbol::Int, &TypeDelimiters });                        // StandardSpaceDelimiters
    file.mSymbols.push_back({ "uint", HlslSymbol::UInt, &TypeDelimiters });                      // StandardSpaceDelimiters
    file.mSymbols.push_back({ "float", HlslSymbol::Float, &TypeDelimiters });                    // StandardSpaceDelimiters
    file.mSymbols.push_back({ "float2", HlslSymbol::Float2, &TypeDelimiters });                  // StandardSpaceDelimiters
    file.mSymbols.push_back({ "float3", HlslSymbol::Float3, &TypeDelimiters });                  // StandardSpaceDelimiters
    file.mSymbols.push_back({ "float4", HlslSymbol::Float4, &TypeDelimiters });                  // StandardSpaceDelimiters
    file.mSymbols.push_back({ "float3x3", HlslSymbol::Float3x3, &TypeDelimiters });              // StandardSpaceDelimiters
    file.mSymbols.push_back({ "float4x4", HlslSymbol::Float4x4, &TypeDelimiters });              // StandardSpaceDelimiters

    std::sort(file.mSymbols.begin(), file.mSymbols.end(),
        [](const HlslSymbol& a, const HlslSymbol& b)
        {
            return a.mSymbolText.Size() > b.mSymbolText.Size();
        });

    TVector<HlslSymbol> parsedSymbols;
    Timer t;
    t.Start();
    file.ParseSymbols(shaderText, parsedSymbols);
    t.Stop();
    gSysLog.Info(LogMessage("Parsed in (ms)") << ToMilliseconds(TimeTypes::Seconds(t.GetDelta())).mValue);

    
    file.ParseReplaceIncludes(parsedSymbols);

    String outText;
    file.Aggregate(parsedSymbols, outText);
    LF_DEBUG_BREAK;

    // SizeT value = HlslParseFindLast("#if", file.GetSymbol(HlslSymbol::PreprocessIf));
    // value = HlslParseFindLast(" #if", file.GetSymbol(HlslSymbol::PreprocessIf));
    // value = HlslParseFindLast("const Token& /*formatToken*/", file.GetSymbol(HlslSymbol::PreprocessIf));


}

} // namespace lf