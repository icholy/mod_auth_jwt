/* Include the required headers from httpd */
#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_protocol.h"
#include "http_request.h"

#include <jansson.h>
#include <libjosec.h>
#include "cookies.h"

#define bool int
#define true 1
#define false 0

/* Define prototypes of our functions in this module */

typedef struct {
    const char *claim_name;
    const char *cookie_name;
    const char *key; 
    bool   claim_name_is_set;
    bool   cookie_name_is_set;
    bool   key_is_set;
    size_t key_length;
} example_config;

const char *example_set_key(cmd_parms *cmd, void *cfg, const char *arg);
const char *example_set_cookie_name(cmd_parms *cmd, void *cfg, const char *arg);
const char *example_set_claim_name(cmd_parms *cmd, void *cfg, const char *arg);
static int example_verify_jwt(const char *jwt, example_config *config);
static int example_handler(request_rec *r);
static void register_hooks(apr_pool_t *pool);
static void *create_dir_conf(apr_pool_t *pool, char *context);

static const command_rec example_directives[] = 
{
    AP_INIT_TAKE1("exampleKey", example_set_key, NULL, ACCESS_CONF, "Set the HS256 key"),
    AP_INIT_TAKE1("exampleCookieName", example_set_cookie_name, NULL, ACCESS_CONF, "Cookie name"),
    AP_INIT_TAKE1("exampleClaimName", example_set_claim_name, NULL, ACCESS_CONF, "Claim name"),
    { NULL }
};

module AP_MODULE_DECLARE_DATA   example_module =
{
    STANDARD20_MODULE_STUFF,
    create_dir_conf,    // Per-directory configuration handler
    NULL,               // Merge handler for per-directory configurations
    NULL,               // Per-server configuration handler
    NULL,               // Merge handler for per-server configurations
    example_directives, // Any directives we may have for httpd
    register_hooks      // Our hook registering function
};

void *create_dir_conf(apr_pool_t *pool, char *context)
{
  context = context ? context : "(undefined context)";
  example_config *config = apr_palloc(pool, sizeof(example_config));
  if (config) {
    config->claim_name = "name";
    config->claim_name_is_set = false;
    config->cookie_name = "jwt";
    config->cookie_name_is_set = false;
    config->key = "";
    config->key_length = 0;
    config->key_is_set = false;
  }
  return config;
}

const char *example_set_key(cmd_parms *cmd, void *cfg, const char *arg)
{
    example_config *config = (example_config*)cfg;
    config->key = arg;
    config->key_length = strlen(arg);
    config->key_is_set = true;
    return NULL;
}

const char *example_set_cookie_name(cmd_parms *cmd, void *cfg, const char *arg)
{
    example_config *config = (example_config*)cfg;
    config->cookie_name = arg; 
    config->cookie_name_is_set = true;
    return NULL;
}

const char *example_set_claim_name(cmd_parms *cmd, void *cfg, const char *arg)
{
    example_config *config = (example_config*)cfg;
    config->claim_name = arg;
    config->claim_name_is_set = true;
    return NULL;
}

static int example_verify_jwt(const char *jwt, example_config *config) 
{
    int rc = 0;

    jose_context_t ctx;
    if (jose_create_context(&ctx, NULL, NULL, NULL)) {
        rc = 1;
        goto OUT;
    }

    jose_key_t key;
    key.alg_type = HS256;
    key.key = strdup(config->key);
    key.k_len = config->key_length;

    if (jose_add_key(&ctx, key)) {
        rc = 1;
        goto CLOSE_CONTEXT;
    }

    rc = jwt_verify_sig(&ctx, jwt, HS256);

CLOSE_CONTEXT:
    jose_close_context(&ctx);
OUT:
    return rc;
}

static int example_show_cookies(void *rec, const char *key, const char *value) {
  request_rec *r = (request_rec*)rec;
  ap_rprintf(r, "Key: %s, Value: %s, ", key, value);

  // zero stops iterating
  return 1;
}

/* The handler function for our module.
 * This is where all the fun happens!
 */
static int example_handler(request_rec *r)
{

    /* First off, we need to check if this is a call for the "example" handler.
     * If it is, we accept it and do our things, it not, we simply return DECLINED,
     * and Apache will try somewhere else.
     */
    if (!r->handler || strcmp(r->handler, "example-handler")) return (DECLINED);

    example_config *config = (example_config*) ap_get_module_config(r->per_dir_config, &example_module);
    if (!config) {
      goto OUT;
    }

    // set the content type
    ap_set_content_type(r, "application/json");

    // const char* foo = apr_pstrdup(r->pool, "this is a test");

    const char *cookies_text = apr_table_get(r->headers_in, "cookie");
    if (!cookies_text) {
        goto OUT;
    }

    const char *jwt_text = cookies_lookup(cookies_text, config->cookie_name, r->pool);
    if (!jwt_text) {
        goto OUT;
    }

    // Get the JWT from the cookie
    /* const char *jwt = cookies_lookup(cookies, config.cookie_name, r->pool); */
    /* if (!jwt) { */
    /*     goto OUT; */
    /* } */

    if (example_verify_jwt(jwt_text, config)) {
        ap_rprintf(r, "Not Authenticated");
    } else {
        ap_rprintf(r, "Is Authenticated");
    }

    json_t *head, *claims, *name;

    /* if (jwt_split(jwt, &head, &claims)) { */
    /*     goto OUT; */
    /* } */
    /*  */
    /* if (!json_is_object(claims)) { */
    /*     goto FREE_JSON; */
    /* } */
    /*  */
    /* name = json_object_get(claims, config.claim_name); */
    /* if (!json_is_string(name)) { */
    /*     goto FREE_JSON; */
    /* } */
    /*  */
    /* ap_rprintf(r, "%s is Authenticated", json_string_value(name)); */

FREE_JSON:

    /* json_decref(head); */
    /* json_decref(claims); */

OUT:

    return OK;
}


/* register_hooks: Adds a hook to the httpd process */
static void register_hooks(apr_pool_t *pool) 
{
    /* Hook the request handler */
    ap_hook_handler(example_handler, NULL, NULL, APR_HOOK_LAST);
}
