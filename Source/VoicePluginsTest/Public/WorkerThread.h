// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class FSocket;
class RtPacketSession;
class RtUDPPacketSession;
class SendBuffer_UDP;
class SendBuffer;

class VOICEPLUGINSTEST_API RecvWorker : public FRunnable
{
protected:
	FRunnableThread* Thread = nullptr;
	bool bRun = true;
	FSocket* Socket;
	TWeakPtr< RtPacketSession > Session;

public:
	RecvWorker(FSocket* sock, TSharedPtr< RtPacketSession > session);
	~RecvWorker();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;

	void Destroy();

private:
	bool ReceivePacket(TArray<uint8>& OutPacket);
	bool ReceiveDesiredBytes(uint8* Results, int32 Size);
};

class VOICEPLUGINSTEST_API SendWorker : public FRunnable
{
protected:
	FRunnableThread* Thread = nullptr;
	bool bRun = true;
	FSocket* Socket;
	TWeakPtr< RtPacketSession > Session;

public:
	SendWorker(FSocket* sock, TSharedPtr< RtPacketSession > session);
	~SendWorker();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;

	bool SendPacket(TSharedPtr<SendBuffer> SendBuffer);

	void Destroy();

private:
	bool SendDesiredBytes(const uint8* Buffer, int32 Size);


};

class VOICEPLUGINSTEST_API RecvWorker_UDP : public FRunnable
{
public:
	FRunnableThread* Thread = nullptr;
	bool bRun = true;
	FSocket* Socket;
	TWeakPtr< RtUDPPacketSession > Session;

public:
	RecvWorker_UDP(FSocket* sock, TSharedPtr< RtUDPPacketSession > session);
	~RecvWorker_UDP();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;

	void Destroy();

private:
	bool ReceivePacket(TArray<uint8>& OutPacket);
	// 제거예정
	//bool ReceiveDesiredBytes(uint8* Results, int32 Size);
};


class VOICEPLUGINSTEST_API SendWorker_UDP : public FRunnable
{
public:
	FRunnableThread* Thread = nullptr;
	bool bRun = true;
	FSocket* Socket;
	TWeakPtr< RtUDPPacketSession > Session;

public:
	SendWorker_UDP(FSocket* sock, TSharedPtr< RtUDPPacketSession > session);
	~SendWorker_UDP();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;

	bool SendPacket(TSharedPtr<SendBuffer_UDP> SendBuffer);

	void Destroy();
};


class VOICEPLUGINSTEST_API RtPacketSession : public TSharedFromThis< RtPacketSession >	//스마트 포인터로
{
public:
	FSocket* Socket;
	TSharedPtr< class RecvWorker > RecvWorkerThread;
	TSharedPtr< class SendWorker > SendWorkerThread;

	// GameThread와 NetworkThread가 데이터 주고 받는 공용 큐.
	TQueue<TArray<uint8>> RecvPacketQueue;
	TQueue<TSharedPtr<SendBuffer>> SendPacketQueue;

	UPROPERTY()
	class AUDPClient* UDPClient;

public:
	RtPacketSession(FSocket* sock);
	~RtPacketSession();

	void RunThread();
	void DisConnect();

	void SetClient(AUDPClient* InUDPClient);

	UFUNCTION(BlueprintCallable)
	void HandleRecvPackets();
};


class SendBuffer : public TSharedFromThis<SendBuffer>
{
public:
	SendBuffer(int32 bufferSize);
	~SendBuffer();


	BYTE* Buffer() { return _buffer.GetData(); }
	int32 WriteSize() { return _writeSize; }
	int32 Capacity() { return static_cast<int32>(_buffer.Num()); }

	void CopyData(void* data, int32 len);
	void Close(uint32 writeSize);


private:
	TArray<BYTE>	_buffer;
	int32			_writeSize = 0;
};

//===========udp==================

class VOICEPLUGINSTEST_API RtUDPPacketSession : public TSharedFromThis<RtUDPPacketSession>
{
public:
	FSocket* Socket_U;
	TSharedPtr<FInternetAddr> InternetAddr_Recv_U;
	TSharedPtr<FInternetAddr> InternetAddr_Send_U;
	TSharedPtr< class RecvWorker_UDP > RecvWorkerThread_U;
	TSharedPtr< class SendWorker_UDP > SendWorkerThread_U;

	TQueue<TArray<uint8>> RecvPacketQueue_U;
	TQueue<TSharedPtr<class SendBuffer_UDP>> SendPacketQueue_U;

	UPROPERTY()
	class AUDPClient* UDPClient;

public:
	RtUDPPacketSession(FSocket* sock, TSharedPtr<FInternetAddr> InternetAddr_recv, TSharedPtr<FInternetAddr> InternetAddr_send);
	~RtUDPPacketSession();

	void RunThread();
	void DisConnect();

	void SetClient(AUDPClient* InUDPClient);

	UFUNCTION(BlueprintCallable)
	void HandleRecvPackets();

};


class SendBuffer_UDP : public TSharedFromThis<SendBuffer_UDP>
{
public:
	SendBuffer_UDP(int32 bufferSize);
	~SendBuffer_UDP();


	BYTE* Buffer() { return _buffer.GetData(); }
	int32 WriteSize() { return _writeSize; }
	int32 Capacity() { return static_cast<int32>(_buffer.Num()); }

	void CopyData(void* data, int32 len);
	void Close(uint32 writeSize);


private:
	TArray<BYTE>	_buffer;
	int32			_writeSize = 0;
};



