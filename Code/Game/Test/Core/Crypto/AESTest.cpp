// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
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
#include "Core/Test/Test.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/SecureRandom.h"

namespace lf {

REGISTER_TEST(AESTest, "Core.Crypto")
{
    // todo: Support 128 keysize?
    // todo: ctor: copy/move
    // todo: dtor:
    // todo: assignment: copy/move
    // todo: GetKeySize

    // todo: test in.size % block_size != 0
    // todo: test in.size % block_size == 0
    String MESSAGE_85 = "ehre is my message its a bunch of text of a odd length but should encrypt just fine?";
    String MESSAGE_64 = "Text message of a length 64 bytes that fits the block perfectly";
    String encrypted;
    String decrypted;

    Crypto::AESKey key;
    TEST(key.GetKeySize() == Crypto::AES_KEY_Unknown);
    TEST_CRITICAL(key.Generate(Crypto::AES_KEY_128));
    TEST(key.GetKeySize() == Crypto::AES_KEY_128);

    ByteT ivA[16] = { 38, 18, 21, 99, 21, 239, 40, 99, 04, 90, 83, 40, 98, 34, 23, 10 };
    ByteT ivB[16] = { 49, 39, 43, 79, 80, 45, 128, 28, 120, 167, 177, 200, 2, 54, 2, 0 };


    TEST(Crypto::AESEncrypt(&key, ivA, MESSAGE_64, encrypted));
    TEST(Crypto::AESDecrypt(&key, ivA, encrypted, decrypted));
    TEST(MESSAGE_64 != encrypted);
    TEST(MESSAGE_64 == decrypted);
    TEST(encrypted.Size() == 64);

    TEST(Crypto::AESEncrypt(&key, ivA, MESSAGE_85, encrypted));
    TEST(Crypto::AESDecrypt(&key, ivA, encrypted, decrypted));
    TEST(MESSAGE_85 != encrypted);
    TEST(MESSAGE_85 == decrypted);
    TEST(encrypted.Size() == 96);

    key.Clear();
    TEST(key.GetKeySize() == Crypto::AES_KEY_Unknown);
    TEST_CRITICAL(key.Generate(Crypto::AES_KEY_256));
    TEST(key.GetKeySize() == Crypto::AES_KEY_256);

    TEST(Crypto::AESEncrypt(&key, ivA, MESSAGE_64, encrypted));
    TEST(Crypto::AESDecrypt(&key, ivA, encrypted, decrypted));
    TEST(MESSAGE_64 != encrypted);
    TEST(MESSAGE_64 == decrypted);
    TEST(encrypted.Size() == 64);

    TEST(Crypto::AESEncrypt(&key, ivA, MESSAGE_85, encrypted));
    TEST(Crypto::AESDecrypt(&key, ivA, encrypted, decrypted));
    TEST(MESSAGE_85 != encrypted);
    TEST(MESSAGE_85 == decrypted);
    TEST(encrypted.Size() == 96);


    TEST(Crypto::AESEncrypt(&key, ivA, MESSAGE_64, encrypted));
    TEST(Crypto::AESDecrypt(&key, ivA, encrypted, decrypted));
    String encryptedA = encrypted;
    String decryptedA = decrypted;

    TEST(Crypto::AESEncrypt(&key, ivB, MESSAGE_64, encrypted));
    TEST(Crypto::AESDecrypt(&key, ivB, encrypted, decrypted));
    TEST(encryptedA != MESSAGE_64);
    TEST(encrypted != MESSAGE_64);
    TEST(encryptedA != encrypted);
    TEST(decryptedA == decrypted);
    TEST(MESSAGE_64 == decrypted);

    key.Clear();
}

REGISTER_TEST(AESSizeTest, "Core.Crypto")
{
    Crypto::AESKey key;
    key.Generate(Crypto::AES_KEY_256);
    ByteT iv[16];
    Crypto::SecureRandomBytes(iv, 16);

    ByteT input[1000];
    ByteT output[1000];

    Crypto::SecureRandomBytes(input, 32);

    SizeT cap = 32;
    TEST(Crypto::AESEncrypt(&key, iv, input, 31, output, cap));
    TEST(cap == 32);

    cap = 48;
    TEST(Crypto::AESEncrypt(&key, iv, input, 32, output, cap));
    TEST(cap == 48);
}

}