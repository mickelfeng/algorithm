#include "util.h"

config_t          gconfig;
global_variable_t g_vars;

void usage(void)
{
    printf(PACKAGE " " VERSION "\n");
    printf("Build-date %s\n", build_date);
    printf("-p <file>     set ABS path(prefix), Necessarily\n"
           "-f <file>     filter keywords file name\n"
           "-v            show version and help\n"
           "-h            show this help and exit\n");
    printf("-H <hostname> hostname(default: %s)\n", DEFAULT_HOSTNAME);
    printf("-P <num>      listen port(default: %d)\n", DEFAULT_PORT);
    printf("-t <timeout>  set HTTP timeout(default: %d)\n", DEFAULT_TIMEOUT);
    printf("-c [0|1]      case switch(default: %d)\n", DEFAULT_CASE_SWITCH);

    return;
}

int parse_args(int argc, char *argv[])
{
    int c = 0;
    int ret = 0;
    int t_opt;
    size_t str_len = 0;
    char tmp_str[FILE_PATH_LEN] = {0};

#if (_DEBUG)
    logprintf("grgc = %d", argc);
#endif

    while (-1 != (c = getopt(argc, argv,
        "p:"    /* ABS path for work(prefix) */
        "f:"    /* filter keywords file name*/
        "H:"    /* hostname */
        "P:"    /* listen port */
        "t:"    /* set HTTP timeout */
        "c:"    /* case switch */
        "v"     /* show version */
        "h"     /* show usage */
    )))
    {
        switch (c)
        {
        case 'p':
            str_len = strlen(optarg);
            if (str_len)
            {
                snprintf(gconfig.prefix, FILE_PATH_LEN, "%s", optarg);
            }
            else
            {
                usage();
                ret = -1;
                goto FINISH;
            }
            break;

        case 'f':
            str_len = strlen(optarg);
            if (str_len)
            {
                snprintf(tmp_str, sizeof(tmp_str), "%s", optarg);
            }
            else
            {
                usage();
                ret = -1;
                goto FINISH;
            }
            break;

        case 'H':
            str_len = strlen(optarg);
            if (str_len)
            {
                snprintf(gconfig.hostname, HOST_NAME_LEN, "%s", optarg);
            }
            else
            {
                usage();
                ret = -1;
                goto FINISH;
            }
            break;

        case 'P':
            t_opt = atoi(optarg);
            if (t_opt > 0)
            {
                gconfig.port = t_opt;
            }
            break;

        case 't':
            t_opt = atoi(optarg);
            if (t_opt > 0 && t_opt <= 60 )
            {
                gconfig.timeout = t_opt;
            }
            break;

        case 'c':
            t_opt = atoi(optarg);
            gconfig.no_case = t_opt;
            break;

        case 'v':
        case 'h':
            usage();
            exit(EXIT_SUCCESS);
        }
    }

    snprintf(gconfig.keyword_file, sizeof(gconfig.keyword_file),
        "%s/data/%s", gconfig.prefix, tmp_str[0] ? tmp_str : DEFINE_KEYWORD_FILE);

FINISH:
    return ret;
}

void print_config(void)
{
    printf("---gconfig---\n");
    printf("prefix:       %s\n", gconfig.prefix);
    printf("keyword_file: %s\n", gconfig.keyword_file);
    printf("hostname:     %s\n", gconfig.hostname);
    printf("port:         %d\n", gconfig.port);
    printf("timeout:      %d\n", gconfig.timeout);
    printf("no_case:      %d\n", gconfig.no_case);
    printf("---end print gconfig---\n");

    return;
}

int daemonize(int nochdir, int noclose)
{
    int fd;

    switch (fork())
    {
    case -1:
        return (-1);

    case 0:
        break;

    default:
        _exit(EXIT_SUCCESS);
    }

    if (setsid() == -1)
        return (-1);

    if (nochdir == 0)
    {
        if(chdir("/") != 0)
        {
            perror("chdir");
            return (-1);
        }
    }

    if (noclose == 0 && (fd = open("/dev/null", O_RDWR, 0)) != -1)
    {
        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("dup2 stdin");
            return (-1);
        }
        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2 stdout");
            return (-1);
        }
        if (dup2(fd, STDERR_FILENO) < 0)
        {
            perror("dup2 stderr");
            return (-1);
        }

        if (fd > STDERR_FILENO)
        {
            if (close(fd) < 0)
            {
                perror("close");
                return (-1);
            }
        }
    }

    return(0);
}

void handler(int signo)
{
    fprintf(stderr, "Get one signal: %d. man sigaction.\n", signo);
    return;
}

void signal_setup(void)
{
    static int signo[] = {
        SIGHUP,
        SIGINT,     /* ctrl + c */
        SIGCHLD,    /* 僵死进程或线程的信号 */
        SIGPIPE,
        SIGALRM,
        SIGUSR1,
        SIGUSR2,
        SIGTERM,
        //SIGCLD,

#ifdef  SIGTSTP
        /* background tty read */
        SIGTSTP,
#endif
#ifdef  SIGTTIN
        /* background tty read */
        SIGTTIN,
#endif
#ifdef SIGTTOU
        SIGTTOU,
#endif
        SIGNO_END
    };

    int i = 0;
    struct sigaction sa;
    //sa.sa_handler = SIG_IGN;    //设定接受到指定信号后的动作为忽略
    sa.sa_handler = handler;
    sa.sa_flags   = SA_SIGINFO;

    if (-1 == sigemptyset(&sa.sa_mask))   //初始化信号集为空
    {
        fprintf(stderr, "failed to init sa_mask.\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; SIGNO_END != signo[i]; i++)
    {
        //屏蔽信号
        if (-1 == sigaction(signo[i], &sa, NULL))
        {
            fprintf(stderr, "failed to ignore: %d\n", signo[i]);
            exit(EXIT_FAILURE);
        }
    }

    return (void)0;
}

void init_config(void)
{

    gconfig.log_level = DEFAULT_LOG_LEVEL;
    gconfig.no_case   = DEFAULT_CASE_SWITCH;
    gconfig.port      = DEFAULT_PORT;
    gconfig.timeout   = DEFAULT_TIMEOUT;

    snprintf(gconfig.prefix, sizeof(gconfig.prefix), "%s", DEFAULT_PREFIX);
    snprintf(gconfig.hostname, HOST_NAME_LEN, "%s", DEFAULT_HOSTNAME);
    snprintf(gconfig.keyword_file, FILE_PATH_LEN, "%s/data/%s",
        gconfig.prefix, DEFINE_KEYWORD_FILE);

    return;
}

int init_acsm(ACSM_STRUCT **acsm)
{
    int ret = 0;

    FILE *fp = NULL;
    char  line[READ_LINE_BUF_LEN] = {0};
    char *p  = NULL;

    *acsm = acsmNew();
    fp   = fopen(gconfig.keyword_file, "r");
    if (fp == NULL)
    {
        fprintf(stderr,"Open file error: %s\n", gconfig.keyword_file);
        exit(1);
    }

    while (! feof(fp))
    {
        if (NULL == fgets(line, READ_LINE_BUF_LEN - 1, fp))
        {
            continue;
        }

        line[READ_LINE_BUF_LEN - 1] = '\0';
        if (NULL != (p = strstr(line, "\n")))
        {
            *p = '\0';
        }

#if (_DEBUG)
        logprintf("line: [%s]", line);
#endif
        acsmAddPattern(*acsm, (unsigned char *)line, strlen(line), gconfig.no_case);
    }

    return ret;
}

void clear_match_count(void)
{
    ACSM_PATTERN *mlist = NULL;

    mlist = g_vars.acsm->acsmPatterns;
    for (; NULL != mlist; mlist = mlist->next)
    {
        mlist->nmatch = 0;
    }

    return;
}

int filter_process(const char *data, int output_format, struct evbuffer *ev_buf)
{
    assert(NULL != ev_buf);

    int   ret = 0;
    int   hit = 0;
    ACSM_PATTERN * mlist = NULL;

    if (data)
    {
        clear_match_count();
        hit = acsmSearch(g_vars.acsm, (unsigned char *)data, strlen(data), PrintMatch);
        //logprintf("acsmSearch() = %d", ret);
        mlist = g_vars.acsm->acsmPatterns;

        switch (output_format)
        {
        case OUTPUT_AS_JSON:    /** json */
        {
            evbuffer_add_printf(ev_buf, "{\"error\": 0, \"stat\": [");

            int flag = 0;
            for (; NULL != mlist; mlist = mlist->next)
            {
                if (! mlist->nmatch)
                {
                    continue;
                }

                evbuffer_add_printf(ev_buf, "%s{\"keyword\": \"%s\", \"hit_count\": %d}",
                    flag ? ", " : "",
                    mlist->nocase ? mlist->patrn : mlist->casepatrn, mlist->nmatch);
                flag = 1;
            }

            evbuffer_add_printf(ev_buf, "]}");
            break;
        }

        case OUTPUT_AS_HTML:    /** html */
        default:
        {
            evbuffer_add_printf(ev_buf, "<html><head><title>ac server demo</title></head>\
</head><body>\
<div>word: %s</div>\
<div>***filter result***</div>\
<div>",
        data ? data : "No input.");
            for (; NULL != mlist; mlist = mlist->next)
            {
                if (! mlist->nmatch)
                {
                    continue;
                }

                evbuffer_add_printf(ev_buf, "<div>filter-key: %s, hit count: %d</div>",
                    mlist->nocase ? mlist->patrn : mlist->casepatrn, mlist->nmatch);
            }
            evbuffer_add_printf(ev_buf, "</div></body></html>");
            break;
        }
        }
    }

    return ret;
}

int list_keyword(struct evbuffer *ev_buf)
{
    assert(NULL != ev_buf);

    int ret = 0;
    int fd = 0;
    struct stat stat_buf;
    size_t size = 0;
    char read_buf[1024 * 10] = {0};
    ssize_t r_size = 0;

    fd = open(gconfig.keyword_file, O_RDONLY);
    if (-1 == fd || 0 == fd)
    {
        evbuffer_add_printf(ev_buf, "Can NOT open('%s')\n", gconfig.keyword_file);
        ret = -1;
        goto FINISH;
    }

    if (0 != fstat(fd, &stat_buf))
    {
        evbuffer_add_printf(ev_buf, "Can get fstat()\n");
        ret = -1;
        goto STAT_ERR;
    }

    evbuffer_add_printf(ev_buf, "<pre>");
    // stat_buf.st_size;
    size = 0;
    while (size <= stat_buf.st_size)
    {
        lseek(fd, size, SEEK_SET);
        r_size = read(fd, read_buf, sizeof(read_buf) - 1);
        if (-1 == r_size)
        {
            break;
        }
        read_buf[r_size] = '\0';
#if (_DEBUG)
        logprintf("read: [%s]", read_buf);
#endif
        evbuffer_add_printf(ev_buf, "%s", read_buf);

        size += r_size;
        if (0 == r_size)
        {
            break;
        }
    }
    evbuffer_add_printf(ev_buf, "</pre>");

STAT_ERR:
    close(fd);

FINISH:
    return ret;
}

int list_memory_pattern(struct evbuffer *ev_buf)
{
    assert(NULL != ev_buf);

    int ret = 0;
    ACSM_PATTERN *mlist = NULL;

    evbuffer_add_printf(ev_buf, "<pre>\n"
            "---dump memory pattern---\n");
    mlist = g_vars.acsm->acsmPatterns;
    for (; NULL != mlist; mlist = mlist->next)
    {
        mlist->nmatch = 0;
        evbuffer_add_printf(ev_buf, "%s\n", mlist->patrn);
    }
    evbuffer_add_printf(ev_buf, "</pre>");

    return ret;
}

void api_proxy_handler(struct evhttp_request *req, void *arg)
{
    // 初始化返回客户端的数据缓存
    struct evbuffer *buf;
    buf = evbuffer_new();

    /* 分析URL参数 */
    char *decode_uri = strdup((char*) evhttp_request_uri(req));
    struct evkeyvalq http_query;
    evhttp_parse_query(decode_uri, &http_query);

    int   output_format = 0;
    int   do_action = ACTION_FILTER;
    char  post_buf[POST_DATA_BUF_LEN] = {0};
    char *p = NULL;
    char *pt   = NULL;
    char *find = NULL;

#if (_DEBUG)
    logprintf("uri: %s", decode_uri);
#endif

    /** free memory */
    free(decode_uri);

#if (_DEBUG)
#ifdef TAILQ_FOREACH
    //遍历整个uri的对应关系值
    {
        logprintf("--- foreach uri ---");
        struct evkeyval *header;
        TAILQ_FOREACH(header, &http_query, next) {
            logprintf("%s: %s", header->key, header->value);
        }
        logprintf("--- end uri ---");
    }
    //遍历整个请求头.
    {
        logprintf("---- foreach request header ----");
        struct evkeyvalq *input_headers = evhttp_request_get_input_headers(req);
        struct evkeyval *header;
        TAILQ_FOREACH(header, input_headers, next) {
            logprintf("%s: %s", header->key, header->value);
        }
        logprintf("---- end request header ----");
    }
#endif
#endif

    /* 接收 GET 解析参数 */
    const char *word   = evhttp_find_header(&http_query, "word");
    const char *action = evhttp_find_header(&http_query, "action");
    const char *uri_format    = evhttp_find_header(&http_query, "format");

    if (NULL != action)
    {
        if (0 == strncmp(action, "list", 4))
        {
            do_action = ACTION_LIST;
        }
        else if (0 == strncmp(action, "memory", 6))
        {
            do_action = ACTION_MEMORY;
        }
    }

    if (uri_format && 0 == strncmp(uri_format, FORMAT_JSON, sizeof(FORMAT_JSON) - 1))
    {
        output_format = OUTPUT_AS_JSON;
    }

    /** 接受 PSOT 数据 */
    const char *post_data = (char *)EVBUFFER_DATA(req->input_buffer);
    if (post_data)
    {
        snprintf(post_buf, sizeof(post_buf), "%s", post_data);
        logprintf("POST: [%s]", post_buf);
        find = post_buf;
        if (NULL != (find = strstr(post_buf, "word=")))
        {
            find += 5;
            p = find;

            if (NULL != (pt = strchr(p, '&')))
            {
                *pt = '\0';
            }

            word = find;
        }
    }
#if (_DEBUG)
    else
    {
        logprintf("NO POST data.");
    }
#endif

    switch (do_action)
    {
    case ACTION_LIST:
        list_keyword(buf);
        break;

    case ACTION_MEMORY:
        list_memory_pattern(buf);
        break;

    case ACTION_FILTER:
    default:
        // portions handle
        filter_process(word, output_format, buf);
        break;
    }
    //处理输出header头
    evhttp_add_header(req->output_headers, "Content-Type", output_format ?
        "application/json; charset=UTF-8" :
            "text/html; charset=UTF-8");
    evhttp_add_header(req->output_headers, "Status", "200 OK");
    evhttp_add_header(req->output_headers, "Connection", "keep-alive");
    evhttp_add_header(req->output_headers, "Cache-Control", "no-cache");
    evhttp_add_header(req->output_headers, "Expires", "-1");
    evhttp_add_header(req->output_headers, "Server", SERVER_TAG);    /** libevent http server */

    //返回code 200
    evhttp_send_reply(req, HTTP_OK, "OK", buf);

    //释放内存
    evhttp_clear_headers(&http_query);
    evbuffer_free(buf);

    return;
}

