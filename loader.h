/* defines */

#define MAXFLDLEN 1024
#define MAXFLDCNT 200

/* + Configuration variables */

const char *host, *user, *pass, *database, *table, *dbcharset; int port;
const char *delimiter, *fields, *incharset, *error_log, *raw_log; int skip_head, total_rows, ttruncate, transaction_limit;

/* - Configuration variables */

static int put_comma;
int fieldCount = 0;
int verboseLevel = 8;
char * inputFile;
char * configFile = "pgcloader.cfg";
char * configSection = "default\0";

PGconn *conn;

FILE *error;
FILE *raw;

typedef unsigned long long ticks;

/* allowed fields for import */

int allowedFieldsCount=0;
char allowedFields[MAXFLDCNT][MAXFLDLEN]={0x0};
char currentFields[MAXFLDCNT][MAXFLDLEN]={0x0};

/* function prototypes */

// pgcloader.c
void fieldHandler ( void *s, size_t i, void *p );
void eolHandler   ( int c, void *p );

// util.c
void initConfig  ();
int  initOptions ( int argc, char *argv[] );
void explode     ( char *record, char *delim, char arr[][MAXFLDLEN],int *fldcnt);
int  isString    ( char * str );
static __inline__ ticks getticks (void);

// import.c
void importFromSTDIN ();
void importFromFile ();

