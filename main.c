#include <stdio.h>
#include <string.h>

#include <concord/discord.h>
#include <concord/discord-internal.h>

#include "database.c"

void on_ready(struct discord *client, const struct discord_ready *bot) {
  (void) client;
  log_trace("[DISCORD_GATEWAY] Logged in as %s#%s!", bot->user->username, bot->user->discriminator);
}

void on_messageCreate(struct discord *client, const struct discord_message *message) {
  if (message->author->bot) return;
  if (0 == strcmp(message->content, ".daily")) {
    PGconn *conn = connectDB(client);

    unsigned long long moneyWin = rand() % 20000;

    /* 
      In case the user installed a new Postgresql database, it will re-create the economy table.
        
      Author: @ThePedroo
    */

    PGresult *res = PQexec(conn, "CREATE TABLE IF NOT EXISTS economy(user_id BIGINT UNIQUE NOT NULL, concoins BIGINT NOT NULL);CREATE TABLE IF NOT EXISTS daily(user_id BIGINT UNIQUE NOT NULL, timestamp TEXT NOT NULL);");

    PQclear(res);

    int resultCode = _PQresultStatus(conn, res, "creating economy and daily tables", NULL);
    if (!resultCode) return;

    char date[32];
    struct tm local;
    time_t now = time(NULL);
    localtime_r(&now, &local);

    snprintf(date, sizeof(date), "%d/%d/%d", local.tm_mday, local.tm_mon + 1, local.tm_year + 1900);

    char command[1024];

    /*
      Before actually getting the timestamp value, we need to check if it exists, to avoid errors from Postgresql.

      Author: @ThePedroo
    */

    snprintf(command, sizeof(command), "SELECT EXISTS(SELECT timestamp FROM daily WHERE user_id = %"PRIu64");", message->author->id);

    res = PQexec(conn, command);

    resultCode = _PQresultStatus(conn, res, "trying to retrieve the timestamp", NULL);
    if (!resultCode) return;

    char *timestampExists = PQgetvalue(res, 0, 0);

    if (0 == strcmp(timestampExists, "t")) {
      PQclear(res);

      snprintf(command, sizeof(command), "SELECT timestamp FROM daily WHERE user_id = %"PRIu64";", message->author->id);

      res = PQexec(conn, command);

      resultCode = _PQresultStatus(conn, res, "trying to retrieve the timestamp", NULL);
      if (!resultCode) return;

      char *timestamp = PQgetvalue(res, 0, 0);

      if (0 == strcmp(timestamp, date)) {
        PQclear(res);

        struct discord_embed embed[] = {
          {
            .description = "You already got your daily for today, come back tomorrow. No concoins for you!",
            .image =
              &(struct discord_embed_image){
                .url = "https://raw.githubusercontent.com/Cogmasters/concord/master/docs/static/social-preview.png",
              },
            .footer =
              &(struct discord_embed_footer){
                .text = "Powered by Concord",
                .icon_url = "https://raw.githubusercontent.com/Cogmasters/concord/master/docs/static/concord-small.png",
              },
            .timestamp = discord_timestamp(client),
            .color = 16711680
          }
        };

        struct discord_create_message params = {
          .flags = 0,
          .embeds =
            &(struct discord_embeds){
              .size = 1,
              .array = embed,
            },
        };

        discord_create_message(client, message->channel_id, &params, NULL);

        return;
      }

      PQclear(res);
    } else {
      PQclear(res);
    }

    snprintf(command, sizeof(command), "SELECT EXISTS(SELECT concoins FROM economy WHERE user_id = %"PRIu64");", message->author->id);

    res = PQexec(conn, command);

    resultCode = _PQresultStatus(conn, res, "trying to retrieve the concoins", NULL);
    if (!resultCode) return;

    char *concoinsExists = PQgetvalue(res, 0, 0);

    unsigned long long concoins = 0;

    if (0 == strcmp(concoinsExists, "t")) {
      PQclear(res);

      snprintf(command, sizeof(command), "SELECT concoins FROM economy WHERE user_id = %"PRIu64";", message->author->id);

      res = PQexec(conn, command);

      resultCode = _PQresultStatus(conn, res, "trying to retrieve the concoins", NULL);
      if (!resultCode) return;

      concoins = strtoull(PQgetvalue(res, 0, 0), NULL, 10);

      PQclear(res);
    } else {
      PQclear(res);
    }

    /*
      Before inserting the records, we need to remove old records to avoid conflicts (due to be UNIQUE), and to not stay with the old records.

      Author: @ThePedroo
    */

    snprintf(command, sizeof(command), "DELETE FROM economy WHERE user_id = %"PRIu64";DELETE FROM daily WHERE user_id = %"PRIu64";INSERT INTO economy(user_id, concoins) values(%"PRIu64", %lld);INSERT INTO daily(user_id, timestamp) values(%"PRIu64", '%s');", message->author->id, message->author->id, message->author->id, concoins + moneyWin, message->author->id, date);

    res = PQexec(conn, command);

    resultCode = _PQresultStatus(conn, res, "inserting records into economy and daily tables", "Successfully inserted reconds into the economy and daily tables.");
    if (!resultCode) return;

    PQclear(res);
    PQfinish(conn);

    char embedDescription[64];
    snprintf(embedDescription, sizeof(embedDescription), "You've received your daily, now you have %lld concoins.", concoins + moneyWin);

    struct discord_embed embed[] = {
      {
        .description = embedDescription,
        .image =
          &(struct discord_embed_image){
            .url = "https://raw.githubusercontent.com/Cogmasters/concord/master/docs/static/social-preview.png",
          },
        .footer =
          &(struct discord_embed_footer){
            .text = "Powered by Concord",
            .icon_url = "https://raw.githubusercontent.com/Cogmasters/concord/master/docs/static/concord-small.png",
        },
        .timestamp = discord_timestamp(client),
        .color = 15615
      }
    };

    struct discord_create_message params = {
      .flags = 0,
      .embeds =
        &(struct discord_embeds){
          .size = 1,
          .array = embed,
        },
    };

    discord_create_message(client, message->channel_id, &params, NULL);
  }
  if (0 == strcmp(message->content, ".bal")) {
    PGconn *conn = connectDB(client);

    char command[256];
    snprintf(command, sizeof(command), "SELECT EXISTS(SELECT concoins FROM economy WHERE user_id = %"PRIu64");", message->author->id);

    PGresult *res = PQexec(conn, command);

    int resultCode = _PQresultStatus(conn, res, "trying to retrieve the concoins", NULL);
    if (!resultCode) return;

    char *concoinsExists = PQgetvalue(res, 0, 0);

    unsigned long long concoins = 0;

    if (0 == strcmp(concoinsExists, "t")) {
      PQclear(res);

      snprintf(command, sizeof(command), "SELECT concoins FROM economy WHERE user_id = %"PRIu64";", message->author->id);

      res = PQexec(conn, command);

      resultCode = _PQresultStatus(conn, res, "trying to retrieve the concoins", NULL);
      if (!resultCode) return;

      concoins = strtoull(PQgetvalue(res, 0, 0), NULL, 10);
    } else {
      PQclear(res);
    }
    
    PQclear(res);
    PQfinish(conn);


    char embedDescription[64];
    snprintf(embedDescription, sizeof(embedDescription), "Right at this moment, you have %lld concoins.", concoins);

    struct discord_embed embed[] = {
      {
        .description = embedDescription,
        .image =
          &(struct discord_embed_image){
            .url = "https://raw.githubusercontent.com/Cogmasters/concord/master/docs/static/social-preview.png",
          },
        .footer =
          &(struct discord_embed_footer){
            .text = "Powered by Concord",
            .icon_url = "https://raw.githubusercontent.com/Cogmasters/concord/master/docs/static/concord-small.png",
        },
        .timestamp = discord_timestamp(client),
        .color = 15615
      }
    };

    struct discord_create_message params = {
      .flags = 0,
      .embeds =
        &(struct discord_embeds){
          .size = 1,
          .array = embed,
        },
    };

    discord_create_message(client, message->channel_id, &params, NULL);
  }
  if (0 == strcmp(message->content, ".work")) {
    PGconn *conn = connectDB(client);

    unsigned long long moneyWin = rand() % 10000;

    /* 
      In case the user installed a new Postgresql database, it will re-create the economy table.
        
      Author: @ThePedroo
    */

    PGresult *res = PQexec(conn, "CREATE TABLE IF NOT EXISTS economy(user_id BIGINT UNIQUE NOT NULL, concoins BIGINT NOT NULL);CREATE TABLE IF NOT EXISTS work(user_id BIGINT UNIQUE NOT NULL, timestamp BIGINT NOT NULL);");

    PQclear(res);

    int resultCode = _PQresultStatus(conn, res, "creating economy and work tables", NULL);
    if (!resultCode) return;

    struct timeval timeval; 
    gettimeofday(&timeval, NULL);

    long long milliseconds = timeval.tv_sec * 1000LL + timeval.tv_usec / 1000;

    char command[1024];

    /*
      Before actually getting the timestamp value, we need to check if it exists, to avoid errors from Postgresql.

      Author: @ThePedroo
    */

    snprintf(command, sizeof(command), "SELECT EXISTS(SELECT timestamp FROM work WHERE user_id = %"PRIu64");", message->author->id);

    res = PQexec(conn, command);

    resultCode = _PQresultStatus(conn, res, "trying to retrieve the timestamp", NULL);
    if (!resultCode) return;

    char *timestampExists = PQgetvalue(res, 0, 0);

    if (0 == strcmp(timestampExists, "t")) {
      PQclear(res);

      snprintf(command, sizeof(command), "SELECT timestamp FROM work WHERE user_id = %"PRIu64";", message->author->id);

      res = PQexec(conn, command);

      resultCode = _PQresultStatus(conn, res, "trying to retrieve the timestamp", NULL);
      if (!resultCode) return;

      unsigned long long timestamp = strtoull(PQgetvalue(res, 0, 0), NULL, 10);

      printf("%lld\n%lld\n", timestamp, milliseconds);

      if ((milliseconds - timestamp) < 600000) {
        PQclear(res);

        struct discord_embed embed[] = {
          {
            .description = "You can only can work from 10 to 10 minutes, come back soon. No concoins for you right now!",
            .image =
              &(struct discord_embed_image){
                .url = "https://raw.githubusercontent.com/Cogmasters/concord/master/docs/static/social-preview.png",
              },
            .footer =
              &(struct discord_embed_footer){
                .text = "Powered by Concord",
                .icon_url = "https://raw.githubusercontent.com/Cogmasters/concord/master/docs/static/concord-small.png",
              },
            .timestamp = discord_timestamp(client),
            .color = 16711680
          }
        };

        struct discord_create_message params = {
          .flags = 0,
          .embeds =
            &(struct discord_embeds){
              .size = 1,
              .array = embed,
            },
        };

        discord_create_message(client, message->channel_id, &params, NULL);

        return;
      }

      PQclear(res);
    } else {
      PQclear(res);
    }

    snprintf(command, sizeof(command), "SELECT EXISTS(SELECT concoins FROM economy WHERE user_id = %"PRIu64");", message->author->id);

    res = PQexec(conn, command);

    resultCode = _PQresultStatus(conn, res, "trying to retrieve the concoins", NULL);
    if (!resultCode) return;

    char *concoinsExists = PQgetvalue(res, 0, 0);

    unsigned long long concoins = 0;

    if (0 == strcmp(concoinsExists, "t")) {
      PQclear(res);

      snprintf(command, sizeof(command), "SELECT concoins FROM economy WHERE user_id = %"PRIu64";", message->author->id);

      res = PQexec(conn, command);

      resultCode = _PQresultStatus(conn, res, "trying to retrieve the concoins", NULL);
      if (!resultCode) return;

      concoins = strtoull(PQgetvalue(res, 0, 0), NULL, 10);

      PQclear(res);
    } else {
      PQclear(res);
    }

    /*
      Before inserting the records, we need to remove old records to avoid conflicts (due to be UNIQUE), and to not stay with the old records.

      Author: @ThePedroo
    */

    snprintf(command, sizeof(command), "DELETE FROM economy WHERE user_id = %"PRIu64";DELETE FROM work WHERE user_id = %"PRIu64";INSERT INTO economy(user_id, concoins) values(%"PRIu64", %lld);INSERT INTO work(user_id, timestamp) values(%"PRIu64", %lld);", message->author->id, message->author->id, message->author->id, concoins + moneyWin, message->author->id, milliseconds);

    res = PQexec(conn, command);

    resultCode = _PQresultStatus(conn, res, "inserting records into economy and work tables", "Successfully inserted reconds into the economy and daily tables.");
    if (!resultCode) return;

    PQclear(res);
    PQfinish(conn);

    char embedDescription[64];
    snprintf(embedDescription, sizeof(embedDescription), "You've worked, now you have %lld concoins.", concoins + moneyWin);

    struct discord_embed embed[] = {
      {
        .description = embedDescription,
        .image =
          &(struct discord_embed_image){
            .url = "https://raw.githubusercontent.com/Cogmasters/concord/master/docs/static/social-preview.png",
          },
        .footer =
          &(struct discord_embed_footer){
            .text = "Powered by Concord",
            .icon_url = "https://raw.githubusercontent.com/Cogmasters/concord/master/docs/static/concord-small.png",
        },
        .timestamp = discord_timestamp(client),
        .color = 15615
      }
    };

    struct discord_create_message params = {
      .flags = 0,
      .embeds =
        &(struct discord_embeds){
          .size = 1,
          .array = embed,
        },
    };

    discord_create_message(client, message->channel_id, &params, NULL);
  }
}

int main(void) {
  struct discord *client = discord_config_init("config.json");

  discord_set_on_ready(client, &on_ready);
  discord_set_on_message_create(client, &on_messageCreate);

  discord_add_intents(client, DISCORD_GATEWAY_MESSAGE_CONTENT);

  discord_run(client);
}