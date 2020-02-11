/***************************************************************************
                          tracksplit.c  -  description
                             -------------------
    begin                : Mon Aug 27 2001
    copyright            : (C) 2001 by Jack Cox
    email                : jack@thecoxes.org

        $Log: tracksplit.c,v $
        Revision 1.9  2001/09/21 19:41:04  jack_cox
        Fixed bug with parsing of command line options.  Doooh!
        
        Revision 1.8  2001/09/21 19:01:22  jack_cox
        Fixed a bug where an out of place byte in the wav file throws off the parser
        
        Revision 1.7  2001/08/30 03:50:03  jack_cox
        A few more comments and optimizations
        
        Revision 1.6  2001/08/30 03:13:47  jack_cox
        Added $Log: tracksplit.c,v $
        Added Revision 1.9  2001/09/21 19:41:04  jack_cox
        Added Fixed bug with parsing of command line options.  Doooh!
        Added
        Added Revision 1.8  2001/09/21 19:01:22  jack_cox
        Added Fixed a bug where an out of place byte in the wav file throws off the parser
        Added
        Added Revision 1.7  2001/08/30 03:50:03  jack_cox
        Added A few more comments and optimizations
        Added directive
        

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/



#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#define _GNU_SOURCE
#include <getopt.h>

#include "tracksplit.h"

_i32 minTrack = 300;                    /* minimum seconds in a track */
char  *fileMask = "Track%02d.wav";      /* sprintf mask for file name */
_i32 threshold = 500;                   /* low volume indicative of silence */
_i32 gapLength = 3;                     /* seconds of silence indicates gap */
_i32 channelSplit=FALSE;
char *inFilename = NULL;
FILE *inFile = NULL;


InputDesc in;

WAVFile trackFiles[99];
_i32    trackCount = 0;

/************************************************************
*       int getFormat -- Read and parse a format tag from a WAV file.
*
************************************************************/
_i32 getFormat(FILE *f, fmtChunk *fc)
{
    fread(&fc->formatTag, sizeof fc->formatTag, 1, f);

    if (fc->formatTag != 1)
    { /* cannot handle compression */
        fprintf(stderr, "Cannot handle compressed files\n");
        return(1);
    }
    fread(&fc->channels, sizeof fc->channels, 1, f);
    if (fc->channels > 2)
    { /* cannot handle above stereo */
        fprintf(stderr, "Cannot do more than 2 channels\n");
        exit(2);
    }
    fread(&fc->samplesPerSec , sizeof fc->samplesPerSec, 1, f);
    fread(&fc->avgBytesPerSec, sizeof fc->avgBytesPerSec, 1, f);
    fread(&fc->blockAlign, sizeof fc->blockAlign, 1, f);
    fread(&fc->bitsPerSample, sizeof fc->bitsPerSample, 1, f);

        in.f = f;
        in.frameSize = fc->blockAlign;
        in.sampleSize = fc->bitsPerSample;
        in.channels = fc->channels;
        in.samplesPerSec = fc->samplesPerSec;

        in.minTrackSamples = in.samplesPerSec * minTrack;
        in.gapLengthSamples = in.samplesPerSec * gapLength;
        in.avgBytesPerSec = fc->avgBytesPerSec;

    fprintf(stderr, "\tSamples/Sec = %u\n", fc->samplesPerSec);
    fprintf(stderr, "\tBytes/Sec = %u\n", fc->avgBytesPerSec);
    fprintf(stderr, "\tFrame = %u\n", fc->blockAlign);
    fprintf(stderr, "\tBits/Sample = %u\n", fc->bitsPerSample);

    return(0);
}
/***************************************************************
* int closeTrack -- Complete a track file.  Writes the various
*                                               chunk information then closes the file
*
***************************************************************/
_i32 
closeTrack(WAVFile *wf) {
        _i32 rcs = wf->bytesWritten+sizeof(fmtChunk)+8;
#ifdef DEBUG
        fprintf(stderr, "closeTrack: entering, name=%s\n", wf->filename);
#endif

        /* record data chunk size */
        fseek(wf->f, wf->dataOffset, SEEK_SET);
        fwrite(&wf->bytesWritten, sizeof wf->bytesWritten, 1, wf->f);

        /* record RIFF chunk size */
        fseek(wf->f, wf->RIFFOffset, SEEK_SET);
        fwrite(&rcs, sizeof rcs, 1, wf->f);

        fclose(wf->f);
#ifdef DEBUG
        fprintf(stderr, "closeTrack: exiting\n");
#endif

        return(TRUE);
}

/*************************************************************
* int copyData -- copy a chunk of the input wave file to the
*                       output wave file.  Optionally look for quiet spots where
*                       the file should be split.  This is the most time critical
*                       piece of the code.
*
*************************************************************/
_i32     
copyData(InputDesc *in, WAVFile *wf, _i32 framesToCopy, _i16 ScanForGaps) {
        _u8 ucSample[2];
        _i16 ssSample[2];
        void *sample;
        _i32 lastLoud = 0;
        _i32 framesDone = 0;
        _i32 frameSize = in->frameSize;
        _i32 gapLengthSamples = in->gapLengthSamples;
        _i32      mode = 0;

        // make this decision once since it won't change for this run
        if ((in->sampleSize == 16) && (in->channels==1)) {
                mode = 1;
        } else if ((in->sampleSize == 16) && (in->channels==2)) {
                mode = 2;
        }

#ifdef DEBUG
        fprintf(stderr, "copyData: Entring\n");
        fprintf(stderr, "copyData: offset: %d framesToCopy: %d, scan=%s\n",
                in->curFrame, framesToCopy, 
                (ScanForGaps ? "TRUE" : "FALSE"));
#endif

        if (in->sampleSize == 16) 
                sample = (void *) ssSample;
        else
                sample = (void *) ucSample;

        /* do bytesToCopy is done */
        while ((in->curFrame < in->frameEnd) && (framesDone < framesToCopy))
        {

                /* read a sample */
                fread(sample, frameSize, 1, in->f);
                in->curFrame++;

                /* write sample */
                fwrite(sample, frameSize, 1, wf->f);
                if (channelSplit) {
                        fwrite(sample, frameSize, 1, wf->f);
                }


                /* if scanning */
                if (ScanForGaps) {
                        if (mode == 1) {
                                if (abs(ssSample[0])  > threshold) {
                                        lastLoud = framesDone;
                                } else if (lastLoud < ((framesDone) - gapLengthSamples)) {
                                        framesDone++;
                                        break;
                                }
                        } else if (mode == 2) {
                                if ((abs(ssSample[0]) > threshold) || (abs(ssSample[1]) > threshold)) {
                                  lastLoud = framesDone;
                                } else if (lastLoud < ((framesDone) - gapLengthSamples)) {
                                  framesDone++;
                                  break;
                                }
                  }
                }
                framesDone++;
                                        

        } 
        wf->bytesWritten += framesDone * (in->frameSize * (channelSplit ? 2 : 1)) ;

#ifdef DEBUG
        fprintf(stderr, "copyData: Leaving\n");
        fprintf(stderr, "copyData: framesDone: %d framesToCopy: %d\n",
                framesDone, framesToCopy);
#endif 

        return(TRUE);
}
/***********************************************
* newTrack -- create a new track file.  Copy the
*                       first 'minimum-track' length seconds into the
*                       file
*
***********************************************/
_i32 
newTrack(InputDesc *in) {

#ifdef DEBUG
        fprintf(stderr, "newTrack: entering, track=%d\n", trackCount+1);
#endif

        // oops too many tracks for a cd audio
        if (trackCount==99) {
                return(FALSE);
        }
        // make space for the filename
        trackFiles[trackCount].filename = malloc(255);
        trackCount++;
        // make the file name
        sprintf(trackFiles[trackCount-1].filename, fileMask, trackCount);

        // initialize the track file
        trackFiles[trackCount-1].f = fopen(trackFiles[trackCount-1].filename, "wb");
        trackFiles[trackCount-1].RIFFOffset = 0;
        trackFiles[trackCount-1].bytesWritten = 0;
        /* write RIFF */
        {
                RIFFChunk rc;
                rc.header.id = RIFFCHUNK;
                rc.header.chunkSize = 0;
                memcpy(&rc.id2, "WAVE" , 4);
                
                fwrite(&rc, sizeof(rc), 1, trackFiles[trackCount-1].f);
                trackFiles[trackCount-1].RIFFOffset = 4;
        }

        {
                fmtChunk newfc;

                trackFiles[trackCount-1].fmtOffset=ftell(trackFiles[trackCount-1].f);

                newfc.header.id = FMTCHUNK;
                newfc.header.chunkSize = sizeof(fmtChunk)-8;
                newfc.formatTag = 1;
                if (!channelSplit) { // single channel
                        newfc.channels = in->channels;
                        newfc.samplesPerSec = in->samplesPerSec;
                        newfc.avgBytesPerSec = in->avgBytesPerSec;
                        newfc.blockAlign = in->frameSize;
                } else {        // dual channel
                        newfc.channels = 2;
                        newfc.samplesPerSec = in->samplesPerSec;
                        newfc.avgBytesPerSec = in->sampleSize*2*in->samplesPerSec;
                        newfc.blockAlign = in->frameSize*2;
                }
                newfc.bitsPerSample = in->sampleSize;

                fwrite(&newfc, sizeof(newfc), 1, trackFiles[trackCount-1].f);

        }
        /* write fmt tag */
        {
                chunkHeader ch;

                trackFiles[trackCount-1].dataOffset=ftell(trackFiles[trackCount-1].f)+4;

                memcpy(&ch.id, "data", 4);

                ch.chunkSize = 0;
                fwrite(&ch, sizeof(ch), 1, trackFiles[trackCount-1].f);
        }


        /* write data tag */


        copyData(in, &trackFiles[trackCount-1], 
                in->minTrackSamples, FALSE);

#ifdef DEBUG
        fprintf(stderr, "newTrack: exiting, track=%d\n", trackCount);
#endif

        return(TRUE);
}
/**********************************************
* int processData -- work through the file starting
*                       tracks, finding gaps, and making new tracks
*
*
***********************************************/
_i32 
processData(InputDesc *in) 
{

#ifdef DEBUG
        fprintf(stderr, "processData: entering\n");
#endif
  /* start the first track automatically */
        if (newTrack(in)) {
                // start copying and scanning for gaps until you find a gap or reach the end
                while (copyData(in, &trackFiles[trackCount-1], in->frameEnd - in->curFrame , TRUE)) {

#ifdef DEBUG
        fprintf(stderr, "processData: start of while loop\n");
#endif
                        // close the track
                        closeTrack(&trackFiles[trackCount-1]);

                        // if the last frame has been processed then break out
                        if ((in->frameEnd - in->curFrame) == 0) {
                                break;
                        }
                        // if more to go then start a new track
                        if (in->curFrame < in->frameEnd) {
                                if (!newTrack(in)) {
                                        return(FALSE); /* error in newTrack */
                                } 
                                if (in->curFrame >= in->frameEnd) {
                                        closeTrack(&trackFiles[trackCount-1]);
                                        return(TRUE);
                                }
                        } 
                }
        }
                        
}
/*****************************************************************
* int init -- read the arguments
*
*****************************************************************/
_i32
init (_i32 argc, char *argv[]) {
        _i32 c;
        _i32 usage = 0;

    static struct option long_options[] =
    {
        {"minimum-track", 1, 0, 0},
        {"file", 1, 0, 0},
        {"gap-length", 1, 0, 0},
        {"threshold", 1, 0, 0},
        {"channel-split", 0,0,0},
        {"input", 1, 0, 0},
        {"help", 0, 0, 0},
        {0, 0, 0, 0}
     };

        while ((1) && (!usage)) {
                _i32 option_index = 0;

                c = getopt_long(argc, argv, "?hm:f:g:t:ci:", long_options, &option_index);
                if (c == -1) 
                        break;
                switch(c) {
                  case '?':
                  case 'h':
                        usage++;
                        break;
                  case 'm' : /* minimum-track */
                                                minTrack = atoi(optarg);
                                                break;
                                        case 'f' : /* file name */
                                                fileMask = (char *) strdup(optarg);
                                                break;
                                        case 'g' : /* gap-length */
                                                gapLength = atoi(optarg);
                                                break;
                                        case 't' : /* silance threshold */
                                                threshold = atoi(optarg);
                                                break;
                                        case 'c' : /* split */
                                          channelSplit=TRUE;
                                          break;
          case 'i' : /* input */
            inFilename = strdup(optarg);
            break;

                        case 0 :
                                switch(option_index) {
                                        case 0 : /* minimum-track */
                                                minTrack = atoi(optarg);
                                                break;
                                        case 1 : /* file name */
                                                fileMask = (char *) strdup(optarg);
                                                break;
                                        case 2 : /* gap-length */
                                                gapLength = atoi(optarg);
                                                break;
                                        case 3 : /* silance threshold */
                                                threshold = atoi(optarg);
                                                break;
                                        case 4 : /* split */
                                          channelSplit=TRUE;
                                          break;
          case 5 : /* input */
            inFilename = strdup(optarg);
            break;
                                        default :
                                                usage++;
                                                break;
                                }
                                break;
                        default :
                                usage++;
                                break;
                }
        }
        if (usage) { /* oops, user error */
                fprintf(stderr, "usage: sts -m | --minimum-track <seconds>\tThe minimum size of a track.\n");
                fprintf(stderr, "\t\t-f | --file <filename-mask>\tThe file mask to use for output files. Default=TrackXX.wav\n");
                fprintf(stderr, "\t\t-g | --gap-length <seconds>\tThe number of seconds of silence to consider a track break.\n");
                fprintf(stderr, "\t\t-t | --threshold <amplitude>\tThe highest amplitude value that indicates silence.\n");
        fprintf(stderr, "\t\t-i | --input <filename>\tRead from named file rather than stdin\n");
        fprintf(stderr, "\t\t-c | --channel-split\tConvert the output from mono input to stereo output tracks while track splitting\n");
        fprintf(stderr, "\t\t-h | -? | --help\t\tShow this message\n");
                return(FALSE);
        }
        return(TRUE);
}
_i32
main(_i32 argc, char *argv[]) {
    RIFFChunk rc;
    fmtChunk fc;
    chunkHeader ch;
        chunkHeader dc;
    _i32 i;
    _i32 preReadOffset;

TP05
GV05(sizeof(_u64))

    
    if (!init(argc, argv)) {
        return(1);
    }

    if (inFilename != NULL)
    {
        inFile = fopen(inFilename, "rb");
    } else {
        inFile = stdin;
    }
    do
    {
#ifdef DEBUG
    fprintf(stderr, "main: @ offset %X\n", ftell(inFile));
#endif
        preReadOffset = ftell(inFile);
        i = fread((void *)&ch, sizeof(chunkHeader), 1, inFile);
        if (i>0)
        {

            switch (ch.id)
            {
            case (RIFFCHUNK) :
                fprintf(stderr, "Got RIFF chunk, size=%u\n", ch.chunkSize);
                fseek(inFile, 4, SEEK_CUR);
                break;
            case (FMTCHUNK) :
                fprintf(stderr, "Got FMT chunk, size=%u\n", ch.chunkSize);
                memcpy(&fc, &ch, sizeof(chunkHeader));
                if (getFormat(inFile, &fc))
                {
                    exit(1);
                }
                break;
            case (DATACHUNK) :
                fprintf(stderr, "Got DATA chunk, size=%u\n", ch.chunkSize);
                memcpy(&dc, &ch, sizeof(chunkHeader));
                                in.dataStart = ftell(in.f);
                                in.dataEnd = in.dataStart + dc.chunkSize;
                                in.frameEnd = dc.chunkSize / in.frameSize;
                                in.curFrame = 0;

                                /* start processing tracks */
                                if (!processData(&in)) {
                                        exit(2);
                                }
                break;
                        case (LISTCHUNK) :
                                fprintf(stderr, "Got a List Chunk, size=%u\n", ch.chunkSize);
                                {
                                        _u8 junk[ch.chunkSize];
                                        fread(junk, ch.chunkSize, 1, inFile);
                                }
                                break;
            default :
                fprintf(stderr, "Unknown data @ offset %X\n", ftell(inFile)-sizeof(chunkHeader));
                fseek(inFile, preReadOffset+1, SEEK_SET);
                break;
            }
        }

    } while (!feof(inFile));

    exit(0);
} 
