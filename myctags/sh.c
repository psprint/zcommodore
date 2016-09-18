/*
*   $Id: sh.c 443 2006-05-30 04:37:13Z darren $
*
*   Copyright (c) 2000-2002, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions for generating tags for scripts for the
*   Bourne shell (and its derivatives, the Korn and Z shells).
*/

/*
*   INCLUDE FILES
*/
#include "general.h"  /* must always come first */

#include <string.h>

#include "parse.h"
#include "read.h"
#include "routines.h"
#include "vstring.h"

/*
*   DATA DEFINITIONS
*/
typedef enum {
    K_FUNCTION,
    K_VARIABLE
} shKind;

static kindOption ShKinds [] = {
    { TRUE, 'f', "function", "functions"},
    { TRUE, 'v', "variable", "variables"}
};

/*
*   FUNCTION DEFINITIONS
*/

/*  Reject any tag "main" from a file named "configure". These appear in
 *  here-documents in GNU autoconf scripts and will add a haystack to the
 *  needle.
 */
static boolean hackReject (const vString* const tagName)
{
    const char *const scriptName = baseFilename (vStringValue (File.name));
    boolean result = (boolean) (
            strcmp (scriptName, "configure") == 0  &&
            strcmp (vStringValue (tagName), "main") == 0);
    return result;
}

#define MAX_NAMES 20

static void clearNames ( vString *names[] ) {
    int idx = 0;
    while( names[ idx ] ) {
        vStringClear( names[ idx ] );
        ++ idx;
    }
}

static void deleteNames ( vString *names[] ) {
    int idx = 0;
    while( names[ idx ] ) {
        vStringDelete( names[ idx ] );
        names[ idx ] = NULL;
        ++ idx;
    }
}

static int appendNewName( vString *names[], int current_size ) {
    ++ current_size;
    if ( current_size > MAX_NAMES ) {
        return 0;
    }

    // Don't overwrite existing vString
    if( NULL == names[ current_size - 1 ] ) {
        names[ current_size - 1 ] = vStringNew ();
    }
    return 1;
}

static void findShTags (void) {
    vString * names[ MAX_NAMES + 1 ] = { 0 };
    int nidx = 0;
    const unsigned char *line;

    while ((line = fileReadLine ()) != NULL)
    {
        const unsigned char* cp = line;
        boolean functionFound = FALSE, localVariableFound = FALSE;
        nidx = 0;

        if (line [0] == '#')
            continue;

        // Skip white space
        while ( isspace( *cp ) )
            cp++;

        // String 'function ' in text?
        if ( strncmp( (const char*) cp, "function", (size_t) 8 ) == 0  && isspace( (int) cp [8]) ) {
            functionFound = TRUE;
            cp += 8;

            // Skip any whitespaces after 'function '
            while ( isspace( (int) *cp ) )
                ++ cp;

        // String 'local ' in text?
        } else if ( strncmp( (const char*) cp, "local", (size_t) 5 ) == 0  && isspace( (int) cp [5] ) ) {
            localVariableFound = TRUE;
            cp += 5;

            // Skip any whitespaces after 'local '
            while ( isspace( (int) *cp ) )
                ++ cp;
        }

        //
        // LOCAL
        //

        if ( localVariableFound ) {
            // After 'local +' if there's no [[:alnum:]_-] then restart
            if ( ! ( isalnum ( (int) *cp ) || *cp == '_' || *cp == '-' ) )
                continue;

            // Skip any options
            while ( *cp == '-' ) {
                ++ cp;

                // Skip letters of options
                if ( !isalnum( (int) *cp ) )
                    continue;
                while ( isalnum ( (int) *cp ) ) {
                    ++ cp;
                    continue;
                }

                // Skip any whitespace
                while ( isspace ( (int) *cp ) ) {
                    ++ cp;
                    continue;
                }
            }

            // Append new name if it's not already there
            if ( appendNewName( names, nidx ) ) {
                // LOAD [[:alnum:]_-]
                // No actual '-' at the beginning - above skip-options code handled that
                while ( isalnum ((int) *cp)  ||  *cp == '_' || *cp == '-' ) {
                    vStringPut( names[ nidx ], (int) *cp );
                    ++ cp;
                }
                vStringTerminate( names[ nidx ] );
            }
        }

        //
        // FUNCTIONS
        //

        else if ( functionFound || ( !localVariableFound ) ) {
            // After 'function +' if there's no [[:alnum:]_-] then restart
            if ( ! ( isalnum ( (int) *cp ) || *cp == '_' || *cp == '-' ) )
                continue;

            // Append new name if it's not already there
            if ( appendNewName( names, nidx ) ) {
                // LOAD [[:alnum:]_-]
                while ( isalnum ((int) *cp)  ||  *cp == '_' || *cp == '-' ) {
                    vStringPut( names[ nidx ], (int) *cp );
                    ++ cp;
                }
                vStringTerminate( names[ nidx ] );
            }

            // Skip spaces after [[:alnum:]_-]
            while ( isspace( (int) *cp ) )
                ++ cp;

            // Detection of function not necessarily beginning with "function"
            if (*cp++ == '(')
            {
                while ( isspace( (int) *cp ) )
                    ++ cp;
                if ( *cp == ')'  && ! hackReject ( names[ nidx ] ) )
                    functionFound = TRUE;
            }
        }

        // Function found?
        if ( functionFound ) {
            makeSimpleTag( names[ nidx ], ShKinds, K_FUNCTION );
        }
        if ( localVariableFound ) {
            makeSimpleTag( names[ nidx ], ShKinds, K_VARIABLE );
        }

        // Forget the function
        clearNames( names );

    }
    deleteNames( names );
}

extern parserDefinition* ShParser (void)
{
    static const char *const extensions [] = {
        "sh", "SH", "bsh", "bash", "BASH", "ksh", "KSH", "zsh", "Zsh", "ZSH", NULL
    };
    parserDefinition* def = parserNew ("Sh");
    def->kinds      = ShKinds;
    def->kindCount  = KIND_COUNT (ShKinds);
    def->extensions = extensions;
    def->parser     = findShTags;
    return def;
}

/* vi:set tabstop=4 shiftwidth=4: */
