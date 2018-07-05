void importFromSTDIN ()
{
    struct csv_parser p;
    int i, x;
    char c;

    csv_init(&p, 0);

    while ( ( i = getc ( stdin ) ) != EOF )
    {
        c = i;
        if ( csv_parse (&p, &c, 1, fieldHandler, eolHandler, NULL ) != 1 )
        {
            pprintf ( 8, "Error: %s\n", csv_strerror ( csv_error ( &p ) ) );

            exit ( EXIT_FAILURE );
        }

        for ( x = 0; x < allowedFieldsCount; x++ )
        {
            if ( isString ( currentFields[x] ) )
            {
                printf ( "'%s' ", currentFields[x] );
            }
            else
            {
                printf ( "%s ", currentFields[x] );
            }
        }

        printf ( "\n" );
    }

    csv_fini(&p, fieldHandler, eolHandler, NULL);
    csv_free(&p);
}

void importFromFile ( FILE * file )
{
    struct csv_parser p;

    ticks tick;
    unsigned time = 0;

    int i, x, lineno = 0, transline = 1;
    int aTransactions = 0, bTransactions = 0, gTransactions = 0;

    char c;
    char * field  = malloc ( (size_t) MAXFLDLEN );

    char * row     = malloc ( (size_t) MAXFLDLEN );
    char * rtmp;
    //char * query  = malloc ( (size_t) (long long)( ( MAXFLDLEN * MAXFLDCNT ) * transaction_limit ) );
    char * query = malloc ( (size_t) (long long) ( MAXFLDLEN * transaction_limit ) );
    char * qtmp;

    rtmp = row;
    qtmp = query;

    tick = getticks();

    csv_init(&p, 0);

    if ( skip_head > 0 )
    {
        pprintf ( 2, "Skipping %d of rows...", skip_head );
    }

    while ( ( i = getc ( file ) ) != EOF )
    {
        c = i;

        if ( lineno >= skip_head )
        {
            if ( csv_parse (&p, &c, 1, fieldHandler, eolHandler, NULL ) != 1 )
            {
                pprintf ( 8, "Error: %s\n", csv_strerror ( csv_error ( &p ) ) );

                exit ( EXIT_FAILURE );
            }
        }

        if ( c == '\n' )
        {
            lineno++;

            if ( lineno > skip_head )
            {

                for ( x = 0; x < allowedFieldsCount; x++ )
                {
                    if ( x < allowedFieldsCount - 1 )
                    {
                        if ( isString ( currentFields[x] ) )
                            sprintf ( field, "'%s',\0", currentFields[x] );
                        else if ( strlen ( currentFields[x] ) == 0 )
                            sprintf ( field, "'',\0" );
                        else
                            sprintf ( field, "%s,\0", currentFields[x] );
                    }
                    else
                    {
                        if ( isString ( currentFields[x] ) )
                            sprintf ( field, "'%s'\0", currentFields[x] );
                        else if ( strlen ( currentFields[x] ) == 0 )
                            sprintf ( field, "''\0" );
                        else
                            sprintf ( field, "%s\0", currentFields[x] );
                    }

                    strncpy ( rtmp, field, strlen ( field ) ); rtmp += strlen ( field );
                }
                strncpy ( rtmp, "\0",1 );
                rtmp = row;

                if ( transline == 1 )
                {
                    strncpy ( qtmp, "insert into ", 12 ); qtmp += 12;
                    strncpy ( qtmp, table, strlen ( table ) ); qtmp += strlen ( table );
                    strncpy ( qtmp, " values ", 8 ); qtmp += 8;
                }

                strncpy ( qtmp, "(", 1 ); qtmp += 1;
                strncpy ( qtmp, row, strlen ( row ) ); qtmp += strlen ( row );

                if ( transline < transaction_limit )
                {
                    strncpy ( qtmp, " ),", 3 ); qtmp += 3;
                }
                else
                {
                    strncpy ( qtmp, " );\n\0", 5 ); qtmp += 5;
                    qtmp = query;
                }

                if ( transline >= transaction_limit )
                {
                    time  = (unsigned)( ( getticks() - tick ) / 1662543 );

                    aTransactions++;

                    //printf ( " ]] %s\n", query );

                    if ( dbquery ( query ) != 0 )
                    {
                        bTransactions++;
                        //fprintf ( raw, "==== RECORD: %d ====\n%s\n", recordcnt, iline );
                        //fflush ( raw );
                        printf ( "error\n" );
                    }
                    else
                    {
                        gTransactions++;
                        pprintf ( 1, "Part %d / %d records loaded > commit in %f seconds", aTransactions, transaction_limit, (float)time / 1000 + (time % 1000 >= 500 ? 1 : 0) );
                    }

                    transline = 0;

                    tick = getticks();
                }

                transline++;

                if ( total_rows > 0 && lineno >= ( skip_head + total_rows ) )
                {
                    break;
                }

            }
        }
    }

    pprintf ( 1, "" );
    pprintf ( 1, "==== SUMMARY ====\n" );
    pprintf ( 1, "All transactions:\t%d", aTransactions );
    pprintf ( 1, "Success transactions:\t%d", gTransactions );
    pprintf ( 1, "Failed transactions:\t%d", bTransactions );

    csv_fini(&p, fieldHandler, eolHandler, NULL);
    csv_free(&p);
}
