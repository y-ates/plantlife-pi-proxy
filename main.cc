#include <mysql.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <iostream>  // delete later

#include "./database.h"
#include "./rcswitch.h"

// Every 30 minutes we care about new data
// that gives us 48 entries per 24 hours.
#define INTERVAL 1  // 30 

long int gLastInserted[SensorType::Last];
unsigned int gNumInsertions;

Database *gDB;

static void daemonize(void) {
    pid_t pid, sid;

    /* already a daemon */
    if (getppid() == 1) {
       return;
    }

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* At this point we are executing as the child process */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    /* Redirect standard files to /dev/null */
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
}

void sig_handler(int signo) {
    if (signo == SIGINT) {
        fprintf(stderr, "received SIGINT\n");
        if (gDB) {
            delete gDB;
        }
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    //daemonize();  // fg process for error codes

    signal(SIGINT, sig_handler);

    memset(gLastInserted, 0x00, sizeof(gLastInserted));

    gNumInsertions = 0;

    // Establish database connection
    Database *db = new Database("localhost", "user", "user", "plantlife");
    if (!db) {
        fprintf(stderr, "Failed to initialize database object\n");
        return 0;
    }

    // Store a reference in case we need to
    // delete it when receiving SIGINT
    gDB = db;

    if (!db->Connect()) {
        fprintf(stderr, "Failed to connect: %s\n", db->GetLastError());
    }

    // Get wiringPi set up
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "wiringPiSetup failed\n");
        return 0;
    }

    RCSwitch rcswitch = RCSwitch();
    rcswitch.enableReceive(2);

    // Enter our endless loop for receiving data
    while (1) {
        if (rcswitch.available()) {
            if (rcswitch.getReceivedValue() == 0) {
                rcswitch.resetAvailable();
                continue;
            }

            int encodedData = rcswitch.getReceivedValue();

            // Received integers are encoded like this:
            // XYZZZZ
            //  X = identifies the sender arduino (not used)
            //  Y = idnetifies the sensor supplying the data
            //  Z = the value read from the sensor

            // TODO(jhector): when supporting more arduino,
            // something needs to change here.

            unsigned int type = (encodedData/10000) % 10;
            float sensorValue = static_cast<float>(encodedData % 10000);

            /* TODO: Something goes wrong here */
            // // Only accept sensor data within the range of sensor we support
            // if (type < 0 || type >= SensorType::Last) {
            //     rcswitch.resetAvailable();
            //     continue;
            // }

            long int ts = static_cast<long int>(time(0));

            /* TODO: Something goes wrong here */
            // // Only care if our interval is expired
            // if (ts >= gLastInserted[type] + (INTERVAL * 60)) {
            //     rcswitch.resetAvailable();
            //     continue;
            // }

            if (!db->InsertValue(static_cast<SensorType>(type), sensorValue)) {
                fprintf(stderr, "Insert failed: value %3.2f for type %d: %s\n",
                    sensorValue, type, db->GetLastError());
            }

            gNumInsertions++;
            gLastInserted[type] = ts;

            rcswitch.resetAvailable();
        }

        // Once we received values from each sensor we
        // sleep for INTERVAL min to avoid waisting cycles.
        if ((gNumInsertions % SensorType::Last) == 0) {
            sleep(INTERVAL*60);
        }
    }

#if 0
    if (!db->InsertValue(SensorType::Moisture, 3.2f)) {
        fprintf(stderr, "Failed to insert value: %s\n", db->GetLastError());
    }
#endif

    if (db) {
        delete db;
        db = gDB = nullptr;
    }

    return 1;
}
