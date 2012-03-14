/*

  ESQ-1 program/bank transmit via MIDI sysex using MacOS X CoreMIDI

  (c) 2012 Jani Halme <jani.halme@gmail.com>

*/
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <CoreMIDI/CoreMIDI.h>

#include "esq_pcb.h"


#define VERSION "0.8"


// globals
int verbosity=0;
int destNum=1, destChan=1, progNum=-1, dontSend=0, printSysex=0;

// sysex data
char *sysex;
unsigned int sysexlen;


// load a program or a complete bank from a sq80toolkit bank file
// and build a sysex message for esq-1
int makeSysex(FILE *f, int progNum)
{
  int i, j, r, bank;
  char pname[7];
  int sq80only=0, waveforms;
  esq_pcb pcb;

  // calculate the number of programs in the bankfile
  // and select either single program or all program dump
  fseek(f, 0, SEEK_END);
  i=ftell(f);
  j=(i-23)/sizeof(esq_pcb);
  bank=(j==40 && progNum==-1);

  // sysex header and end byte
  sysexlen=6+2*(bank ? j : 1)*sizeof(esq_pcb);
  sysex=malloc(sysexlen);
  sysex[0]=0xf0;
  sysex[1]=0x0f;
  sysex[2]=0x02;
  sysex[3]=destChan-1;
  sysex[4]=(bank ? 0x02 : 0x01);
  sysex[sysexlen-1]=0xf7;

  // dump program(s) to sysex
  if (!bank) {
    fseek(f, 23+(progNum-1)*sizeof(esq_pcb), SEEK_SET);
    i=progNum-1;
  } else {
    fseek(f, 23, SEEK_SET);
    i=0;
  }
  pname[6]=0;
  while ((r=fread(&pcb, sizeof(esq_pcb), 1, f))) {
    // check if it's compatible with esq-1
    waveforms=(pcb.osc1.waveform & pcb.osc2.waveform & pcb.osc3.waveform) & 0xe0;
    sq80only|=waveforms;

    // print
    memcpy(pname, pcb.name, 6*sizeof(char));
    if (verbosity > 0)
      fprintf(stderr, "%03d: %s  [%s]\n", i+1, (char*)&pname, (waveforms ? "SQ-80 only" : "ESQ-1 | SQ-80"));
    
    // copy pcb into sysex
    for(j=0;j<sizeof(esq_pcb);j++) {
      sysex[5+2*i*sizeof(esq_pcb)+j*2]=((char*)(&pcb))[j]&0x0f;
      sysex[5+2*i*sizeof(esq_pcb)+j*2+1]=(((char*)(&pcb))[j]&0xf0)>>4;
    }
    i++;
    if (!bank) break;
  }
  if (verbosity >=0 && sq80only) fprintf(stderr, "Warning: SQ-80 program data is not fully compatible with ESQ-1\n");

  return sysexlen;
}



char *getMIDIname(MIDIObjectRef midiobj)
{
  OSStatus ret;
  CFStringRef nref=NULL;
  char *cstring;
  int r;
  
  ret=MIDIObjectGetStringProperty(midiobj, kMIDIPropertyDisplayName, &nref);
  if (ret != noErr) return NULL;
  cstring=malloc(1024*sizeof(char)); // will be released when the program ends
  r=CFStringGetCString (nref, cstring, 1024*sizeof(char), kCFStringEncodingISOLatin1);
  if (!r) { free(cstring); return NULL; }
  return cstring;
}


void print_usage(void)
{
  int i;
  ItemCount numoutputs;
  MIDIEndpointRef epref;

  printf("esqbank %s - ESQ-1 program/bank MIDI transmit tool for MacOS X\n(c) 2012 Jani Halme <jani.halme@gmail.com>\n\n", VERSION);
  printf("Usage: esqbank [-lovqh] [-m destination] [-c channel] [-p program] <bankfile>\n\n");
  printf("Options:\n\t-l\t\tList the programs in the bank only\n\t-o\t\tOutput sysex to stdout instead of sending via MIDI\n\t-m destination\tMIDI destination index (see list below)\n\t-c channel\tMIDI channel number (1 to 16)\n\t-p program\tSend a single program from the bank (number 1 to 40)\n\t-v\t\tIncrease verbosity\n\t-q\t\tQuiet mode - print only errors\n\t-h\t\tDisplay this help message and list destinations\n\n");
  printf("Destinations:\n");
  numoutputs=MIDIGetNumberOfDestinations();
  for(i=0; i<numoutputs; i++) {
    epref=MIDIGetDestination(i);
    if (epref) printf("\t%d: %s\n", i, getMIDIname(epref));
  }
}


int main(int argc, char *argv[])
{

  OSStatus ret;
  ItemCount numoutputs;
  MIDIEndpointRef midiDest;
  MIDIClientRef midiClient;
  MIDIPortRef midiPort;
  MIDISysexSendRequest sysexReq;
  CFStringRef clientName=NULL, portName=NULL;
  static const char *options="m:c:p:lovqh";
  struct timespec rgtp;  
  int opt;
  FILE *f;

  // needs at least one argument
  if (argc < 2) {
    print_usage();
    return EXIT_FAILURE;
  }

  // read options
  opt=getopt( argc, argv, options);
  while(opt!=-1) {
    switch(opt) {
      case 'm':
        sscanf(optarg, "%d", &destNum);
        numoutputs=MIDIGetNumberOfDestinations();        
        if (destNum < 0 || destNum > numoutputs) {
          fprintf(stderr, "Invalid MIDI destination\n");
          return EXIT_FAILURE;
        }
        break;
      case 'c':
        sscanf(optarg, "%d", &destChan);
        if (destChan < 1 || destChan > 16) {
          fprintf(stderr, "Invalid MIDI channel number\n");
          return EXIT_FAILURE;
        }
        break;
      case 'p':
        sscanf(optarg, "%d", &progNum);
        if (progNum < 1 || progNum > 40) {
          fprintf(stderr, "Invalid program number\n");
          return EXIT_FAILURE;
        }
        break;

      case 'v':
        verbosity++;
        break;
      case 'q':
        verbosity=-999;
        break;
        
      case 'l':
        dontSend=1;
        verbosity=1;
        progNum=-1;
        break;
        
      case 'o':
        verbosity=-1;
        printSysex=1;
        break;
        
      case 'h':
      case '?':
      default:
        print_usage();
        return EXIT_FAILURE;
        break;
    }
    opt=getopt(argc, argv, options);
    if (dontSend) break;
  }
  if (printSysex) dontSend=1;

  // open bankfile
  if (argv[argc-1][0]=='-') {
    fprintf(stderr, "Bank file not specified on command line\n");
    return EXIT_FAILURE;
  }
  f=fopen(argv[argc-1], "r");
  if (!f) {
    fprintf(stderr, "Cannot open bank file!\n");
    return EXIT_FAILURE;
  }

  if (!dontSend) {

    // get MIDI destination object
    midiDest=MIDIGetDestination(destNum);
    if (!midiDest) {
        fprintf(stderr, "Cannot aquire MIDI destination object\n");
        return EXIT_FAILURE;
    }

    // get MIDI client  
    clientName=CFStringCreateWithCString(NULL, "ESQ-1 sysex", kCFStringEncodingISOLatin1);
    if (!clientName) {
      fprintf(stderr, "Cannot create MIDI client name\n");
      return EXIT_FAILURE;
    }
    ret=MIDIClientCreate(clientName, NULL, NULL, &midiClient);
    if (ret!=noErr) {
      fprintf(stderr, "Cannot create MIDI client\n");
      return EXIT_FAILURE;
    }

    // get MIDI output port
    portName=CFStringCreateWithCString(NULL, "Output", kCFStringEncodingISOLatin1);
    if (!portName) {
      fprintf(stderr, "Cannot create MIDI port name\n");
      return EXIT_FAILURE;
    }
    ret=MIDIOutputPortCreate(midiClient, portName, &midiPort);
    if (ret!=noErr) {
      fprintf(stderr, "Cannot create MIDI port\n");
      return EXIT_FAILURE;
    }
  }

  // read bank, make sysex
  makeSysex(f, progNum);
  sysexReq.destination=midiDest;
  sysexReq.data=(Byte*)sysex;
  sysexReq.bytesToSend=sysexlen;
  sysexReq.complete=FALSE;
  sysexReq.completionProc=NULL;
  sysexReq.completionRefCon=NULL;

  // send sysex
  if (!dontSend) {
    if (verbosity >=0 )
      fprintf(stdout, "Sending sysex via '%s' on channel %d... [  0%%]", getMIDIname(midiDest), destChan);
    ret=MIDISendSysex(&sysexReq);
    if (ret!=noErr) {
      fprintf(stderr, "Sysex transfer initiation failed\n");
      return EXIT_FAILURE;
    }
    while(!sysexReq.complete) {
      if (verbosity >= 0) {
        float compl=(float)(100*(sysexlen-sysexReq.bytesToSend))/(float)(sysexlen);
        fprintf(stdout, "\b\b\b\b\b%3.f%%]", compl);
        fflush(stdout);
      }
      rgtp.tv_sec=0;
      rgtp.tv_nsec=100000000L;
      nanosleep(&rgtp, NULL);
    }
    if (verbosity >= 0)
      fprintf(stdout, "\b\b\b\b\b100%%]\n");
  } else if (printSysex) {
    fwrite(sysex, sizeof(char), sysexlen, stdout);
  }

  // done! clean up.
  free(sysex);
  if (!dontSend) {
    MIDIPortDispose(midiPort);
    MIDIClientDispose(midiClient);
    MIDIEndpointDispose(midiDest);
  }

  return 0;  
}
