#include <strings.h>
#include <apr_pools.h>
#include <apr_strings.h>

typedef struct {
    const char *header;
    const char *claims;
    const char *signature;
} jwt_parts_t;

jwt_parts_t *jwt_split(const char *jwt_text, apr_pool_t *pool)
{
    char *text = apr_pstrdup(pool, jwt_text);
    if (!text) {
        return NULL;
    }

    const char *text_end = (char*)(text + strlen(text));
    char *first_dot;
    char *second_dot; 

    // find the first dot
    first_dot = strchr(text, '.');

    // make sure there's more stuff after it
    if (!first_dot || first_dot + 1 >= text_end) {
        return NULL;
    }

    // find the second dot
    second_dot = strchr(first_dot + 1, '.');

    // make sure there's more stuff after it
    if (!second_dot || second_dot + 1 >= text_end) {
        return NULL;
    }

    // These will act as the string terminators
    first_dot[0] = 0x00;
    second_dot[0] = 0x00;

    jwt_parts_t *parts = apr_palloc(pool, sizeof(jwt_parts_t));
    if (!parts) {
        return NULL;
    }

    parts->header = text;
    parts->claims = (char*)(first_dot + 1);
    parts->signature = (char*)(second_dot + 1);

    return parts;
}
