// Fill out your copyright notice in the Description page of Project Settings.


#include "WorkerThread.h"
#include "Kismet/GameplayStatics.h"
// Socket
#include "Sockets.h"
#include "Common/UdpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"

#include "UDPClient.h"

// protocol
#include "VProtocol.h"
#include "VPacket.h"

RecvWorker::RecvWorker(FSocket* sock, TSharedPtr<RtPacketSession> session) : Socket(sock), Session(session)
{
	Thread = FRunnableThread::Create(this, TEXT("RecvWorkerThread"));

}

RecvWorker::~RecvWorker()
{
	Destroy();
}

bool RecvWorker::Init()
{
	return true;
}

uint32 RecvWorker::Run()
{
	while (bRun)
	{
		TArray<uint8> Packet;
		if (ReceivePacket(OUT Packet))
		{
			if (TSharedPtr<RtPacketSession> pSession = Session.Pin())
			{
				pSession->RecvPacketQueue.Enqueue(Packet);
			}
		}
	}
	return 0;
}

bool RecvWorker::ReceivePacket(TArray<uint8>& OutPacket)
{

	// 패킷 헤더 파싱
	const int32 HeaderSize = HEADSIZE;//sizeof( stHeader );
	TArray<uint8> HeaderBuffer;
	HeaderBuffer.AddZeroed(HeaderSize);

	if (ReceiveDesiredBytes(HeaderBuffer.GetData(), HeaderSize) == false)
		return false;

	// ID, Size 추출
	stHeader Header;
	{
		FMemoryReader Reader(HeaderBuffer);
		Reader << Header;
		UE_LOG(LogTemp, Log, TEXT("Recv PacketID : %d, PacketSize : %d"), Header.nID, Header.nSize);
	}
	// 패킷 헤더 복사
	OutPacket = HeaderBuffer;

	// 패킷 내용 파싱
	TArray<uint8> PayloadBuffer;
	const int32 PayloadSize = Header.nSize - HeaderSize;
	if (PayloadSize == 0)
		return true;

	OutPacket.AddZeroed(PayloadSize);

	if (ReceiveDesiredBytes(&OutPacket[HeaderSize], PayloadSize))
		return true;

	return false;
}

bool RecvWorker::ReceiveDesiredBytes(uint8* Results, int32 Size)
{
	if (!Socket) return false;

	uint32 PendingDataSize;
	if (Socket->HasPendingData(PendingDataSize) == false || PendingDataSize <= 0)
		return false;

	int32 Offset = 0;

	while (Size > 0)
	{
		int32 NumRead = 0;
		Socket->Recv(Results + Offset, Size, OUT NumRead);
		check(NumRead <= Size);

		if (NumRead <= 0)
			return false;

		Offset += NumRead;
		Size -= NumRead;
	}

	return true;
}

void RecvWorker::Exit()
{
	bRun = false;
}

void RecvWorker::Destroy()
{
	bRun = false;
}


//--------------------------------------------------------------------------------------------------------------------------
// SendWorker
//--------------------------------------------------------------------------------------------------------------------------
SendWorker::SendWorker(FSocket* sock, TSharedPtr<RtPacketSession> session) : Socket(sock), Session(session)
{
	Thread = FRunnableThread::Create(this, TEXT("SendWorkerThread"));
}

SendWorker::~SendWorker()
{
	Destroy();
}

bool SendWorker::Init()
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Send Thread Init")));

	return true;
}

uint32 SendWorker::Run()
{
	while (bRun)
	{
		TSharedPtr<SendBuffer> SendBuffer;

		if (TSharedPtr<RtPacketSession> pSession = Session.Pin())
		{
			if (pSession->SendPacketQueue.Dequeue(OUT SendBuffer))
			{
				SendPacket(SendBuffer);
			}
		}
	}

	return 0;
}

void SendWorker::Exit()
{
	bRun = false;
}

bool SendWorker::SendPacket(TSharedPtr<SendBuffer> SendBuffer)
{
	if (SendDesiredBytes(SendBuffer->Buffer(), SendBuffer->WriteSize()) == false)
		return false;

	return true;
}

void SendWorker::Destroy()
{
	bRun = false;
}

bool SendWorker::SendDesiredBytes(const uint8* Buffer, int32 Size)
{
	while (Size > 0)
	{
		int32 BytesSent = 0;
		if (Socket->Send(Buffer, Size, BytesSent) == false)
			return false;

		Size -= BytesSent;
		Buffer += BytesSent;
	}

	return true;
}

//--------------------------------------------------------------------------------------------------------------------------
// RecvWorker_UDP
//--------------------------------------------------------------------------------------------------------------------------


RecvWorker_UDP::RecvWorker_UDP(FSocket* sock, TSharedPtr<RtUDPPacketSession> session) : Socket(sock), Session(session)
{
	Thread = FRunnableThread::Create(this, TEXT("RecvWorkerThreadForUDP"));
}

RecvWorker_UDP::~RecvWorker_UDP()
{
	Destroy();
}

bool RecvWorker_UDP::Init()
{
	return true;
}

uint32 RecvWorker_UDP::Run()
{
	while (bRun)
	{
		TArray<uint8> Packet;
		if (ReceivePacket(OUT Packet))
		{
			if (Packet.IsEmpty()) continue;

			if (TSharedPtr<RtUDPPacketSession> pSession = Session.Pin())
			{
				pSession->RecvPacketQueue_U.Enqueue(Packet);
			}
		}
	}
	return 0;
}

void RecvWorker_UDP::Exit()
{
	bRun = false;
}

void RecvWorker_UDP::Destroy()
{
	bRun = false;
}

bool RecvWorker_UDP::ReceivePacket(TArray<uint8>& OutPacket)
{
	if (!Socket) return false;
	uint32 ReceivedDataSize;
	TArray<uint8> ReceivedData;

	while (Socket->HasPendingData(ReceivedDataSize))
	{
		ReceivedData.SetNumUninitialized(ReceivedDataSize);
		// maybe need empty addr
		int32 BytesRead = 0;
		Socket->RecvFrom(ReceivedData.GetData(), ReceivedDataSize, OUT BytesRead, *Session.Pin()->InternetAddr_Recv_U);
		OutPacket = ReceivedData;
	}
	FPlatformProcess::Sleep(0.01f);
	return true;
}

//--------------------------------------------------------------------------------------------------------------------------
// SendWorker_UDP
//--------------------------------------------------------------------------------------------------------------------------

SendWorker_UDP::SendWorker_UDP(FSocket* sock, TSharedPtr<RtUDPPacketSession> session) : Socket(sock), Session(session)
{
	Thread = FRunnableThread::Create(this, TEXT("SendWorkerThreadForUDP"));
}

SendWorker_UDP::~SendWorker_UDP()
{
	Destroy();
}

bool SendWorker_UDP::Init()
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Send UDP Thread Init")));

	return true;
}

uint32 SendWorker_UDP::Run()
{
	while (bRun)
	{
		TSharedPtr<SendBuffer_UDP> SendBuffer;

		if (TSharedPtr<RtUDPPacketSession> pSession = Session.Pin())
		{
			if (pSession->SendPacketQueue_U.Dequeue(OUT SendBuffer))
			{
				SendPacket(SendBuffer);
			}
		}
	}

	return 0;
}

void SendWorker_UDP::Exit()
{
	bRun = false;
}

bool SendWorker_UDP::SendPacket(TSharedPtr<SendBuffer_UDP> SendBuffer)
{
	if (Socket == nullptr) {
		return false;
	}
	uint8* data = SendBuffer->Buffer();
	int32 size = SendBuffer->WriteSize();
	int32 sent = 0;

	if (size <= 0) return true;
	// Server's Recv Port
	Socket->SendTo(data, size, sent, *Session.Pin()->InternetAddr_Send_U);
	return true;
}

void SendWorker_UDP::Destroy()
{
	bRun = false;
}



RtPacketSession::RtPacketSession(FSocket* sock) : Socket(sock)
{
}

RtPacketSession::~RtPacketSession()
{
	DisConnect();
}

// PacketSeesion's Main Function
void RtPacketSession::RunThread()
{
	RecvWorkerThread = MakeShared<RecvWorker>(Socket, AsShared());
	SendWorkerThread = MakeShared<SendWorker>(Socket, AsShared());
}

void RtPacketSession::DisConnect()
{
	if (RecvWorkerThread)
	{
		RecvWorkerThread->Destroy();
		RecvWorkerThread = nullptr;
	}
}

void RtPacketSession::SetClient(AUDPClient* InUDPClient)
{
	UDPClient = InUDPClient;
}

void RtPacketSession::HandleRecvPackets()
{
	while (true)
	{
		TArray<uint8> Packet;
		if (RecvPacketQueue.Dequeue(OUT Packet) == false)
			break;

		if (UDPClient)
		{
			stHeader header;

			memcpy(&header, Packet.GetData(), sizeof(stHeader));

			int32 protocol = header.nID;

			UDPClient->Parse(protocol, Packet);
		}
	}
}



RtUDPPacketSession::RtUDPPacketSession(FSocket* sock, TSharedPtr<FInternetAddr> InternetAddr_recv, TSharedPtr<FInternetAddr> InternetAddr_send) :
	Socket_U(sock), InternetAddr_Recv_U(InternetAddr_recv), InternetAddr_Send_U(InternetAddr_send)
{

}

RtUDPPacketSession::~RtUDPPacketSession()
{
	DisConnect();
}

void RtUDPPacketSession::RunThread()
{
	RecvWorkerThread_U = MakeShared<RecvWorker_UDP>(Socket_U, AsShared());
	SendWorkerThread_U = MakeShared<SendWorker_UDP>(Socket_U, AsShared());
}

void RtUDPPacketSession::DisConnect()
{
	if (RecvWorkerThread_U)
	{
		RecvWorkerThread_U->Destroy();
		RecvWorkerThread_U = nullptr;
	}
}

void RtUDPPacketSession::SetClient(AUDPClient* InUDPClient)
{
	UDPClient = InUDPClient;
}

void RtUDPPacketSession::HandleRecvPackets()
{
	while (true)
	{
		TArray<uint8> Packet;
		if (RecvPacketQueue_U.Dequeue(OUT Packet) == false)
			break;

		if (Packet.IsEmpty())
			break;

		// 수정 필요 
		if (UDPClient) {
			UDPClient->RecvVoiceBuffer(Packet);
		}
	}
}

SendBuffer_UDP::SendBuffer_UDP(int32 bufferSize)
{
	_buffer.SetNum(bufferSize);
}

SendBuffer_UDP::~SendBuffer_UDP()
{
}

void SendBuffer_UDP::CopyData(void* data, int32 len)
{
	::memcpy(_buffer.GetData(), data, len);
	_writeSize = len;
}

void SendBuffer_UDP::Close(uint32 writeSize)
{
	_writeSize = writeSize;
}
