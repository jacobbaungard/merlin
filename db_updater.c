#include "nagios/broker.h"
#include "daemon.h"

#define STATUS_QUERY(type) \
	"UPDATE " type " SET " \
	"instance_id = %d, " \
	"initial_state = %d, flap_detection_enabled = %d, " \
	"low_flap_threshold = %f, high_flap_threshold = %f, " \
	"check_freshness = %d, freshness_threshold = %d, " \
	"process_performance_data = %d, " \
	"active_checks_enabled = %d, passive_checks_enabled = %d, " \
	"event_handler_enabled = %d, " \
	"obsess_over_" type " = %d, problem_has_been_acknowledged = %d, " \
	"acknowledgement_type = %d, check_type = %d, " \
	"current_state = %d, last_state = %d, " /* 17 - 18 */ \
	"last_hard_state = %d, state_type = %d, " \
	"current_attempt = %d, current_event_id = %lu, " \
	"last_event_id = %lu, current_problem_id = %lu, " \
	"last_problem_id = %lu, " \
	"latency = %f, execution_time = %lf, "  /* 26 - 27 */ \
	"notifications_enabled = %d, " \
	"last_notification = %lu, " \
	"next_check = %lu, should_be_scheduled = %d, last_check = %lu, " \
	"last_state_change = %lu, last_hard_state_change = %lu, " \
	"has_been_checked = %d, " \
	"current_notification_number = %d, current_notification_id = %lu, " \
	"check_flapping_recovery_notification = %d, " \
	"scheduled_downtime_depth = %d, pending_flex_downtime = %d, " \
	"is_flapping = %d, flapping_comment_id = %lu, " /* 41 - 42 */ \
	"percent_state_change = %f, " \
	"output = %s, long_output = %s, perf_data = %s"

#define STATUS_ARGS(output, long_output, perf_data) \
	p->state.initial_state, p->state.flap_detection_enabled, \
	p->state.low_flap_threshold, p->state.high_flap_threshold, \
	p->state.check_freshness, p->state.freshness_threshold, \
	p->state.process_performance_data, \
	p->state.checks_enabled, p->state.accept_passive_checks, \
	p->state.event_handler_enabled, \
	p->state.obsess, p->state.problem_has_been_acknowledged, \
	p->state.acknowledgement_type, p->state.check_type, \
	p->state.current_state, p->state.last_state, \
	p->state.last_hard_state, p->state.state_type, \
	p->state.current_attempt, p->state.current_event_id, \
	p->state.last_event_id, p->state.current_problem_id, \
	p->state.last_problem_id, \
	p->state.latency, p->state.execution_time, \
	p->state.notifications_enabled, \
	p->state.last_notification, \
	p->state.next_check, p->state.should_be_scheduled, p->state.last_check, \
	p->state.last_state_change, p->state.last_hard_state_change, \
	p->state.has_been_checked, \
	p->state.current_notification_number, p->state.current_notification_id, \
	p->state.check_flapping_recovery_notification, \
	p->state.scheduled_downtime_depth, p->state.pending_flex_downtime, \
	p->state.is_flapping, p->state.flapping_comment_id, \
	p->state.percent_state_change, \
	safe_str(output), safe_str(long_output), safe_str(perf_data)


static int handle_host_status(merlin_node *node, int cb, const merlin_host_status *p)
{
	char *host_name;
	char *output, *long_output, *perf_data;
	int result, node_id;

	node_id = node == &ipc ? 0 : node->id + 1;

	sql_quote(p->name, &host_name);
	sql_quote(p->state.plugin_output, &output);
	sql_quote(p->state.long_plugin_output, &long_output);
	sql_quote(p->state.perf_data, &perf_data);
	result = sql_query(STATUS_QUERY("host") " WHERE host_name = %s",
					   node_id,
					   STATUS_ARGS(output, long_output, perf_data),
					   host_name);

	/* this check is only done when we have new checkresults */
	if (cb == NEBCALLBACK_HOST_CHECK_DATA) {
		if (host_has_new_state(p->name, p->state.current_state, p->state.state_type)) {
			result = sql_query
				("INSERT INTO %s(timestamp, event_type, host_name, state, "
				 "hard, retry, output) "
				 "VALUES(%lu, %d, %s, %d, %d, %d, %s)",
				 sql_table_name(), p->state.last_check,
				 NEBTYPE_HOSTCHECK_PROCESSED, host_name,
				 p->state.current_state,
				 p->state.state_type == HARD_STATE,
				 p->state.current_attempt, output);
		}

		/*
		 * Stash host performance data separately, in case
		 * people people are using Merlin with Nagiosgrapher or
		 * similar performance data graphing solutions.
		 */
		if (host_perf_table && p->state.perf_data && *p->state.perf_data) {
			char *perfdata;
			sql_quote(p->state.perf_data, &perfdata);
			result = sql_query
				("INSERT INTO %s(timestamp, host_name, perfdata) "
				 "VALUES(%lu, %s, %s)",
				 host_perf_table, p->state.last_check, host_name, perfdata);
			free(perfdata);
		}
	}

	free(host_name);
	safe_free(output);
	safe_free(long_output);
	safe_free(perf_data);
	return result;
}

static int handle_service_status(merlin_node *node, int cb, const merlin_service_status *p)
{
	char *host_name, *service_description;
	char *output, *long_output, *perf_data;
	int result, node_id;

	node_id = node == &ipc ? 0 : node->id + 1;

	sql_quote(p->host_name, &host_name);
	sql_quote(p->service_description, &service_description);
	sql_quote(p->state.plugin_output, &output);
	sql_quote(p->state.long_plugin_output, &long_output);
	sql_quote(p->state.perf_data, &perf_data);
	result = sql_query
		(STATUS_QUERY("service")
		 " WHERE host_name = %s AND service_description = %s",
		 node_id, STATUS_ARGS(output, long_output, perf_data),
		 host_name, service_description);

	if (cb == NEBCALLBACK_SERVICE_CHECK_DATA) {
		if (service_has_new_state(p->host_name, p->service_description, p->state.current_state, p->state.state_type)) {
			result = sql_query
				("INSERT INTO %s(timestamp, event_type, host_name, "
				 "service_description, state, hard, retry, output) "
				 "VALUES(%lu, %d, %s, %s, %d, '%d', '%d', %s)",
				 sql_table_name(), p->state.last_check,
				 NEBTYPE_SERVICECHECK_PROCESSED, host_name,
				 service_description, p->state.current_state,
				 p->state.state_type == HARD_STATE,
				 p->state.current_attempt, output);
		}
		/*
		 * Stash service performance data separately, in case
		 * people people are using Merlin with Nagiosgrapher or
		 * similar performance data graphing solutions.
		 */
		if (service_perf_table && p->state.perf_data && *p->state.perf_data) {
			char *perfdata;
			sql_quote(p->state.perf_data, &perfdata);
			result = sql_query
				("INSERT INTO %s(timestamp, host_name, "
				 "service_description, perfdata) "
				 "VALUES(%lu, %s, %s, %s)",
				 service_perf_table, p->state.last_check,
				 host_name, service_description, perfdata);
			free(perfdata);
		}
	}

	free(host_name);
	free(service_description);
	safe_free(output);
	safe_free(long_output);
	safe_free(perf_data);
	return result;
}

static int rpt_downtime(void *data)
{
	nebstruct_downtime_data *ds = (nebstruct_downtime_data *)data;
	int depth, result;
	char *host_name;

	switch (ds->type) {
	case NEBTYPE_DOWNTIME_START:
	case NEBTYPE_DOWNTIME_STOP:
		break;
	case NEBTYPE_DOWNTIME_DELETE:
		/*
		 * if we're deleting a downtime that hasn't started yet, nothing
		 * should be added to the database. Otherwise, transform it to a
		 * NEBTYPE_DOWNTIME_STOP event to mark the downtime as stopped.
		 */
		if (ds->start_time > time(NULL))
			return 0;
		ds->type = NEBTYPE_DOWNTIME_STOP;
		break;
	default:
		return 0;
	}

	sql_quote(ds->host_name, &host_name);
	if (ds->service_description) {
		char *service_description;

		sql_quote(ds->service_description, &service_description);
		depth = ds->type == NEBTYPE_DOWNTIME_START;
		result = sql_query("INSERT INTO %s"
						   "(timestamp, event_type, host_name,"
						   "service_description, downtime_depth) "
						   "VALUES(%lu, %d, %s, %s, %d)",
						   sql_table_name(),
						   ds->timestamp.tv_sec, ds->type, host_name,
						   service_description, depth);
		free(service_description);
	} else {
		depth = ds->type == NEBTYPE_DOWNTIME_START;
		result = sql_query("INSERT INTO %s"
						   "(timestamp, event_type, host_name, downtime_depth)"
						   "VALUES(%lu, %d, %s, %d)",
						   sql_table_name(),
						   ds->timestamp.tv_sec, ds->type, host_name, depth);
	}
	free(host_name);

	return result;
}

static int rpt_process_data(void *data)
{
	nebstruct_process_data *ds = (nebstruct_process_data *)data;

	switch(ds->type) {
	case NEBTYPE_PROCESS_START:
	case NEBTYPE_PROCESS_SHUTDOWN:
		break;
	case NEBTYPE_PROCESS_RESTART:
		ds->type = NEBTYPE_PROCESS_SHUTDOWN;
		break;
	default:
		return 0;
	}

	return sql_query("INSERT INTO %s(timestamp, event_type) "
					 "VALUES(%lu, %d)",
					 sql_table_name(), ds->timestamp.tv_sec, ds->type);
}

static int handle_program_status(merlin_node *node, const nebstruct_program_status_data *p)
{
	char *global_host_event_handler;
	char *global_service_event_handler;
	int result, node_id;

	sql_quote(p->global_host_event_handler, &global_host_event_handler);
	sql_quote(p->global_service_event_handler, &global_service_event_handler);

	node_id = node == &ipc ? 0 : node->id + 1;
	result = sql_query
		("UPDATE program_status SET is_running = 1, "
		 "last_alive = %lu, program_start = %lu, pid = %d, daemon_mode = %d, "
		 "last_command_check = %lu, last_log_rotation = %lu, "
		 "notifications_enabled = %d, "
		 "active_service_checks_enabled = %d, passive_service_checks_enabled = %d, "
		 "active_host_checks_enabled = %d, passive_host_checks_enabled = %d, "
		 "event_handlers_enabled = %d, flap_detection_enabled = %d, "
		 "failure_prediction_enabled = %d, process_performance_data = %d, "
		 "obsess_over_hosts = %d, obsess_over_services = %d, "
		 "modified_host_attributes = %lu, modified_service_attributes = %lu, "
		 "global_host_event_handler = %s, global_service_event_handler = %s "
		 "WHERE instance_id = %d",
		 time(NULL), p->program_start, p->pid, p->daemon_mode,
		 p->last_command_check, p->last_log_rotation,
		 p->notifications_enabled,
		 p->active_service_checks_enabled, p->passive_service_checks_enabled,
		 p->active_host_checks_enabled, p->passive_host_checks_enabled,
		 p->event_handlers_enabled, p->flap_detection_enabled,
		 p->failure_prediction_enabled, p->process_performance_data,
		 p->obsess_over_hosts, p->obsess_over_services,
		 p->modified_host_attributes, p->modified_service_attributes,
		 safe_str(global_host_event_handler), safe_str(global_service_event_handler),
		 node_id);

	free(global_host_event_handler);
	free(global_service_event_handler);
	return result;
}

static int handle_downtime(const nebstruct_downtime_data *p)
{
	int result = 0;
	char *host_name = NULL, *service_description = NULL;
	char *comment_data = NULL, *author_name = NULL;

	/*
	 * If we stop downtime that's already started, we'll get a
	 * downtime stop event, but no downtime delete event (weird,
	 * but true).
	 * Since we can't retroactively upgrade all Nagios instances
	 * in the world, we have to make sure STOP also means DELETE
	 */
	if (p->type == NEBTYPE_DOWNTIME_DELETE ||
		p->type == NEBTYPE_DOWNTIME_STOP)
	{
		result = sql_query("DELETE FROM scheduled_downtime "
						   "WHERE downtime_id = %lu", p->downtime_id);

		/* NEBTYPE_DOWNTIME_STOP has further actions to take */
		if (p->type == NEBTYPE_DOWNTIME_DELETE)
			return result;
	}

	sql_quote(p->host_name, &host_name);
	sql_quote(p->service_description, &service_description);

	switch (p->type) {
	case NEBTYPE_DOWNTIME_START:
	case NEBTYPE_DOWNTIME_STOP:
		if (!service_description) {
			result = sql_query
				("UPDATE host SET "
				 "scheduled_downtime_depth = scheduled_downtime_depth %c 1 "
				 "WHERE host_name = %s",
				 p->type == NEBTYPE_DOWNTIME_START ? '+' : '-', host_name);
		} else {
			result = sql_query
				("UPDATE service SET "
				 "scheduled_downtime_depth = scheduled_downtime_depth %c 1 "
				 "WHERE host_name = %s AND service_description = %s",
				 p->type == NEBTYPE_DOWNTIME_START ? '+' : '-',
				 host_name, service_description);
		}
		break;
	case NEBTYPE_DOWNTIME_LOAD:
		result = sql_query
			("DELETE FROM scheduled_downtime WHERE downtime_id = %lu",
			 p->downtime_id);
		/* fallthrough */
	case NEBTYPE_DOWNTIME_ADD:
		sql_quote(p->author_name, &author_name);
		sql_quote(p->comment_data, &comment_data);
		result = sql_query
			("INSERT INTO scheduled_downtime "
			 "(downtime_type, host_name, service_description, entry_time, "
			 "author_name, comment_data, start_time, end_time, fixed, "
			 "duration, triggered_by, downtime_id) "
			 "VALUES(%d, %s, %s, %lu, "
			 "       %s, %s, %lu, %lu, %d, "
			 "       %lu, %lu, %lu)",
			 p->downtime_type, host_name, safe_str(service_description),
			 p->entry_time, author_name, comment_data, p->start_time,
			 p->end_time, p->fixed, p->duration, p->triggered_by,
			 p->downtime_id);
		free(author_name);
		free(comment_data);
		break;
	default:
		linfo("Unknown downtime type %d", p->type);
		break;
	}

	free(host_name);
	safe_free(service_description);

	return result;
}

static int handle_flapping(const nebstruct_flapping_data *p)
{
	int result;
	char *host_name, *service_description = NULL;
	unsigned long comment_id;

	sql_quote(p->host_name, &host_name);
	sql_quote(p->service_description, &service_description);

	if (p->type == NEBTYPE_FLAPPING_STOP) {
		sql_query("DELETE FROM comment_tbl WHERE comment_id = '%lu'",
				  p->comment_id);
		comment_id = 0;
	} else {
		comment_id = p->comment_id;
	}

	if (service_description) {
		result = sql_query
			("UPDATE service SET is_flapping = %d, "
			 "flapping_comment_id = %lu, percent_state_change = %f "
			 "WHERE host_name = %s AND service_description = %s",
			 p->type == NEBTYPE_FLAPPING_START,
			 comment_id, p->percent_change,
			 host_name, service_description);
		free(service_description);
	} else {
		result = sql_query
			("UPDATE host SET is_flapping = %d, "
			 "flapping_comment_id = %lu, percent_state_change = %f "
			 "WHERE host_name = %s",
			 p->type == NEBTYPE_FLAPPING_START,
			 comment_id, p->percent_change, host_name);
	}

	free(host_name);

	return result;
}

static int handle_comment(const nebstruct_comment_data *p)
{
	int result;
	char *host_name, *author_name, *comment_data, *service_description;

	if (p->type == NEBTYPE_COMMENT_DELETE) {
		result = sql_query
			("DELETE FROM comment_tbl WHERE comment_id = %lu", p->comment_id);
		return result;
	}

	sql_quote(p->host_name, &host_name);
	sql_quote(p->author_name, &author_name);
	sql_quote(p->comment_data, &comment_data);
	sql_quote(p->service_description, &service_description);

	result = sql_query
		("INSERT INTO comment_tbl(comment_type, host_name, "
		 "service_description, entry_time, author_name, comment_data, "
		 "persistent, source, entry_type, expires, expire_time, "
		 "comment_id) "
		 "VALUES(%d, %s, %s, %lu, %s, %s, %d, %d, %d, %d, %lu, %lu)",
		 p->comment_type, host_name,
		 safe_str(service_description), p->entry_time,
		 author_name, comment_data, p->persistent, p->source,
		 p->entry_type, p->expires, p->expire_time, p->comment_id);

	free(host_name);
	free(author_name);
	free(comment_data);
	safe_free(service_description);

	return result;
}

static int handle_contact_notification(const nebstruct_contact_notification_data *p)
{
	int result;
	char *contact_name, *host_name, *service_description;
	char *output, *ack_author, *ack_data;

	sql_quote(p->contact_name, &contact_name);
	sql_quote(p->host_name, &host_name);
	sql_quote(p->service_description, &service_description);
	sql_quote(p->output, &output);
	sql_quote(p->ack_author, &ack_author);
	sql_quote(p->ack_data, &ack_data);

	result = sql_query
		("INSERT INTO notification "
		 "(notification_type, start_time, end_time, "
		 "contact_name, host_name, service_description, "
		 "reason_type, state, output,"
		 "ack_author, ack_data, escalated) "
		 "VALUES(%d, %lu, %lu, %s, %s,"
		 "%s, %d, %d, %s, %s, %s, %d)",
		 p->notification_type, p->start_time.tv_sec, p->end_time.tv_sec,
		 contact_name, host_name,  safe_str(service_description),
		 p->reason_type, p->state, safe_str(output),
		 safe_str(ack_author), safe_str(ack_data), p->escalated);

	free(host_name);
	free(contact_name);
	safe_free(service_description);
	safe_free(output);
	safe_free(ack_author);
	safe_free(ack_data);

	return result;
}

static int handle_notification(const nebstruct_notification_data *p)
{
	int result;
	char *host_name, *service_description;
	char *output, *ack_author, *ack_data;

	sql_quote(p->host_name, &host_name);
	sql_quote(p->service_description, &service_description);
	sql_quote(p->output, &output);
	sql_quote(p->ack_author, &ack_author);
	sql_quote(p->ack_data, &ack_data);

	result = sql_query
		("INSERT INTO notification "
		 "(notification_type, start_time, end_time, host_name,"
		 "service_description, reason_type, state, output,"
		 "ack_author, ack_data, escalated, contacts_notified) "
		 "VALUES(%d, %lu, %lu, %s,"
		 "%s, %d, %d, %s, %s, %s, %d, %d)",
		 p->notification_type, p->start_time.tv_sec, p->end_time.tv_sec,
		 host_name,  safe_str(service_description), p->reason_type, p->state,
		 safe_str(output), safe_str(ack_author), safe_str(ack_data),
		 p->escalated, p->contacts_notified);

	safe_free(host_name);
	safe_free(service_description);
	safe_free(output);
	safe_free(ack_author);
	safe_free(ack_data);

	return result;
}

static int handle_contact_notification_method(const nebstruct_contact_notification_method_data *p)
{
	int result;
	char *contact_name, *host_name, *service_description;
	char *output, *ack_author, *ack_data, *command_name;

	sql_quote(p->contact_name, &contact_name);
	sql_quote(p->host_name, &host_name);
	sql_quote(p->service_description, &service_description);
	sql_quote(p->output, &output);
	sql_quote(p->ack_author, &ack_author);
	sql_quote(p->ack_data, &ack_data);
	sql_quote(p->command_name, &command_name);

	result = sql_query
		("INSERT INTO notification "
		 "(notification_type, start_time, end_time, "
		 "contact_name, host_name, service_description, "
		 "command_name, reason_type, state, output,"
		 "ack_author, ack_data, escalated) "
		 "VALUES(%d, %lu, %lu, "
		 "%s, %s, %s, "
		 "%s, %d, %d, %s, "
		 "%s, %s, %d)",
		 p->notification_type, p->start_time.tv_sec, p->end_time.tv_sec,
		 contact_name, host_name,  safe_str(service_description),
		 command_name, p->reason_type, p->state, safe_str(output),
		 safe_str(ack_author), safe_str(ack_data), p->escalated);

	free(host_name);
	free(contact_name);
	safe_free(service_description);
	safe_free(output);
	safe_free(ack_author);
	safe_free(ack_data);
	free(command_name);

	return result;
}

int mrm_db_update(merlin_node *node, merlin_event *pkt)
{
	int errors = 0;

	if (!sql_is_connected())
		return 0;

	if (!pkt) {
		lerr("pkt is NULL in mrm_db_update");
		return 0;
	}
	if (!pkt->body) {
		lerr("pkt->body is NULL in mrm_db_update");
		return 0;
	}
	merlin_decode(pkt->body, pkt->hdr.len, pkt->hdr.type);
	if (!pkt->body) {
		lerr("merlin_decode() made pkt->body NULL in mrm_db_update");
		return 0;
	}
	switch (pkt->hdr.type) {
	case NEBCALLBACK_PROGRAM_STATUS_DATA:
		errors = handle_program_status(node, (void *)pkt->body);
		errors |= rpt_process_data(pkt->body);
		break;
	case NEBCALLBACK_COMMENT_DATA:
		errors = handle_comment((void *)pkt->body);
		break;
	case NEBCALLBACK_DOWNTIME_DATA:
		errors = handle_downtime((void *)pkt->body);
		errors |= rpt_downtime((void *)pkt->body);
		break;
	case NEBCALLBACK_FLAPPING_DATA:
		errors = handle_flapping((void *)pkt->body);
		break;
	case NEBCALLBACK_NOTIFICATION_DATA:
		errors = handle_notification((void *)pkt->body);
		break;
	case NEBCALLBACK_CONTACT_NOTIFICATION_DATA:
		errors = handle_contact_notification((void *)pkt->body);
		break;
	case NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA:
		errors = handle_contact_notification_method((void *)pkt->body);
		break;
	case NEBCALLBACK_HOST_CHECK_DATA:
	case NEBCALLBACK_HOST_STATUS_DATA:
		errors = handle_host_status(node, pkt->hdr.type, (void *)pkt->body);
		break;
	case NEBCALLBACK_SERVICE_CHECK_DATA:
	case NEBCALLBACK_SERVICE_STATUS_DATA:
		errors = handle_service_status(node, pkt->hdr.type, (void *)pkt->body);
		break;

		/* some callbacks are unhandled by design */
	case NEBCALLBACK_EXTERNAL_COMMAND_DATA:
		return 0;

	default:
		lerr("Unknown callback type %d. Weird, to say the least...",
			 pkt->hdr.type);
		return -1;
		break;
	}
	sql_free_result();

	return errors;
}
