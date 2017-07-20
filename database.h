#ifndef DATABASE_H_
#define DATABASE_H_

#include <mysql.h>

class Database {
 public:
    Database(const char *host, const char *user, const char *pass,
             const char *db = nullptr, unsigned int port = 0);
    ~Database();

    bool Connect();

    const char *GetLastError() { return mysql_error(con_); }

 private:
    MYSQL *con_;

    const char *user_;
    const char *pass_;

    const char *db_;

    const char *host_;
    unsigned int port_;

    bool connected_;
};

#endif  // DATABASE_H_
