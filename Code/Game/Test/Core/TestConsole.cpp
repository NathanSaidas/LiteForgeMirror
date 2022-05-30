#include "Core/Test/Test.h"
#include "Core/Utility/Console.h"

namespace lf {

	REGISTER_TEST(ConsoleTest, "Core.Utility.Console", TestFlags::TF_DISABLED) {
		Console console;
		console.Create();
		console.Write("Hello\n\n");
		console.Write(console.Read());
		Sleep(5000);
		console.Destroy();
	}
}