#include "Core/PCH.h"
#include <iostream>
#include <Windows.h>

#include "Console.h"

using namespace std;
namespace lf
{
    Console::Console()
        : mInputHandle(NULL)
        , mOutputHandle(NULL)
    {}
    bool Console::Create()
    {
        bool alreadyCreated = GetStdHandle(STD_INPUT_HANDLE) != NULL;
        if (alreadyCreated)
        {
            return false;
        }
        AllocConsole();
        mInputHandle = GetStdHandle(STD_INPUT_HANDLE);
        mOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);


        return true;
    }

    void Console::Write(const String& string) {

        DWORD numberOfCharactersWritten;
        WriteConsole(mOutputHandle, string.CStr(), static_cast<DWORD>(string.Size()), &numberOfCharactersWritten, NULL);
    }

    String Console::Read() {
        char buffer[10] = { 0 };
        DWORD numberOfCharactersRead;
        ReadConsole(mInputHandle, buffer, sizeof(buffer)-1, &numberOfCharactersRead, NULL);
        return buffer;
    }

    void Console::Destroy()
    {
        if (mInputHandle == NULL)
        {
            return;
        }
        FreeConsole();
        mInputHandle = NULL;
        mOutputHandle = NULL;
    }

}