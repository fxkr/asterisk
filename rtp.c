/*
 * Asterisk -- A telephony toolkit for Linux.
 *
 * Real-time Protocol Support
 * 
 * Copyright (C) 1999, Mark Spencer
 *
 * Mark Spencer <markster@linux-support.net>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <asterisk/rtp.h>
#include <asterisk/frame.h>
#include <asterisk/logger.h>
#include <asterisk/options.h>
#include <asterisk/channel.h>
#include <asterisk/channel_pvt.h>

#define TYPE_SILENCE	 0x2
#define TYPE_HIGH	 0x0
#define TYPE_LOW	 0x1
#define TYPE_MASK	 0x3

static int dtmftimeout = 300;	/* 300 samples */

// The value of each payload format mapping:
struct rtpPayloadType {
  int isAstFormat; // whether the following code is an AST_FORMAT
  int code;
};

#define MAX_RTP_PT 256

struct ast_rtp {
	int s;
	char resp;
	struct ast_frame f;
	unsigned char rawdata[1024 + AST_FRIENDLY_OFFSET];
	unsigned int ssrc;
	unsigned int lastts;
	unsigned int lastrxts;
	int lasttxformat;
	int lastrxformat;
	int dtmfcount;
	int nat;
	struct sockaddr_in us;
	struct sockaddr_in them;
	struct timeval rxcore;
	struct timeval txcore;
	struct ast_smoother *smoother;
	int *ioid;
	unsigned short seqno;
	struct sched_context *sched;
	struct io_context *io;
	void *data;
	ast_rtp_callback callback;
        struct rtpPayloadType current_RTP_PT[MAX_RTP_PT];
        // a cache for the result of rtp_lookup_code():
        int rtp_lookup_code_cache_isAstFormat;
        int rtp_lookup_code_cache_code;
        int rtp_lookup_code_cache_result;
};

static struct ast_rtp_protocol *protos = NULL;

int ast_rtp_fd(struct ast_rtp *rtp)
{
	return rtp->s;
}

static int g723_len(unsigned char buf)
{
	switch(buf & TYPE_MASK) {
	case TYPE_MASK:
	case TYPE_SILENCE:
		return 4;
		break;
	case TYPE_HIGH:
		return 24;
		break;
	case TYPE_LOW:
		return 20;
		break;
	default:
		ast_log(LOG_WARNING, "Badly encoded frame (%d)\n", buf & TYPE_MASK);
	}
	return -1;
}

static int g723_samples(unsigned char *buf, int maxlen)
{
	int pos = 0;
	int samples = 0;
	int res;
	while(pos < maxlen) {
		res = g723_len(buf[pos]);
		if (res < 0)
			break;
		samples += 240;
		pos += res;
	}
	return samples;
}

void ast_rtp_set_data(struct ast_rtp *rtp, void *data)
{
	rtp->data = data;
}

void ast_rtp_set_callback(struct ast_rtp *rtp, ast_rtp_callback callback)
{
	rtp->callback = callback;
}

void ast_rtp_setnat(struct ast_rtp *rtp, int nat)
{
	rtp->nat = nat;
}

static struct ast_frame *send_dtmf(struct ast_rtp *rtp)
{
	ast_log(LOG_DEBUG, "Sending dtmf: %d (%c)\n", rtp->resp, rtp->resp);
	rtp->f.frametype = AST_FRAME_DTMF;
	rtp->f.subclass = rtp->resp;
	rtp->f.datalen = 0;
	rtp->f.samples = 0;
	rtp->f.mallocd = 0;
	rtp->f.src = "RTP";
	rtp->resp = 0;
	return &rtp->f;
	
}

static struct ast_frame *process_rfc2833(struct ast_rtp *rtp, unsigned char *data, int len)
{
	unsigned int event;
	char resp = 0;
	struct ast_frame *f = NULL;
	event = ntohl(*((unsigned int *)(data)));
	event >>= 24;
#if 0
	printf("Event: %08x (len = %d)\n", event, len);
#endif	
	if (event < 10) {
		resp = '0' + event;
	} else if (event < 11) {
		resp = '*';
	} else if (event < 12) {
		resp = '#';
	} else if (event < 16) {
		resp = 'A' + (event - 12);
	}
	if (rtp->resp && (rtp->resp != resp)) {
		f = send_dtmf(rtp);
	}
	rtp->resp = resp;
	rtp->dtmfcount = dtmftimeout;
	return f;
}

static struct ast_frame *process_rfc3389(struct ast_rtp *rtp, unsigned char *data, int len)
{
	struct ast_frame *f = NULL;
	/* Convert comfort noise into audio with various codecs.  Unfortunately this doesn't
	   totally help us out becuase we don't have an engine to keep it going and we are not
	   guaranteed to have it every 20ms or anything */
#if 0
	printf("RFC3389: %d bytes, format is %d\n", len, rtp->lastrxformat);
#endif	
	ast_log(LOG_NOTICE, "RFC3389 support incomplete.  Turn off on client if possible\n");
	if (!rtp->lastrxformat)
		return 	NULL;
	switch(rtp->lastrxformat) {
	case AST_FORMAT_ULAW:
		rtp->f.frametype = AST_FRAME_VOICE;
		rtp->f.subclass = AST_FORMAT_ULAW;
		rtp->f.datalen = 160;
		rtp->f.samples = 160;
		memset(rtp->f.data, 0x7f, rtp->f.datalen);
		f = &rtp->f;
		break;
	case AST_FORMAT_ALAW:
		rtp->f.frametype = AST_FRAME_VOICE;
		rtp->f.subclass = AST_FORMAT_ALAW;
		rtp->f.datalen = 160;
		rtp->f.samples = 160;
		memset(rtp->f.data, 0x7e, rtp->f.datalen); /* XXX Is this right? XXX */
		f = &rtp->f;
		break;
	case AST_FORMAT_SLINEAR:
		rtp->f.frametype = AST_FRAME_VOICE;
		rtp->f.subclass = AST_FORMAT_SLINEAR;
		rtp->f.datalen = 320;
		rtp->f.samples = 160;
		memset(rtp->f.data, 0x00, rtp->f.datalen);
		f = &rtp->f;
		break;
	default:
		ast_log(LOG_NOTICE, "Don't know how to handle RFC3389 for receive codec %d\n", rtp->lastrxformat);
	}
	return f;
}

static int rtpread(int *id, int fd, short events, void *cbdata)
{
	struct ast_rtp *rtp = cbdata;
	struct ast_frame *f;
	f = ast_rtp_read(rtp);
	if (f) {
		if (rtp->callback)
			rtp->callback(rtp, f, rtp->data);
	}
	return 1;
}

struct ast_frame *ast_rtp_read(struct ast_rtp *rtp)
{
	int res;
	struct sockaddr_in sin;
	int len;
	unsigned int seqno;
	int payloadtype;
	int hdrlen = 12;
	unsigned int timestamp;
	unsigned int *rtpheader;
	static struct ast_frame *f, null_frame = { AST_FRAME_NULL, };
	struct rtpPayloadType rtpPT;
	
	len = sizeof(sin);
	
	res = recvfrom(rtp->s, rtp->rawdata + AST_FRIENDLY_OFFSET, sizeof(rtp->rawdata) - AST_FRIENDLY_OFFSET,
					0, (struct sockaddr *)&sin, &len);


	rtpheader = (unsigned int *)(rtp->rawdata + AST_FRIENDLY_OFFSET);
	if (res < 0) {
		ast_log(LOG_WARNING, "RTP Read error: %s\n", strerror(errno));
		if (errno == EBADF)
			CRASH;
		return &null_frame;
	}
	if (res < hdrlen) {
		ast_log(LOG_WARNING, "RTP Read too short\n");
		return &null_frame;
	}
	if (rtp->nat) {
		/* Send to whoever sent to us */
		if ((rtp->them.sin_addr.s_addr != sin.sin_addr.s_addr) ||
		    (rtp->them.sin_port != sin.sin_port)) {
			memcpy(&rtp->them, &sin, sizeof(rtp->them));
			ast_log(LOG_DEBUG, "RTP NAT: Using address %s:%d\n", inet_ntoa(rtp->them.sin_addr), ntohs(rtp->them.sin_port));
		}
	}
	/* Get fields */
	seqno = ntohl(rtpheader[0]);
	payloadtype = (seqno & 0x7f0000) >> 16;
	seqno &= 0xffff;
	timestamp = ntohl(rtpheader[1]);
#if 0
	printf("Got RTP packet from %s:%d (type %d, seq %d, ts %d, len = %d)\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), payloadtype, seqno, timestamp,res - hdrlen);
#endif	
	rtp->f.frametype = AST_FRAME_VOICE;
	rtpPT = ast_rtp_lookup_pt(rtp, payloadtype);
	if (!rtpPT.isAstFormat) {
	  // This is special in-band data that's not one of our codecs
	  if (rtpPT.code == AST_RTP_DTMF) {
	    /* It's special -- rfc2833 process it */
	    f = process_rfc2833(rtp, rtp->rawdata + AST_FRIENDLY_OFFSET + hdrlen, res - hdrlen);
	    if (f) return f; else return &null_frame;
	  } else if (rtpPT.code == AST_RTP_CN) {
	    /* Comfort Noise */
	    f = process_rfc3389(rtp, rtp->rawdata + AST_FRIENDLY_OFFSET + hdrlen, res - hdrlen);
	    if (f) return f; else return &null_frame;
	  } else {
	    ast_log(LOG_NOTICE, "Unknown RTP codec %d received\n", payloadtype);
	    return &null_frame;
	  }
	}
	rtp->f.subclass = rtpPT.code;
	rtp->lastrxformat = rtp->f.subclass;

	if (!rtp->lastrxts)
		rtp->lastrxts = timestamp;

	if (rtp->dtmfcount) {
#if 0
		printf("dtmfcount was %d\n", rtp->dtmfcount);
#endif		
		rtp->dtmfcount -= (timestamp - rtp->lastrxts);
		if (rtp->dtmfcount < 0)
			rtp->dtmfcount = 0;
#if 0
		if (dtmftimeout != rtp->dtmfcount)
			printf("dtmfcount is %d\n", rtp->dtmfcount);
#endif
	}
	rtp->lastrxts = timestamp;

	/* Send any pending DTMF */
	if (rtp->resp && !rtp->dtmfcount) {
		ast_log(LOG_DEBUG, "Sending pending DTMF\n");
		return send_dtmf(rtp);
	}
	rtp->f.mallocd = 0;
	rtp->f.datalen = res - hdrlen;
	rtp->f.data = rtp->rawdata + hdrlen + AST_FRIENDLY_OFFSET;
	rtp->f.offset = hdrlen + AST_FRIENDLY_OFFSET;
	switch(rtp->f.subclass) {
	case AST_FORMAT_ULAW:
	case AST_FORMAT_ALAW:
		rtp->f.samples = rtp->f.datalen;
		break;
	case AST_FORMAT_SLINEAR:
		rtp->f.samples = rtp->f.datalen / 2;
		break;
	case AST_FORMAT_GSM:
		rtp->f.samples = 160 * (rtp->f.datalen / 33);
		break;
	case AST_FORMAT_ADPCM:
		rtp->f.samples = rtp->f.datalen * 2;
		break;
	case AST_FORMAT_G729A:
		rtp->f.samples = rtp->f.datalen * 8;
		break;
	case AST_FORMAT_G723_1:
		rtp->f.samples = g723_samples(rtp->f.data, rtp->f.datalen);
		break;
	case AST_FORMAT_SPEEX:
	        rtp->f.samples = 160;
		// assumes that the RTP packet contained one Speex frame
		break;
	default:
		ast_log(LOG_NOTICE, "Unable to calculate samples for format %d\n", rtp->f.subclass);
		break;
	}
	rtp->f.src = "RTP";
	return &rtp->f;
}

// The following array defines the MIME type (and subtype) for each
// of our codecs, or RTP-specific data type.
static struct {
  struct rtpPayloadType payloadType;
  char* type;
  char* subtype;
} mimeTypes[] = {
  {{1, AST_FORMAT_G723_1}, "audio", "G723"},
  {{1, AST_FORMAT_GSM}, "audio", "GSM"},
  {{1, AST_FORMAT_ULAW}, "audio", "PCMU"},
  {{1, AST_FORMAT_ALAW}, "audio", "PCMA"},
  {{1, AST_FORMAT_MP3}, "audio", "MPA"},
  {{1, AST_FORMAT_ADPCM}, "audio", "DVI4"},
  {{1, AST_FORMAT_SLINEAR}, "audio", "L16"},
  {{1, AST_FORMAT_LPC10}, "audio", "LPC"},
  {{1, AST_FORMAT_G729A}, "audio", "G729"},
  {{1, AST_FORMAT_SPEEX}, "audio", "SPEEX"},
  {{0, AST_RTP_DTMF}, "audio", "telephone-event"},
  {{0, AST_RTP_CN}, "audio", "CN"},
  {{1, AST_FORMAT_JPEG}, "video", "JPEG"},
  {{1, AST_FORMAT_PNG}, "video", "PNG"},
  {{1, AST_FORMAT_H261}, "video", "H261"},
  {{1, AST_FORMAT_H263}, "video", "H263"},
};

// Static (i.e., well-known) RTP payload types for our "AST_FORMAT..."s:
static struct rtpPayloadType static_RTP_PT[MAX_RTP_PT] = {
  [0] = {1, AST_FORMAT_ULAW},
  [3] = {1, AST_FORMAT_GSM},
  [4] = {1, AST_FORMAT_G723_1},
  [5] = {1, AST_FORMAT_ADPCM}, // 8 kHz
  [6] = {1, AST_FORMAT_ADPCM}, // 16 kHz
  [7] = {1, AST_FORMAT_LPC10},
  [8] = {1, AST_FORMAT_ALAW},
  [10] = {1, AST_FORMAT_SLINEAR}, // 2 channels
  [11] = {1, AST_FORMAT_SLINEAR}, // 1 channel
  [13] = {0, AST_RTP_CN},
  [14] = {1, AST_FORMAT_MP3},
  [16] = {1, AST_FORMAT_ADPCM}, // 11.025 kHz
  [17] = {1, AST_FORMAT_ADPCM}, // 22.050 kHz
  [18] = {1, AST_FORMAT_G729A},
  [26] = {1, AST_FORMAT_JPEG},
  [31] = {1, AST_FORMAT_H261},
  [34] = {1, AST_FORMAT_H263},
  [101] = {0, AST_RTP_DTMF},
};

void ast_rtp_pt_clear(struct ast_rtp* rtp) 
{
  int i;

  for (i = 0; i < MAX_RTP_PT; ++i) {
    rtp->current_RTP_PT[i].isAstFormat = 0;
    rtp->current_RTP_PT[i].code = 0;
  }

  rtp->rtp_lookup_code_cache_isAstFormat = 0;
  rtp->rtp_lookup_code_cache_code = 0;
  rtp->rtp_lookup_code_cache_result = 0;
}

void ast_rtp_pt_default(struct ast_rtp* rtp) 
{
  int i;
  /* Initialize to default payload types */
  for (i = 0; i < MAX_RTP_PT; ++i) {
    rtp->current_RTP_PT[i].isAstFormat = static_RTP_PT[i].isAstFormat;
    rtp->current_RTP_PT[i].code = static_RTP_PT[i].code;
  }

  rtp->rtp_lookup_code_cache_isAstFormat = 0;
  rtp->rtp_lookup_code_cache_code = 0;
  rtp->rtp_lookup_code_cache_result = 0;
}

// Make a note of a RTP payload type that was seen in a SDP "m=" line.
// By default, use the well-known value for this type (although it may
// still be set to a different value by a subsequent "a=rtpmap:" line):
void ast_rtp_set_m_type(struct ast_rtp* rtp, int pt) {
  if (pt < 0 || pt > MAX_RTP_PT) return; // bogus payload type

  if (static_RTP_PT[pt].code != 0) {
    rtp->current_RTP_PT[pt] = static_RTP_PT[pt];
  }
} 

// Make a note of a RTP payload type (with MIME type) that was seen in
// a SDP "a=rtpmap:" line.
void ast_rtp_set_rtpmap_type(struct ast_rtp* rtp, int pt,
			 char* mimeType, char* mimeSubtype) {
  int i;

  if (pt < 0 || pt > MAX_RTP_PT) return; // bogus payload type

  for (i = 0; i < sizeof mimeTypes/sizeof mimeTypes[0]; ++i) {
    if (strcasecmp(mimeSubtype, mimeTypes[i].subtype) == 0 &&
	strcasecmp(mimeType, mimeTypes[i].type) == 0) {
      rtp->current_RTP_PT[pt] = mimeTypes[i].payloadType;
      return;
    }
  }
} 

// Return the union of all of the codecs that were set by rtp_set...() calls
// They're returned as two distinct sets: AST_FORMATs, and AST_RTPs
void ast_rtp_get_current_formats(struct ast_rtp* rtp,
			     int* astFormats, int* nonAstFormats) {
  int pt;

  *astFormats = *nonAstFormats = 0;
  for (pt = 0; pt < MAX_RTP_PT; ++pt) {
    if (rtp->current_RTP_PT[pt].isAstFormat) {
      *astFormats |= rtp->current_RTP_PT[pt].code;
    } else {
      *nonAstFormats |= rtp->current_RTP_PT[pt].code;
    }
  }
}

struct rtpPayloadType ast_rtp_lookup_pt(struct ast_rtp* rtp, int pt) {
  if (pt < 0 || pt > MAX_RTP_PT) {
    struct rtpPayloadType result;
    result.isAstFormat = result.code = 0;
    return result; // bogus payload type
  }
  return rtp->current_RTP_PT[pt];
}

int ast_rtp_lookup_code(struct ast_rtp* rtp, int isAstFormat, int code) {
  int pt;

  if (isAstFormat == rtp->rtp_lookup_code_cache_isAstFormat &&
      code == rtp->rtp_lookup_code_cache_code) {
    // Use our cached mapping, to avoid the overhead of the loop below
    return rtp->rtp_lookup_code_cache_result;
  }

  for (pt = 0; pt < MAX_RTP_PT; ++pt) {
    if (rtp->current_RTP_PT[pt].code == code &&
	rtp->current_RTP_PT[pt].isAstFormat == isAstFormat) {
      rtp->rtp_lookup_code_cache_isAstFormat = isAstFormat;
      rtp->rtp_lookup_code_cache_code = code;
      rtp->rtp_lookup_code_cache_result = pt;
      return pt;
    }
  }
  return -1;
}

char* ast_rtp_lookup_mime_subtype(int isAstFormat, int code) {
  int i;

  for (i = 0; i < sizeof mimeTypes/sizeof mimeTypes[0]; ++i) {
    if (mimeTypes[i].payloadType.code == code &&
	mimeTypes[i].payloadType.isAstFormat == isAstFormat) {
      return mimeTypes[i].subtype;
    }
  }
  return "";
}

struct ast_rtp *ast_rtp_new(struct sched_context *sched, struct io_context *io)
{
	struct ast_rtp *rtp;
	int x;
	int flags;
	rtp = malloc(sizeof(struct ast_rtp));
	if (!rtp)
		return NULL;
	memset(rtp, 0, sizeof(struct ast_rtp));
	rtp->them.sin_family = AF_INET;
	rtp->us.sin_family = AF_INET;
	rtp->s = socket(AF_INET, SOCK_DGRAM, 0);
	rtp->ssrc = rand();
	rtp->seqno = rand() & 0xffff;
	if (rtp->s < 0) {
		free(rtp);
		ast_log(LOG_WARNING, "Unable to allocate socket: %s\n", strerror(errno));
		return NULL;
	}
	flags = fcntl(rtp->s, F_GETFL);
	fcntl(rtp->s, F_SETFL, flags | O_NONBLOCK);
	for (;;) {
		/* Find us a place */
		x = (rand() % (65000-1025)) + 1025;
		/* Must be an even port number by RTP spec */
		x = x & ~1;
		rtp->us.sin_port = htons(x);
		if (!bind(rtp->s, &rtp->us, sizeof(rtp->us)))
			break;
		if (errno != EADDRINUSE) {
			ast_log(LOG_WARNING, "Unexpected bind error: %s\n", strerror(errno));
			close(rtp->s);
			free(rtp);
			return NULL;
		}
	}
	if (io && sched) {
		/* Operate this one in a callback mode */
		rtp->sched = sched;
		rtp->io = io;
		rtp->ioid = ast_io_add(rtp->io, rtp->s, rtpread, AST_IO_IN, rtp);
	}
	ast_rtp_pt_default(rtp);
	return rtp;
}

int ast_rtp_settos(struct ast_rtp *rtp, int tos)
{
	int res;
	if ((res = setsockopt(rtp->s, SOL_IP, IP_TOS, &tos, sizeof(tos)))) 
		ast_log(LOG_WARNING, "Unable to set TOS to %d\n", tos);
	return res;
}

void ast_rtp_set_peer(struct ast_rtp *rtp, struct sockaddr_in *them)
{
	rtp->them.sin_port = them->sin_port;
	rtp->them.sin_addr = them->sin_addr;
}

void ast_rtp_get_peer(struct ast_rtp *rtp, struct sockaddr_in *them)
{
	them->sin_family = AF_INET;
	them->sin_port = rtp->them.sin_port;
	them->sin_addr = rtp->them.sin_addr;
}

void ast_rtp_get_us(struct ast_rtp *rtp, struct sockaddr_in *us)
{
	memcpy(us, &rtp->us, sizeof(rtp->us));
}

void ast_rtp_destroy(struct ast_rtp *rtp)
{
	if (rtp->smoother)
		ast_smoother_free(rtp->smoother);
	if (rtp->ioid)
		ast_io_remove(rtp->io, rtp->ioid);
	if (rtp->s > -1)
		close(rtp->s);
	free(rtp);
}

static unsigned int calc_txstamp(struct ast_rtp *rtp)
{
	struct timeval now;
	unsigned int ms;
	if (!rtp->txcore.tv_sec && !rtp->txcore.tv_usec) {
		gettimeofday(&rtp->txcore, NULL);
	}
	gettimeofday(&now, NULL);
	ms = (now.tv_sec - rtp->txcore.tv_sec) * 1000;
	ms += (now.tv_usec - rtp->txcore.tv_usec) / 1000;
	/* Use what we just got for next time */
	rtp->txcore.tv_sec = now.tv_sec;
	rtp->txcore.tv_usec = now.tv_usec;
	return ms;
}

int ast_rtp_senddigit(struct ast_rtp *rtp, char digit)
{
	unsigned int *rtpheader;
	int hdrlen = 12;
	int res;
	int ms;
	int pred;
	int x;
	char data[256];

	if ((digit <= '9') && (digit >= '0'))
		digit -= '0';
	else if (digit == '*')
		digit = 10;
	else if (digit == '#')
		digit = 11;
	else if ((digit >= 'A') && (digit <= 'D')) 
		digit = digit - 'A' + 12;
	else if ((digit >= 'a') && (digit <= 'd')) 
		digit = digit - 'a' + 12;
	else {
		ast_log(LOG_WARNING, "Don't know how to represent '%c'\n", digit);
		return -1;
	}
	

	/* If we have no peer, return immediately */	
	if (!rtp->them.sin_addr.s_addr)
		return 0;

	ms = calc_txstamp(rtp);
	/* Default prediction */
	pred = rtp->lastts + ms * 8;
	
	/* Get a pointer to the header */
	rtpheader = (unsigned int *)data;
	rtpheader[0] = htonl((2 << 30) | (1 << 23) | (101 << 16) | (rtp->seqno++));
	rtpheader[1] = htonl(rtp->lastts);
	rtpheader[2] = htonl(rtp->ssrc); 
	rtpheader[3] = htonl((digit << 24) | (0xa << 16) | (0));
	for (x=0;x<4;x++) {
		if (rtp->them.sin_port && rtp->them.sin_addr.s_addr) {
			res = sendto(rtp->s, (void *)rtpheader, hdrlen + 4, 0, &rtp->them, sizeof(rtp->them));
			if (res <0) 
				ast_log(LOG_NOTICE, "RTP Transmission error to %s:%d: %s\n", inet_ntoa(rtp->them.sin_addr), ntohs(rtp->them.sin_port), strerror(errno));
	#if 0
		printf("Sent %d bytes of RTP data to %s:%d\n", res, inet_ntoa(rtp->them.sin_addr), ntohs(rtp->them.sin_port));
	#endif		
		}
		if (x ==0) {
			/* Clear marker bit and increment seqno */
			rtpheader[0] = htonl((2 << 30)  | (101 << 16) | (rtp->seqno++));
			/* Make duration 240 */
			rtpheader[3] |= htonl((240));
			/* Set the End bit for the last 3 */
			rtpheader[3] |= htonl((1 << 23));
		}
	}
	return 0;
}

static int ast_rtp_raw_write(struct ast_rtp *rtp, struct ast_frame *f, int codec)
{
	unsigned int *rtpheader;
	int hdrlen = 12;
	int res;
	int ms;
	int pred;

	ms = calc_txstamp(rtp);
	/* Default prediction */
	pred = rtp->lastts + ms * 8;
	
	switch(f->subclass) {
	case AST_FORMAT_ULAW:
	case AST_FORMAT_ALAW:
		/* If we're within +/- 20ms from when where we
		   predict we should be, use that */
		pred = rtp->lastts + f->datalen;
		break;
	case AST_FORMAT_G729A:
		pred = rtp->lastts + f->datalen * 8;
		break;
	case AST_FORMAT_GSM:
		pred = rtp->lastts + (f->datalen * 160 / 33);
		break;
	case AST_FORMAT_G723_1:
		pred = rtp->lastts + g723_samples(f->data, f->datalen);
		break;
	case AST_FORMAT_SPEEX:
	        pred = rtp->lastts + 160;
		// assumes that the RTP packet contains one Speex frame
		break;
	default:
		ast_log(LOG_WARNING, "Not sure about timestamp format for codec format %d\n", f->subclass);
	}

	/* Re-calculate last TS */
	rtp->lastts = rtp->lastts + ms * 8;
	/* If it's close to ou prediction, go for it */
	if (abs(rtp->lastts - pred) < 640)
		rtp->lastts = pred;
	else
		ast_log(LOG_DEBUG, "Difference is %d, ms is %d\n", abs(rtp->lastts - pred), ms);
	/* Get a pointer to the header */
	rtpheader = (unsigned int *)(f->data - hdrlen);
	rtpheader[0] = htonl((2 << 30) | (codec << 16) | (rtp->seqno++));
	rtpheader[1] = htonl(rtp->lastts);
	rtpheader[2] = htonl(rtp->ssrc); 
	if (rtp->them.sin_port && rtp->them.sin_addr.s_addr) {
		res = sendto(rtp->s, (void *)rtpheader, f->datalen + hdrlen, 0, &rtp->them, sizeof(rtp->them));
		if (res <0) 
			ast_log(LOG_NOTICE, "RTP Transmission error to %s:%d: %s\n", inet_ntoa(rtp->them.sin_addr), ntohs(rtp->them.sin_port), strerror(errno));
#if 0
		printf("Sent %d bytes of RTP data to %s:%d\n", res, inet_ntoa(rtp->them.sin_addr), ntohs(rtp->them.sin_port));
#endif		
	}
	return 0;
}

int ast_rtp_write(struct ast_rtp *rtp, struct ast_frame *_f)
{
	struct ast_frame *f;
	int codec;
	int hdrlen = 12;
	

	/* If we have no peer, return immediately */	
	if (!rtp->them.sin_addr.s_addr)
		return 0;
	
	/* Make sure we have enough space for RTP header */
	if (_f->frametype != AST_FRAME_VOICE) {
		ast_log(LOG_WARNING, "RTP can only send voice\n");
		return -1;
	}

	codec = ast_rtp_lookup_code(rtp, 1, _f->subclass);
	if (codec < 0) {
		ast_log(LOG_WARNING, "Don't know how to send format %d packets with RTP\n", _f->subclass);
		return -1;
	}

	if (rtp->lasttxformat !=  _f->subclass) {
		/* New format, reset the smoother */
		ast_log(LOG_DEBUG, "Ooh, format changed from %d to %d\n", rtp->lasttxformat, _f->subclass);
		rtp->lasttxformat = _f->subclass;
		if (rtp->smoother)
			ast_smoother_free(rtp->smoother);
		rtp->smoother = NULL;
	}


	switch(_f->subclass) {
	case AST_FORMAT_ULAW:
	case AST_FORMAT_ALAW:
		if (!rtp->smoother) {
			rtp->smoother = ast_smoother_new(160);
		}
		if (!rtp->smoother) {
			ast_log(LOG_WARNING, "Unable to create smoother :(\n");
			return -1;
		}
		ast_smoother_feed(rtp->smoother, _f);
		
		while((f = ast_smoother_read(rtp->smoother)))
			ast_rtp_raw_write(rtp, f, codec);
		break;
	case AST_FORMAT_G729A:
		if (!rtp->smoother) {
			rtp->smoother = ast_smoother_new(20);
		}
		if (!rtp->smoother) {
			ast_log(LOG_WARNING, "Unable to create g729 smoother :(\n");
			return -1;
		}
		ast_smoother_feed(rtp->smoother, _f);
		
		while((f = ast_smoother_read(rtp->smoother)))
			ast_rtp_raw_write(rtp, f, codec);
		break;
	case AST_FORMAT_GSM:
		if (!rtp->smoother) {
			rtp->smoother = ast_smoother_new(33);
		}
		if (!rtp->smoother) {
			ast_log(LOG_WARNING, "Unable to create GSM smoother :(\n");
			return -1;
		}
		ast_smoother_feed(rtp->smoother, _f);
		while((f = ast_smoother_read(rtp->smoother)))
			ast_rtp_raw_write(rtp, f, codec);
		break;
	default:	
		ast_log(LOG_WARNING, "Not sure about sending format %d packets\n", _f->subclass);
		// fall through to...
	case AST_FORMAT_SPEEX:
	        // Don't buffer outgoing frames; send them one-per-packet:
		if (_f->offset < hdrlen) {
			f = ast_frdup(_f);
		} else {
			f = _f;
		}
		ast_rtp_raw_write(rtp, f, codec);
	}
		
	return 0;
}

void ast_rtp_proto_unregister(struct ast_rtp_protocol *proto)
{
	struct ast_rtp_protocol *cur, *prev;
	cur = protos;
	prev = NULL;
	while(cur) {
		if (cur == proto) {
			if (prev)
				prev->next = proto->next;
			else
				protos = proto->next;
			return;
		}
		prev = cur;
		cur = cur->next;
	}
}

int ast_rtp_proto_register(struct ast_rtp_protocol *proto)
{
	struct ast_rtp_protocol *cur;
	cur = protos;
	while(cur) {
		if (cur->type == proto->type) {
			ast_log(LOG_WARNING, "Tried to register same protocol '%s' twice\n", cur->type);
			return -1;
		}
		cur = cur->next;
	}
	proto->next = protos;
	protos = proto;
	return 0;
}

static struct ast_rtp_protocol *get_proto(struct ast_channel *chan)
{
	struct ast_rtp_protocol *cur;
	cur = protos;
	while(cur) {
		if (cur->type == chan->type) {
			return cur;
		}
		cur = cur->next;
	}
	return NULL;
}

int ast_rtp_bridge(struct ast_channel *c0, struct ast_channel *c1, int flags, struct ast_frame **fo, struct ast_channel **rc)
{
	struct ast_frame *f;
	struct ast_channel *who, *cs[3];
	struct ast_rtp *p0, *p1;
	struct ast_rtp_protocol *pr0, *pr1;
	void *pvt0, *pvt1;
	int to;

	/* XXX Wait a half a second for things to settle up 
			this really should be fixed XXX */
	ast_autoservice_start(c0);
	ast_autoservice_start(c1);
	usleep(500000);
	ast_autoservice_stop(c0);
	ast_autoservice_stop(c1);

	/* if need DTMF, cant native bridge */
	if (flags & (AST_BRIDGE_DTMF_CHANNEL_0 | AST_BRIDGE_DTMF_CHANNEL_1))
		return -2;
	ast_pthread_mutex_lock(&c0->lock);
	ast_pthread_mutex_lock(&c1->lock);
	pr0 = get_proto(c0);
	pr1 = get_proto(c1);
	if (!pr0) {
		ast_log(LOG_WARNING, "Can't find native functions for channel '%s'\n", c0->name);
		ast_pthread_mutex_unlock(&c0->lock);
		ast_pthread_mutex_unlock(&c1->lock);
		return -1;
	}
	if (!pr1) {
		ast_log(LOG_WARNING, "Can't find native functions for channel '%s'\n", c1->name);
		ast_pthread_mutex_unlock(&c0->lock);
		ast_pthread_mutex_unlock(&c1->lock);
		return -1;
	}
	pvt0 = c0->pvt->pvt;
	pvt1 = c1->pvt->pvt;
	p0 = pr0->get_rtp_info(c0);
	p1 = pr1->get_rtp_info(c1);
	if (!p0 || !p1) {
		/* Somebody doesn't want to play... */
		ast_pthread_mutex_unlock(&c0->lock);
		ast_pthread_mutex_unlock(&c1->lock);
		return -2;
	}
	if (pr0->set_rtp_peer(c0, p1)) 
		ast_log(LOG_WARNING, "Channel '%s' failed to talk to '%s'\n", c0->name, c1->name);
	if (pr1->set_rtp_peer(c1, p0)) 
		ast_log(LOG_WARNING, "Channel '%s' failed to talk back to '%s'\n", c1->name, c0->name);
	ast_pthread_mutex_unlock(&c0->lock);
	ast_pthread_mutex_unlock(&c1->lock);
	cs[0] = c0;
	cs[1] = c1;
	cs[2] = NULL;
	for (;;) {
		if ((c0->pvt->pvt != pvt0)  ||
			(c1->pvt->pvt != pvt1) ||
			(c0->masq || c0->masqr || c1->masq || c1->masqr)) {
				ast_log(LOG_DEBUG, "Oooh, something is weird, backing out\n");
				if (c0->pvt->pvt == pvt0) {
					if (pr0->set_rtp_peer(c0, NULL)) 
						ast_log(LOG_WARNING, "Channel '%s' failed to revert\n", c0->name);
				}
				if (c1->pvt->pvt == pvt1) {
					if (pr1->set_rtp_peer(c1, NULL)) 
						ast_log(LOG_WARNING, "Channel '%s' failed to revert back\n", c1->name);
				}
				/* Tell it to try again later */
				return -3;
		}
		to = -1;
		who = ast_waitfor_n(cs, 2, &to);
		if (!who) {
			ast_log(LOG_DEBUG, "Ooh, empty read...\n");
			continue;
		}
		f = ast_read(who);
		if (!f || ((f->frametype == AST_FRAME_DTMF) &&
				   (((who == c0) && (flags & AST_BRIDGE_DTMF_CHANNEL_0)) || 
			       ((who == c1) && (flags & AST_BRIDGE_DTMF_CHANNEL_1))))) {
			*fo = f;
			*rc = who;
			ast_log(LOG_DEBUG, "Oooh, got a %s\n", f ? "digit" : "hangup");
			if ((c0->pvt->pvt == pvt0) && (!c0->_softhangup)) {
				if (pr0->set_rtp_peer(c0, NULL)) 
					ast_log(LOG_WARNING, "Channel '%s' failed to revert\n", c0->name);
			}
			if ((c1->pvt->pvt == pvt1) && (!c1->_softhangup)) {
				if (pr1->set_rtp_peer(c1, NULL)) 
					ast_log(LOG_WARNING, "Channel '%s' failed to revert back\n", c1->name);
			}
			/* That's all we needed */
			return 0;
		} else 
			ast_frfree(f);
		/* Swap priority not that it's a big deal at this point */
		cs[2] = cs[0];
		cs[0] = cs[1];
		cs[1] = cs[2];
		
	}
	return -1;
}
