/**
 * 
 * Claim: Java can parse XML faster than C can overflow its buffers.
 *
 * Nothing can parse XML faster than C can overflow its buffers. C is
 * very VERY good at overflowing its buffers. And by good, I mean man
 * it's like nothing you've ever seen before.
 *
 * But we wont go there. Instead, we'll pseudo parse XML with verious
 * assumptions about the sanity of said XML. Break it you can, with
 * very little effort. But in spirit, this will work.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <assert.h>

/**
 * things we can't declare as constants
 */
#define ID_LEN   100
#define NAME_LEN 1000

/**
 * useful constants 
 */
const char LIST_START[]    = "<widgetList>";
const char LIST_END[]      = "</widgetList>";
const char WIDGET_START[]  = "<widget>";
const char WIDGET_END[]    = "</widget>";
const char NAME_START[]    = "<widgetName>";
const char NAME_END[]      = "</widgetName>";
const char ID_START[]      = "<widgetID>";
const char ID_END[]        = "</widgetID>";

const size_t MAX_LEN_TOP     = (1 * 1024 * 1024);
const size_t POOL_SIZE_START = 100;


/**
 * declared states for our, uh, state machine 
 */
enum state_t {
    none              = 1,
    widget_list       = 2,
    widget            = 3
};

/**
 * widget shape
 */
typedef struct {
    char id[ID_LEN];
    char name[NAME_LEN];
} widget_t;

/**
 * for sorting our widgets
 */
static int compare_widgets(const void *a, const void *b) 
{
    return strcmp( ((widget_t*)a)->id, ((widget_t*)b)->id );
}

/**
 * in order to read the xml data in chunks
 */
static size_t read_chunk(int file, char *buffer, char **pos, size_t max)
{
    char *end = buffer + max - 1;
    char *p = buffer;

    if (*pos > buffer) {
        p = buffer + (end - *pos);
        memmove(buffer, *pos, end - *pos);
    }

    *pos = buffer;
    return read(file, p, end - p);
}

/**
 * runability 
 */
int main(int argc, char *argv[])
{
    char *buffer = 0, *pos = 0, *end = 0, *next = 0;
    int file;
    size_t len, max_len, widget_count = 0, pool_size = POOL_SIZE_START;
    enum state_t state = none;
    struct stat file_statistics;
    widget_t *current = 0;
    widget_t *pool;
    char *widget_list_end = 0;

    /**
     * Let us get the data file from the command line, or tell the user
     * how to give it to us.
     */
    if (1 >= argc) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return 0;
    }

    /**
     * Get the file statistics, we'll use its size to determine how much
     * memory to allocate for reading the file. If the file is missing then
     * we want to fail gracefully.
     */
    if (0 != lstat(argv[1], &file_statistics)) {
        fprintf(stderr, "File '%s' was not found.\n", argv[1]);
        return 1;
    }

    /**
     * Determine the size of our read buffer, allocate memory for the read
     * buffer and the widget pool.
     */
    max_len = (file_statistics.st_size > MAX_LEN_TOP ? MAX_LEN_TOP : file_statistics.st_size) + 1;
    pos = buffer = (char*)malloc(max_len);
    memset(buffer, 0, max_len);
    pool = malloc(sizeof(widget_t) * pool_size);
    
    /**
     * Open file and read the first chunk.
     */
    file = open(argv[1], O_RDONLY);
    len = read_chunk(file, buffer, &pos, max_len-1);

    /**
     * Loop, running our state machine until we run out of data.
     */
    while (len > 0) {
        switch (state) {
        case none:
            /** 
             * We don't have any state here so scan for the next widgetList.
             * If we find one, update our state accordingly and update our 
             * position. Otherwise, read the next chunk and try again.
             */

            /* read chunks until we find a LIST_START */
            while (!(next = strstr(pos, LIST_START))) {
                pos = (buffer + max_len) - sizeof(LIST_START) - 1;
                len = read_chunk(file, buffer, &pos, max_len-1);
                if (!len) break;
            }

            state = widget_list;
            pos = next + sizeof(LIST_START) - 1;
            break;

        case widget_list:
            /**
             * We're in a widgetList. Find its end and scan for the next
             * widget. If the next widget is within this widgetList (positioned
             * before the end of the widgetList) then we use it. If not then
             * we'll assume we are done and revert our state back to none. If
             * we don't find either a widget or the end of the widgetList then
             * we need to read the next chunk and try again.
             */
            if (!widget_list_end) {
                widget_list_end = strstr(pos, LIST_END);
            }

            /**
             * find the next widget
             */
            next = strstr(pos, WIDGET_START);
            if (next && (!widget_list_end || widget_list_end > next)) {
                widget_count++;

                /**
                 * double our widget pool size if we exceed its current 
                 * capacity
                 */
                if (widget_count >= pool_size) {
                    pool_size <<= 1; 
                    pool = (widget_t*)realloc(pool, sizeof(widget_t) * pool_size);
                }
                current = &pool[widget_count - 1];
                state = widget;
                pos = next + sizeof(WIDGET_START);
                break;
            }

            /**
             * if the list end was found, move buffer position up and revert state 
             * back to none
             */
            if (widget_list_end) {
                state = none;
                pos = end + sizeof(LIST_END);
                widget_list_end = 0;
                break;
            }

            len = read_chunk(file, buffer, &pos, max_len-1);
            break;

        case widget:
            /**
             * let us make an assumption: an entire widget will fit in our buffer.
             * if not, we'll end up getting only tail of the widgets that did.
             */
            while (!(end = strstr(pos, WIDGET_END))) {
                len = read_chunk(file, buffer, &pos, max_len-1);
                if (!len) break;
            }

            /**
             * capture the id and name
             */
            char *id = strstr(pos, ID_START);
            char *name = strstr(pos, NAME_START);

            if (id && (end > id)) {
                /* id found, use it */
                id += strlen(ID_START);
                char *endofit = (strstr(id, ID_END) - 1);

                /* trim it */
                while (isspace(*id)) ++id;
                while (isspace(*endofit)) *endofit--=0;

                /* store it */
                strncpy(current->id, id, endofit-id+1);
            }

            if (name && (end > name)) {
                /* name found, use it */
                name += strlen(NAME_START);
                char *endofit = (strstr(name, NAME_END) - 1);

                /* trim it */
                while (isspace(*name)) ++name;
                while (isspace(*endofit)) *endofit--=0;

                /* store it */
                strncpy(current->name, name, endofit-name+1);
            }

            state = widget_list;
            pos = end + sizeof(WIDGET_END);
            break;
        }
    }

    /* sort them */
    qsort(pool, widget_count, sizeof(widget_t), compare_widgets);

    /* spit em out */
    for (len=0; len<widget_count; ++len) {
        puts(pool[len].name);
    }

    /* cleanse the soul */
    close(file);
    free(buffer);
    free(pool);

    return 0;
}

