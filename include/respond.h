/*
** mouse:~ppr/src/include/respond.h
** Copyright 1995--2005, Trinity College Computing Center.
** Written by David Chappell.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
** 
** * Redistributions of source code must retain the above copyright notice,
** this list of conditions and the following disclaimer.
** 
** * Redistributions in binary form must reproduce the above copyright
** notice, this list of conditions and the following disclaimer in the
** documentation and/or other materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE 
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
** POSSIBILITY OF SUCH DAMAGE.
**
** Last modified 23 March 2005.
*/

/* The upper two bits specify the type of the message. */
#define RESP_TYPE_MASK 0xC0
#define RESP_TYPE_JOB_FATE 0				/** response is job fate */
#define RESP_TYPE_COMMENTARY 64				/** message is printer commentary */

/* RESP_TYPE_JOB_FATE messages */
#define RESP_FINISHED 0						/** job printed normally */
#define RESP_ARRESTED 1						/** job placed on hold due to error */
#define RESP_CANCELED 2						/** canceled while not printing */
#define RESP_CANCELED_PRINTING 3			/** canceled while printing */
#define RESP_CANCELED_BADDEST 4				/** canceled because bad destination */
#define RESP_CANCELED_REJECTING 5			/** destination is in reject mode */
#define RESP_CANCELED_NOCHARGEACCT 6		/** user no charge account */
#define RESP_CANCELED_BADAUTH 7				/** incorrect authcode */
#define RESP_CANCELED_OVERDRAWN 8			/** overdraft too large */
#define RESP_STRANDED_PRINTER_INCAPABLE 9	/** exceeds printer capabilities */
#define RESP_STRANDED_GROUP_INCAPABLE 10	/** exceeds capabilities of each member */
#define RESP_CANCELED_NONCONFORMING 11		/** DSC not good enough */
#define RESP_NOFILTER 12					/** suitable filter not available */
#define RESP_FATAL 13						/** Unspecified fatal ppr error */
#define RESP_NOSPOOLER 14					/** pprd not running */
#define RESP_BADMEDIA 16					/** unrecognized media */
#define RESP_BADPJLLANG 17					/** PJL header specifies unknown language */
#define RESP_FATAL_SYNTAX 18				/** ppr command invoked with bad syntax */
#define RESP_CANCELED_NOPAGES 19			/** can't honour page list */
#define RESP_CANCELED_ACL 20				/** ACL forbids queue access */

/*
** RESP_TYPE_COMMENTARY messages
**
** Lower bits (defined by the COM_* macros will indicate the specific type of
** the commentary message.  Every commentary message will be in one and only 
** one of the four categories shown below.
*/
#define COM_PRINTER_ERROR 1		/** commentary message in style of hrPrinterDetectedErrorState */
#define COM_PRINTER_STATUS 2	/** commentary message in style of hrDeviceStatus and hrPrinterStatus */
#define COM_STALL 4				/** commentary message along lines of "printing bogged down" */
#define COM_EXIT 8				/** commentary message when pprdrv exits */

/* end of file */
