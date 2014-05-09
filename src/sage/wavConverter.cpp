/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module   :  wavConverter.cpp
 *   Author   :   Hyejung Hur
 *   Description:   Code file for
 *
 * Copyright (C) 2004 Electronic Visualization Laboratory,
 * University of Illinois at Chicago
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the distribution.
 *  * Neither the name of the University of Illinois at Chicago nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Direct questions, comments etc about SAGE to bijeong@evl.uic.edu or
 * http://www.evl.uic.edu/cavern/forum/
 *
 ***************************************************************************************************************************/

#include "wavConverter.h"

wavConverter::wavConverter()
{
}


wavConverter::~wavConverter()
{
}

int wavConverter::readHeader(int& nchan, long& samrate, int& nframes, sageSampleFmt& fmt)
{
  if(fileID == NULL) return -1;
  blockCount =0;
  //dataPos = -1;

  char id[4];
  long size;      //32 bit value to hold file size

  // 16 bit values : short
  // 32 bit values : long
  bool done = false;

  fread(id, sizeof(char), 4, fileID); //read in first four bytes
  /** strange facts:: id has strange string more... so......i also did strangely....*/
  //std::cout << "-------------------check name : " << id << " sizechar : " << sizeof(char) << " sizelong : " <<  sizeof(long) << " sizeshort: " << sizeof(short) << std::endl;
  if ((id[0] =='R') && (id[1] =='I') && (id[2] =='F') && (id[3] =='F')) { // RIFF
    //we had 'RIFF' let's continue
    //fread(&size,sizeof(long) , 1, fileID); //read in 32bit size value
    fread(&size, 4, 1, fileID); //read in 32bit size value
    //std::cout << "-------------------check size : " << size << std::endl;
    fread(&id, sizeof(char), 4, fileID); //read in 4 byte string now
    //std::cout << "-------------------check name : " << id << std::endl;
    if ((id[0] == 'W') && (id[1] == 'A') && (id[2] == 'V') && (id[3] == 'E')) { // WAVE
      //this is probably a wave file since it contained "WAVE"
      fread(id, sizeof(char), 4, fileID); //read in 4 bytes "fmt ";
      if ((id[0] == 'f') && (id[1] == 'm') && (id[2] == 't'))  { // fmt
        //fread(&formatLength, sizeof(long),1,fileID);
        fread(&formatLength, 4,1,fileID);

        fread(&formatTag, sizeof(short), 1, fileID); //check mmreg.h (i think?) for other
        // possible format tags like ADPCM
        fread(&channels, sizeof(short),1,fileID); //1 mono, 2 stereo

        //fread(&sampleRate, sizeof(long), 1, fileID); //like 44100, 22050, etc...
        //fread(&avgBytesSec, sizeof(long), 1, fileID);
        fread(&sampleRate, 4, 1, fileID); //like 44100, 22050, etc...
        fread(&avgBytesSec, 4, 1, fileID);

        fread(&blockAlign, sizeof(short), 1, fileID);
        fread(&bitsPerSample, sizeof(short), 1, fileID); //8 bit or 16 bit file?
        fread(id, sizeof(char), 4, fileID); //read in 'data'
        if((id[0] == 'd') && (id[1] == 'a') && (id[2] == 't') && (id[3] == 'a'))  { // data
          //fread(&dataSize, sizeof(long), 1, fileID); //how many bytes of sound data we have
          fread(&dataSize, 4, 1, fileID); //how many bytes of sound data we have
          done = true;
        }
        fgetpos(fileID, &dataPos);
      }
    }
    else {
      std::cerr << "Error: RIFF file but not a wave file" << std::endl;
    }
  }
  else {
    std::cerr << "Error: not a RIFF file" << std::endl;
  }

  if(done) {
    std::cout << "formatLength: " << formatLength << std::endl;
    std::cout << "formatTag: " << formatTag << std::endl;

    std::cout << "channels: " <<  channels << std::endl;
    std::cout << "sampleRate: " <<  sampleRate << std::endl;
    std::cout << "avgBytesSec: " <<  avgBytesSec << std::endl;
    std::cout << "blockAlign: " <<  blockAlign << std::endl;
    std::cout << "bitsPerSample: " <<  bitsPerSample << std::endl;
    std::cout << "dataSize: " << dataSize << std::endl;

    // temporaly
    framesPerBuffer = nframes;

    blockSize =  channels * framesPerBuffer;

    if(bitsPerSample == 8) {
      sampleFmt = SAGE_SAMPLE_INT8;
      totalBlocks = dataSize / blockSize;
    } else if(bitsPerSample == 16) {
      sampleFmt = SAGE_SAMPLE_INT16;
      totalBlocks = dataSize / (blockSize * 2);
    } // about 32 bit:: check IEEE FLOAT
    std::cout << "totalBlocks: " << totalBlocks << std::endl;

  }

  nchan = channels;
  samrate = sampleRate;
  fmt =sampleFmt;
  //nframes = framesPerBuffer;

  return totalBlocks;
}

int wavConverter::writeHeader(int nchan, long samrate, int nframes, int bits)
{
  if(fileID == NULL) return -1;
  blockCount =0;
#ifdef WIN32
  dataPos = -1;
#endif
  writeMode = 1;

  channels = nchan;
  sampleRate = samrate;
  framesPerBuffer = nframes;
  bitsPerSample = bits;

  char headerInfo[255];
  long size;      //32 bit value to hold file size

  // 16 bit values : short
  // 32 bit values : long
  bool done = false;
  formatLength = bitsPerSample;
  formatTag = 1;   // PCM
  blockAlign = (channels * bitsPerSample) / 8 ;
  avgBytesSec = sampleRate * blockAlign;

  fwrite( "RIFF", sizeof(char), 4, fileID );
  fwrite( &size, sizeof(long), 1, fileID );
  fwrite( "WAVEfmt ", sizeof(char), 8, fileID );
  fwrite( &formatLength, sizeof(long), 1,fileID );   // ??
  fwrite( &formatTag, sizeof(short), 1, fileID );   // ??
  fwrite( &channels, sizeof(short),1,fileID );
  fwrite( &sampleRate, sizeof(long), 1, fileID);
  fwrite( &avgBytesSec, sizeof(long), 1, fileID);   // ?
  fwrite( &blockAlign, sizeof(short), 1, fileID);   // ?
  fwrite( &bitsPerSample, sizeof(short), 1, fileID);   // ??
  fwrite( "data", sizeof(char), 4, fileID);
  // later...
  //fwrite( dataSize, sizeof(long), 1, fileID); //how many bytes of sound data we have
  dataSize =0;
  fgetpos(fileID, &dataPos);

  blockSize =  channels * framesPerBuffer;

  std::cout << "formatLength: " << formatLength << std::endl;
  std::cout << "formatTag: " << formatTag << std::endl;

  std::cout << "channels: " <<  channels << std::endl;
  std::cout << "sampleRate: " <<  sampleRate << std::endl;
  std::cout << "avgBytesSec: " <<  avgBytesSec << std::endl;
  std::cout << "blockAlign: " <<  blockAlign << std::endl;
  std::cout << "bitsPerSample: " <<  bitsPerSample << std::endl;
  std::cout << "dataSize: " << dataSize << std::endl;
  std::cout << "blockSize: " << blockSize << std::endl;


  if(bitsPerSample == 8) {
    sampleFmt = SAGE_SAMPLE_INT8;
  } else if(bitsPerSample == 16) {
    sampleFmt = SAGE_SAMPLE_INT16;
  } // about 32 bit:: check IEEE FLOAT

  return 0;

}


int wavConverter::readFrames(void* frames)
{
  if(blockCount >= dataSize) return -1;
  int size = blockSize + blockCount;
  if(size > dataSize) {
    size = dataSize - blockCount;
  }
  else {
    size = blockSize;
  }

  size_t result;
  if(sampleFmt == SAGE_SAMPLE_INT8) {
    char* buff = (char*) frames;
    result = fread(buff, sizeof(char), size, fileID); //read in our whole sound data chunk
    for(int i=0; i < blockSize - size; i++)   {
      *buff++ = '0';
    }
    //std::cout << "SAGE_SAMPLE_INT8 test : " << result << std::endl;
  } else if(sampleFmt == SAGE_SAMPLE_INT16) {   // short
    short* buff = (short*) frames;
    result = fread(buff, sizeof(short), size, fileID); //read in our whole sound data chunk
    for(int i=0; i < blockSize - size; i++)   {
      *buff++ = 0;
    }
    //std::cout << "SAGE_SAMPLE_INT16 test : " << result << std::endl;
  } // about 32 bit:: check IEEE FLOAT

  if(result == 0) {
    //std::cout << "readFrames : end of file " << std::endl;
    return -1;
  }

  blockCount += size;
  return 0;

}

int wavConverter::writeFrames(void* frames)
{
  size_t result;
  if(sampleFmt == SAGE_SAMPLE_INT8) {         // char
    char* buff = (char*) frames;
    result = fwrite(buff, sizeof(char), blockSize, fileID); //read in our whole sound data chunk
    //std::cout << "SAGE_SAMPLE_INT8 test : " << result << std::endl;
  } else if(sampleFmt == SAGE_SAMPLE_INT16) {   // short
    short* buff = (short*) frames;
    result = fwrite(buff, sizeof(short), blockSize, fileID); //read in our whole sound data chunk
    //std::cout << "SAGE_SAMPLE_INT16 test : " << result << std::endl;
  } // about 32 bit:: check IEEE FLOAT

  if(result == 0) {
    //std::cout << "writeFrames : could not write " << std::endl;
    return -1;
  }
  dataSize += result;

  return 0;
}

/** Rewind the file pointer to the beginning of the audio stream */
void wavConverter::begin()
{
  if(fileID) {
    rewind(fileID);
#ifdef WIN32
    fseek (fileID, dataPos, SEEK_SET);
#else
    fseek (fileID, 0L, SEEK_SET);
    fsetpos(fileID, &dataPos);
#endif
    blockCount =0;
  }
}

void wavConverter::close()
{
  size_t result;
  if(writeMode == 1) {
    rewind(fileID);
#ifdef WIN32
    fseek (fileID, dataPos, SEEK_SET);
#else
    fseek (fileID, 0L, SEEK_SET);
    fsetpos(fileID, &dataPos);
#endif
    result = fwrite(&dataSize, sizeof(long), 1, fileID); //read in our whole sound data chunk
  }
  std::cout << "dataSize: " << dataSize << std::endl;
}
