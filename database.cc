#include "./database.h"

#include <sstream>
#include <string>

#include <cstdio>
#include <ctime>
#include <cstring>

const char* gTypeNames[SensorType::Last] = {
    "moisture",
    "humidity",
    "temperature",
    "light"
};

Database::Database(const char *host, const char *user, const char *pass,
                   const char *db /* = nullptr */, unsigned int port /* = 0 */)
  : host_(host), user_(user), pass_(pass), db_(db),
    port_(port), connected_(false) {
    con_ = mysql_init(nullptr);
}

Database::~Database() {
    if (con_) {
        mysql_close(con_);
    }

    connected_ = false;
}

bool Database::Connect() {
    if (mysql_real_connect(con_, host_, user_, pass_, db_, port_, nullptr, 0)) {
        connected_ = true;
        return true;
    }

    return false;
}

bool Database::InsertValue(SensorType type, float value) {
    if (type < 0 || type >= SensorType::Last) {
        return false;
    }

    // Get the id of the corresponding type
    // from our sensortype table
    std::stringstream ss;
    ss << "SELECT id FROM sensortype WHERE name = '";
    ss << gTypeNames[type];
    ss << "'";

    const std::string query = ss.str();

    if (mysql_query(con_, query.c_str()) != 0) {
        return false;
    }

    MYSQL_RES *result = mysql_store_result(con_);
    if (result == nullptr) {
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    // Now get the integer of the returned id from the database
    int type_id = atoi(row[0]);

    // Prepare our statement to inser the value
    MYSQL_TIME  ts;
    MYSQL_BIND  bind[3];
    MYSQL_STMT  *stmt;

    const char *insstmt =
        "INSERT INTO sensordata(typeid, value, date) VALUES(?,?,?)";

    stmt = mysql_stmt_init(con_);
    if (!stmt) {
        return false;
    }

    if (mysql_stmt_prepare(stmt, insstmt, strlen(insstmt)) != 0) {
        return false;
    }

    // Bind our parameters
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = reinterpret_cast<char*>(&type_id);
    bind[0].is_null = 0;
    bind[0].length = 0;

    bind[1].buffer_type = MYSQL_TYPE_FLOAT;
    bind[1].buffer = &value;
    bind[1].buffer_length = sizeof(value);
    bind[1].is_null = 0;
    bind[1].length = 0;

    bind[2].buffer_type = MYSQL_TYPE_DATETIME;
    bind[2].buffer = reinterpret_cast<char*>(&ts);
    bind[2].is_null = 0;
    bind[2].length = 0;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        return false;
    }

    // Set to current time
    time_t t = time(0);
    struct tm now = {0};

    localtime_r(&t, &now);

    ts.year = now.tm_year + 1900;  // tm_year is year since 1900
    ts.month = now.tm_mon;
    ts.day = now.tm_mday;

    ts.hour = now.tm_hour;
    ts.minute = now.tm_min;
    ts.second = now.tm_sec;

    // Finally execute our statement
    if (mysql_stmt_execute(stmt) != 0) {
        return false;
    }

    // Check if it failed to insert
    if (mysql_stmt_affected_rows(stmt) != 1) {
        return false;
    }

    mysql_free_result(result);
    return true;
}
