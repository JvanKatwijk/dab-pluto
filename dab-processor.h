#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of dab-new
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
 *    along with dab-2; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#
#ifndef	__DAB_PROCESSOR__
#define	__DAB_PROCESSOR__
/*
 *	dabProcessor is the embodying of all functionality related
 *	to the actal DAB processing.
 */
#include	"dab-constants.h"
#include	<vector>
#include	"stdint.h"
#include	"phasereference.h"
#include	"ofdm-decoder.h"
#include	"fic-handler.h"
#include	"msc-handler.h"
#include	"dab-api.h"
//

#define	DUMPSIZE	4096
class	RadioInterface;
class	dabParams;

#define	START		0000
#define	INIT		0001
#define	LOOKING_FOR_DIP	0002
#define	DIP_FOUND	0003
#define	END_OF_DIP	0004
#define	GO_FOR_BLOCK_0	0005
#define	SYNC_COMPLETE	0006
#define	BLOCK_READING	0007
#define	END_OF_FRAME	0010
#define	PREPARE_FOR_SKIP_NULL_PERIOD	0011
#define	SKIP_NULL_PERIOD		0012
#define	TO_NEXT_FRAME	0013

//
//	return values for addSymbol
#define	GO_ON			0
//#define	INITIAL_STRENGTH	1
#define	DEVICE_UPDATE		2

class dabProcessor {
public:
		dabProcessor    (syncsignal_t,
                                 systemdata_t,
                                 ensemblename_t,
                                 programname_t,
                                 fib_quality_t,
                                 audioOut_t,
                                 dataOut_t,
                                 programdata_t,
                                 programQuality_t,
                                 motdata_t,
                                 void   *);
        virtual ~dabProcessor   (void);

	int		addSymbol	(std::complex<float> *, int, float *);
	void		reset		(void);
	void		stop		(void);
	void		start		();
//
	uint8_t         kindofService           (std::string);
        void            dataforAudioService     (std::string,   audiodata *);
        void            dataforAudioService     (std::string,
                                                     audiodata *, int16_t);
        void            set_audioChannel        (audiodata *);
        std::string     get_ensembleName        (void);

private:
	syncsignal_t   syncsignalHandler;
	systemdata_t   systemdataHandler;
	programdata_t	programdataHandler;
	void		*userData;
	dabParams	params;
	phaseReference	phaseSynchronizer;
	ofdmDecoder	my_ofdmDecoder;
	ficHandler	my_ficHandler;
	mscHandler	my_mscHandler;
	int32_t         localCounter;
	int32_t         bufferSize;
//	int		frameCount;
//	int		nrFrames;
//	int		sampleCounter;
//	int		totalSamples;
	int16_t		attempts;
	int32_t		T_null;
	int32_t		T_u;
	int32_t		T_s;
	int32_t		T_g;
	int32_t		T_F;
	int32_t		nrBlocks;
	int32_t		carriers;
	int32_t		carrierDiff;
	int16_t		fineOffset;
	int32_t		coarseOffset;
	int32_t		totalOffset;
	int32_t		sampleCount;
	int32_t		nullCount;
	uint8_t		processorMode;
	bool		correctionNeeded;
	bool		isSynced;
	std::vector<std::complex<float>	>ofdmBuffer;
	std::vector<float>dataBuffer;
	int		bufferP;
	int		ofdmBufferIndex;
	float		avgSignalValue;
	float		avgLocalValue;
	int		counter;
	float		dipValue;
	int		dipCnt;

	std::complex<float>		FreqCorr;
	int		ofdmSymbolCount;
	std::vector<int16_t>		ibits;
};
#endif

