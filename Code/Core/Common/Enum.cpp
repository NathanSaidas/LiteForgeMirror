#include "Enum.h"
#include "Core/String/StringCommon.h"

#include <algorithm>

namespace lf {

void EnumRegistry::Clear()
{
    for (auto& data : mEnumDatas)
    {
        data->Release();
    }
    mEnumDatas.Clear();
}

bool EnumData::Initialized(const char* name, const char* details)
{
    bool initiailized = (enumValues != nullptr && prettyStrings != nullptr && rawStrings != nullptr);
    if (initiailized)
    {
        return true;
    }

    BuildEnumData(name, details, this);

    TArray<EnumData*>& datas = GetEnumRegistry().GetData();
    TArray<EnumData*>::iterator findIt = std::find_if(datas.begin(), datas.end(), [&name](const EnumData* data)
    {
        return strcmp(data->enumName, name) == 0;
    });

    if (findIt == datas.end())
    {
        GetEnumRegistry().Add(this);
    }

    return (enumValues != nullptr && prettyStrings != nullptr && rawStrings != nullptr);
}

void EnumData::Release()
{
    if (!(enumValues != nullptr && prettyStrings != nullptr && rawStrings != nullptr))
    {
        return;
    }
    LFFree(prettyStrings);
    LFFree(rawStrings);
    LFFree(enumValues);
    LFFree(buffer);
}

EnumRegistry& GetEnumRegistry()
{
    static EnumRegistry sRegistry;
    return sRegistry;
}

void BuildEnumData(const char* name, const char* args, EnumData* enumData)
{
    Assert(enumData);
    Assert(name);
    Assert(args);
    SizeT length = strlen(args);
    Assert(length > 0);

    // strip whitespace & linefeeds 
    char* buffer = static_cast<char*>(LFAlloc(length + 1, 1));
    enumData->buffer = buffer;
    buffer[length] = '\0';
    char* bufferIter = buffer;
    const char* argsIter = args;
    SizeT numStrings = 1; // start at 1 to include last string
    while (argsIter && *argsIter != '\0')
    {
        const char c = *argsIter;
        if (c == ' ' || c == '\n' || c == '\r')
        {
            ++argsIter;
            continue;
        }
        if (c == ',')
        {
            ++numStrings;
        }
        *bufferIter = *argsIter;
        ++bufferIter;
        ++argsIter;
    }
    *bufferIter = '\0';

    // Identify Pretty Prefix:
    static const SizeT MAX_PRETTY_BUFFER = 4;
    char prettyBuffer[MAX_PRETTY_BUFFER + 2];
    SizeT prettyBufferIndex = 0;
    SizeT actualPrettyBufferSize = 0;
    const char* nameIter = name;
    while (nameIter && *nameIter != '\0')
    {
        if (CharIsUpper(*nameIter))
        {
            prettyBuffer[prettyBufferIndex++] = *nameIter;
            if (prettyBufferIndex == MAX_PRETTY_BUFFER)
            {
                break;
            }
        }
        ++nameIter;
    }
    prettyBuffer[prettyBufferIndex] = '_';
    prettyBuffer[prettyBufferIndex + 1] = '\0';
    actualPrettyBufferSize = prettyBufferIndex + 1;
    // Allocate:
    enumData->numberOfStrings = numStrings;
    enumData->prettyStrings = static_cast<char**>(LFAlloc(sizeof(char*) * numStrings, alignof(char*)));
    enumData->rawStrings = static_cast<char**>(LFAlloc(sizeof(char*) * numStrings, alignof(char*)));
    enumData->enumValues = static_cast<Int32*>(LFAlloc(sizeof(char*) * numStrings, alignof(char*)));
    enumData->enumName = const_cast<char*>(name);

    // Tokenize:
    bufferIter = buffer;
    char* startIter = bufferIter;
    SizeT index = 0;
    while (bufferIter && *bufferIter != '\0')
    {
        const char c = *bufferIter;
        if (c == ',')
        {
            enumData->rawStrings[index++] = startIter;
            *bufferIter = '\0';
            startIter = bufferIter + 1;
        }
        ++bufferIter;
    }
    enumData->rawStrings[index] = startIter;

    // Refine in case of a=1
    // Also assign enumValues
    Int32 value = 0;
    for (SizeT i = 0; i < numStrings; ++i)
    {
        char* stringIter = enumData->rawStrings[i];
        char* valueIter = nullptr;
        bool parsingValue = false;
        while (stringIter && *stringIter != '\0')
        {
            if (*stringIter == '=')
            {
                *stringIter = '\0';
                valueIter = stringIter + 1;
                parsingValue = true;
            }

            ++stringIter;
        }
        if (valueIter && *valueIter != '\0')
        {
            // verify number
            
            bool isNumber = StrIsNumber(valueIter);
            if (isNumber)
            {
                value = atoi(valueIter);
            }
            else
            {
                // find value for string..
                for (SizeT j = 0; j < i; ++j)
                {
                    if (strcmp(enumData->rawStrings[j], valueIter) == 0)
                    {
                        value = enumData->enumValues[j];
                        break;
                    }
                }
            }
        }
        enumData->enumValues[i] = value;
        ++value;

    }

    // eg BigCoolAvatarType should have prefix BCAT_ and so we should strip BCAT_ 
    // ie start == BCAT_ + 1
    // Calculate Pretty String:
    for (SizeT i = 0; i < numStrings; ++i)
    {
        enumData->prettyStrings[i] = enumData->rawStrings[i];
        char* stringIter = enumData->rawStrings[i];
        SizeT cmpIndex = 0;
        while (stringIter && *stringIter != '\0' && cmpIndex < actualPrettyBufferSize)
        {
            if (*stringIter != prettyBuffer[cmpIndex])
            {
                enumData->prettyStrings[i] = enumData->rawStrings[i];
                break;
            }
            if (cmpIndex == actualPrettyBufferSize - 1)
            {
                enumData->prettyStrings[i] = stringIter + 1;
                break;
            }

            ++stringIter;
            ++cmpIndex;
        }
    }

    enumData->actualSize = enumData->numberOfStrings;
    enumData->invalidIndex = enumData->actualSize - 1;

    for (SizeT i = 0; i < numStrings; ++i)
    {
        if (strcmp(enumData->rawStrings[i], "MAX_VALUE") == 0)
        {
            enumData->actualSize = i;
            enumData->invalidIndex = i;
            break;
        }
    }

}

}