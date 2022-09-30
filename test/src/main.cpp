#include <Arduino.h>
#include <WiFi.h>

#include "FS.h"
#include "SPIFFS.h"
#include "libssh/libssh.h"
#include "libssh/scp.h"
#include "libssh_esp32.h"

#define FORMAT_SPIFFS_IF_FAILED false

const char *ssid = "nah no free wifi here";
const char *password = "no you don't";

const char *ssh_host = "localhost";  // this is temporary
const char *ssh_user = "username";
const char *ssh_password = "password";
int ssh_port = 22;

const char *scp_path = ".";  // this is temporary
SET_LOOP_TASK_STACK_SIZE(16 * 1024); // try 16k stack

void wifi_setup(const char *ssid, const char *password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("Connecting...");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println(WiFi.localIP());
}

int ssh_setup(ssh_session session, const char *ssh_host, int ssh_port) {
    int rc;

    ssh_options_set(session, SSH_OPTIONS_HOST, ssh_host);
    ssh_options_set(session, SSH_OPTIONS_PORT, &ssh_port);

    Serial.println("SSH options set");

    rc = ssh_connect(session);
    Serial.println("ssh_connect run");
    if (rc != SSH_OK) {
        Serial.printf("Error connecting to %s: %s\n", ssh_host,
                      ssh_get_error(session));
        return rc;
    }
    Serial.println("SSH connected");
    return 0;
}

int ssh_authenticate(ssh_session session, const char *host, const char *user,
                     const char *password) {
    int rc;

    rc = ssh_userauth_password(session, user, password);
    if (rc != SSH_OK) {
        Serial.printf("Error authenticating to %s: %s\n", host,
                      ssh_get_error(session));
        return rc;
    }
    Serial.println("SSH authenticated");
    return 0;
}

ssh_scp scp_setup(int *rc, ssh_session session, const char *scp_path) {
    ssh_scp scp;

    scp = ssh_scp_new(session, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, scp_path);
    if (scp == NULL) {
        Serial.printf("Error creating SCP session: %s\n",
                      ssh_get_error(session));
        *rc = -1;
        return NULL;
    }
    Serial.println("Created SCP session");

    *rc = ssh_scp_init(scp);
    if (*rc != SSH_OK) {
        Serial.printf("Error initializing SCP session: %s\n",
                      ssh_get_error(session));
        ssh_scp_free(scp);
        return NULL;
    }
    Serial.println("Initialized SCP session");

    return scp;
}

void reset() {
    Serial.println("Restarting");
    WiFi.disconnect();
    ESP.restart();
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) {
                listDir(fs, file.name(), levels - 1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void readFile(fs::FS &fs, const char *path) {
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory()) {
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while (file.available()) {
        Serial.write(file.read());
    }
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial.println("- file written");
    } else {
        Serial.println("- frite failed");
    }
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("- failed to open file for appending");
        return;
    }
    if (file.print(message)) {
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("- file renamed");
    } else {
        Serial.println("- rename failed");
    }
}

void deleteFile(fs::FS &fs, const char *path) {
    Serial.printf("Deleting file: %s\r\n", path);
    if (fs.remove(path)) {
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

void setup() {
    int rc;
    Serial.begin(115200);
    wifi_setup(ssid, password);

    Serial.println("wifi connected");

    libssh_begin();
    ssh_session my_ssh_session = ssh_new();
    if (my_ssh_session == NULL) {
        // something went very wrong
        Serial.println("something broke, failed to create ssh session");
        reset();
    }
    Serial.println("ssh session created");

    ssh_setup(my_ssh_session, ssh_host, ssh_port);

    // Serial.println("let's get a ssh connection");

    ssh_authenticate(my_ssh_session, ssh_host, ssh_user, ssh_password);

    // Serial.println("ssh should be ready");

    // now let's send a file via scp (is this actually an option or do i need to
    // find a sftp implementation?)

    // first write a file to the fs
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    } else {
        Serial.println("SPIFFS Mount Succeeded");
    }

    writeFile(SPIFFS, "/test.txt", "test file\r\n");  // write a test file to fs
    readFile(SPIFFS, "/test.txt");

    ssh_scp scp;
    int length;
    int scp_rc = 0;

    scp = scp_setup(&scp_rc, my_ssh_session, scp_path);
    if (scp == NULL) {
        return;
    }

    Serial.print(scp_rc);
    Serial.print("\n");

    File file;
    file = SPIFFS.open("/test.txt");

    length = file.size();
    file.close();

    Serial.printf("%d\n", length);
    rc = ssh_scp_push_file(scp, "test.txt", length, S_IRUSR | S_IWUSR);
    if (rc != SSH_OK) {
        Serial.printf("Can't open remote file: %s\n",
                      ssh_get_error(my_ssh_session));
        delay(2000);
        return;
    }
    Serial.println("Opened remote file");

    char contents[length];
    File file1 = SPIFFS.open("/test.txt");
    int i = 0;
    while (file1.available()) {
        contents[i] = file1.read();
        i++;
    }
    file1.close();

    rc = ssh_scp_write(scp, contents, length);
    if (rc != SSH_OK) {
        Serial.printf("Can't write to remote file: %s\n",
                      ssh_get_error(my_ssh_session));
        return;
    }
    Serial.println("Wrote to remote file");
    ssh_scp_close(scp);
    ssh_scp_free(scp);
    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);
    ssh_finalize();
}

void loop() {
    // put your main code here, to run repeatedly:
}
