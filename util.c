/**
 * Messages:
 *
 * 1 - INFO
 * 2 - NOTICE
 * 4 - WARNING
 * 8 - ERROR
 * 16 - DEBUG
 */

void pprintf ( int level, char *fmt, ... )
{
    if ( level > verboseLevel )
        return;

    va_list argp;

    char *tlevel;
    switch ( level )
    {
        case 1:
            tlevel = "INFO";
            break;
        case 2:
            tlevel = "NOTICE";
            break;
        case 4:
            tlevel = "WARNING";
            break;
        case 8:
            tlevel = "ERROR";
            break;
        case 16:
            tlevel = "DEBUG";
            break;
    }

    fprintf  ( stderr, " >> %s: ", tlevel );

    va_start ( argp, fmt );
    vfprintf ( stderr, fmt, argp );
    va_end   ( argp );

    fprintf  ( stderr, " \n" );
}

/**
 * Reading configuration file - file path is hardcoded
 */

void pgc_config_lookup_string ( const config_t *config, const char *path, const char **value )
{
    size_t size = 7 + strlen ( configSection ) + strlen ( path );
    char * variable = malloc ( size+1 );
    const char * rValue;

    strncpy ( variable, "pgcl.\0", 6 );
    strncat ( variable, configSection, strlen ( configSection ) );
    strncat ( variable, ".", 1 );
    strncat ( variable, path, strlen ( path ) );
    strncat ( variable, "\0", 1 );

    if ( config_lookup_string    ( config, variable, &rValue ) )
    {
        *value = rValue;
    }
    else
    {
        pprintf ( 8, "Section '%s' or variable '%s ( path %s )' not found in '%s' config file.", configSection, path, variable, configFile );
        exit(EXIT_FAILURE);
    }
}

void pgc_config_lookup_int ( const config_t *config, const char *path, int *value )
{
    size_t size = 7 + strlen ( configSection ) + strlen ( path );
    char * variable = malloc ( size +1 );
    int rValue;

    strncpy ( variable, "pgcl.\0", 6 );
    strncat ( variable, configSection, 7 );
    strncat ( variable, ".", 1 );
    strncat ( variable, path, strlen ( path ) );
    strncat ( variable, "\0", 1 );

    if ( config_lookup_int    ( config, variable, &rValue ) )
    {
        *value = rValue;
    }
    else
    {
        pprintf ( 8, "Section '%s' or variable '%s ( path %s )' not found in '%s' config file.", configSection, path, variable, configFile );
        exit(EXIT_FAILURE);
    }
}

void initConfig ( char * cfgFile )
{
    struct config_t cfg;

    config_init(&cfg);

    if ( !config_read_file ( &cfg, cfgFile ) )
    {
        pprintf ( 8, "Config file error");
        exit(EXIT_FAILURE);
    }

    config_lookup_string ( &cfg, "pgcl.server.host", &host );
    config_lookup_int    ( &cfg, "pgcl.server.port", &port );
    config_lookup_string ( &cfg, "pgcl.server.user", &user );
    config_lookup_string ( &cfg, "pgcl.server.pass", &pass );
    config_lookup_string ( &cfg, "pgcl.server.database", &database );
    config_lookup_string ( &cfg, "pgcl.server.table", &table );
    config_lookup_string ( &cfg, "pgcl.server.charset", &dbcharset );

    pgc_config_lookup_int    ( &cfg, "skip_head", &skip_head );
    pgc_config_lookup_int    ( &cfg, "total_rows", &total_rows );
    pgc_config_lookup_string ( &cfg, "delimiter", &delimiter );
    pgc_config_lookup_string ( &cfg, "fields", &fields );
    pgc_config_lookup_int    ( &cfg, "truncate", &ttruncate );
    pgc_config_lookup_string ( &cfg, "charset", &incharset );
    pgc_config_lookup_int    ( &cfg, "transaction_limit", &transaction_limit );
    pgc_config_lookup_string ( &cfg, "errorlog", &error_log );
    pgc_config_lookup_string ( &cfg, "rawlog", &raw_log );

    pprintf ( 1, ""   );
    pprintf ( 1, "=============================="   );
    pprintf ( 1, "  Database bulk loader v0.2   "   );
    pprintf ( 1, "==============================\n" );
    pprintf ( 1, "host: %s", host );
    pprintf ( 1, "port: %d", port );
    pprintf ( 1, "user: %s", user );
    pprintf ( 1, "pass: ****" );
    pprintf ( 1, "database: %s", database );
    pprintf ( 1, "table: %s\n", table );
    pprintf ( 1, "skip header: %d", skip_head );
    pprintf ( 1, "total rows: %d", total_rows );
    pprintf ( 1, "delimiter: %s", delimiter );
    pprintf ( 1, "fields: %s\n", fields );

    explode ( (char*)fields, ",", allowedFields, &allowedFieldsCount ); // get allowed fields - comma is not a CSV delimiter here!

    dbconnect ();

    config_destroy ( &cfg );
}

/**
 * Reading inline optioms
 */

int initOptions ( int argc, char *argv[] )
{
    int aflag = 0;
    int bflag = 0;
    char *cvalue = NULL;
    int index;
    int c;

    opterr = 0;

    while ( ( c = getopt ( argc, argv, "c:s:v:" ) ) != -1 )
    {
       switch (c)
       {
           case 'c':
               configFile = optarg;
               break;
           case 's':
               configSection = optarg;
               break;
           case 'v':
               verboseLevel = atoi ( optarg );
               break;
           case '?':
               if ( optopt == 'c' )
               {
                   pprintf ( 8, "Option -%c requires an argument.", optopt );
                   exit(EXIT_FAILURE);
               }
               else if ( isprint ( optopt ) )
               {
                   pprintf ( 8, "Unknown option `-%c'.", optopt );
                   exit(EXIT_FAILURE);
               }
               else
               {
                   pprintf ( 8, "Unknown option character `\\x%x'.", optopt );
                   exit(EXIT_FAILURE);
               }
             return 1;
           default:
             abort ();
       }
    }

    pprintf ( 16, "aflag = %d, bflag = %d, cvalue = %s", aflag, bflag, cvalue );

    int opts = 0;

    for ( index = optind; index < argc; index++ )
    {
        pprintf ( 16, "Non-option argument %s", argv[index] );
    }

    inputFile = argv [ argc - 1 ];
}

/**
 * Exploding a string into array with given delimiter
 */

void explode ( char *record, char *delim, char arr[][MAXFLDLEN],int *fldcnt)
{
    char*p=strtok(record,delim);
    int fld=0;

    while(p)
    {
        strcpy(arr[fld],p);
        fld++;
        p=strtok('\0',delim);
    }
    *fldcnt=fld;
}

/**
 * This function can be modified to return INT, FLOAT or STRING
 */

int isString ( char * str )
{
    int idx;
    int STRINGLENGTH = strlen ( str );
    int numbersfound = 0;
    int dotsfound = 0;

    for (idx=0; (idx<STRINGLENGTH) && (str[idx]); idx++)
    {
        numbersfound += ((str[idx] >= '0') && (str[idx] <= '9'));
        dotsfound += (str[idx] == '.');
    }

    if (numbersfound == STRINGLENGTH) { }
    else if ((dotsfound == 1) && (numbersfound == STRINGLENGTH-1)) { }
    else return 1;

    return 0;
}

int dbquery ( const char* query )
{
    PGresult *res;

    res = PQexec( conn, query );

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf (stderr, "%s query failed: %s\n", query, PQerrorMessage(conn)); // setup into config file?
        //fprintf ( error , "%s query failed: %s\n", query, PQerrorMessage(conn));

        //fflush  ( error );
        PQclear (res);

        //PQexec( conn, "commit" );
        //PQexec( conn, "begin" );

        return -1;
    }

    PQclear(res);
    return 0;
}

int dbconnect ()
{
    char * dsn = malloc ( 300 );
    sprintf ( dsn, "host=%s port=%d dbname=%s user=%s password=%s", host, port, database, user, pass );

    conn = PQconnectdb( dsn ); // connection to database

    free ( dsn );

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf ( stderr, "Connection to database failed: %s > %s", PQerrorMessage ( conn ), dsn );
        return 0;
    }

    PQsetnonblocking ( conn, 1 );

    if ( ttruncate == 1 )
    {
        char * truncquery = malloc ( strlen ( table ) + 10 );
        sprintf ( truncquery, "truncate %s", table );
        PQexec( conn, truncquery );
        pprintf ( 2, "Truncating table %s\n", table );
        free ( truncquery );
    }

    //PQexec( conn, "begin" ); // start first transaction
}

static __inline__ ticks getticks(void)
{
     unsigned a, d;
     asm("cpuid");
     asm volatile("rdtsc" : "=a" (a), "=d" (d));

     return (((ticks)a) | (((ticks)d) << 32));
}

