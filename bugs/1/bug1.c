#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



// prototypes
void touch(const char *name);
void url_decode(char *dst, const char *src);
int http_read_line(int fd, char *buf, size_t size);
const char *http_request_line(int fd, char *reqpath, char *env, size_t *env_len);

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
    static char env[8192];  /* static variables are not on the stack */
    static size_t env_len;
    char reqpath[2048];
    const char *errmsg;

    /* get the request line */
    if ((errmsg = http_request_line(fd, reqpath, env, &env_len)))
        printf("http_request_line: %s", errmsg);
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

const char *http_request_line(int fd, char *reqpath, char *env, size_t *env_len)
{
    static char buf[8192];      /* static variables are not on the stack */
    char *sp1, *sp2, *qp, *envp = env;

    if (http_read_line(fd, buf, sizeof(buf)) < 0)
        return "Socket IO error";

    /* Parse request like "GET /foo.html HTTP/1.0" */
    sp1 = strchr(buf, ' ');
    if (!sp1)
        return "Cannot parse HTTP request (1)";
    *sp1 = '\0';
    sp1++;
    if (*sp1 != '/')
        return "Bad request path";

    sp2 = strchr(sp1, ' ');
    if (!sp2)
        return "Cannot parse HTTP request (2)";
    *sp2 = '\0';
    sp2++;

    /* We only support GET and POST requests */
    if (strcmp(buf, "GET") && strcmp(buf, "POST"))
        return "Unsupported request (not GET or POST)";

    envp += sprintf(envp, "REQUEST_METHOD=%s", buf) + 1;
    envp += sprintf(envp, "SERVER_PROTOCOL=%s", sp2) + 1;

    /* parse out query string, e.g. "foo.py?user=bob" */
    if ((qp = strchr(sp1, '?')))
    {
        *qp = '\0';
        envp += sprintf(envp, "QUERY_STRING=%s", qp + 1) + 1;
    }

    /* decode URL escape sequences in the requested path into reqpath */
    url_decode(reqpath, sp1);

    envp += sprintf(envp, "REQUEST_URI=%s", reqpath) + 1;

    envp += sprintf(envp, "SERVER_NAME=zoobar.org") + 1;

    *envp = 0;
    *env_len = envp - env + 1;

    printf("env_len = %zu\n", *env_len);
    for (int i = 0; i < *env_len; i++)
        printf("%c",env[i]);
    printf("\n");
    return NULL;
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