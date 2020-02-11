/***************************************************************************
                          tracksplit.h  -  description
                             -------------------
    begin                : Mon Aug 27 2001
    copyright            : (C) 2001 by Jack Cox
    email                : jack@thecoxes.org

        $Log: tracksplit.h,v $
        Revision 1.2  2001/08/28 01:20:42  jack_cox
        Added comments and a bug fix for when a source file ended in the middle of a
        track.
        
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

typedef unsigned char  _u8;
typedef unsigned short _u16;
typedef unsigned int   _u32;
typedef unsigned long  _u64;
typedef _u8           *_u8p;
typedef _u16          *_u16p;
typedef _u32          *_u32p;
typedef _u64          *_u64p;

typedef char           _i8;
typedef short          _i16;
typedef int            _i32;
typedef long           _i64;
typedef _i8           *_i8p;
typedef _i16          *_i16p;
typedef _i32          *_i32p;
typedef _i64          *_i64p;

#define _NE(a) (sizeof(a) / sizeof *(a))



#define PR05_(fmt, a...) printf(fmt, ##a)              // sel1: activate debug to dmesg
#define TP05 PR05_("%s %d\n", __func__, __LINE__);
#define GV05(a) PR05_("val: %s == 0x%lx\n", #a, (_u64)(a));





#define RIFFCHUNK 0x46464952 /* RIFF */
#define FMTCHUNK 0x20746d66 /* fmt  */
#define DATACHUNK 0x61746164 /* data */
#define LISTCHUNK 0x5453494C /* LIST */

#define TRUE    1
#define FALSE   0

typedef _u32 ID;

typedef struct {
        char *filename;
        FILE *f;
        _i32 RIFFOffset;
        _i32 dataOffset;
        _i32 fmtOffset;
        _i32 bytesWritten;
} WAVFile;

typedef struct
{
    _u32 id;
    _u32 chunkSize;
} chunkHeader;

typedef struct
{
    chunkHeader header;

    _i16 formatTag;
    _u16 channels;
    _u32 samplesPerSec;
    _u32 avgBytesPerSec;
    _u16 blockAlign;
    _u16 bitsPerSample;
} fmtChunk;

typedef struct
{
    chunkHeader header;
    ID id2;
} RIFFChunk;

typedef struct {
        chunkHeader header;
        ID id2;
} LISTChunk;

typedef struct {
        FILE *f;
        _i32 dataStart; /* in bytes */
        _i32 dataEnd; /* in bytes */
        _i32 frameEnd; /* in Frames */
        _i32 curFrame;

        _i16 frameSize; /* in bytes */
        _i16 sampleSize; /* in bits */
        _i16 channels;

        _i32 samplesPerSec;
        _i32 minTrackSamples; /* in frames */
        _i32 gapLengthSamples; /* in frames */
        _i32 avgBytesPerSec;

} InputDesc;



