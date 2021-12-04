#include "stdafx.h"
#include "ServerFramework.h"

ServerFramework f{};

int main() {
	if (f.Initialize() == -1)
	{
		return 0;
	}

	while (true)
	{
		if (!f.Connect())
		{
			break;
		}

		f.Disconnect();
	}

	f.Close();
}