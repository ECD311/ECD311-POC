#include <Arduino.h>
#include <WiFi.h>

#include "FS.h"
#include "SPIFFS.h"
#include "libssh_esp32.h"
#include "libssh/scp.h"

#define FORMAT_SPIFFS_IF_FAILED false

const char *ssid = "nah no free wifi here";
const char *password = "no you don't";

const char *ssh_host = "localhost";  // this is temporary
const char *ssh_user = "username";
const char *ssh_password = "password";
int ssh_port = 22;

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
    // put your setup code here, to run once:
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("Connecting...");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println(WiFi.localIP());

    //libssh_begin();
    ssh_session my_ssh_session = ssh_new();
    if (my_ssh_session == NULL)
    {
        //something went very wrong
        Serial.println("something broke, failed to create ssh session");
        return;
    }

    ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, ssh_host);
    ssh_options_set(my_ssh_session, SSH_OPTIONS_PORT, &ssh_port);

    Serial.println("let's get a ssh connection");

    rc = ssh_connect(my_ssh_session);

    Serial.println(rc);
    Serial.printf(ssh_get_error(my_ssh_session));

    rc = ssh_userauth_password(my_ssh_session, ssh_user, ssh_password);

    Serial.println(rc);
    Serial.printf(ssh_get_error(my_ssh_session));


    Serial.println("ssh is hopefully ready");

    ssh_scp scp;

    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    } else {
        Serial.println("SPIFFS Mount Succeeded");
    }

    writeFile(SPIFFS, "/test.txt", "test file\r\n"); // write a test file to fs
    readFile(SPIFFS, "/test.txt");

    /*listDir(SPIFFS, "/", 0);
    writeFile(SPIFFS, "/hello.txt", "Hello ");
    appendFile(SPIFFS, "/hello.txt", "World!\r\n");
    readFile(SPIFFS, "/hello.txt");
    renameFile(SPIFFS, "/hello.txt", "/foo.txt");
    readFile(SPIFFS, "/foo.txt");
    deleteFile(SPIFFS, "/foo.txt");
    Serial.println("Test complete");
    */
}

void loop() {
    // put your main code here, to run repeatedly:
}
