// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UDPClient.generated.h"

class RtPacketSession;
class RtUDPPacketSession;
class AVoiceCharacter;

UCLASS()
class VOICEPLUGINSTEST_API AUDPClient : public AActor
{
	GENERATED_BODY()
	
public:
	class FSocket* TCPSocket;
	class FSocket* UDPSocket;

	// TCP
	FString IpAddress_TCP;
	int16   Port_TCP;

	TSharedPtr<RtPacketSession> GameServerPacketSession;

	// UDP
	FString IpAddress_UDP;
	int16 SendPort_UDP;      
	int16 RecvPort_UDP;    

	TSharedPtr<FInternetAddr> InternetAddr_UDP_Send;
	TSharedPtr<FInternetAddr> InternetAddr_UDP_Recv;
	TSharedPtr<RtUDPPacketSession> GameServerPacketSession_UDP;

	FTimerHandle ReceiveTimerHandle;
	FTimerHandle ReceiveTimerHandle_UDP;

	UPROPERTY()
	AVoiceCharacter* VoiceCharacter;



public:	
	// Sets default values for this actor's properties
	AUDPClient();


	// TCP Connect
	UFUNCTION(BlueprintCallable)
	void TCP_Connect();

	void UDP_Start();

	void SendPacket_UDP(const TArray<uint8>& Data, int CompressedSize);

	// Recv Packet 
	void RecvConnectAck(TArray<uint8> packet);

	//////////////////////////////////////////////////////////////////////////////
    ///// UDP Recv //////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////

	void RecvVoiceBuffer(TArray<uint8> packet);


	UFUNCTION(BlueprintCallable)
	void HandleRecvPackets();

	UFUNCTION(BlueprintCallable)
	void HandleRecvPackets_UDP();

	void Parse(int32 protocol, TArray<uint8> packet);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
