#include <libpq-fe.h>

PGconn *connectDB(struct discord *client) {
  char connInfo[512];
  struct ccord_szbuf_readonly value = discord_config_get_field(client, (char *[2]){ "postgresql", "connInfo" }, 2);
  snprintf(connInfo, sizeof(connInfo), "%.*s", (int)value.size, value.start);

  PGconn *conn = PQconnectdb(connInfo);

  switch (PQstatus(conn)) {
    case CONNECTION_OK:
    case CONNECTION_MADE:
      log_trace("[libpq] Successfully connected to the postgres database server.");
      break;
    case CONNECTION_STARTED:
      log_trace("[libpq] Waiting for connection to be made.");
      break;
    case CONNECTION_AWAITING_RESPONSE:
      log_trace("[libpq] Waiting for a response from the server.");
      break;
    case CONNECTION_AUTH_OK:
      log_trace("[libpq] Received authentication; waiting for backend start-up to finish.");
      break;
    case CONNECTION_SSL_STARTUP:
      log_trace("[libpq] Almost connecting; negotiating SSL encryption.");
      break;
    case CONNECTION_SETENV:
      log_trace("[libpq] Almost connecting; negotiating environment-driven parameter settings.");
      break;

    default:
      log_fatal("[libpq] Error when trying to connect to the postgres database server. [%s]\n", PQerrorMessage(conn));
      PQfinish(conn);
      return NULL;
  }

  return conn;
}

int _PQresultStatus(PGconn *conn, PGresult *res, char *action, char *msgDone) {
  int resStatus = 0;
  switch (PQresultStatus(res)) {
    case PGRES_EMPTY_QUERY:
      log_fatal("[libpq] Error while %s, the string sent to the postgresql database server was empty.", action);
      PQclear(res);
      PQfinish(conn);
      break;
    case PGRES_TUPLES_OK:
    case PGRES_COMMAND_OK:
      if (msgDone) log_trace("[libpq]: %s", msgDone);
      resStatus = 1;
      break;
    case PGRES_BAD_RESPONSE:
      log_fatal("[libpq] Error while %s, the postgresql database server's response was not understood.", action);
      PQclear(res);
      PQfinish(conn);
      break;
    case PGRES_NONFATAL_ERROR:
      log_warn("[libpq] Warning or a notice while %s");
      resStatus = 1;
      break;
    case PGRES_FATAL_ERROR:
      log_fatal("[libpq] Error while %s.\n%s", action, PQresultErrorMessage(res));
      PQclear(res);
      PQfinish(conn);
      break;

    default:
      log_warn("[SYSTEM] A resultStatus is unknown for the system; setting it as \"no errors\". [PQresultStatus(res) = %d]", PQresultStatus(res));
      break;
  }
  return resStatus;
}