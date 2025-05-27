// Fill out your copyright notice in the Description page of Project Settings.


#include "VoiceCharacter.h"
#include "Kismet/GameplayStatics.h"
// RAI
#include "RuntimeAudioImporterLibrary.h"
#include "Sound/CapturableSoundWave.h"
// En, De
#include "VoiceModule.h"
#include "Voice.h"
#include "Interfaces/VoiceCodec.h"
// Network
#include "UDPClient.h"

// Sets default values
AVoiceCharacter::AVoiceCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AVoiceCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	CapturableSoundWave = UCapturableSoundWave::CreateCapturableSoundWave();
	CapturableSoundWave->GetAvailableAudioInputDevices(FOnGetAvailableAudioInputDevicesResultNative::CreateWeakLambda(this, [](const TArray<FRuntimeAudioInputDeviceInfo>& AvailableDevices)
	{
		// Handle the result
	}));

	CapturableSoundWave->SetSampleRate(48000);

	StreamingSoundWave = UStreamingSoundWave::CreateStreamingSoundWave();
	StreamingSoundWave->SetSampleRate(48000);

	const int32 NumSamples = 1920; // 약 46ms의 오디오 (44.1kHz 기준)
	//const int32 NumSamples = 48000; // 약 46ms의 오디오 (44.1kHz 기준)
	RawCaptureData.AddUninitialized(NumSamples * sizeof(int16) * 1);
	RawCaptureData.Init(0, RawCaptureData.Num());


	CompressedDataSize = 1024;
	//CompressedDataSize = 480 * sizeof(int16) * 1;
	DeCompressedDataSize = 48000 * sizeof(int16) * 1;

	//CompressedData.Empty(CompressedDataSize);
	CompressedData.AddUninitialized(CompressedDataSize);
	CompressedData.Init(0, CompressedDataSize);

	//DeCompressedData.Empty(DeCompressedDataSize);
	DeCompressedData.AddUninitialized(DeCompressedDataSize);
	DeCompressedData.Init(0, DeCompressedDataSize);

	// Encoder, Decoder Init
	Encoder = FVoiceModule::Get().CreateVoiceEncoder(48000, 1, EAudioEncodeHint::VoiceEncode_Voice);
	Decoder = FVoiceModule::Get().CreateVoiceDecoder(48000, 1);

	UDPClient = Cast<AUDPClient>(UGameplayStatics::GetActorOfClass(GetWorld(), AUDPClient::StaticClass()));


}

// Called every frame
void AVoiceCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);



}

// Called to bind functionality to input
void AVoiceCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AVoiceCharacter::ToggleCapture(bool bSwitch)
{
	if (bSwitch) {
        // On
		CapturableSoundWave->StartCapture(0);
		GetWorld()->GetTimerManager().SetTimer(CapturedHandler, this, &AVoiceCharacter::ProcessCapturedPCMData
			, 0.02f, true, 0.1f);
		GetWorld()->GetTimerManager().SetTimer(StreamingHandler, this, &AVoiceCharacter::ProcessStreamingPCMData
			, 2.0f, true, 0.1f);
	}
	else {
		// Off
		CapturableSoundWave->StopCapture();
		GetWorld()->GetTimerManager().ClearTimer(StreamingHandler);
		GetWorld()->GetTimerManager().ClearTimer(CapturedHandler);
	}
}

void AVoiceCharacter::ToggleMute(bool bSwitch)
{
	CapturableSoundWave->ToggleMute(bSwitch);
}

void AVoiceCharacter::ProcessCapturedPCMData()
{
	if (CapturableSoundWave->IsCapturing()) {
		// 확인됨

		int32 sampleneeded = 960; // 960

		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, "GenearatePCM");
		CapturableSoundWave->GeneratePCMData(RawCaptureData.GetData(), sampleneeded);
		// convert
		TArray<uint8> Int16Buffer = ConvertFloat32ToInt16Bytes(RawCaptureData);
		
		uint32 CompressedSize = CompressedDataSize;

		uint32 RawSize = Int16Buffer.Num();

		Encoder->Encode(Int16Buffer.GetData(), RawSize,
			CompressedData.GetData(), CompressedSize);
		


		// SendTo
		if (UDPClient && OnUDP)
		{
			UDPClient->SendPacket_UDP(CompressedData, CompressedSize);
		}
		else
		{
		    uint32 DeCompressedSize = DeCompressedDataSize;
		    
		    Decoder->Decode(CompressedData.GetData(), CompressedSize,
		    	DeCompressedData.GetData(), DeCompressedSize);
		    
		    TArray<uint8> TempBuffer;
		    TempBuffer.AddUninitialized(RawSize);
		    TempBuffer.Init(0, RawSize);
		    
		    FMemory::Memcpy(TempBuffer.GetData(), DeCompressedData.GetData(), DeCompressedSize);
		    
		    StreamingSoundWave->AppendAudioDataFromRAW(TempBuffer, ERuntimeRAWAudioFormat::Int16, 48000, 1);
		    //StreamingSoundWave->AppendAudioDataFromRAW(RawCaptureData, ERuntimeRAWAudioFormat::Float32, 48000, 1);
		    //StreamingSoundWave->AppendAudioDataFromRAW(Int16Buffer, ERuntimeRAWAudioFormat::Int16, 48000, 1);
		    
		    UE_LOG(LogTemp, Warning, TEXT("Raw Size: %d, Compressed Size: %d, Decompressed Size: %d"), RawSize, CompressedSize, DeCompressedSize);
		}
	
	}

	// Init
	CompressedData.Init(0, CompressedDataSize);

}

void AVoiceCharacter::ProcessStreamingPCMData()
{
	if (StreamingSoundWave) {

		// Play the sound wave
		UGameplayStatics::PlaySound2D(GetWorld(), StreamingSoundWave);
		StreamingSoundWave->ReleaseMemory();
	}
}

TArray<uint8> AVoiceCharacter::ConvertFloat32ToInt16Bytes(const TArray<uint8>& FloatBuffer)
{
	// 결과를 저장할 TArray<uint8>
	TArray<uint8> Int16Bytes;

	// Float32 샘플 수 계산
	int32 NumSamples = FloatBuffer.Num() / sizeof(float);

	// 결과 배열의 크기를 미리 설정 (2 bytes per int16 sample)
	Int16Bytes.SetNum(NumSamples * sizeof(int16));

	for (int32 i = 0; i < NumSamples; i++)
	{
		// Float32 값 읽기
		float Sample = *reinterpret_cast<const float*>(&FloatBuffer[i * sizeof(float)]);

		// Float32를 int16으로 변환
		int16 Int16Sample = FMath::Clamp<int32>(Sample * 32767.0f, -32768, 32767);

		// int16을 두 개의 uint8로 변환하여 결과 배열에 저장
		Int16Bytes[i * 2] = Int16Sample & 0xFF;
		Int16Bytes[i * 2 + 1] = (Int16Sample >> 8) & 0xFF;
	}

	return Int16Bytes;
}

void AVoiceCharacter::DecodingAndAppending(const TArray<uint8>& buffer, uint32 CompressedSize)
{
	uint32 DeCompressedSize = DeCompressedDataSize;

	Decoder->Decode(buffer.GetData(), CompressedSize,
		DeCompressedData.GetData(), DeCompressedSize);

	TArray<uint8> TempBuffer;
	TempBuffer.AddUninitialized(1920);
	TempBuffer.Init(0, 1920);

	FMemory::Memcpy(TempBuffer.GetData(), DeCompressedData.GetData(), DeCompressedSize);

	StreamingSoundWave->AppendAudioDataFromRAW(TempBuffer, ERuntimeRAWAudioFormat::Int16, 48000, 1);
	//StreamingSoundWave->AppendAudioDataFromRAW(RawCaptureData, ERuntimeRAWAudioFormat::Float32, 48000, 1);
	//StreamingSoundWave->AppendAudioDataFromRAW(Int16Buffer, ERuntimeRAWAudioFormat::Int16, 48000, 1);

}

void AVoiceCharacter::PlayVoice()
{
	if (StreamingSoundWave) {

		// Play the sound wave
		UGameplayStatics::PlaySound2D(GetWorld(), StreamingSoundWave);
	}
}

void AVoiceCharacter::SetOnUDP(bool Switch)
{
	OnUDP = Switch;
}

