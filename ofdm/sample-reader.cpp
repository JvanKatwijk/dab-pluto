#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library
 *
 *    DAB library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#
#include	"sample-reader.h"
#include	"device-handler.h"
#include	"dab-processor.h"

	sampleReader::sampleReader (dabProcessor *parent,
	                            deviceHandler *theDevice) {
	theParent		= parent;
	this	-> theDevice	= theDevice;
	sLevel			= 0;
	running. store (true);
}

	sampleReader::~sampleReader (void) {
}

void	sampleReader::reset	(void) {
	sLevel                  = 0;
}

void	sampleReader::setRunning (bool b) {
	running. store (b);
}

float	sampleReader::get_sLevel (void) {
	return sLevel;
}

std::complex<float> sampleReader::getSample () {
std::complex<float> temp;

	if (!running. load ())
	   throw 21;

	while (running. load () &&
	      (theDevice -> Samples () < 2048))
	      usleep (100);

	if (!running. load ())	
	   throw 20;
//
	theDevice -> getSamples (&temp, 1);

	sLevel		= 0.00001 * jan_abs (temp) + (1 - 0.00001) * sLevel;
	return temp;
}

void	sampleReader::getSamples (std::complex<float>  *v, int32_t n) {
int32_t		i;

	while (running. load () &&
	       (theDevice -> Samples () < n))
	   usleep (100);

	if (!running. load ())	
	   throw 20;
//
	n = theDevice -> getSamples (v, n);

	for (i = 0; i < n; i ++) 
	   sLevel	= 0.00001 * jan_abs (v [i]) + (1 - 0.00001) * sLevel;
}

