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
        while ( isspace( (int) *cp ) )
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
        } else if ( strncmp( (const char*) cp, "float", (size_t) 5 ) == 0  && isspace( (int) cp [5] ) ) {
            localVariableFound = TRUE;
            cp += 5;
        } else if ( strncmp( (const char*) cp, "integer", (size_t) 7 ) == 0  && isspace( (int) cp [7] ) ) {
            localVariableFound = TRUE;
            cp += 7;
        } else if ( strncmp( (const char*) cp, "typeset", (size_t) 7 ) == 0  && isspace( (int) cp [7] ) ) {
            localVariableFound = TRUE;
            cp += 7;
        } else if ( strncmp( (const char*) cp, "declare", (size_t) 7 ) == 0  && isspace( (int) cp [7] ) ) {
            localVariableFound = TRUE;
            cp += 7;
        } else if ( strncmp( (const char*) cp, "readonly", (size_t) 8 ) == 0  && isspace( (int) cp [8] ) ) {
            localVariableFound = TRUE;
            cp += 8;
        }

        if ( localVariableFound ) {
            // Skip any whitespaces after 'local '
            while ( isspace( (int) *cp ) )
                ++ cp;
        }

        //
        // LOCAL
        //

        if ( localVariableFound ) {
            // After 'local ##' if there's no [[:alnum:]_+-] then next line
            if ( ! ( isalnum( (int) *cp ) || *cp == '_' || *cp == '-' || *cp == '+' ) )
                continue;

            // This is in fact to detect ; or end of line
            while ( isalnum( (int) *cp ) || *cp == '_' || *cp == '-' || *cp == '+' ) {
                // Skip any options
                while ( *cp == '-' || *cp == '+' ) {
                    ++ cp;

                    // Skip letters of options
                    if ( !isalnum( (int) *cp ) )
                        continue;
                    while ( isalnum( (int) *cp ) )
                        ++ cp;

                    // Skip any whitespace
                    while ( isspace ( (int) *cp ) )
                        ++ cp;
                }

                // Append new name if it's not already there
                if ( appendNewName( names, nidx ) ) {
                    // LOAD [[:alnum:]_-]
                    // No actual '-' at the beginning - above skip-options code handled that
                    while ( isalnum( (int) *cp )  ||  *cp == '_' || *cp == '-' ) {
                        vStringPut( names[ nidx ], (int) *cp );
                        ++ cp;
                    }
                    vStringTerminate( names[ nidx ] );

                    ++ nidx;
                }

                if ( *cp != '=' ) {
                    while ( isspace( (int) *cp ) )
                        ++ cp;
                } else {
                    ++ cp;

                    // Skip whitespace:
                    // - when " open
                    // - when ' open
                    // - when \ preceedes
                    //
                    // Don't open/close " and ' when \ preceedes
                    // or when opposite " or ' already open
                    //
                    boolean qm_open = FALSE;
                    boolean ap_open = FALSE;
                    boolean bs_precedes = FALSE;
                    while ( *cp != '\0' ) {
                        if ( *cp == '"' ) {
                            if ( !bs_precedes && !ap_open ) {
                                if ( qm_open )
                                    qm_open = FALSE;
                                else
                                    qm_open = TRUE;
                            }
                        }

                        if ( *cp == '\'' ) {
                            if ( !bs_precedes && !qm_open ) {
                                if ( ap_open ) {
                                    ap_open = FALSE;
                                } else {
                                    ap_open = TRUE;
                                }
                            }
                        }

                        if ( *cp == '\\' ) {
                            if ( bs_precedes )
                                bs_precedes = FALSE;
                            else
                                bs_precedes = TRUE;
                        }

                        if ( isspace( (int) *cp ) ) {
                            if ( qm_open || ap_open || bs_precedes ) {
                                bs_precedes = FALSE;
                                ++ cp;
                            } else {
                                // Skip all white spaces and break
                                while ( isspace( (int) *cp ) )
                                    ++ cp;
                                break;
                            }
                        } else {
                            if ( *cp != '\\' )
                                bs_precedes = FALSE;
                            ++ cp;
                        }
                    }
                }
            }
        }

        //
        // FUNCTIONS
        //

        else if ( functionFound || ( !localVariableFound ) ) {
            // After 'function +' if there's no [[:alnum:]_-] then restart
            if ( ! ( isalnum( (int) *cp ) || *cp == '_' || *cp == '-' ) )
                continue;

            // Append new name if it's not already there
            if ( appendNewName( names, nidx ) ) {
                // LOAD [[:alnum:]_-]
                while ( isalnum( (int) *cp )  ||  *cp == '_' || *cp == '-' ) {
                    vStringPut( names[ nidx ], (int) *cp );
                    ++ cp;
                }
                vStringTerminate( names[ nidx ] );

                ++ nidx;

                // Skip spaces after [[:alnum:]_-]
                while ( isspace( (int) *cp ) )
                    ++ cp;

                // Detection of function not necessarily beginning with "function"
                if (*cp++ == '(')
                {
                    while ( isspace( (int) *cp ) )
                        ++ cp;
                    if ( *cp == ')'  && ! hackReject ( names[ nidx - 1 ] ) )
                        functionFound = TRUE;
                }
            }
        }

        // Function found?
        if ( functionFound ) {
            for ( int i = 0; i < nidx; ++ i )
                makeSimpleTag( names[ i ], ShKinds, K_FUNCTION );
        }
        if ( localVariableFound ) {
            for ( int i = 0; i < nidx; ++ i )
                makeSimpleTag( names[ i ], ShKinds, K_VARIABLE );
        }

        // Forget the gathered data
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
