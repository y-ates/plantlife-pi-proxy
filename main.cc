#include <mysql.h>

#include <cstdio>
#include <cstdlib>

#include "database.h"

int main(int argc, char *argv[]) {
    Database *db = new Database("localhost", "user", "user", "plantlife");
    if (!db) {
        fprintf(stderr, "Failed to initialize database object\n");
        return 1;
    }

    if (!db->Connect()) {
        fprintf(stderr, "Failed to connect: %s\n", db->GetLastError());
    }

    delete db;
    return 0;
}
