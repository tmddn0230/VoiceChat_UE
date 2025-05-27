// Fill out your copyright notice in the Description page of Project Settings.


#include "UDPClient.h"
#include "Kismet/GameplayStatics.h"

// Socket
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"

// protocol
#include "VProtocol.h"
#include "VPacket.h"

#include "WorkerThread.h"

#include "VoiceCharacter.h"

// Sets default values
AUDPClient::AUDPClient()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AUDPClient::BeginPlay()
{
	Super::BeginPlay();
	
	// Init
	IpAddress_TCP = "192.168.0.20";
	Port_TCP = 12000;

	IpAddress_UDP = "192.168.0.20";
	SendPort_UDP = 8000;

	// Search Character
	VoiceCharacter = Cast<AVoiceCharacter>(UGameplayStatics::GetActorOfClass(GetWorld(), AVoiceCharacter::StaticClass()));

	TCP_Connect();

}

// Called every frame
void AUDPClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AUDPClient::TCP_Connect()
{
	TCPSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(TEXT("Stream"), TEXT("Client Socket"));

	FIPv4Address Ip;
	FIPv4Address::Parse(IpAddress_TCP, Ip);

	TSharedRef<FInternetAddr> InternetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	InternetAddr->SetIp(Ip.Value);
	InternetAddr->SetPort(Port_TCP);

	bool Connected = TCPSocket->Connect(*InternetAddr);

	if (Connected)
	{
		// Session
		GetWorld()->GetTimerManager().SetTimer(ReceiveTimerHandle, this, &AUDPClient::HandleRecvPackets, 0.01f, true);
		GameServerPacketSession = MakeShared<RtPacketSession>(TCPSocket);
		GameServerPacketSession->SetClient(this);
		GameServerPacketSession->RunThread();
	}

}

void AUDPClient::UDP_Start()
{
	UDPSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(TEXT("DGram"), TEXT("Client UDP Socket"));

	// Set Port from TCP for Recv
	FIPv4Address Addr;
	FIPv4Address::Parse(IpAddress_UDP, Addr);

	InternetAddr_UDP_Recv = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	InternetAddr_UDP_Recv->SetIp(Addr.Value);
	InternetAddr_UDP_Recv->SetPort(RecvPort_UDP);

	// Set Port from Setting for Send
	FIPv4Address Addr_s;
	FIPv4Address::Parse(IpAddress_UDP, Addr_s);


	InternetAddr_UDP_Send = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	InternetAddr_UDP_Send->SetIp(Addr.Value);
	InternetAddr_UDP_Send->SetPort(SendPort_UDP);


	if (UDPSocket) {

		GetWorld()->GetTimerManager().SetTimer(ReceiveTimerHandle_UDP, this, &AUDPClient::HandleRecvPackets_UDP, 0.04f, true);
		GameServerPacketSession_UDP = MakeShared<RtUDPPacketSession>(UDPSocket, InternetAddr_UDP_Recv, InternetAddr_UDP_Send);
		GameServerPacketSession_UDP->SetClient(this);
		GameServerPacketSession_UDP->RunThread();

		// for hole puching
		TArray<uint8> holePacket;
		holePacket.AddZeroed(4);
		SendPacket_UDP(holePacket, sizeof(holePacket));

	}
}

void AUDPClient::SendPacket_UDP(const TArray<uint8>& Data, int CompressedSize)
{
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, Data, CompressedSize]()
	{
		// 헤더 
		stVoiceHeader Header;
		int totalsize;
		totalsize = VHEADSIZE + Data.Num();
		// 송신 버퍼
		TArray<uint8> totalPacket;
		totalPacket.SetNumUninitialized(totalsize);

		Header.SetHeader(CompressedSize, totalsize);
		memcpy(totalPacket.GetData(), &Header, VHEADSIZE);
		memcpy(totalPacket.GetData() + VHEADSIZE, Data.GetData(), Data.Num());

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
		TSharedPtr<class SendBuffer_UDP> sendBuffer = MakeShared<SendBuffer_UDP>(totalsize);
#else
		SendBufferRef sendBuffer = nake_shared<SendBuffer_UDP>(totalsize);
#endif
		sendBuffer->CopyData(totalPacket.GetData(), totalsize);
		sendBuffer->Close(totalsize);

		if (GameServerPacketSession_UDP) {
			GameServerPacketSession_UDP->SendPacketQueue_U.Enqueue(sendBuffer);
		}

	});
}

void AUDPClient::RecvConnectAck(TArray<uint8> packet)
{
	stConnectAck ConnectAck;
	memcpy(&ConnectAck, packet.GetData(), sizeof(stConnectAck));

	RecvPort_UDP = ConnectAck.UDPPort;

	// UDP Start, only Client
    UDP_Start();
}

void AUDPClient::RecvVoiceBuffer(TArray<uint8> packet)
{
	stVoiceHeader Header;
	memcpy(&Header, packet.GetData(), VHEADSIZE);

	int CompressedSize = Header.nCompressedSize;
	int TotalPacketSize = Header.nTotalPacketSize;
	int PayLoadSize = TotalPacketSize - VHEADSIZE;

	if (CompressedSize != 0) // 홀펀칭 거르고 이상한 패킷 거름
	{
		TArray<uint8> VoiceData;
		VoiceData.AddUninitialized(PayLoadSize);
		VoiceData.Init(0, PayLoadSize);
		memcpy(VoiceData.GetData(), packet.GetData() + VHEADSIZE, PayLoadSize);

		// VoiceCharacter
		if (VoiceCharacter) {
			VoiceCharacter->DecodingAndAppending(VoiceData, CompressedSize);
		}
	}
}

void AUDPClient::HandleRecvPackets()
{
	if (TCPSocket == nullptr || GameServerPacketSession == nullptr)
		return;

	GameServerPacketSession->HandleRecvPackets();
}

void AUDPClient::Parse(int32 protocol, TArray<uint8> packet)
{
	switch (protocol)
	{
	case prConnectAck:	RecvConnectAck(packet);	 break;
	}
}

void AUDPClient::HandleRecvPackets_UDP()
{
	if (UDPSocket == nullptr || GameServerPacketSession_UDP == nullptr)
		return;

	GameServerPacketSession_UDP->HandleRecvPackets();
}


