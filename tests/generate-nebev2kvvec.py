#!/usr/bin/env python

import sys, os

gperf_file = False
nebev2kvvec_file = False
kvvec2nebev_file = False

# event_structs should be parsed from headers eventually
event_structs = {
	'process': [],
	'timed_event': [
		'int:event_type',
		'int:recurring',
		'time_t:run_time',
	],
	'log': [
		'time_t:entry_time',
		'int:data_type',
		'str:data',
	],
	'system_command': [
		'timeval:start_time',
		'timeval:end_time',
		'int:timeout',
		'str:command_line',
		'int:early_timeout',
		'double:execution_time',
		'int:return_code',
		'str:output',
	],
	'event_handler': [
		'int:eventhandler_type',
		'str:host_name',
		'str:service_description',
		'int:state_type',
		'int:state',
		'int:timeout',
		'str:command_name',
		'str:command_args',
		'str:command_line',
		'timeval:start_time',
		'timeval:end_time',
		'int:early_timeout',
		'double:execution_time',
		'int:return_code',
		'str:output',
	],
	'host_check': [
		'str:host_name',
		'int:current_attempt',
		'int:check_type',
		'int:max_attempts',
		'int:state_type',
		'int:state',
		'int:timeout',
		'str:command_name',
		'str:command_args',
		'str:command_line',
		'timeval:start_time',
		'timeval:end_time',
		'int:early_timeout',
		'double:execution_time',
		'double:latency',
		'int:return_code',
		'str:output',
		'str:long_output',
		'str:perf_data',
	],
	'service_check': [
		'str:host_name',
		'str:service_description',
		'int:check_type',
		'int:current_attempt',
		'int:max_attempts',
		'int:state_type',
		'int:state',
		'int:timeout',
		'str:command_name',
		'str:command_args',
		'str:command_line',
		'timeval:start_time',
		'timeval:end_time',
		'int:early_timeout',
		'double:execution_time',
		'double:latency',
		'int:return_code',
		'str:output',
		'str:long_output',
		'str:perf_data',
	],
	'comment': [
		'int:comment_type',
		'str:host_name',
		'str:service_description',
		'time_t:entry_time',
		'str:author_name',
		'str:comment_data',
		'int:persistent',
		'int:source',
		'int:entry_type',
		'int:expires',
		'time_t:expire_time',
		'ulong:comment_id',
	],
	'downtime': [
		'int:downtime_type',
		'str:host_name',
		'str:service_description',
		'time_t:entry_time',
		'str:author_name',
		'str:comment_data',
		'time_t:start_time',
		'time_t:end_time',
		'int:fixed',
		'ulong:duration',
		'ulong:triggered_by',
		'ulong:downtime_id',
	],
	'flapping': [
		'int:flapping_type',
		'str:host_name',
		'str:service_description',
		'double:percent_change',
		'double:high_threshold',
		'double:low_threshold',
		'ulong:comment_id',
	],
	'program_status': [
		'time_t:program_start',
		'int:pid',
		'int:daemon_mode',
		'time_t:last_log_rotation',
		'int:notifications_enabled',
		'int:active_service_checks_enabled',
		'int:passive_service_checks_enabled',
		'int:event_handlers_enabled',
		'int:flap_detection_enabled',
		'int:process_performance_data',
		'int:obsess_over_hosts',
		'int:obsess_over_services',
		'ulong:modified_host_attributes',
		'ulong:modified_service_attributes',
		'str:global_host_event_handler',
		'str:global_service_event_handler',
	],
	'host_status': [],
	'service_status': [],
	'contact_status': [],
	'notification': [
		'int:notification_type',
		'timeval:start_time',
		'timeval:end_time',
		'str:host_name',
		'str:service_description',
		'int:reason_type',
		'int:state',
		'str:output',
		'str:ack_author',
		'int:escalated',
		'int:contacts_notified',
	],
	'contact_notification': [
		'int:notification_type',
		'timeval:start_time',
		'timeval:end_time',
		'str:host_name',
		'str:service_description',
		'str:contact_name',
		'int:reason_type',
		'int:state',
		'str:output',
		'str:ack_author',
		'str:ack_data',
		'int:escalated',
	],
	'contact_notification_method': [
		'int:notification_type',
		'timeval:start_time',
		'timeval:end_time',
		'str:host_name',
		'str:service_description',
		'str:contact_name',
		'int:reason_type',
		'str:command_name',
		'str:command_args',
		'int:state',
		'str:output',
		'str:ack_author',
		'str:ack_data',
		'int:escalated',
	],
	'adaptive_program': [
		'int:command_type',
		'ulong:modified_host_attribute',
		'ulong:modified_host_attributes',
		'ulong:modified_service_attribute',
		'ulong:modified_service_attributes',
	],
	'adaptive_host': [
		'int:command_type',
		'ulong:modified_attribute',
		'ulong:modified_attributes',
	],
	'adaptive_service': [
		'int:command_type',
		'ulong:modified_attribute',
		'ulong:modified_attributes',
	],
	'adaptive_contact': [
		'int:command_type',
		'ulong:modified_attribute',
		'ulong:modified_attributes',
		'ulong:modified_host_attribute',
		'ulong:modified_host_attributes',
		'ulong:modified_service_attribute',
		'ulong:modified_service_attributes',
	],
	'external_command': [
		'int:command_type',
		'time_t:entry_time',
		'str:command_string',
		'str:command_args',
	],
	'aggregated_status': [],
	'retention': [],
	'acknowledgement': [
		'int:acknowledgement_type',
		'str:host_name',
		'str:service_description',
		'int:state',
		'str:author_name',
		'str:comment_data',
		'int:is_sticky',
		'int:persistent_comment',
		'int:notify_contacts',
	],
	'statechange': [
		'int:statechange_type',
		'str:host_name',
		'str:service_description',
		'int:state',
		'int:state_type',
		'int:current_attempt',
		'int:max_attempts',
		'str:output',
	],
}

foo = """
obj_structs = {
	'host': [
	char    *name;
	char    *display_name;
	char	*alias;
	char    *address;
	struct rbtree *parent_hosts;
	struct rbtree *child_hosts;
	struct servicesmember *services;
	char    *check_command;
	int     initial_state;
	double  check_interval;
	double  retry_interval;
	int     max_attempts;
	char    *event_handler;
	struct contactgroupsmember *contact_groups;
	struct contactsmember *contacts;
	double  notification_interval;
	double  first_notification_delay;
	unsigned int notification_options;
	unsigned int hourly_value;
	char	*notification_period;
	char    *check_period;
	int     flap_detection_enabled;
	double  low_flap_threshold;
	double  high_flap_threshold;
	int     flap_detection_options;
	unsigned int stalking_options;
	int     check_freshness;
	int     freshness_threshold;
	int     process_performance_data;
	int     checks_enabled;
	const char *check_source;
	int     accept_passive_checks;
	int     event_handler_enabled;
	int     retain_status_information;
	int     retain_nonstatus_information;
	int     obsess;
	char    *notes;
	char    *notes_url;
	char    *action_url;
	char    *icon_image;
	char    *icon_image_alt;
	char    *statusmap_image; /* used by lots of graphing tools */
	char    *vrml_image;
	int     have_2d_coords;
	int     x_2d;
	int     y_2d;
	int     have_3d_coords;
	double  x_3d;
	double  y_3d;
	double  z_3d;
	customvariablesmember *custom_variables;
	int     problem_has_been_acknowledged;
	int     acknowledgement_type;
	int     check_type;
	int     current_state;
	int     last_state;
	int     last_hard_state;
	char	*plugin_output;
	char    *long_plugin_output;
	char    *perf_data;
	int     state_type;
	int     current_attempt;
	unsigned long current_event_id;
	unsigned long last_event_id;
	unsigned long current_problem_id;
	unsigned long last_problem_id;
	double  latency;
	double  execution_time;
	int     is_executing;
	int     check_options;
	int     notifications_enabled;
	time_t  last_notification;
	time_t  next_notification;
	time_t  next_check;
	time_t  last_check;
	time_t	last_state_change;
	time_t	last_hard_state_change;
	time_t  last_time_up;
	time_t  last_time_down;
	time_t  last_time_unreachable;
	int     has_been_checked;
	int     is_being_freshened;
	int     notified_on;
	int     current_notification_number;
	int     no_more_notifications;
	unsigned long current_notification_id;
	int     check_flapping_recovery_notification;
	int     scheduled_downtime_depth;
	int     pending_flex_downtime;
	int     state_history[MAX_STATE_HISTORY_ENTRIES];    /* flap detection */
	int     state_history_index;
	time_t  last_state_history_update;
	int     is_flapping;
	unsigned long flapping_comment_id;
	double  percent_state_change;
	int     total_services;
	unsigned long modified_attributes;
	struct command *event_handler_ptr;
	struct command *check_command_ptr;
	struct timeperiod *check_period_ptr;
	struct timeperiod *notification_period_ptr;
	struct objectlist *hostgroups_ptr;
	/* objects we depend upon */
	struct objectlist *exec_deps, *notify_deps;
	struct objectlist *escalation_list;
	struct  host *next;
	struct timed_event *next_check_event;
};"""


outfile = {}

def add_common_elements(structs, elements):
	for cb_type in structs:
		entries = structs[cb_type]
		for elem in elements:
			entries.insert(0, elem)

def mk_nebev2kvvec(structs):
	hdr_ent_buf = "/* GENERATED FILE! DO NOT EDIT! */\n"
	complete_ent_buf = """/* GENERATED FILE! DO NOT EDIT! */
#include <stdio.h>
#include <naemon/naemon.h>
"""
	hdr_buf = """
#include <naemon/naemon.h>
"""

	for cb_type in structs:
		entries = structs[cb_type]
		struct_type = 'nebstruct_' + cb_type + '_data'
		decl = "void %s_to_kvvec(struct kvvec *kvv, void *data)" % cb_type
		hdr_buf += "%s;\n" % decl
		ent_buf = """
%s\n{
	nebstruct_%s_data *ds;
	char str[32];

	ds = (nebstruct_%s_data *)data;

""" % (decl, cb_type, cb_type)
		indent = 1
		for ent in entries:
			(etype, key) = ent.split(':')
			var_code = ""

			if etype == 'int':
				var_code += '%ssnprintf(str, sizeof(str), "%%d", ds->%s);\n' % ("\t" * indent, key)
				var_code += '%skvvec_addkv_str(kvv, strdup("%s"), strdup(str));\n' % ("\t" * indent, key)
			elif etype == 'timeval':
				var_code += '%ssnprintf(str, sizeof(str), "%%lu.%%.06lu", ds->%s.tv_sec, ds->%s.tv_usec);' % ("\t" * indent, key, key)
				var_code += '%skvvec_addkv_str(kvv, strdup("%s"), strdup(str));\n' % ("\t" * indent, key)
			elif etype == 'str':
				var_code += '%sif (ds->%s)\n' % ("\t" * indent, key)
				var_code += '%skvvec_addkv_str(kvv, strdup("%s"), strdup(ds->%s));\n' % ("\t" * (indent + 1), key, key)
			elif etype == 'time_t' or etype == 'ulong':
				var_code += '%ssnprintf(str, sizeof(str), "%%lu", ds->%s);\n' % ("\t" * indent, key)
				var_code += '%skvvec_addkv_str(kvv, strdup("%s"), strdup(str));\n' % ("\t" * indent, key)
			elif etype == 'double':
				var_code += '%ssnprintf(str, sizeof(str), "%%f", ds->%s);\n' % ("\t" * indent, key)
				var_code += '%skvvec_addkv_str(kvv, strdup("%s"), strdup(str));\n' % ("\t" * indent, key)
			else:
				print("Unrecognized entry type: %s" % etype)
				sys.exit(1)
			ent_buf += var_code
		ent_buf += "}\n"
		complete_ent_buf += ent_buf

	outfile[nebev2kvvec_file + ".c"] = complete_ent_buf
	outfile[nebev2kvvec_file + ".h"] = hdr_buf


def mk_kvvec2nebev(structs):
	hdr_ent_buf = "/* GENERATED FILE! DO NOT EDIT! */\n"
	complete_ent_buf = """/* GENERATED FILE! DO NOT EDIT! */
#include <stdio.h>
#include <naemon/naemon.h>
#include \"nebev-col2key.c\"
"""
	hdr_buf = """
#include <naemon/naemon.h>
"""

	for cb_type in structs:
		entries = structs[cb_type]
		struct_type = 'nebstruct_' + cb_type + '_data'
		decl = "int kvvec_to_%s(struct kvvec *kvv, void *data)" % cb_type
		hdr_buf += "%s;\n" % decl
		ent_buf = """
%s\n{
	nebstruct_%s_data *ds;
	int i;

	ds = (nebstruct_%s_data *)data;
	for (i = 0; i < kvv->kv_pairs; i++) {
		struct key_value *kv = &kvv->kv[i];
		const struct nebev_column_code *keycode;
		char *endptr;

		keycode = nebev_col_key(kv->key, kv->key_len);
		if (!keycode) {
			printf(\"%%s is not a valid nebevent column\\n\", kv->key);
			return 1 + i;
		}

		switch (keycode->code) {
""" % (decl, cb_type, cb_type)
		indent = 3
		for ent in entries:
			(etype, key) = ent.split(':')
			var_code = ""
			var_code += "%scase NEBEV_COL_%s:\n%s" % ("\t" * (indent - 1), key, "\t" * indent)

			if etype == 'int':
				var_code += 'ds->%s = atoi(kv->value);\n' % key
			elif etype == 'timeval':
				var_code += 'ds->%s.tv_sec = strtoul(kv->value, &endptr, 10);\n' % key
				var_code += '%sds->%s.tv_usec = strtoul(endptr + 1, NULL, 10);\n' % ("\t" * indent, key)
			elif etype == 'str':
				var_code += 'ds->%s = kv->value;\n' % key
			elif etype == 'time_t' or etype == 'ulong':
				var_code += 'ds->%s = strtoul(kv->value, &endptr, 10);\n' % key
			elif etype == 'double':
				var_code += 'ds->%s = strtod(kv->value, &endptr);\n' % key
			else:
				print("Unrecognized column name: %s" % key)
				sys.exit(1)
			var_code += "%sbreak;\n" % ("\t" * indent)
			ent_buf += var_code

		ent_buf += "%sdefault:\n%sprintf(\"Unrecognized key for this event type '%%s'\\n\", kv->key);\n" % ("\t" * (indent - 1), "\t" * indent)
		ent_buf += "%sreturn -1;\n" % ("\t" * indent)
		ent_buf += "%s}\n%s}\n" % ("\t" * (indent - 1), "\t" * (indent - 2))
		ent_buf += "\n\treturn 0;\n}\n"
		complete_ent_buf += ent_buf

	outfile[kvvec2nebev_file + ".c"] = complete_ent_buf
	outfile[kvvec2nebev_file + ".h"] = hdr_buf


def mk_gperf_input(prefix, columns, hardcoded={}):
	"""
	Create gperf input from a list of words. We sort the list and
	make things explicitly numbered so we have a chance to maintain
	backwards compatibility when things change.
	"""
	global gperf_file
	columns.sort()
	buf = ""
	ent_list = []
	ent_enum = []
	i = 0
	skip = {}

	# hardcoded overrides generated/parsed
	for (k, v) in hardcoded.items():
		skip[v] = True

	for ent in columns:
		if skip.get(ent, False):
			continue
		i += 1
		hc_ent = hardcoded.get(i, False)
		if hc_ent:
			ent_enum.append("\t%s%s = %d" % (prefix, hc_ent, i))
			ent_list.append("%s, %s%s" % (hc_ent, prefix, hc_ent))
			continue

		ent_enum.append("\t%s%s = %d" % (prefix, ent, i))
		ent_list.append("%s, %s%s" % (ent, prefix, ent))

	buf = """%%{
/* GENERATED FILE! DO NOT EDIT! */
#include <string.h> /* for strcmp */
enum {
%s
};
%%}
struct nebev_column_code {
	const char *name;
	int code;
};
%%%%
%s
""" % (',\n'.join(ent_enum), ',\n'.join(ent_list))
	outfile[gperf_file] = buf


def get_columns(structs):
	"""Fetch a unique list of variable names from a list of structures"""
	ents = {}
	for cb_type in structs:
		entries = structs[cb_type]
		for ent in entries:
			(etype, key) = ent.split(':')
			ents[key] = True
	return ents.keys()


for arg in sys.argv[1:]:
	if arg.startswith('--gperf='):
		gperf_file = arg.split('=')[1]
	elif arg.startswith('--nebev2kvvec='):
		nebev2kvvec_file = arg.split('=')[1]
	elif arg.startswith('--kvvec2nebev='):
		kvvec2nebev_file = arg.split('=')[1]
	else:
		print("Usage: %s --gperf=<filename> --kvvec=<filename>")
		print("  --gperf default is col2key, producing col2key.gperf")
		print("  --kvvec default is nebev2key, producing nebev2key.[ch]")

if gperf_file == False:
	gperf_file = os.path.dirname(sys.argv[0]) + "/nebev-col2key.gperf"

if nebev2kvvec_file == False:
	nebev2kvvec_file = os.path.dirname(sys.argv[0]) + "/nebev2kvvec"

if kvvec2nebev_file == False:
	kvvec2nebev_file = os.path.dirname(sys.argv[0]) + "/kvvec2nebev"

if not gperf_file.endswith('.gperf'):
	gperf_file = gperf_file + '.gperf'

add_common_elements(event_structs, ['timeval:timestamp', 'int:attr', 'int:flags', 'int:type'])
columns = get_columns(event_structs)
mk_gperf_input("NEBEV_COL_", columns)
mk_nebev2kvvec(event_structs)
mk_kvvec2nebev(event_structs)

for (f, data) in outfile.items():
	fd = os.open(f, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0644)
	os.write(fd, data)