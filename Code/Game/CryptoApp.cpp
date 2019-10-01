// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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

#include "Engine/App/Application.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/RSA.h"
#include "Core/IO/EngineConfig.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Platform/File.h"
#include "Core/Utility/CmdLine.h"
#include "Core/Utility/Log.h"

namespace lf {
class CryptoApp : public Application
{
    DECLARE_CLASS(CryptoApp, Application);
public:

    void OnStart() override
    {
        if (!GetConfig())
        {
            gSysLog.Error(LogMessage("CryptoApp failed to start, requires EngineConfig."));
            return;
        }

        // -app /type=CryptoApp -crypto /rsa=1024 /rsa=2048 /rsa=4096

        if (CmdLine::HasArgOption("crypto", "rsa"))
        {
            GenerateRSAKey();
        }
        else if (CmdLine::HasArgOption("crypto", "aes"))
        {
            GenerateAESKey();
        }
    }

private:
    String GetKeyOutput()
    {
        String keyOutput = FileSystem::PathJoin(GetConfig()->GetTempDirectory(), "CryptoApp");
        if (!FileSystem::PathExists(keyOutput) && !FileSystem::PathCreate(keyOutput))
        {
            return String();
        }
        return keyOutput;
    }

    void GenerateRSAKey()
    {
        String filename;
        Int32 keySize = 0;
        if (!CmdLine::GetArgOption("crypto", "rsa", keySize))
        {
            gSysLog.Error(LogMessage("-crypto /rsa requires key size. eg '-crypto /rsa=1024' or '-crypto /rsa=2048' or '-crypto /rsa=4096'"));
            return;
        }
        if (!CmdLine::GetArgOption("crypto", "filename", filename))
        {
            gSysLog.Error(LogMessage("-crypto filename=<filename> required for RSA key generation."));
            return;
        }

        Crypto::RSAKeySize rsaKeySize;
        if (keySize == 1024)
        {
            rsaKeySize = Crypto::RSA_KEY_1024;
        }
        else if (keySize == 2048)
        {
            rsaKeySize = Crypto::RSA_KEY_2048;
        }
        else if (keySize == 4096)
        {
            rsaKeySize = Crypto::RSA_KEY_4096;
        }
        else
        {
            gSysLog.Error(LogMessage("Unsupported RSA key size. ") << keySize);
            return;
        }

        String keyOutput = GetKeyOutput();
        if (keyOutput.Empty())
        {
            gSysLog.Error(LogMessage("Failed to create 'key output' folder in the temp directory."));
            return;
        }

        String publicPath = FileSystem::PathJoin(keyOutput, filename + "_public.key");
        String privatePath = FileSystem::PathJoin(keyOutput, filename + "_private.key");

        gSysLog.Info(LogMessage("Generating RSA key pair..."));
        gSysLog.Info(LogMessage("  Public Path=") << publicPath);
        gSysLog.Info(LogMessage("  Private Path=") << privatePath);
        gSysLog.Sync();

        Crypto::RSAKey key;
        if (!key.GeneratePair(rsaKeySize))
        {
            gSysLog.Error(LogMessage("Failed to generate RSA key pair. Internal error."));
            return;
        }

        String publicKey = key.GetPublicKey();
        String privateKey = key.GetPrivateKey();

        gSysLog.Info(LogMessage("Saving public..."));
        File file;
        if (!file.Open(publicPath, FF_READ | FF_WRITE, FILE_OPEN_ALWAYS))
        {
            gSysLog.Error(LogMessage("Failed to save public key."));
            return;
        }
        file.Write(publicKey.CStr(), publicKey.Size());
        file.Close();

        gSysLog.Info(LogMessage("Saving private..."));
        if (!file.Open(privatePath, FF_READ | FF_WRITE, FILE_OPEN_ALWAYS))
        {
            gSysLog.Error(LogMessage("Failed to save private key."));
        }
        file.Write(privateKey.CStr(), privateKey.Size());
        file.Close();

        gSysLog.Info(LogMessage("Keys generated!"));
    }

    void GenerateAESKey()
    {
        // /aes=128 /aes=256

        String filename;
        Int32 keySize = 0;
        if (!CmdLine::GetArgOption("crypto", "aes", keySize))
        {
            gSysLog.Error(LogMessage("-crypto /aes requires key size. eg '-crypto /aes=128' or '-crypto /aes=256'"));
            return;
        }
        if (!CmdLine::GetArgOption("crypto", "filename", filename))
        {
            gSysLog.Error(LogMessage("-crypto filename=<filename> required for AES key generation."));
            return;
        }

        Crypto::AESKeySize aesKeySize;
        if (keySize == 128)
        {
            aesKeySize = Crypto::AES_KEY_128;
        }
        else if (keySize == 256)
        {
            aesKeySize = Crypto::AES_KEY_256;
        }
        else
        {
            gSysLog.Error(LogMessage("Unsupported AES key size. ") << keySize);
            return;
        }

        String keyOutput = GetKeyOutput();
        if (keyOutput.Empty())
        {
            gSysLog.Error(LogMessage("Failed to create 'key output' folder in the temp directory."));
            return;
        }

        String filepath = FileSystem::PathJoin(keyOutput, filename + "_aes.key");

        gSysLog.Info(LogMessage("Generating AES key..."));
        gSysLog.Info(LogMessage("  Filepath=") << filepath);
        gSysLog.Sync();

        Crypto::AESKey key;
        if (!key.Generate(aesKeySize))
        {
            gSysLog.Error(LogMessage("Failed to generate AES key. Internal error."));
            return;
        }

        gSysLog.Info(LogMessage("Saving key..."));
        File file;
        if (!file.Open(filepath, FF_READ | FF_WRITE, FILE_OPEN_ALWAYS))
        {
            gSysLog.Error(LogMessage("Failed to save key."));
            return;
        }
        UInt32 keySizeBytes = static_cast<UInt32>(key.GetKeySizeBytes());
        file.Write(&keySizeBytes, sizeof(UInt32));
        file.Write(key.GetKey(), key.GetKeySizeBytes());
        file.Close();

        gSysLog.Info(LogMessage("Key generated!"));

    }
};
DEFINE_CLASS(CryptoApp) { NO_REFLECTION; }

}