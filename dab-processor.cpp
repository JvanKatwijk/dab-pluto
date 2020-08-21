#
/*
 *    Copyright (C) 2017 .. 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the dab-2
 *
 *    dab-2 is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dab-2 is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab-2 if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	"dab-processor.h"
#include	"fic-handler.h"
#include	"msc-handler.h"
#include	"dab-params.h"
/**
  *	\brief dabProcessor
  *	The dabProcessor class is the driver of the processing
  *	of the samplestream.
  *	It is the main interface to the main program,
  *	local are classes ofdmDecoder, ficHandler and mschandler.
  */
#define BUFSIZE 64
#define BUFMASK (64 - 1)
#define	N	5


#define	DAB_MODE	1

static  inline
int16_t valueFor (int16_t b) {
int16_t res     = 1;
        while (--b > 0)
           res <<= 1;
        return res;
}


	dabProcessor::dabProcessor	(syncsignal_t	syncsignalHandler,
	                                 systemdata_t	systemdataHandler,
	                                 ensemblename_t	ensemblename_Handler,
	                                 programname_t	programname_Handler,
	                                 fib_quality_t	fibquality_Handler,
	                                 audioOut_t	audioOut,
	                                 dataOut_t	dataOut_handler,
	                                 programdata_t	programData,
	                                 programQuality_t mscQuality,
	                                 motdata_t	motdata_Handler,
	                                 void		*userData):
	                                    params (DAB_MODE),
	                                    phaseSynchronizer (DAB_MODE,
	                                                       DIFF_LENGTH),
	                                    my_ofdmDecoder (DAB_MODE),
	                                    my_ficHandler (DAB_MODE,
	                                                   ensemblename_Handler,
                                                           programname_Handler,
                                                           fibquality_Handler,
	                                                   userData),
	                                    my_mscHandler  (DAB_MODE,
                                                            audioOut,
                                                            dataOut_handler,
                                                            mscQuality,
                                                            motdata_Handler,
                                                            userData) {
	this	-> syncsignalHandler	= syncsignalHandler;
	this	-> systemdataHandler	= systemdataHandler;
	this	-> programdataHandler	= programData;
	this	-> userData		= userData;
	this	-> T_null		= params. get_T_null ();
	this	-> T_s			= params. get_T_s ();
	this	-> T_u			= params. get_T_u ();
	this	-> T_g			= params. get_T_g();
	this	-> T_F			= params. get_T_F ();
	this	-> nrBlocks		= params. get_L ();
	this	-> carriers		= params. get_carriers ();
	this	-> carrierDiff		= params. get_carrierDiff ();
	isSynced			= false;
	processorMode			= START;
	nullCount			= 0;
	ofdmBuffer. resize (2 * T_s);
	ofdmBufferIndex			= 0;
	avgSignalValue			= 0;
	avgLocalValue			= 0;
	counter				= 0;
//	sampleCounter			= 0;
//	nrFrames			= 0;
	dataBuffer. resize (BUFSIZE);
	memset (dataBuffer. data (), 0, BUFSIZE * sizeof (float));
	bufferP				= 0;
	fineOffset			= 0;	
	coarseOffset			= 0;	
	totalOffset			= 0;
	correctionNeeded		= true;

	ibits. resize (2 * carriers);
}

	dabProcessor::~dabProcessor	(void) {
}
//
//	Since in this implementation, the callback of the device
//	is the driving force, i.e. pumping symbols into the system,
//	the basic interpretation is using an explicit state-based
//	approach

int	dabProcessor::addSymbol	(std::complex<float> *buffer,
	                         int count, float *offset) {
int	retValue	= 0;		// default

	for (int i = 0; i < count; i ++) {
	   std::complex<float> symbol = buffer [i];
	   avgSignalValue	= 0.9999 * avgSignalValue +
	                                   0.0001 * jan_abs (symbol);
	   dataBuffer [bufferP] = jan_abs (symbol);
	   avgLocalValue	+= jan_abs (symbol) -
	                           dataBuffer [(bufferP - 50) & BUFMASK];
	   bufferP		= (bufferP + 1) & BUFMASK;

	   switch (processorMode) {
	      default:
	      case START:
	         avgSignalValue		= 0;
	         avgLocalValue		= 0;
	         counter		= 0;
	         dipCnt			= 0;
	         fineOffset		= 0;
	         correctionNeeded	= true;
	         coarseOffset		= 0;
	         totalOffset		= 0;
	         attempts		= 0;
	         memset (dataBuffer. data (), 0, BUFSIZE * sizeof (float));
	         bufferP		= 0;
	         processorMode		= INIT;
	         break;

	      case INIT:
	         if (++counter >= 2 * T_F) {
	            processorMode	= LOOKING_FOR_DIP;
	            counter	= 0;
	         }
	         break;
//
//	After initialization, we start looking for a dip,
//	After recognizing a frame, we pass this and continue
//	at end of dip
	      case LOOKING_FOR_DIP:
	         counter	++;
	         if (avgLocalValue / 50 < avgSignalValue * 0.45) {
	            processorMode	= DIP_FOUND;
	            dipCnt		= 0;
	         }
	         else	
	         if (counter > T_F) {
	            counter	= 0;
	            attempts ++;
	            if (attempts > 5) {
	               syncsignalHandler (false, userData);
	               processorMode	= START;
	            }
	            else {
	               counter		= 0;
	               processorMode	= INIT;
	            }
	         }
	         break;
	         
	      case DIP_FOUND:
	         dipCnt		++;
	         if (avgLocalValue / BUFSIZE > avgSignalValue * 0.8) {
	            processorMode  	= END_OF_DIP;
	            ofdmBufferIndex	= 0;
	         }
	         else 
	         if (dipCnt > T_null + 100) {	// no luck here
	            attempts ++;
	            if (attempts > 5) {
	               syncsignalHandler (false, userData);
	               processorMode       = START;
	            }
	            else {
	               counter		= 0;
	               processorMode       = INIT;
	            }
	         }
	         break;

	      case END_OF_DIP:
	         ofdmBuffer [ofdmBufferIndex ++] = symbol;
	         if (ofdmBufferIndex >= T_u) {
	            int startIndex =
	                 phaseSynchronizer. findIndex (ofdmBuffer. data (), 3);
	            if (startIndex < 0) {		// no sync
	               if (attempts > 5) {
	                  syncsignalHandler (false, userData);
                          processorMode       = START;
	                  break;
                       }
	               else {
	                  processorMode = LOOKING_FOR_DIP;
	                  break;
	               }
	            }
	            attempts	= 0;	// we made it!!!
	            memmove (ofdmBuffer. data (),
	                     &((ofdmBuffer. data ()) [startIndex]),
                           (T_u - startIndex) * sizeof (std::complex<float>));
	            ofdmBufferIndex  = T_u - startIndex;
	            processorMode	= GO_FOR_BLOCK_0;
	            syncsignalHandler (true, userData);
	         }
	         break;

	      case TO_NEXT_FRAME:
	         ofdmBuffer [ofdmBufferIndex ++] = symbol;
	         if (ofdmBufferIndex >= T_u) {
	            int startIndex =
	                phaseSynchronizer. findIndex (ofdmBuffer. data (),
	                                                                 10);
	            if (startIndex < 0) {		// no sync
	               if (attempts > 5) {
	                  syncsignalHandler (false, userData);
                          processorMode       = START;
	                  break;
	               }
	               else {
	                  processorMode = LOOKING_FOR_DIP;
	                  break;
	               }
	            }

	            attempts	= 0;	// we made it!!!
	            memmove (ofdmBuffer. data (),
	                     &((ofdmBuffer. data ()) [startIndex]),
                           (T_u - startIndex) * sizeof (std::complex<float>));
	            ofdmBufferIndex  = T_u - startIndex;
	            processorMode	= GO_FOR_BLOCK_0;
	         }
	         break;

	      case GO_FOR_BLOCK_0:
	         ofdmBuffer [ofdmBufferIndex] = symbol;
	         if (++ofdmBufferIndex < T_u)
	            break;

	         my_ofdmDecoder. processBlock_0 (ofdmBuffer. data ());
	         my_mscHandler.  process_mscBlock (ofdmBuffer. data (), 0);
//      Here we look only at the block_0 when we need a coarse
//      frequency synchronization.
	         correctionNeeded     = !my_ficHandler. syncReached ();
	         if (correctionNeeded) {
	            int correction    =
                        phaseSynchronizer. estimateOffset (ofdmBuffer. data ());
	            if (correction != 100) {
	               coarseOffset   = correction * carrierDiff;
	               totalOffset	+= coarseOffset;
	               if (abs (totalOffset) > Khz (25)) {
	                  totalOffset	= 0;
	                  coarseOffset	= 0;
	               }
	            }
	         }
	         else
	            coarseOffset	= 0;
//
//	and prepare for reading the data blocks
	         FreqCorr		= std::complex<float> (0, 0);
	         ofdmSymbolCount	= 1;
	         ofdmBufferIndex	= 0;
	         processorMode		= BLOCK_READING;
	         break;

	      case BLOCK_READING:
	         ofdmBuffer [ofdmBufferIndex ++] = symbol;
	         if (ofdmBufferIndex < T_s) 
	            break;

	         for (int i = (int)T_u; i < (int)T_s; i ++)
	            FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
	         if (ofdmSymbolCount < 4) {
	            my_ofdmDecoder. decode (ofdmBuffer. data (),
	                                    ofdmSymbolCount, ibits. data ());
	            my_ficHandler. process_ficBlock (ibits, ofdmSymbolCount);
	         }

	         my_mscHandler.
	                   process_mscBlock  (&((ofdmBuffer. data ()) [T_g]),
	                                      ofdmSymbolCount);
	         ofdmBufferIndex	= 0;
	         if (++ofdmSymbolCount >= nrBlocks) {
	            processorMode	= END_OF_FRAME;
	         }
	         break;

	      case END_OF_FRAME:
	         fineOffset = arg (FreqCorr) / M_PI * carrierDiff / 2;

	         if (fineOffset > carrierDiff / 2) {
	            coarseOffset += carrierDiff;
	            fineOffset -= carrierDiff;
	         }
	         else
	         if (fineOffset < -carrierDiff / 2) {
	            coarseOffset -= carrierDiff;
	            fineOffset += carrierDiff;
	         }
//
//	Once here, we are - without even looking - sure
//	that we are in a dip period
	         processorMode	= PREPARE_FOR_SKIP_NULL_PERIOD;
	         break;
//
//	here, we skip the next null period
	      case PREPARE_FOR_SKIP_NULL_PERIOD:
	         nullCount		= 0;
	         ofdmBuffer [nullCount ++] = symbol;
	         processorMode	= SKIP_NULL_PERIOD;
	         break;

	      case SKIP_NULL_PERIOD:
	         ofdmBuffer [nullCount] = symbol;
	         nullCount ++;
	         if (nullCount >= T_null - 1) {
	            processorMode	= TO_NEXT_FRAME;
	            *offset	= coarseOffset + fineOffset;
	            coarseOffset	= 0;
	            fineOffset	= 0;
	            retValue	= 1;
	         }
	         break;
	   }
	}
	return retValue;
}

void	dabProcessor:: reset	(void) {
	processorMode		= START;
	nullCount		= 0;
	correctionNeeded	= true;
	my_ficHandler.  reset ();
}

void	dabProcessor::stop	(void) {
	my_ficHandler.  reset ();
	my_mscHandler. stop ();
}

void	dabProcessor::start	() {
	processorMode		= START;
	my_ficHandler.  reset ();
	my_mscHandler. start ();
}

//
//	to be handled by delegates
uint8_t dabProcessor::kindofService	(std::string s) {
        return my_ficHandler. kindofService (s);
}

void    dabProcessor::dataforAudioService	(std::string s,audiodata *dd) {
        my_ficHandler. dataforAudioService (s, dd, 0);
}

void    dabProcessor::dataforAudioService	(std::string s,
                                                  audiodata *d, int16_t c) {
        my_ficHandler. dataforAudioService (s, d, c);
}

void    dabProcessor::set_audioChannel (audiodata *d) {
        my_mscHandler. set_audioChannel (d);
	programdataHandler (d, userData);
}

