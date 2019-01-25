#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>

#define PORT 80
#define BACKLOG 10 // Request que size

void logtime();
int is_regular_file(const char *path);
int is_directory(const char *path);
int sessdetach();
int drop_root_privileges();
char *get_content_type(char *file_path);
char *get_header(char *file_path);
char *get_dir_content(char *file_path);
char *linkwrap(char *file_name);

int main () {
    int server_socket_fd;
    int client_socket_fd;
    int error_log_fd;
    int bindstatus;
    int sockoptstatus;
    int enable = 1;
    char *file_path;
    char http_response[1024];
    char *http_response_save_ptr;
    char asis_file_line[BUFSIZ];
    const char *http_bad_request = "File is not supported or does not exist!";
    const char *this_is_dir ="This is a directory"; // Temporary, remove later
    int served_file_fd;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t client_address_size;
    FILE *asisFp;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    /* Open error log file and redirecto stdout */
    error_log_fd = open("/var/error.log", O_RDWR | O_CREAT, mode);
    dup2(error_log_fd, 2);

    /* Change root directory (for docker)*/
    chroot("/var/www/");

    /* Harvest child processes */
    signal(SIGCHLD, SIG_IGN);

    /* Daemonize */
    sessdetach();

    /* Redirect stdout to log descripter after detaching */
    dup2(error_log_fd, 2);

    /* Create socket */
    server_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server_socket_fd < 0) {
        perror("Error creating socket");
        exit(1);
    }

    /* Allow port re-use */
    sockoptstatus = setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    if (sockoptstatus < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    // Initialize server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind socket with server address
    bindstatus = bind(server_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address));

    if (bindstatus < 0) {
        perror("Bind failed");
        exit(1);
    }

    drop_root_privileges();

    // Listen to server socket
    listen(server_socket_fd, BACKLOG);
    while(1) {
        // Accept connection
        client_socket_fd = accept(server_socket_fd, (struct sockaddr*) &client_address, &client_address_size);
        // TODO: Uncomment
        logtime();
        dprintf(2, "Connection accepted from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        if (0 == fork()) {
            recv(client_socket_fd, http_response, sizeof(http_response), 0);
            strtok_r(http_response, " ", &http_response_save_ptr);
            file_path = strtok_r(NULL, " ", &http_response_save_ptr);
            asisFp = fopen(file_path, "rb");

            char *header = get_header(file_path);

            if (asisFp == NULL) {
                send(client_socket_fd, header, strlen(header), 0);
                send(client_socket_fd, http_bad_request, strlen(http_bad_request), 0);
            } else if (is_directory(file_path)) { // Path is a directory or root
                char *dir_list = get_dir_content(file_path);
                send(client_socket_fd, header, strlen(header), 0);
                send(client_socket_fd, dir_list, strlen(dir_list), 0);
                free(dir_list);
//                send(client_socket_fd, header, strlen(header), 0);
//                send(client_socket_fd, this_is_dir, strlen(this_is_dir), 0);
            } else {
                send(client_socket_fd, header, strlen(header), 0);
                while (fgets(asis_file_line, BUFSIZ, asisFp) != NULL) {
                    send(client_socket_fd, asis_file_line, strlen(asis_file_line), 0);
                }
            }

            /* Close sockets and file descriptors */
            shutdown(client_socket_fd, SHUT_RDWR);
            close(server_socket_fd);
            fclose(asisFp);

            /* Free memory occupied by header */
            free(header);
            logtime();
            dprintf(2, "Closing connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
            exit(0);
        }
        else {
            close(client_socket_fd);
        }
    }
    return 0;
}

void logtime() {
    time_t t = time(NULL);
    struct tm time = *localtime(&t);

    dprintf(2, "\n%d-%d-%d %d:%d:%d:\n",
            time.tm_mday,
            time.tm_mon + 1,
            time.tm_year + 1900,
            time.tm_hour,
            time.tm_min,
            time.tm_sec);
}

int is_regular_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int is_directory(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

int sessdetach() {
    pid_t pid = 0;
    pid_t sid = 0;

    pid = fork();

    if (pid < 0) {
        dprintf(2, "Fork failed");
        exit(1);
    }

    if (pid > 0) {
        dprintf(2, "Parent: %d   Child: %d\n", getpid(), pid);
        raise(SIGSTOP);
        // exit(0);
    }

    sid = setsid();

    if (sid < 0) {
        raise(SIGSTOP);
        // exit(0);
    }

    for (int fd = 0; fd <= 2; fd++) {
        close(fd);
    }

    chdir("/");
    umask(0);

    return 0;
}

int drop_root_privileges() {
    if (getuid() == 0) {
        /* process is running as root, drop privileges */
        if (setgid(1337) != 0)
            perror("setgid: Unable to drop group privileges");
        if (setuid(1337) != 0)
            perror("setuid: Unable to drop user privileges");
    }
    return 0;
}

char *get_content_type(char *file_path) {
    char *file_path_dup = strdup(file_path);
    char *file_path_save_ptr;

    if (is_regular_file(file_path) != 0) {
        strtok_r(file_path_dup, ".", &file_path_save_ptr);
        char *suffix = strtok_r(NULL, ".", &file_path_save_ptr);
        free(file_path_dup);

        if (strcmp(suffix, "html") == 0) {
            return "text/html";
        } else if (strcmp(suffix, "txt") == 0) {
            return "text/plain";
        } else if (strcmp(suffix, "asis") == 0) {
            return "text/plain";
        }else if (strcmp(suffix, "png") == 0) {
            return "image/png";
        } else if (strcmp(suffix, "svg") == 0) {
            return "image/svg";
        } else if (strcmp(suffix, "xml") == 0) {
            return "application/xml";
        } else if (strcmp(suffix, "xsl") == 0) {
            return "application/xslt+xml";
        } else if (strcmp(suffix, "css") == 0) {
            return "text/css";
        } else if (strcmp(suffix, "js") == 0) {
            return "application/javascript";
        } else if (strcmp(suffix, "json") == 0) {
            return "application/json";
        } else {
            return NULL;
        }
    }
    // TODO: Controll may not reach end of function
}

char *get_header(char *file_path) {

    char header[1024] = "";
    char *type = get_content_type(file_path);

    if (is_directory(file_path) != 0) {
        strcat(header, "HTTP/1.1 200 OK\n");
        strcat(header, "Content-Type: text/html");
        strcat(header, "\n\n");
    } else if (type == NULL) {
        strcat(header, "HTTP/1.1 400 Bad Request\n\n");
    } else {
        strcat(header, "HTTP/1.1 200 OK\n");
        strcat(header, "Content-Type: ");
        strcat(header, type);
        strcat(header, "\n\n");
    }

    char *returnedHeader = strdup(header);

    return returnedHeader;
}

char *get_dir_content(char *file_path) {
    char buffer[1024] = "<!DOCTYPE html>\n<html>\n<head>\n</head>\n<body>";
    char temp[128];
    struct stat stat_buffer;
    struct dirent *entry;
    DIR *dir;

    dir = opendir(file_path);

    if (dir == NULL) {
        perror("Unable to open directory");
    }

    // chdir(file_path);

    strcat(buffer, "--------------------------------------------<br>");
    strcat(buffer, "Rettigheter\tUID\tGID\tNavn<br>");
    strcat(buffer, "--------------------------------------------<br>");

    while ((entry = readdir(dir)) != NULL) {
        if (stat (entry -> d_name, &stat_buffer) < 0) {
            perror("");
            // exit(2);
        }
        sprintf(temp, "%o\t\t", stat_buffer.st_mode & 0777);
        strcat(buffer, temp);
        sprintf(temp, "%d\t",   stat_buffer.st_uid);
        strcat(buffer, temp);
        sprintf(temp, "%d\t",   stat_buffer.st_gid);
        strcat(buffer, temp);

        if (is_directory(entry -> d_name) != 0) {
            sprintf(temp,"%s<br>",   entry -> d_name);
            strcat(buffer, temp);
        } else {
            char *link_text = linkwrap(entry -> d_name);
            strcat(buffer, link_text);
            strcat(buffer, "<br>");
            free(link_text);
        }
    }
    closedir(dir);
    
    strcat(buffer, "\n</body>\n</html>");
    
    char *returnedContent = strdup(buffer);
    return returnedContent;
}

char *linkwrap(char *file_name) {
    char link[128] = "<a href=\"";
    strcat(link, file_name);
    strcat(link, "\">");
    strcat(link, file_name);
    strcat(link, "</a>");

    char *returnedLink = strdup(link);
    return returnedLink;
}