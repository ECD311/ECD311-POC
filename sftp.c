#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

const char *ssh_host = "localhost";
const char *ssh_user = "username";
const char *ssh_password = "password";
int ssh_port = 22;

const char *remote_path = "\\";

const char *local_path = "\\";

ssh_session ssh_setup(int *rc, const char *ssh_host, int ssh_port);
int ssh_authenticate(ssh_session *session, const char *host, const char *user,
                     const char *password);
sftp_session sftp_setup(ssh_session *session, int *rc);

int main() {
    int rc;

    ssh_session session = ssh_setup(&rc, ssh_host, ssh_port);

    if (rc != SSH_OK) {
#ifdef DEBUG
        printf("Failed to setup SSH session with host %s\n", ssh_host);
#endif
        exit(-1);
    }
    rc = ssh_authenticate(&session, ssh_host, ssh_user, ssh_password);

    if (rc != SSH_OK) {
        ssh_free(session);
        exit(-1);
    }

    sftp_session sftp = sftp_setup(&session, &rc);

    if (sftp == NULL) {
        exit(-1);
    }

    sftp_free(sftp);
    ssh_free(session);

    return 0;
}

ssh_session ssh_setup(int *rc, const char *ssh_host, int ssh_port) {

    ssh_session session = ssh_new();

    if (session == NULL) {
        *rc = -1;
        return NULL;
    }

    ssh_options_set(session, SSH_OPTIONS_HOST, ssh_host);
    ssh_options_set(session, SSH_OPTIONS_PORT, &ssh_port);

    *rc = ssh_connect(session);

    if (*rc != SSH_OK) {
        printf("Error connecting to %s: %s\n", ssh_host,
               ssh_get_error(session));
        ssh_free(session);
        return NULL;
    }
#ifdef DEBUG
    printf("Connected to %s\n", ssh_host);
#endif
    return 0;
}

int ssh_authenticate(ssh_session *session, const char *host, const char *user,
                     const char *password) {
    int rc;

    rc = ssh_userauth_password(*session, user, password);
    if (rc != SSH_OK) {
        printf("Error authenticating to %s: %s\n", host,
               ssh_get_error(*session));
        return rc;
    }
#ifdef DEBUG
    printf("Authenticated to %s\n", ssh_host);
#endif
    return 0;
}

sftp_session sftp_setup(ssh_session *session, int *rc) {
    sftp_session sftp = sftp_new(*session);
    if (sftp == NULL) {
        printf("Error creating SFTP session: %s\n", ssh_get_error(session));
        *rc = -1;
        return NULL;
    }

    *rc = sftp_init(sftp);
    if (*rc != SSH_OK) {
        printf("Error initializing SFTP session: %d\n", sftp_get_error(sftp));
        sftp_free(sftp);
        return NULL;
    }

    return sftp;
}