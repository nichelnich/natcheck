#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "pjnath.h"
#include "pjlib-util.h"
#include "pjlib.h"

#define THIS_FILE	"main.c"

static struct global
{
    pj_caching_pool	 cp;
    pj_pool_t		*pool;
    pj_stun_config	 stun_config;
    pj_thread_t		*thread;
    pj_bool_t		 quit;

    pj_dns_resolver	*resolver;
} g;

static struct options
{
    char	*srv_addr;
    char	*srv_port;
} o;

#define DEFAULT_PORT    "3478"

static void my_perror(const char *title, pj_status_t status)
{
    char errmsg[PJ_ERR_MSG_SIZE];
    pj_strerror(status, errmsg, sizeof(errmsg));

    PJ_LOG(3,(THIS_FILE, "%s: %s", title, errmsg));
}

#define CHECK(expr)	status=expr; \
			if (status!=PJ_SUCCESS) { \
			    my_perror(#expr, status); \
			    return status; \
			}



static int worker_thread(void *unused)
{
    PJ_UNUSED_ARG(unused);

    while (!g.quit) {
    const pj_time_val delay = {0, 10};

    /* Poll ioqueue for the TURN client */
    pj_ioqueue_poll(g.stun_config.ioqueue, &delay);

    /* Poll the timer heap */
    pj_timer_heap_poll(g.stun_config.timer_heap, NULL);

    }

    return 0;
}

static int init()
{
    pj_status_t status;

    pj_log_set_decor(PJ_LOG_HAS_TIME | PJ_LOG_HAS_NEWLINE);
    pj_log_set_level(5);

    CHECK( pj_init() );
    CHECK( pjlib_util_init() );
    CHECK( pjnath_init() );

    /* Check that server is specified */
    if (!o.srv_addr) {
    printf("Error: server must be specified\n");
    return PJ_EINVAL;
    }

    pj_caching_pool_init(&g.cp, &pj_pool_factory_default_policy, 0);

    g.pool = pj_pool_create(&g.cp.factory, "main", 1000, 1000, NULL);

    /* Init global STUN config */
    pj_stun_config_init(&g.stun_config, &g.cp.factory, 0, NULL, NULL);

    /* Create global timer heap */
    CHECK( pj_timer_heap_create(g.pool, 1000, &g.stun_config.timer_heap) );

    /* Create global ioqueue */
    CHECK( pj_ioqueue_create(g.pool, 16, &g.stun_config.ioqueue) );


    /* Start the worker thread */
    CHECK( pj_thread_create(g.pool, "stun", &worker_thread, NULL, 0, 0, &g.thread) );


    return PJ_SUCCESS;
}


void nat_detect_cb(void *user_data,
                   const pj_stun_nat_detect_result *res)
{
    if(PJ_SUCCESS != res->status) {
        PJ_LOG(3, (THIS_FILE, "-=-=-=-=-==-=-=-=-=result error:%s", res->status_text));
    } else {
        PJ_LOG(3, (THIS_FILE, "-=-=-=-=-=-=-=-=-=-NAT type:%d :%s", res->nat_type, res->nat_type_name));
    }
    g.quit = 1;
}

static void task_main(void)
{
    pj_status_t status;
    pj_sockaddr_in server;
    pj_hostent he;
    pj_str_t host = pj_str(o.srv_addr);
    pj_uint16_t port = atoi(o.srv_port); //FIXME
    pj_str_t addr;
    
    status = pj_gethostbyname( &host, &he);
    if (status != PJ_SUCCESS) {
        PJ_LOG(1, (THIS_FILE, "get hostbyname error"));
	    return ;
    }

    addr = pj_str(pj_inet_ntoa(*((pj_in_addr *)he.h_addr)));


    PJ_LOG(3, (THIS_FILE, "gethotbyname:%s", addr.ptr));
    PJ_LOG(3, (THIS_FILE, "stun server port:%d", port));

    pj_sockaddr_in_init(&server, &addr, port);
    
    pj_stun_detect_nat_type (&server, &g.stun_config, NULL, nat_detect_cb);
}

static int client_shutdown()
{
    if (g.thread) {
        pj_thread_join(g.thread);
        pj_thread_destroy(g.thread);
        g.thread = NULL;
    }
    
    if (g.stun_config.timer_heap) {
        pj_timer_heap_destroy(g.stun_config.timer_heap);
        g.stun_config.timer_heap = NULL;
    }

    if (g.stun_config.ioqueue) {
        pj_ioqueue_destroy(g.stun_config.ioqueue);
        g.stun_config.ioqueue = NULL;
    }

    if (g.pool) {
        pj_pool_release(g.pool);
        g.pool = NULL;
    }

    pj_pool_factory_dump(&g.cp.factory, PJ_TRUE);
    pj_caching_pool_destroy(&g.cp);

    return PJ_SUCCESS;
}

void signal_handler(int sig)
{
    g.quit = 1;
}

static void usage(void)
{
    puts("Usage: mynatcheck STUN-SERVER [OPTIONS]");
    puts("");
    puts("where STUN-SERVER is \"host[:port]\"");
    puts("");
    puts("and OPTIONS:");
    puts(" --help, -h");
}

int main(int argc, char *argv[])
{
    struct pj_getopt_option long_options[] = {
    { "help",       0, 0, 'h'},
    };
    int c, opt_id;
    char *pos;
    pj_status_t status;

    while((c=pj_getopt_long(argc,argv, "r:u:p:N:hFT", long_options, &opt_id))!=-1) {
    switch (c) {
    case 'h':
        usage();
        return 0;
    default:
        printf("Argument \"%s\" is not valid. Use -h to see help",
           argv[pj_optind]);
        return 1;
    }
    }

    if (pj_optind == argc) {
        puts("Error: TARGET is needed");
        usage();
        return 1;
    }

    if ((pos=pj_ansi_strchr(argv[pj_optind], ':')) != NULL) {
        o.srv_addr = argv[pj_optind];
        *pos = '\0';
        o.srv_port = pos+1;
    } else {
        o.srv_addr = argv[pj_optind];
        o.srv_port = DEFAULT_PORT;
    }

    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_handler = signal_handler;

    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGKILL, &act, NULL);
    
    if ((status=init()) != 0) {
        goto on_return;
    }

    task_main();

on_return:
    client_shutdown();
    return status ? 1 : 0; 
}


