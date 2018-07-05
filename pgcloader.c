#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

/* external libraries required by pgcloader */

#include "libs/libconfig/libconfig.h"
#include "libs/libcsv/csv.h"
#include <libpq-fe.h>
#include <iconv.h>

/* internals */

#include "loader.h"
#include "util.c"
#include "import.c"

/**
 * CSV_PARSE field callback function
 */

void fieldHandler ( void *s, size_t i, void *p )
{
    fieldCount++;

    char *line = malloc ( i+1 );
    int c,y = 0;

    for ( c = 0; c < allowedFieldsCount; c++ )
    {
        if ( atoi ( allowedFields[c] ) == fieldCount ) // checking if current field is on allowed fields list
        {
            snprintf ( line, i+1, "%s\0", s );
            strcpy ( currentFields[c], line );
        }
    }

    free ( line );
}

/**
 * CSV_PARSE end of line callback function
 */

void eolHandler (int c, void *p)
{
    fieldCount = 0;
    //printf ( "\n" );
}

int main ( int argc, char *argv[] )
{
    initOptions ( argc, argv );
    initConfig  ( configFile );

    pprintf ( 1, "Using config file %s", configFile );

    if ( isatty ( fileno ( stdin ) ) )
    {
        FILE * file = fopen ( inputFile, "r" );
        pprintf ( 1, "File to process: %s\n", inputFile );

        importFromFile ( file );
    }
    else
    {
        pprintf ( 2, "File to process: STDIN\n" );

        importFromSTDIN ();
    }

    return EXIT_SUCCESS;
}
