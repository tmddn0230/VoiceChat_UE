// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VoiceCharacter.generated.h"

class UCapturableSoundWave;
class UStreamingSoundWave;

class IVoiceCapture;
class IVoiceEncoder;
class IVoiceDecoder;

class AUDPClient;

UCLASS()
class VOICEPLUGINSTEST_API AVoiceCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	UCapturableSoundWave* CapturableSoundWave;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	UStreamingSoundWave*  StreamingSoundWave;


	FTimerHandle CapturedHandler;
	FTimerHandle StreamingHandler;

	TArray<uint8> RawCaptureData;
	TArray<uint8> CompressedData;
	TArray<uint8> DeCompressedData;

	TSharedPtr<IVoiceCapture> Capture;
	TSharedPtr<IVoiceEncoder> Encoder;
	TSharedPtr<IVoiceDecoder> Decoder;

	UPROPERTY()
	AUDPClient* UDPClient;

	uint32 CompressedDataSize = 0;
	uint32 DeCompressedDataSize = 0;

public:
	// Sets default values for this character's properties
	AVoiceCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:

	// Start : 데이터 쌓기 시작, Stop : 데이터 쌓기 중단
	UFUNCTION(BlueprintCallable)
	void ToggleCapture(bool bSwitch);


	// 데이터 축적 일시중단
	UFUNCTION(BlueprintCallable)
	void ToggleMute(bool bSwitch);



	void ProcessCapturedPCMData();
	void ProcessStreamingPCMData();

	// 데이터 가공
	TArray<uint8> ConvertFloat32ToInt16Bytes(const TArray<uint8>& FloatBuffer);


	void DecodingAndAppending(const TArray<uint8>& buffer, uint32 CompressedSize);

	//-================================
	UFUNCTION(BlueprintCallable)
	void PlayVoice();


	bool OnUDP = true;


	UFUNCTION(BlueprintCallable)
	void SetOnUDP(bool Switch);

};
