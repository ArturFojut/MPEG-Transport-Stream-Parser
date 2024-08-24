#include "tsCommon.h"
#include "tsTransportStream.h"

//=============================================================================================================================================================================

int main(int argc, char *argv[ ], char *envp[ ])
{
  // TODO - open file
  FILE* TS_File = std::fopen("example_new.ts", "rb");
  // TODO - check if file if opened
  if (!TS_File)
  {
    std::perror("File opening failed");
    return EXIT_FAILURE;
  } 

  FILE* MP2_File = std::fopen("PID136.mp2", "wb");
  // TODO - check if file if opened
  if (!MP2_File)
  {
    std::perror("File opening failed");
    return EXIT_FAILURE;
  } 

  FILE* Video_File = std::fopen("PID174.264", "wb");
  // TODO - check if file if opened
  if (!Video_File)
  {
    std::perror("File opening failed");
    return EXIT_FAILURE;
  }

  xTS_PacketHeader    TS_PacketHeader;
  uint8_t TS_PacketBuffer[xTS::TS_PacketLength];
  xTS_AdaptationField TS_AdaptationField; 
  xPES_Assembler PES_AssemblerAudio;
  PES_AssemblerAudio.Init(136);
  xPES_Assembler PES_AssemblerVideo;
  PES_AssemblerVideo.Init(174);

  int32_t TS_PacketId = 0;
  while(!std::feof(TS_File))
  {
    // TODO - read from file
    size_t bytesRead = fread(TS_PacketBuffer, 1, xTS::TS_PacketLength, TS_File);
    if (bytesRead != xTS::TS_PacketLength) break;

    TS_PacketHeader.Reset();
    TS_PacketHeader.Parse(TS_PacketBuffer);

    if (TS_PacketHeader.getSyncByte() == 'G' && TS_PacketHeader.getPacketIdentifier() == 136) {

      printf("%010d ", TS_PacketId);
      TS_PacketHeader.Print();

      if (TS_PacketHeader.hasAdaptationField()) {
        TS_AdaptationField.Reset();
        TS_AdaptationField.Parse(TS_PacketBuffer, TS_PacketHeader.getAdaptationFieldControl());
        TS_AdaptationField.Print();
      }

      xPES_Assembler::eResult Result = PES_AssemblerAudio.AbsorbPacket(TS_PacketBuffer, &TS_PacketHeader, &TS_AdaptationField, MP2_File);
      switch (Result)
      {
        case xPES_Assembler::eResult::StreamPackedLost: printf("PcktLost "); break;
        case xPES_Assembler::eResult::AssemblingStarted: printf("Started "); PES_AssemblerAudio.PrintPESH(); break;
        case xPES_Assembler::eResult::AssemblingContinue: printf("Continue "); break;
        case xPES_Assembler::eResult::AssemblingFinished: 
          printf("Finished ");
          printf("PES: PcktLen=%d HeadLen=%d DataLen=%u", PES_AssemblerAudio.getNumPacketBytes(), PES_AssemblerAudio.getHeadLen(), PES_AssemblerAudio.getDataLen());
          //fwrite(PES_AssemblerAudio.getPacket() + PES_AssemblerAudio.getHeadLen(), 1, PES_AssemblerAudio.getDataLen(), MP2_File);
          break;   
        default: break;
      }

      printf("\n");
    }

    if (TS_PacketHeader.getSyncByte() == 'G' && TS_PacketHeader.getPacketIdentifier() == 174) {

      if (TS_PacketHeader.hasAdaptationField()) {
        TS_AdaptationField.Reset();
        TS_AdaptationField.Parse(TS_PacketBuffer, TS_PacketHeader.getAdaptationFieldControl());
      }

      xPES_Assembler::eResult Result = PES_AssemblerVideo.AbsorbPacket(TS_PacketBuffer, &TS_PacketHeader, &TS_AdaptationField, Video_File);

      if (Result == xPES_Assembler::eResult::AssemblingFinishedAndStarted) {
        printf("Finished ");
        printf("PES: PcktLen=%d HeadLen=%d DataLen=%u", PES_AssemblerVideo.getNumPacketBytesPrev(), PES_AssemblerVideo.getHeadLenPrev(), PES_AssemblerVideo.getDataLen());
        //fwrite(PES_AssemblerVideo.getPacket() + PES_AssemblerVideo.getHeadLen(), 1, PES_AssemblerVideo.getDataLen(), Video_File);

        printf("\n%010d ", TS_PacketId);
        TS_PacketHeader.Print();
        if (TS_PacketHeader.hasAdaptationField()) {
          TS_AdaptationField.Print();
        }
        printf("Started "); 
        PES_AssemblerVideo.PrintPESH();

      }
      else {
        printf("%010d ", TS_PacketId);
        TS_PacketHeader.Print();
        if (TS_PacketHeader.hasAdaptationField()) {
          TS_AdaptationField.Print();
        }
      }

      switch (Result)
      {
        case xPES_Assembler::eResult::StreamPackedLost: printf("PcktLost "); break;
        case xPES_Assembler::eResult::AssemblingStarted: printf("Started "); PES_AssemblerVideo.PrintPESH(); break;
        case xPES_Assembler::eResult::AssemblingContinue: printf("Continue "); break;
        case xPES_Assembler::eResult::AssemblingFinished:
          printf("Finished ");
          printf("PES: PcktLen=%d HeadLen=%d DataLen=%u", PES_AssemblerVideo.getNumPacketBytes(), PES_AssemblerVideo.getHeadLen(), PES_AssemblerVideo.getDataLen());
          //fwrite(PES_AssemblerVideo.getPacket() + PES_AssemblerVideo.getHeadLen(), 1, PES_AssemblerVideo.getDataLen(), Video_File);
          break;
        default: break;
      }

      printf("\n");
    }

    TS_PacketId++;
     //if (TS_PacketId == 500) break;
  }

  // TODO - close file
  std::fclose(TS_File);
  std::fclose(MP2_File);
  std::fclose(Video_File);

  return EXIT_SUCCESS;
}

//=============================================================================================================================================================================
