#include "stdafx.h"
#include "ServerFramework.h"

ServerFramework framework{};

int main() {
	if (framework.Initialize() == -1)
	{
		return 0;
	}

	while (true)
	{
		if (!framework.Connect())
		{
			break;
		}

		
	}

	framework.Close();
}