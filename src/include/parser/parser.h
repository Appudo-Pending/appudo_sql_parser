/*-------------------------------------------------------------------------
 *
 * parser.h
 *		Definitions for the "raw" parser (flex and bison phases only)
 *
 * This is the external API for the raw lexing/parsing functions.
 *
 * Portions Copyright (c) 2003-2016, PgPool Global Development Group
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/parser/parser.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSER_H
#define PARSER_H

#include "parsenodes.h"


typedef enum
{
	BACKSLASH_QUOTE_OFF,
	BACKSLASH_QUOTE_ON,
	BACKSLASH_QUOTE_SAFE_ENCODING
}	BackslashQuoteType;

/* GUC variables in scan.l (every one of these is a bad idea :-() */
extern int	backslash_quote;
extern bool escape_string_warning;
extern PGDLLIMPORT bool standard_conforming_strings;

typedef enum
{
    QUERYTYPE_SELECT,
    QUERYTYPE_DELETE,
    QUERYTYPE_INSERT,
    QUERYTYPE_UPDATE
} QueryType;

struct Data_t;
typedef struct Data_t Data;

struct Data_t
{
    void * obj;
    void * (*addTable)(Data *);
    void (*removeTable)(Data *, void *);
    const char * (*hasRel)(Data *, const char*);
    const char * (*hasRelFor)(Data *, const char*, QueryType);
    bool (*hasAlias)(Data *, void *, const char*);
    bool (*addAlias)(Data *, void*, const char*);
    void (*setQuery)(Data *, const char*);
    void (*notHandled)(Data *, const char *);
};

struct QueryInfo_t;
typedef struct QueryInfo_t QueryInfo;

struct QueryInfo_t
{
    QueryType type;
    QueryInfo * parent;
    void * alias_table;
};

extern void verify(const char * query, Data * data);
void check_node(void * node, Data * obj, QueryInfo * info, unsigned int location);
void check_list(void * node, Data * obj, QueryInfo * info, unsigned int location);

/* Primary entry point for the raw parsing functions */
extern List *raw_parser(const char *str, bool *error);
extern void free_parser(void);

/* Utility functions exported by gram.y (perhaps these should be elsewhere) */
extern List *SystemFuncName(char *name);
extern TypeName *SystemTypeName(char *name);

extern void parser_set_param(const char *name, const char *value);

#endif   /* PARSER_H */
