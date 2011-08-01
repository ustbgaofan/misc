#include <stdio.h>
#include <zmq.h>
#include "c60_internal.h"
#include "utstring.h"

int c60_load_map(c60_t *c60, char *file, UT_string *err) {
  int rc = -1, lines=0;
  unsigned start, end, i;
  char line[100], cmd[100], uri[100]; /* TODO safer parse */
  FILE *f=NULL;
  void *s;

  if ( (f = fopen(file,"r")) == NULL) {
    utstring_printf(err,"can't open %s: %s\n", file, strerror(errno));
    goto done;
  }
 
  while (fgets(line,sizeof(line),f) != NULL) {
    lines++;
    if (sscanf(line,"%s %u %u %s", cmd, &start, &end, uri) != 4) {
      utstring_printf(err,"%s: syntax error at line %u\n", file, lines);
      goto done;
    }
    if (strcmp(cmd, "assign")) {
      utstring_printf(err,"%s: unknown command at line %u\n", file, lines);
      goto done;
    }
    if (((start < 0) || (start >= C60_NUM_BUCKETS)) || 
        ((end < 0)   || (end >= C60_NUM_BUCKETS)) || 
        (end < start)) {
      utstring_printf(err,"%s: invalid range at line %u\n", file, lines);
      goto done;
    }
    if ( (s = zmq_socket(c60->zcontext, ZMQ_PUSH)) == NULL) {
      utstring_printf(err,"%s: can't connect to %s: %s\n", file, uri,
        zmq_strerror(errno));
      goto done;
    }
    if (zmq_connect(s, uri) != 0) {
      utstring_printf(err,"%s: can't connect to %s: %s\n", file, uri,
        zmq_strerror(errno));
      goto done;
    }
    for(i=start; i<=end; i++) {
      c60->bucket[i].socket = s;
    }
  }
  for(i=0; i<C60_NUM_BUCKETS; i++) {
    if (c60->bucket[i].socket == NULL) {
      utstring_printf(err,"%s: unassigned bucket: %u\n", file, i);
      goto done;
    }
  }
  rc = 0; /* success */

 done:
  if (f) fclose(f);
  if (rc == -1) {
    /* TODO deep free c60 but leave outer structure for caller to free */
  }
  return rc;
}
