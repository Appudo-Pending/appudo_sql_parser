/*-------------------------------------------------------------------------
 *
 * parser.c
 *		Main entry point/driver for PostgreSQL grammar
 *
 * Note that the grammar is not allowed to perform any table access
 * (since we need to be able to do basic parsing even while inside an
 * aborted transaction).  Therefore, the data structures returned by
 * the grammar are "raw" parsetrees that still need to be analyzed by
 * analyze.c and related files.
 *
 *
 * Portions Copyright (c) 2003-2016, PgPool Global Development Group
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/parser/parser.c
 *
 *-------------------------------------------------------------------------
 */

#include <string.h>
#include "pool_parser.h"
#include "utils/palloc.h"
#include "utils/memutils.h"
#include "gramparse.h"	/* required before parser/gram.h! */
#include "gram.h"
#include "parser.h"
#include "pg_wchar.h"
#include "utils/elog.h"

int server_version_num = 0;
static pg_enc server_encoding = PG_SQL_ASCII;

static int
parse_version(const char *versionString);

void check_first(void * o, Data * data, QueryInfo * ninfo, unsigned int location)
{
    if(o == NULL)
        return;

    if(IsA(o, List) ||
       IsA(o, IntList) ||
       IsA(o, OidList))
    {
        ListCell *lc = ((List*)o)->head;
        o = lfirst(lc);
    }
    if(IsA(o, String) ||
       IsA(o, BitString))
    {
        Value * n = (Value *)o;
        QueryInfo * p = ninfo;
        char * name = n->val.str;
        bool hasAlias = false;
        do
        {
            if(data->hasAlias(data, p->alias_table, name))
            {
                hasAlias = true;
                break;
            }

        }
        while((p = p->parent) != NULL);
        const char * nn = NULL;
        if(!hasAlias && (nn = data->hasRel(data, name)) != NULL)
        {
            // replace name
            pfree(n->val.str);
            n->val.str = pstrdup(nn);
        }
    }
}

void check_FuncCall(FuncCall *node, Data * data, QueryInfo * info)
{
    char *funcname;

    if(!node->funcname)
        goto FAIL;

    funcname = strVal(lfirst(list_head(node->funcname)));

    if(strcmp(funcname,"row_number") != 0)
        goto FAIL;

    return;

    FAIL:

    data->notHandled(data, nodeToString(node));
}

void check_node(void * o, Data * data, QueryInfo * info, unsigned int location)
{
    QueryInfo * ninfo = info;
    if(o == NULL)
        return;

    //fprintf(stderr, "node:%i\n", nodeTag(o));
    if(IsA(o, List) ||
       IsA(o, IntList) ||
       IsA(o, OidList))
    {
        check_list(o, data, ninfo, location);
    }
    else
    if(IsA(o, Integer) ||
       IsA(o, Float))
    {

    }
    else
    if(IsA(o, String) ||
       IsA(o, BitString))
    {
        Value * n = (Value *)o;
    }
    else
    switch (nodeTag(o))
    {
    case T_A_Const:
    case T_ParamRef:
        break;
    case T_SortBy:
    {
        SortBy *n = (SortGroupClause *)o;
        check_node(n->node, data, ninfo, n->location);
    }
    case T_Alias:
        {
            Alias *n = (Alias *)o;
            if(ninfo->alias_table == NULL)
                ninfo->alias_table = data->addTable(data);
            data->addAlias(data, ninfo->alias_table, n->aliasname);
            break;
        }
    case T_A_Expr:
        {
            A_Expr * n = (A_Expr *)o;
            check_node(n->lexpr, data, ninfo, n->location);
            check_node(n->rexpr, data, ninfo, n->location);
            break;
        }
    case T_BoolExpr:
        {
        	BoolExpr * n = (BoolExpr *)o;
            check_list(n->args, data, ninfo, n->location);
            break;
        }
    case T_RangeVar:
        {
            RangeVar * n = (RangeVar *)o;
            const char * name = data->hasRelFor(data, n->relname, ninfo->type);
            if(name != NULL)
            {
                // replace name
                pfree(n->relname);
                n->relname = pstrdup(name);
            }

            check_node(n->alias, data, ninfo, n->location);
            break;
        }
    case T_JoinExpr:
        {
            JoinExpr * n = (JoinExpr *)o;
            check_node(n->larg, data, ninfo, location);
            check_node(n->rarg, data, ninfo, location);
            check_node(n->usingClause, data, ninfo, location);
            check_node(n->quals, data, ninfo, location);
            check_node(n->alias, data, ninfo, location);

            break;
        }

    case T_A_Indirection:
        {
            A_Indirection * n = (A_Indirection *)o;
            check_node(n->arg, data, ninfo, location);
            check_node(n->indirection, data, ninfo, location);


            break;
        }
    case T_ResTarget:
        {
            ResTarget * n = (ResTarget *)o;
            if(n->indirection == NIL)
            {
            }
            check_node(n->val, data, ninfo, location);


            break;
        }
    case T_TargetEntry:
        {
            TargetEntry * n = (TargetEntry *)o;


            break;
        }

    case T_ColumnRef:
        {
            ColumnRef * n = (ColumnRef *)o;
            //  check if we have a list here
            if(IsA(n->fields, List) && ((List*)n->fields)->length > 1)
                check_first(n->fields, data, ninfo, n->location);

            break;
        }
    case T_FromExpr:
        {
            FromExpr * n = (FromExpr *)o;
            check_node(n->fromlist, data, ninfo, location);
            check_node(n->quals, data, ninfo, location);

            break;
        }
    case T_SelectStmt:
        {
            QueryInfo qi;
            qi.parent = ninfo;
            qi.alias_table = NULL;
            qi.type = QUERYTYPE_SELECT;
            ninfo = &qi;
            SelectStmt * n = (SelectStmt *)o;
            check_node(n->distinctClause, data, ninfo, location);
            check_node(n->intoClause, data, ninfo, location);
            check_node(n->targetList, data, ninfo, location);
            check_node(n->fromClause, data, ninfo, location);
            check_node(n->whereClause, data, ninfo, location);
            check_node(n->groupClause, data, ninfo, location);
            check_node(n->havingClause, data, ninfo, location);
            check_node(n->windowClause, data, ninfo, location);
            check_node(n->withClause, data, ninfo, location);
            check_node(n->valuesLists, data, ninfo, location);
            check_node(n->sortClause, data, ninfo, location);
            check_node(n->limitOffset, data, ninfo, location);
            check_node(n->limitCount, data, ninfo, location);
            check_node(n->lockingClause, data, ninfo, location);
            check_node(n->larg, data, ninfo, location);
            check_node(n->rarg, data, ninfo, location);
            data->removeTable(data, qi.alias_table);
            break;
        }
    case T_InsertStmt:
        {
            QueryInfo qi;
            qi.parent = ninfo;
            qi.alias_table = NULL;
            qi.type = QUERYTYPE_INSERT;
            ninfo = &qi;
            InsertStmt * n = (InsertStmt *)o;
            check_node(n->relation, data, ninfo, location);
            check_node(n->cols, data, ninfo, location);
            check_node(n->selectStmt, data, ninfo, location);
            check_node(n->returningList, data, ninfo, location);
            data->removeTable(data, qi.alias_table);
            break;
        }
    case T_DeleteStmt:
        {
            QueryInfo qi;
            qi.parent = ninfo;
            qi.alias_table = NULL;
            qi.type = QUERYTYPE_DELETE;
            ninfo = &qi;
            DeleteStmt * n = (DeleteStmt *)o;
            check_node(n->relation, data, ninfo, location);
            check_node(n->usingClause, data, ninfo, location);
            check_node(n->whereClause, data, ninfo, location);
            check_node(n->returningList, data, ninfo, location);
            data->removeTable(data, qi.alias_table);
            break;
        }
    case T_FuncCall:
        {
            FuncCall * n = (FuncCall *)o;
            check_FuncCall(n, data, info);
            break;
        }
    case T_RangeSubselect:
		{
            QueryInfo qi;
            qi.parent = ninfo;
            qi.alias_table = NULL;
            qi.type = QUERYTYPE_SELECT;
            ninfo = &qi;
            RangeSubselect * n = (RangeSubselect *)o;

            check_node(n->subquery, data, ninfo, location);
            check_node(n->alias, data, qi.parent, location);
            data->removeTable(data, qi.alias_table);
			break;
		}
    case T_TypeCast:
        {
            break;
        }
    case T_UpdateStmt:
        {
            QueryInfo qi;
            qi.parent = ninfo;
            qi.alias_table = NULL;
            qi.type = QUERYTYPE_UPDATE;
            ninfo = &qi;
            UpdateStmt * n = (UpdateStmt *)o;
            check_node(n->relation, data, ninfo, location);
            check_node(n->targetList, data, ninfo, location);
            check_node(n->whereClause, data, ninfo, location);
            check_node(n->fromClause, data, ninfo, location);
            check_node(n->returningList, data, ninfo, location);
            data->removeTable(data, qi.alias_table);
            break;
        }
    case T_CommonTableExpr:
        {
            CommonTableExpr * n = (CommonTableExpr *)o;

            break;
        }
    default:
        {
            Node * n = (Node *)o;
            int v = nodeTag(o);
            data->notHandled(data, nodeToString(n));
            break;
        }
    }
}

void check_list(void * node, Data * data, QueryInfo * info, unsigned int location)
{
    ListCell   *lc;
    foreach(lc, node)
    {
        void* o = lfirst(lc);
        check_node(o, data, info, location);
    }
}

void verify(const char * query, Data * data)
{
    bool error;
    MemoryContextInit();
    if(CurrentMemoryContext)
    {
        List * node = raw_parser(query, &error);
        QueryInfo qi;
        qi.parent = NULL;
        qi.alias_table = NULL;
        if(node == NULL)
            data->notHandled(data, "error");
        else
            check_list(node, data, &qi, 0);
        data->setQuery(data, nodeToString(node));
    }
    else
    {
    }
}

void free_parser(void)
{
    if(CurrentMemoryContext)
    {
        MemoryContextDeleteChildren(CurrentMemoryContext);
        MemoryContextReset(CurrentMemoryContext);
    }

    if(ErrorContext)
    {
        MemoryContextDeleteChildren(ErrorContext);
        MemoryContextReset(ErrorContext);
    }
}

/*
 * raw_parser
 *		Given a query in string form, do lexical and grammatical analysis.
 *
 * Returns a list of raw (un-analyzed) parse trees.
 * Set *error to true if there's any parse error.
 */
List *
raw_parser(const char *str, bool *error)
{
	core_yyscan_t yyscanner;
	base_yy_extra_type yyextra;
	int			yyresult;

    MemoryContext oldContext = CurrentMemoryContext;

	/* initialize error flag */
	*error = false;

	/* initialize the flex scanner */
	yyscanner = scanner_init(str, &yyextra.core_yy_extra,
							 ScanKeywords, NumScanKeywords);

	/* base_yylex() only needs this much initialization */
	yyextra.have_lookahead = false;

	/* initialize the bison parser */
	parser_init(&yyextra);

	PG_TRY();
	{
		/* Parse! */
		yyresult = base_yyparse(yyscanner);

		/* Clean up (release memory) */
		scanner_finish(yyscanner);
	}
	PG_CATCH();
	{
		MemoryContextSwitchTo(oldContext);
		scanner_finish(yyscanner);
		yyresult = -1;
		FlushErrorState();
	}
	PG_END_TRY();

	if (yyresult)				/* error */
	{
		*error = true;
		return NIL;
	}

	return yyextra.parsetree;
}


/*
 * Intermediate filter between parser and core lexer (core_yylex in scan.l).
 *
 * This filter is needed because in some cases the standard SQL grammar
 * requires more than one token lookahead.  We reduce these cases to one-token
 * lookahead by replacing tokens here, in order to keep the grammar LALR(1).
 *
 * Using a filter is simpler than trying to recognize multiword tokens
 * directly in scan.l, because we'd have to allow for comments between the
 * words.  Furthermore it's not clear how to do that without re-introducing
 * scanner backtrack, which would cost more performance than this filter
 * layer does.
 *
 * The filter also provides a convenient place to translate between
 * the core_YYSTYPE and YYSTYPE representations (which are really the
 * same thing anyway, but notationally they're different).
 */
int
base_yylex(YYSTYPE *lvalp, YYLTYPE *llocp, core_yyscan_t yyscanner)
{
	base_yy_extra_type *yyextra = pg_yyget_extra(yyscanner);
	int			cur_token;
	int			next_token;
	int			cur_token_length;
	YYLTYPE		cur_yylloc;

	/* Get next token --- we might already have it */
	if (yyextra->have_lookahead)
	{
		cur_token = yyextra->lookahead_token;
		lvalp->core_yystype = yyextra->lookahead_yylval;
		*llocp = yyextra->lookahead_yylloc;
		*(yyextra->lookahead_end) = yyextra->lookahead_hold_char;
		yyextra->have_lookahead = false;
	}
	else
		cur_token = core_yylex(&(lvalp->core_yystype), llocp, yyscanner);

	/*
	 * If this token isn't one that requires lookahead, just return it.  If it
	 * does, determine the token length.  (We could get that via strlen(), but
	 * since we have such a small set of possibilities, hardwiring seems
	 * feasible and more efficient.)
	 */
	switch (cur_token)
	{
		case NOT:
			cur_token_length = 3;
			break;
		case NULLS_P:
			cur_token_length = 5;
			break;
		case WITH:
			cur_token_length = 4;
			break;
		default:
			return cur_token;
	}

	/*
	 * Identify end+1 of current token.  core_yylex() has temporarily stored a
	 * '\0' here, and will undo that when we call it again.  We need to redo
	 * it to fully revert the lookahead call for error reporting purposes.
	 */
	yyextra->lookahead_end = yyextra->core_yy_extra.scanbuf +
		*llocp + cur_token_length;
	Assert(*(yyextra->lookahead_end) == '\0');

	/*
	 * Save and restore *llocp around the call.  It might look like we could
	 * avoid this by just passing &lookahead_yylloc to core_yylex(), but that
	 * does not work because flex actually holds onto the last-passed pointer
	 * internally, and will use that for error reporting.  We need any error
	 * reports to point to the current token, not the next one.
	 */
	cur_yylloc = *llocp;

	/* Get next token, saving outputs into lookahead variables */
	next_token = core_yylex(&(yyextra->lookahead_yylval), llocp, yyscanner);
	yyextra->lookahead_token = next_token;
	yyextra->lookahead_yylloc = *llocp;

	*llocp = cur_yylloc;

	/* Now revert the un-truncation of the current token */
	yyextra->lookahead_hold_char = *(yyextra->lookahead_end);
	*(yyextra->lookahead_end) = '\0';

	yyextra->have_lookahead = true;

	/* Replace cur_token if needed, based on lookahead */
	switch (cur_token)
	{
		case NOT:
			/* Replace NOT by NOT_LA if it's followed by BETWEEN, IN, etc */
			switch (next_token)
			{
				case BETWEEN:
				case IN_P:
				case LIKE:
				case ILIKE:
				case SIMILAR:
					cur_token = NOT_LA;
					break;
			}
			break;

		case NULLS_P:
			/* Replace NULLS_P by NULLS_LA if it's followed by FIRST or LAST */
			switch (next_token)
			{
				case FIRST_P:
				case LAST_P:
					cur_token = NULLS_LA;
					break;
			}
			break;

		case WITH:
			/* Replace WITH by WITH_LA if it's followed by TIME or ORDINALITY */
			switch (next_token)
			{
				case TIME:
				case ORDINALITY:
					cur_token = WITH_LA;
					break;
			}
			break;
	}

	return cur_token;
}

static int
parse_version(const char *versionString)
{
	int			cnt;
	int			vmaj,
	vmin,
	vrev;

	cnt = sscanf(versionString, "%d.%d.%d", &vmaj, &vmin, &vrev);

	if (cnt < 2)
		return -1;

	if (cnt == 2)
		vrev = 0;

	return (100 * vmaj + vmin) * 100 + vrev;
}

void
parser_set_param(const char *name, const char *value)
{
	if (strcmp(name, "server_version") == 0)
	{
		server_version_num = parse_version(value);
	}
	else if (strcmp(name, "server_encoding") == 0)
	{
		if (strcmp(value, "UTF8") == 0)
			server_encoding = PG_UTF8;
		else
			server_encoding = PG_SQL_ASCII;
	}
	else if (strcmp(name, "standard_conforming_strings") == 0)
	{
		if (strcmp(value, "on") == 0)
			standard_conforming_strings = true;
		else
			standard_conforming_strings = false;
	}
}

int
GetDatabaseEncoding(void)
{
	return server_encoding;
}

int
pg_mblen(const char *mbstr)
{
	return pg_utf_mblen((const unsigned char *) mbstr);
}

