/*===========================================================================
 *  FileName : read.c
 *  About    : S-Expression reader
 *
 *  Copyright (C) 2000-2001 by Shiro Kawai (shiro@acm.org)
 *  Copyright (C) 2005      by Kazuki Ohta (mover@hct.zaq.ne.jp)
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of authors nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
===========================================================================*/
/*
 * FIXME: Large fixed-size buffer on stack without limit checking
 *
 * Fix some functions contained in this file since:
 *
 * - danger
 * - some embedded platform cannot allocate such large stack (approx. 20KB)
 * - inefficient from the viewpoint of memory locality (cache, page, power
 *   consumption etc)
 *
 * Search "FIXME" on this file to locate the codes. I wonder other Scheme
 * implementations may have sophisticated code. Please consider porting them to
 * save development cost, since this part is not the primary value of
 * SigScheme.  -- YamaKen 2005-09-05
 */

/*=======================================
  System Include
=======================================*/
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/*=======================================
  Local Include
=======================================*/
#include "sigscheme.h"
#include "sigschemeinternal.h"

/*=======================================
  File Local Struct Declarations
=======================================*/

/*=======================================
  File Local Macro Declarations
=======================================*/

/*=======================================
  Variable Declarations
=======================================*/

/*=======================================
  File Local Function Declarations
=======================================*/
static int    skip_comment_and_space(ScmObj port);
static char*  read_word(ScmObj port);
static char*  read_char_sequence(ScmObj port);

static ScmObj read_sexpression(ScmObj port);
static ScmObj read_list(ScmObj port, int closeParen);
static ScmObj read_char(ScmObj port);
static ScmObj read_string(ScmObj port);
static ScmObj read_symbol(ScmObj port);
static ScmObj parse_number(ScmObj port);
static ScmObj read_number_or_symbol(ScmObj port);
static ScmObj read_quote(ScmObj port, ScmObj quoter);

/*=======================================
  Function Implementations
=======================================*/
/*===========================================================================
  S-Expression Parser
===========================================================================*/
ScmObj SigScm_Read(ScmObj port)
{
    ScmObj sexp = SCM_FALSE;
    DECLARE_INTERNAL_FUNCTION("SigScm_Read");

    ASSERT_PORTP(port);

    sexp = read_sexpression(port);
#if SCM_DEBUG
    if ((SigScm_DebugCategories() & SCM_DBG_READ) && !EOFP(sexp)) {
        SigScm_WriteToPort(scm_current_error_port, sexp);
        SigScm_ErrorNewline();
    }
#endif

    return sexp;
}

ScmObj SigScm_Read_Char(ScmObj port)
{
    DECLARE_INTERNAL_FUNCTION("SigScm_Read_Char");

    ASSERT_PORTP(port);

    return read_char(port);
}


static int skip_comment_and_space(ScmObj port)
{
    int c = 0;
    while (1) {
        SCM_PORT_GETC(port, c);
        if (c == EOF) {
            return c;
        } else if(c == ';') {
            while (1) {
                SCM_PORT_GETC(port, c);
                if (c == '\n') {
                    break;
                }
                if (c == EOF) return c;
            }
            continue;
        } else if(isspace(c)) {
            continue;
        }

        return c;
    }
}

static ScmObj read_sexpression(ScmObj port)
{
    int c  = 0;
    int c1 = 0;

    CDBG((SCM_DBG_PARSER, "read_sexpression"));

    while (1) {
        c = skip_comment_and_space(port);

        CDBG((SCM_DBG_PARSER, "read_sexpression c = %c", c));

        switch (c) {
        case '(':
            return read_list(port, ')');
        case '\"':
            return read_string(port);
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case '+': case '-':
            SCM_PORT_UNGETC(port, c);
            return read_number_or_symbol(port);
        case '\'':
            return read_quote(port, SCM_QUOTE);
        case '`':
            return read_quote(port, SCM_QUASIQUOTE);
        case ',':
            SCM_PORT_GETC(port, c1);
            if (c1 == EOF) {
                SigScm_Error("EOF in unquote");
            } else if (c1 == '@') {
                return read_quote(port, SCM_UNQUOTE_SPLICING);
            } else {
                SCM_PORT_UNGETC(port, c1);
                return read_quote(port, SCM_UNQUOTE);
            }
            break;
        case '#':
            SCM_PORT_GETC(port, c1);
            switch (c1) {
            case 't': case 'T':
                return SCM_TRUE;
            case 'f': case 'F':
                return SCM_FALSE;
            case '(':
                return ScmOp_list2vector(read_list(port, ')'));
            case '\\':
                return read_char(port);
            case 'b': case 'o': case 'd': case 'x':
                SCM_PORT_UNGETC(port, c1);
                return parse_number(port);
            case EOF:
                SigScm_Error("end in #");
            default:
                SigScm_Error("Unsupported # : %c", c1);
            }
            break;
        /* Error sequence */
        case ')':
            SigScm_Error("invalid close parenthesis");
            break;
        case EOF:
            return SCM_EOF;
        default:
            SCM_PORT_UNGETC(port, c);
            return read_symbol(port);
        }
    }
}

static ScmObj read_list(ScmObj port, int closeParen)
{
    ScmObj list_head = SCM_NULL;
    ScmObj list_tail = SCM_NULL;
    ScmObj item   = SCM_NULL;
    ScmObj cdr    = SCM_NULL;
    int    line   = SCM_PORT_LINE(port);
    int    c      = 0;
    int    c2     = 0;
    char  *token  = NULL;

    CDBG((SCM_DBG_PARSER, "read_list"));

    while (1) {
        c = skip_comment_and_space(port);

        CDBG((SCM_DBG_PARSER, "read_list c = [%c]", c));

        if (c == EOF) {
            if (SCM_PORT_PORTTYPE(port) == PORT_FILE)
                SigScm_Error("EOF inside list. (starting from line %d)", line + 1);
            else
                SigScm_Error("EOF inside list.");
        } else if (c == closeParen) {
            return list_head;
        } else if (c == '.') {
            c2 = 0;
            SCM_PORT_GETC(port, c2);
            CDBG((SCM_DBG_PARSER, "read_list process_dot c2 = [%c]", c2));
            if (isspace(c2) || c2 == '(' || c2 == '"' || c2 == ';') {
                cdr = read_sexpression(port);
                if (NULLP(list_tail))
                    SigScm_Error(".(dot) at the start of the list.");

                c = skip_comment_and_space(port);
                if (c != ')')
                    SigScm_Error("bad dot syntax");

                SET_CDR(list_tail, cdr);
                return list_head;
            }

            /*
             * This dirty hack here picks up the current token as a
             * symbol beginning with the dot (that's how Guile and
             * Gauche behave).
             */
            SCM_PORT_UNGETC(port, c2);
            token = read_word(port);
            token = (char*)realloc(token, strlen(token) + 1 + 1);
            memmove(token + 1, token, strlen(token)+1);
            token[0] = '.';
            item = Scm_Intern(token);
            free(token);
        } else {
            SCM_PORT_UNGETC(port, c);
            item = read_sexpression(port);
        }

        /* Append item to the list_tail. */
        if (NULLP(list_tail)) {
            /* create new list */
            list_head = CONS(item, SCM_NULL);
            list_tail = list_head;
        } else {
            /* update list_tail */
            SET_CDR(list_tail, CONS(item, SCM_NULL));
            list_tail = CDR(list_tail);
        }
    }
}

static ScmObj read_char(ScmObj port)
{
    char *ch = read_char_sequence(port);

    CDBG((SCM_DBG_PARSER, "read_char : ch = %s", ch));

    /* FIXME: Simplify with Scm_special_char_table */
    /* check special sequence "space" and "newline" */
    if (strcmp(ch, "space") == 0) {
        ch[0] = ' ';
        ch[1] = '\0';
#if 0
    /* to avoid portability problem, we should not support #\Space and so on */
    } else if (strcmp(ch, "Space") == 0) {
        ch[0] = ' ';
        ch[1] = '\0';
#endif
    } else if (strcmp(ch, "newline") == 0) {
        ch[0] = '\n';
        ch[1] = '\0';
    }

    /* FIXME: memory leak */
    return Scm_NewChar(ch);
}

static ScmObj read_string(ScmObj port)
{
    char  stringbuf[1024]; /* FIXME! */
    int   stringlen = 0;
    int   c = 0;

    CDBG((SCM_DBG_PARSER, "read_string"));

    while (1) {
        SCM_PORT_GETC(port, c);

        CDBG((SCM_DBG_PARSER, "read_string c = %c", c));

        switch (c) {
        case EOF:
            stringbuf[stringlen] = '\0';
            SigScm_Error("EOF in the string : str = %s", stringbuf);
            break;

        case '\"':
            stringbuf[stringlen] = '\0';
            return Scm_NewStringCopying(stringbuf);

        case '\\':
            /* FIXME: Simplify with Scm_special_char_table */
            /*
             * (R5RS) 6.3.5 String
             * A double quote can be written inside a string only by
             * escaping it with a backslash (\).
             */
            SCM_PORT_GETC(port, c);
            switch (c) {
            case '\"': stringbuf[stringlen] = c;    break;
            case 'n':  stringbuf[stringlen] = '\n'; break;
            case 'r':  stringbuf[stringlen] = '\r'; break;
            case 'f':  stringbuf[stringlen] = '\f'; break;
            case 't':  stringbuf[stringlen] = '\t'; break;
            case '\\': stringbuf[stringlen] = '\\'; break;
            default:
                stringbuf[stringlen] = '\\';
                stringbuf[++stringlen] = c;
                break;
            }
            stringlen++;
            break;

        default:
            stringbuf[stringlen] = c;
            stringlen++;
            break;
        }
    }
}

static ScmObj read_symbol(ScmObj port)
{
    char  *sym_name = read_word(port);
    ScmObj sym = Scm_Intern(sym_name);
    free(sym_name);

    CDBG((SCM_DBG_PARSER, "read_symbol"));

    return sym;
}

static ScmObj read_number_or_symbol(ScmObj port)
{
    int number = 0;
    int str_len = 0;
    char *str = NULL;
    char *first_nondigit = NULL;
    ScmObj ret = SCM_NULL;

    CDBG((SCM_DBG_PARSER, "read_number_or_symbol"));

    /* read char sequence */
    str = read_word(port);
    str_len = strlen(str);

    /* see if it's a decimal integer */
    number = (int)strtol(str, &first_nondigit, 10);

    /* set return obj */
    ret = (*first_nondigit) ? Scm_Intern(str) : Scm_NewInt(number);

    /* free */
    free(str);

    return ret;
}


static char *read_word(ScmObj port)
{
    char  stringbuf[1024];  /* FIXME! */
    int   stringlen = 0;
    int   c = 0;
    char *dst = NULL;

    while (1) {
        SCM_PORT_GETC(port, c);

        CDBG((SCM_DBG_PARSER, "c = %c", c));

        switch (c) {
        case EOF: /* don't became an error for handling c-eval, like Scm_eval_c_string("some-symbol"); */
        case ' ':  case '(':  case ')':  case ';':
        case '\n': case '\t': case '\"': case '\'':
            SCM_PORT_UNGETC(port, c);
            stringbuf[stringlen] = '\0';
            dst = strdup(stringbuf);
            return dst;

        default:
            stringbuf[stringlen++] = (char)c;
            break;
        }
    }
}

static char *read_char_sequence(ScmObj port)
{
    char  stringbuf[1024];  /* FIXME! */
    int   stringlen = 0;
    int   c = 0;
    char *dst = NULL;

    while (1) {
        SCM_PORT_GETC(port, c);

        CDBG((SCM_DBG_PARSER, "c = %c", c));

        switch (c) {
        case EOF:
            stringbuf[stringlen] = '\0';
            SigScm_Error("EOF in the char sequence : char = %s", stringbuf);
            break;

        case ' ':  case '\"': case '\'':
        case '(':  case ')':  case ';':
        case '\n': case '\r': case '\f': case '\t':
            /* pass through first char */
            if (stringlen == 0) {
                stringbuf[stringlen++] = (char)c;
                break;
            }
            /* return buf */
            SCM_PORT_UNGETC(port, c);
            stringbuf[stringlen] = '\0';
            dst = strdup(stringbuf);
            return dst;

        default:
            stringbuf[stringlen++] = (char)c;
            break;
        }
    }
}

static ScmObj read_quote(ScmObj port, ScmObj quoter)
{
    return SCM_LIST_2(quoter, read_sexpression(port));
}

/* str should be what appeared right after '#' (eg. #b123) */
static ScmObj parse_number(ScmObj port)
{
    int radix  = 0;
    int number = 0;
    char *first_nondigit = NULL;
    char *numstr = read_word(port);

    switch (numstr[0]) {
    case 'b': radix = 2;  break;
    case 'o': radix = 8;  break;
    case 'd': radix = 10; break;
    case 'x': radix = 16; break;
    default:
        SigScm_Error("ill-formatted number: #%s", numstr);
    }

    /* get num */
    number = (int)strtol(numstr+1, &first_nondigit, radix);
    if (*first_nondigit)
        SigScm_Error("ill-formatted number: #%s", numstr);

    /* free str */
    free(numstr);

    return Scm_NewInt(number);
}
