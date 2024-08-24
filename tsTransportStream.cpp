#include "tsTransportStream.h"

//=============================================================================================================================================================================
// xTS_PacketHeader
//=============================================================================================================================================================================

/// @brief Reset - reset all TS packet header fields
void xTS_PacketHeader::Reset()
{
  this->m_SB = 0;
  this->m_E = 0;
  this->m_S = 0;
  this->m_P = 0;
  this->m_PID = 0;
  this->m_TSC = 0;
  this->m_AFC = 0;
  this->m_CC = 0;
}

/**
  @brief Parse all TS packet header fields
  @param Input is pointer to buffer containing TS packet
  @return Number of parsed bytes (4 on success, -1 on failure)
 */
int32_t xTS_PacketHeader::Parse(const uint8_t *Input)
{
  if (Input == nullptr)
  {
    return NOT_VALID;
  }
  uint32_t *headerPtr = (uint32_t *)Input;
  uint32_t header = xSwapBytes32(*headerPtr);

  this->m_SB = ((0b11111111000000000000000000000000 & header) >> 24);
  this->m_E = ((0b00000000100000000000000000000000 & header) >> 23);
  this->m_S = ((0b00000000010000000000000000000000 & header) >> 22);
  this->m_P = ((0b00000000001000000000000000000000 & header) >> 21);
  this->m_PID = ((0b00000000000111111111111100000000 & header) >> 8);
  this->m_TSC = ((0b00000000000000000000000011000000 & header) >> 6);
  this->m_AFC = ((0b00000000000000000000000000110000 & header) >> 4);
  this->m_CC = (0b00000000000000000000000000001111 & header);
  return VALID;
}

/// @brief Print all TS packet header fields
void xTS_PacketHeader::Print() const
{
  printf("TS: SB=%u E=%u S=%u P=%u PID=%u TSC=%u AF=%u CC=%u ",
         this->m_SB, this->m_E, this->m_S, this->m_P, this->m_PID, this->m_TSC, this->m_AFC, this->m_CC);
}

//=============================================================================================================================================================================
/// @brief Reset - reset all TS packet header fields
void xTS_AdaptationField::Reset()
{
  m_AdaptationFieldControl = 0;
  m_AdaptationFieldLength = 0;
  m_DC = 0;
  m_RA = 0;
  m_SP = 0;
  m_PR = 0;
  m_OR = 0;
  m_SF = 0;
  m_TP = 0;
  m_EX = 0;
  m_PCR = 0;
  m_OPCR = 0;
  m_PCRtime = 0.0;
  m_OPCRtime = 0.0;
  m_StuffingBytes = 0;
}
/**
@brief Parse adaptation field
@param PacketBuffer is pointer to buffer containing TS packet
@param AdaptationFieldControl is value of Adaptation Field Control field of
corresponding TS packet header
@return Number of parsed bytes (length of AF or -1 on failure)
*/
int32_t xTS_AdaptationField::Parse(const uint8_t *PacketBuffer, uint8_t AdaptationFieldControl)
{
  if (PacketBuffer == nullptr)
  {
    return NOT_VALID;
  }  
  this->m_AdaptationFieldControl = AdaptationFieldControl;
  const uint8_t* AFL_Ptr = PacketBuffer + xTS::TS_HeaderLength;
  m_AdaptationFieldLength = *AFL_Ptr;
  AFL_Ptr++;
  m_DC = ((0b10000000 & *AFL_Ptr) >> 7);
  m_RA = ((0b01000000 & *AFL_Ptr) >> 6);
  m_SP = ((0b00100000 & *AFL_Ptr) >> 5);
  m_PR = ((0b00010000 & *AFL_Ptr) >> 4);
  m_OR = ((0b00001000 & *AFL_Ptr) >> 3);
  m_SF = ((0b00000100 & *AFL_Ptr) >> 2);
  m_TP = ((0b00000010 & *AFL_Ptr) >> 1);
  m_EX = (0b00000001 & *AFL_Ptr);

  if (m_PR == 1) {
    m_PCR = ParsePCR(PacketBuffer);
    m_PCRtime = double(m_PCR) / xTS::ExtendedClockFrequency_Hz;
  }
  if (m_OR == 1) {
    m_OPCR = ParseOPCR(PacketBuffer);
    m_OPCRtime = double(m_OPCR) / xTS::ExtendedClockFrequency_Hz;
  }

  m_StuffingBytes = CalculateStuffingBytes(PacketBuffer);

  return m_AdaptationFieldLength + 1;
}

uint64_t xTS_AdaptationField::ParsePCR(const uint8_t* PacketBuffer) {
  if (m_AdaptationFieldLength <= 1 || PacketBuffer == nullptr) {
    return NOT_VALID;
  }

  const uint8_t* pcrPtr = PacketBuffer + xTS::TS_HeaderLength + 2;
  uint64_t* pcrDataPtr = (uint64_t*)pcrPtr;
  uint64_t pcrData = xSwapBytes64(*pcrDataPtr);
  pcrData = pcrData >> (64 - 48);
  uint64_t pcrBase = pcrData >> 15;         
  uint16_t pcrExtension = pcrData & 0b111111111;

  uint64_t fullPCR = pcrBase * 300 + pcrExtension;

  return fullPCR;
}

uint64_t xTS_AdaptationField::ParseOPCR(const uint8_t* PacketBuffer) {
  if (m_AdaptationFieldLength <= 1 || PacketBuffer == nullptr) {
    return NOT_VALID;
  }
  const uint8_t* opcrPtr = nullptr;
  if (m_PR == 1) {
    opcrPtr = PacketBuffer + xTS::TS_HeaderLength + 8;
  }
  else {
    opcrPtr = PacketBuffer + xTS::TS_HeaderLength + 2;
  }
  uint64_t* opcrDataPtr = (uint64_t*)opcrPtr;
  uint64_t opcrData = xSwapBytes64(*opcrDataPtr);
  opcrData = opcrData >> (64 - 48);
  uint64_t opcrBase = opcrData >> 15;
  uint16_t opcrExtension = opcrData & 0b111111111;

  uint64_t fullOPCR = opcrBase * 300 + opcrExtension;

  return fullOPCR;
}

uint8_t xTS_AdaptationField::CalculateStuffingBytes(const uint8_t* PacketBuffer) {
  if (PacketBuffer == nullptr) return NOT_VALID;
  if (m_AdaptationFieldLength < 2) return 0;

  uint8_t* fieldPtr = (uint8_t*)PacketBuffer + xTS::TS_HeaderLength;

  size_t usedBytes = 2;
  fieldPtr+=2;

  if (m_PR) { 
    usedBytes += 6; // PCR field - 6 bytes
    fieldPtr += 6;
  }

  if (m_OR) { 
    usedBytes += 6; // OPCR field - also 6 bytes
    fieldPtr += 6;
  }

  if (m_SF) { 
    usedBytes += 1; // splicing_point_flag field - 1 byte
    fieldPtr += 1;
  }

  if (m_TP) { 
    uint8_t privateDataLength = *fieldPtr; // length of private data
    usedBytes += 1 + privateDataLength; 
    fieldPtr += 1 + privateDataLength;
  }

  if (m_EX) { 
    uint8_t extensionLength = *fieldPtr; // length of the extension
    usedBytes += 1 + extensionLength; 
  }

  uint8_t stuffingBytes = m_AdaptationFieldLength + 1 - usedBytes;
  return stuffingBytes;
}


/// @brief Print all TS packet header fields
void xTS_AdaptationField::Print() const
{
  printf(" AF: L=%u DC=%u RA=%u SP=%u PR=%u OR=%u SF=%u TP=%u EX=%u ", this->m_AdaptationFieldLength, this->m_DC, this->m_RA, this->m_SP, this->m_PR, this->m_OR, this->m_SF, this->m_TP, this->m_EX);
  if (m_PR == 1) printf("PCR=%llu (Time = %fs) ", this->m_PCR, this->m_PCRtime);
  if (m_OR == 1) printf("OPCR=%llu (Time = %fs) ", this->m_OPCR, this->m_OPCRtime);
  printf("Stuffing=%u ", this->m_StuffingBytes);
}

xPES_Assembler::xPES_Assembler() : m_PID(-1), m_Buffer(nullptr), m_BufferSize(0), m_DataOffset(0), m_LastContinuityCounter(-1), m_Started(false) {}

xPES_Assembler::~xPES_Assembler() {
  delete[] m_Buffer;  
  m_Buffer = nullptr;
}

void xPES_Assembler::Init(int32_t PID) {
  m_PID = PID;
}

void xPES_Assembler::xBufferReset() {
  delete[] m_Buffer;
  m_Buffer = nullptr;
  m_BufferSize = 0;
  m_DataOffset = 0;
}

int32_t xPES_PacketHeader::Parse(const uint8_t* Input) {
  if (Input == nullptr) return NOT_VALID;

  m_PacketStartCodePrefix = xSwapBytes32(*(uint32_t *)Input);
  m_PacketStartCodePrefix >>= 8;
  m_StreamId = Input[3];
  Input += 4;
  m_PacketLength = xSwapBytes16(*(uint16_t*)Input);
  Input += 2;

  int32_t headerLength = 6; 
  if (m_StreamId != eStreamId_program_stream_map && m_StreamId != eStreamId_padding_stream && m_StreamId != eStreamId_private_stream_2 && m_StreamId != eStreamId_ECM && m_StreamId != eStreamId_EMM && m_StreamId != eStreamId_program_stream_directory && m_StreamId != eStreamId_DSMCC_stream && m_StreamId != eStreamId_ITUT_H222_1_type_E) {
    uint8_t flags = Input[1];
    uint8_t PES_header_data_length = Input[2];
    uint32_t index = 3;
    headerLength += 3;

    if (flags & 0b10000000) { 
      m_PTS = (((uint64_t(Input[index]) >> 1) & 0x07) << 30) | (uint64_t(Input[index + 1]) << 22) | ((uint64_t(Input[index + 2]) >> 1) << 15) | (uint64_t(Input[index + 3]) << 7) | (uint64_t(Input[index + 4]) >> 1);
      m_PTStime = (double)m_PTS / xTS::BaseClockFrequency_Hz;
      index += 5;  
      //headerLength += 5;
    }
    if (flags & 0b01000000) { 
      m_DTS = ((uint64_t(Input[index]) >> 1) & 0x07) << 30 | (uint64_t(Input[index + 1]) << 22) | ((uint64_t(Input[index + 2]) >> 1) << 15) | (uint64_t(Input[index + 3]) << 7) | (uint64_t(Input[index + 4]) >> 1);
      m_DTStime = (double)m_DTS / xTS::BaseClockFrequency_Hz;
      index += 5; 
      //headerLength += 5;
    }
    headerLength += PES_header_data_length;  
  }
  return headerLength;
}

void xPES_PacketHeader::Reset() {
  m_PacketStartCodePrefix = 0;
  m_StreamId = 0;
  m_PacketLength = 0;
  m_PTS = 0;
  m_DTS = 0;
  m_PTStime = 0.0;
  m_DTStime = 0.0;
}

void xPES_PacketHeader::Print() const {
  printf("PES: PSCP=%u SID=%u L=%u ",
    m_PacketStartCodePrefix,
    m_StreamId,
    m_PacketLength);
  if (m_PTS) 
    printf("PTS=%llu (Time=%fs) ", m_PTS, m_PTStime);
  if (m_DTS) 
    printf("DTS=%llu (Time=%fs) ", m_DTS, m_DTStime);
}

void xPES_Assembler::xBufferAppend(const uint8_t* Data, int32_t Size) {
  if (Data == nullptr || Size <= 0) return; 

  if (m_DataOffset + Size <= m_BufferSize) {
    for (size_t i = 0; i < Size; i++) {
      m_Buffer[m_DataOffset + i] = Data[i];
    }
  }

  m_DataOffset += Size; 
}

xPES_Assembler::eResult xPES_Assembler::AbsorbPacket(const uint8_t* TransportStreamPacket, const xTS_PacketHeader* PacketHeader, const xTS_AdaptationField* AdaptationField, FILE* file) {
  if (PacketHeader->getPacketIdentifier() != m_PID) return eResult::UnexpectedPID;
  if (PacketHeader->getContinuityCounter() != ((m_LastContinuityCounter + 1) % 16)) return eResult::StreamPackedLost;

  m_LastContinuityCounter = PacketHeader->getContinuityCounter();
  const uint8_t* data = TransportStreamPacket + xTS::TS_HeaderLength;
  if (PacketHeader->hasAdaptationField()) {
    data += 1 + AdaptationField->getAdaptationFieldLength(); 
  }

  int32_t dataLength = xTS::TS_PacketLength - (data - TransportStreamPacket);

   if (PacketHeader->getPayloadUnitStartIndicator()) {
     if (!m_Started) {
       m_Started = true;
       xBufferReset();
       m_PESH.Reset();
       m_HeadLen = m_PESH.Parse(data);

       if (m_PESH.getPacketLength() == 0) {
         m_BufferSize = 100000;
       }
       else {
         m_BufferSize = m_PESH.getPacketLength() + 6;
       }
       m_Buffer = new uint8_t[m_BufferSize];

       xBufferAppend(data, dataLength);
       return eResult::AssemblingStarted;
     }
     else {
       m_HeadLenPrev = m_HeadLen;
       if (m_HeadLenPrev != NOT_VALID)
         m_DataLen = m_DataOffset - m_HeadLenPrev;
       m_DataOffsetPrev = m_DataOffset;
       fwrite(m_Buffer + m_HeadLenPrev, 1, m_DataLen, file);

       xBufferReset();
       m_PESH.Reset();
       m_HeadLen = m_PESH.Parse(data);

       if (m_PESH.getPacketLength() == 0) {
         m_BufferSize = 100000;
       }
       else {
         m_BufferSize = m_PESH.getPacketLength() + 6;
       }
       m_Buffer = new uint8_t[m_BufferSize];

       xBufferAppend(data, dataLength);

       return eResult::AssemblingFinishedAndStarted;
     }
   }
   else if (m_Started) {
     xBufferAppend(data, dataLength);
     if (m_PESH.getPacketLength() != 0 && m_DataOffset == m_BufferSize) {
        if (m_HeadLen != NOT_VALID)
          m_DataLen = m_DataOffset - m_HeadLen;
        m_Started = false;
        fwrite(m_Buffer + m_HeadLen, 1, m_DataLen, file);
        return eResult::AssemblingFinished;
     }
     return eResult::AssemblingContinue;
   }
   return eResult::UnexpectedPID;
}
