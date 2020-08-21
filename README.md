------------------------------------------------------------------------
experiments with a cmdline DAB decoder for PLUTO
------------------------------------------------------------------------

When experimenting with DAB for Pluto I needed a subset of the dab-cmdline
program. After all, I was not interested in data handling whatsoever,
just the audio services.

The supported samplerates for pluto start slightly higher than the required
2048000 samples/second. So one of the experiments was to compare a
solution with a fractional decimation from 2100000 samples per second
to 2048000 with an integer decimation from 2 * 2048000 to 2048000 samples
per second. The latter obviously required some decent filtering of the 
input. The ad9361 library provides a suitable baseband filtering that
seems to work well.
Since fractional decimation requires interpolation of complex (floating point)
numbers, and integer decimation - provided filtering is OK - only requires
skipping elements, the approach with integer decimation beats the other one,
dramatically.

Of course, the inputrate now doubles. It is 4096000 samples per second, where
each sample is made up of 4 bytes, so around 16 M bytes/second.

A second experiment was to increase the coupling of the device handler,
the one responsible for collecting the samples, with the ofdm relted code.

Note that in Qt-DAB and in dab-cmdline, the ofdm "processor" is built
up running in its own thread, looking for samples in a (kind of) shared
buffer, a buffer maintained in the device handler. Device handler
is completely unaware of who is looking at the data or what happens to
the data. The consequence is that frequency correction is dome -
in software by multiplying the incoming samples with values from an
oscillator (table). Pofiles show that preprocessing the samples
this way is pretty expensive.

In dab-2 (a variant of Qt-DAB) and here another approach is taken: the device handler passes
the samples directly on to the "ofdm processor", the latter now
executing in the same thread as sample collector in the device handler.

Note that in a pluto handler, collecting samples is done in a task executed
in a separate thread, while in e.g. the sdrplay (well, the version 2 library)
this is done in a call back function.

The advantage of the approach is that the delay in collecting a sample
and deciding what the frequency offset is for that and the other samples
is much smaller, and correction can be done by instructing the device
handler to adapt the frequency of the device.

Running the resulting dab-decoder requires less CPU power than the approach
in dab-cmdline (and Qt-DAB). In spite of the fact that some conversion
is to take place on the samplerate of the incoming sample stream, the
dab-pluto decoder runs nicely on an RPI 2.

The software tree contains two versions of the device handler for Pluto.
While the one in the directory "pluto-handler" is the one with the
integer factor 2 decimation, and the one in the directory "pluto-handler-old"
the one with the 2100000 to 2048000 conversion.

The chosen approach has a number of disadvantages as well:

	a. the device driver is not independent anymore, it "knows" it has to pass the incoming samples to a given processor;
	b. the frequency correction is closely coupled to the thread collecting the device. In e.g. the RTLSDR driver it is not allowed to
alter settings from within the callback function;
	c. the code of the ofdm processor is now built up as a huge case statement, a sample is sent to the ofdm processor, and some form of "global" state
has to be maintained to guide the sample to the place where it is processed.

---------------------------------------------------------------------------
Limitations
----------------------------------------------------------------------------

It is tempting to consider building the executable for tunning
on the Pluto itself, after all it contains a Linux system.
However, the CPU on the Pluto is - accoding to the description -
a single core 667 MHz CPU, while the RPI 2, where the software is
running smoothly, has a 4 core CPU with a higher clock rate and
the average load is roughly 50 percent.

-----------------------------------------------------------------------------
Running the program
-----------------------------------------------------------------------------

Operation of the decoder is as in dab-cmdline

	dab-pluto -C 12C -P "NPO Radio 4" -Q -T 6

where the -C option is there to specify the channel;
where the -P option is there to specify (a prefix of) the name of the service;
where the -Q option is there to specify that the agc is on;
where the -T option is there to specify that the program terminates after a
specified number of seconds.

---------------------------------------------------------------------------
Building an executable
---------------------------------------------------------------------------

Assuming the libraries for
	a. FFTW
	b. portaudio
	c. libsamplerate
	d. the device

are installed, building an executable is in two steps

	cd dab-pluto
	mkdir build
	cd build
	cmake .. -DRPI_DEFINED=ON -DPLUTO=ON
	make
	sudo make install

Of course when compiling on an X64 based machine (with compiler support
for SSE instructions), the line with cmake becomes

	cmake .. -DX64_DEFINED=ON -DPLUTO=ON

and in case the executable is being built for the SDRplay, replace "-DPLUTO=ON" by "-DSDRPLAY=ON"

-----------------------------------------------------------------------------
Copyright
-----------------------------------------------------------------------------

	J van Katwijk
	Lazy Chair Computing
	J dot vanKatwijk at gmail dot com



