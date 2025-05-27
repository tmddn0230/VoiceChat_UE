#pragma once

#include "CoreMinimal.h"
#include "VProtocol.h"

#pragma pack(1)
struct stConnectAck : public stHeader
{
	int32 UDPPort;
	int32 Index;

	stConnectAck()
	{
		UDPPort = 0;
		Index = 0;

		SetHeader( prConnectAck, sizeof( stConnectAck ) );
	};
};


//TOOL--------------------------------------------------------------------------------

#pragma pack()
