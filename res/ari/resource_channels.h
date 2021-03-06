/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2012 - 2013, Digium, Inc.
 *
 * David M. Lee, II <dlee@digium.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 *
 * \brief Generated file - declares stubs to be implemented in
 * res/ari/resource_channels.c
 *
 * Channel resources
 *
 * \author David M. Lee, II <dlee@digium.com>
 */

/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !!!!!                               DO NOT EDIT                        !!!!!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * This file is generated by a mustache template. Please see the original
 * template in rest-api-templates/ari_resource.h.mustache
 */

#ifndef _ASTERISK_RESOURCE_CHANNELS_H
#define _ASTERISK_RESOURCE_CHANNELS_H

#include "asterisk/ari.h"

/*! \brief Argument struct for ast_ari_channels_list() */
struct ast_ari_channels_list_args {
};
/*!
 * \brief List all active channels in Asterisk.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_list(struct ast_variable *headers, struct ast_ari_channels_list_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_originate() */
struct ast_ari_channels_originate_args {
	/*! \brief Endpoint to call. */
	const char *endpoint;
	/*! \brief The extension to dial after the endpoint answers */
	const char *extension;
	/*! \brief The context to dial after the endpoint answers. If omitted, uses 'default' */
	const char *context;
	/*! \brief The priority to dial after the endpoint answers. If omitted, uses 1 */
	long priority;
	/*! \brief The application that is subscribed to the originated channel, and passed to the Stasis application. */
	const char *app;
	/*! \brief The application arguments to pass to the Stasis application. */
	const char *app_args;
	/*! \brief CallerID to use when dialing the endpoint or extension. */
	const char *caller_id;
	/*! \brief Timeout (in seconds) before giving up dialing, or -1 for no timeout. */
	int timeout;
	/*! \brief Variables to be set on the channel. */
	struct ast_json *variables;
};
/*!
 * \brief Create a new channel (originate).
 *
 * The new channel is created immediately and a snapshot of it returned. If a Stasis application is provided it will be automatically subscribed to the originated channel for further events and updates.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_originate(struct ast_variable *headers, struct ast_ari_channels_originate_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_get() */
struct ast_ari_channels_get_args {
	/*! \brief Channel's id */
	const char *channel_id;
};
/*!
 * \brief Channel details.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_get(struct ast_variable *headers, struct ast_ari_channels_get_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_hangup() */
struct ast_ari_channels_hangup_args {
	/*! \brief Channel's id */
	const char *channel_id;
	/*! \brief Reason for hanging up the channel */
	const char *reason;
};
/*!
 * \brief Delete (i.e. hangup) a channel.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_hangup(struct ast_variable *headers, struct ast_ari_channels_hangup_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_continue_in_dialplan() */
struct ast_ari_channels_continue_in_dialplan_args {
	/*! \brief Channel's id */
	const char *channel_id;
	/*! \brief The context to continue to. */
	const char *context;
	/*! \brief The extension to continue to. */
	const char *extension;
	/*! \brief The priority to continue to. */
	int priority;
};
/*!
 * \brief Exit application; continue execution in the dialplan.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_continue_in_dialplan(struct ast_variable *headers, struct ast_ari_channels_continue_in_dialplan_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_answer() */
struct ast_ari_channels_answer_args {
	/*! \brief Channel's id */
	const char *channel_id;
};
/*!
 * \brief Answer a channel.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_answer(struct ast_variable *headers, struct ast_ari_channels_answer_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_ring() */
struct ast_ari_channels_ring_args {
	/*! \brief Channel's id */
	const char *channel_id;
};
/*!
 * \brief Indicate ringing to a channel.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_ring(struct ast_variable *headers, struct ast_ari_channels_ring_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_ring_stop() */
struct ast_ari_channels_ring_stop_args {
	/*! \brief Channel's id */
	const char *channel_id;
};
/*!
 * \brief Stop ringing indication on a channel if locally generated.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_ring_stop(struct ast_variable *headers, struct ast_ari_channels_ring_stop_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_send_dtmf() */
struct ast_ari_channels_send_dtmf_args {
	/*! \brief Channel's id */
	const char *channel_id;
	/*! \brief DTMF To send. */
	const char *dtmf;
	/*! \brief Amount of time to wait before DTMF digits (specified in milliseconds) start. */
	int before;
	/*! \brief Amount of time in between DTMF digits (specified in milliseconds). */
	int between;
	/*! \brief Length of each DTMF digit (specified in milliseconds). */
	int duration;
	/*! \brief Amount of time to wait after DTMF digits (specified in milliseconds) end. */
	int after;
};
/*!
 * \brief Send provided DTMF to a given channel.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_send_dtmf(struct ast_variable *headers, struct ast_ari_channels_send_dtmf_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_mute() */
struct ast_ari_channels_mute_args {
	/*! \brief Channel's id */
	const char *channel_id;
	/*! \brief Direction in which to mute audio */
	const char *direction;
};
/*!
 * \brief Mute a channel.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_mute(struct ast_variable *headers, struct ast_ari_channels_mute_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_unmute() */
struct ast_ari_channels_unmute_args {
	/*! \brief Channel's id */
	const char *channel_id;
	/*! \brief Direction in which to unmute audio */
	const char *direction;
};
/*!
 * \brief Unmute a channel.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_unmute(struct ast_variable *headers, struct ast_ari_channels_unmute_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_hold() */
struct ast_ari_channels_hold_args {
	/*! \brief Channel's id */
	const char *channel_id;
};
/*!
 * \brief Hold a channel.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_hold(struct ast_variable *headers, struct ast_ari_channels_hold_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_unhold() */
struct ast_ari_channels_unhold_args {
	/*! \brief Channel's id */
	const char *channel_id;
};
/*!
 * \brief Remove a channel from hold.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_unhold(struct ast_variable *headers, struct ast_ari_channels_unhold_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_start_moh() */
struct ast_ari_channels_start_moh_args {
	/*! \brief Channel's id */
	const char *channel_id;
	/*! \brief Music on hold class to use */
	const char *moh_class;
};
/*!
 * \brief Play music on hold to a channel.
 *
 * Using media operations such as /play on a channel playing MOH in this manner will suspend MOH without resuming automatically. If continuing music on hold is desired, the stasis application must reinitiate music on hold.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_start_moh(struct ast_variable *headers, struct ast_ari_channels_start_moh_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_stop_moh() */
struct ast_ari_channels_stop_moh_args {
	/*! \brief Channel's id */
	const char *channel_id;
};
/*!
 * \brief Stop playing music on hold to a channel.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_stop_moh(struct ast_variable *headers, struct ast_ari_channels_stop_moh_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_start_silence() */
struct ast_ari_channels_start_silence_args {
	/*! \brief Channel's id */
	const char *channel_id;
};
/*!
 * \brief Play silence to a channel.
 *
 * Using media operations such as /play on a channel playing silence in this manner will suspend silence without resuming automatically.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_start_silence(struct ast_variable *headers, struct ast_ari_channels_start_silence_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_stop_silence() */
struct ast_ari_channels_stop_silence_args {
	/*! \brief Channel's id */
	const char *channel_id;
};
/*!
 * \brief Stop playing silence to a channel.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_stop_silence(struct ast_variable *headers, struct ast_ari_channels_stop_silence_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_play() */
struct ast_ari_channels_play_args {
	/*! \brief Channel's id */
	const char *channel_id;
	/*! \brief Media's URI to play. */
	const char *media;
	/*! \brief For sounds, selects language for sound. */
	const char *lang;
	/*! \brief Number of media to skip before playing. */
	int offsetms;
	/*! \brief Number of milliseconds to skip for forward/reverse operations. */
	int skipms;
};
/*!
 * \brief Start playback of media.
 *
 * The media URI may be any of a number of URI's. Currently sound: and recording: URI's are supported. This operation creates a playback resource that can be used to control the playback of media (pause, rewind, fast forward, etc.)
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_play(struct ast_variable *headers, struct ast_ari_channels_play_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_record() */
struct ast_ari_channels_record_args {
	/*! \brief Channel's id */
	const char *channel_id;
	/*! \brief Recording's filename */
	const char *name;
	/*! \brief Format to encode audio in */
	const char *format;
	/*! \brief Maximum duration of the recording, in seconds. 0 for no limit */
	int max_duration_seconds;
	/*! \brief Maximum duration of silence, in seconds. 0 for no limit */
	int max_silence_seconds;
	/*! \brief Action to take if a recording with the same name already exists. */
	const char *if_exists;
	/*! \brief Play beep when recording begins */
	int beep;
	/*! \brief DTMF input to terminate recording */
	const char *terminate_on;
};
/*!
 * \brief Start a recording.
 *
 * Record audio from a channel. Note that this will not capture audio sent to the channel. The bridge itself has a record feature if that's what you want.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_record(struct ast_variable *headers, struct ast_ari_channels_record_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_get_channel_var() */
struct ast_ari_channels_get_channel_var_args {
	/*! \brief Channel's id */
	const char *channel_id;
	/*! \brief The channel variable or function to get */
	const char *variable;
};
/*!
 * \brief Get the value of a channel variable or function.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_get_channel_var(struct ast_variable *headers, struct ast_ari_channels_get_channel_var_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_set_channel_var() */
struct ast_ari_channels_set_channel_var_args {
	/*! \brief Channel's id */
	const char *channel_id;
	/*! \brief The channel variable or function to set */
	const char *variable;
	/*! \brief The value to set the variable to */
	const char *value;
};
/*!
 * \brief Set the value of a channel variable or function.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_set_channel_var(struct ast_variable *headers, struct ast_ari_channels_set_channel_var_args *args, struct ast_ari_response *response);
/*! \brief Argument struct for ast_ari_channels_snoop_channel() */
struct ast_ari_channels_snoop_channel_args {
	/*! \brief Channel's id */
	const char *channel_id;
	/*! \brief Direction of audio to spy on */
	const char *spy;
	/*! \brief Direction of audio to whisper into */
	const char *whisper;
	/*! \brief Application the snooping channel is placed into */
	const char *app;
	/*! \brief The application arguments to pass to the Stasis application */
	const char *app_args;
};
/*!
 * \brief Start snooping.
 *
 * Snoop (spy/whisper) on a specific channel.
 *
 * \param headers HTTP headers
 * \param args Swagger parameters
 * \param[out] response HTTP response
 */
void ast_ari_channels_snoop_channel(struct ast_variable *headers, struct ast_ari_channels_snoop_channel_args *args, struct ast_ari_response *response);

#endif /* _ASTERISK_RESOURCE_CHANNELS_H */
