#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int http_read_line(int fd, char *buf, size_t size);
void url_decode(char *dst, const char *src);
const char *http_request_headers(int fd);

int main(int argc, char** argv){
    if (argv[1] == NULL) {
        printf("Usage: ./bug1 [s|l]\n");
        return 1;
    }
    FILE* fp;
    if (strcmp(argv[1],"s") == 0)
        fp = fopen("small_input.txt","r");
    else if (strcmp(argv[1],"l") == 0)
        fp = fopen("large_input.txt","r");
    else {
        printf("%s is not a valid argument\n", argv[1]);
        return 2;
    }

    int fd = fileno(fp);
    http_request_headers(fd);

}


int http_read_line(int fd, char *buf, size_t size)
{
    size_t i = 0;

    for (;;)
    {
        int cc = read(fd, &buf[i], 1);
        if (cc <= 0)
            break;

        if (buf[i] == '\r')
        {
            buf[i] = '\0';      /* skip */
            continue;
        }

        if (buf[i] == '\n')
        {
            buf[i] = '\0';
            return 0;
        }

        if (i >= size - 1)
        {
            buf[i] = '\0';
            return 0;
        }

        i++;
    }

    return -1;
}

void url_decode(char *dst, const char *src)
{
    for (;;)
    {
        if (src[0] == '%' && src[1] && src[2])
        {
            char hexbuf[3];
            hexbuf[0] = src[1];
            hexbuf[1] = src[2];
            hexbuf[2] = '\0';

            *dst = strtol(&hexbuf[0], 0, 16);
            src += 3;
        }
        else if (src[0] == '+')
        {
            *dst = ' ';
            src++;
        }
        else
        {
            *dst = *src;
            src++;

            if (*dst == '\0')
                break;
        }

        dst++;
    }
}

const char *http_request_headers(int fd)
{
    static char buf[8192];      /* static variables are not on the stack */
    int i;
    char value[512];
    char envvar[512];

    /* Now parse HTTP headers */
    for (;;)
    {
        if (http_read_line(fd, buf, sizeof(buf)) < 0)
            return "Socket IO error";

        if (buf[0] == '\0')     /* end of headers */
            break;
        printf("%s\n",buf);

        /* Parse things like "Cookie: foo bar" */
        char *sp = strchr(buf, ' ');
        if (!sp)
            return "Header parse error (1)";
        *sp = '\0';
        sp++;

        /* Strip off the colon, making sure it's there */
        if (strlen(buf) == 0)
            return "Header parse error (2)";

        char *colon = &buf[strlen(buf) - 1];
        if (*colon != ':')
            return "Header parse error (3)";
        *colon = '\0';

        /* Set the header name to uppercase and replace hyphens with underscores */
        for (i = 0; i < strlen(buf); i++) {
            buf[i] = toupper(buf[i]);
            if (buf[i] == '-')
                buf[i] = '_';
        }

        /* Decode URL escape sequences in the value */
        url_decode(value, sp);

        /* Store header in env. variable for application code */
        /* Some special headers don't use the HTTP_ prefix. */
        if (strcmp(buf, "CONTENT_TYPE") != 0 &&
            strcmp(buf, "CONTENT_LENGTH") != 0) {
            sprintf(envvar, "HTTP_%s", buf);
            setenv(envvar, value, 1);
        } else {
            setenv(buf, value, 1);
        }
    }

    return 0;
}