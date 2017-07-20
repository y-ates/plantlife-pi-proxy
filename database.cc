#include "database.h"

#include <cstdio>

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
