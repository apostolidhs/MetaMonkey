/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* JS reflection package. */

#include "jsreflect.h"

#include <stdlib.h>
#include <iostream>

#include "mozilla/DebugOnly.h"
#include "mozilla/Util.h"

#include "jspubtd.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsobj.h"

//metadev
#include "js/Value.h"

#include "frontend/Parser.h"
#include "frontend/ParseNode-inl.h"
#include "frontend/TokenStream.h"
#include "js/CharacterEncoding.h"
#include "vm/RegExpObject.h"

#include "jsobjinlines.h"

using namespace js;
using namespace js::frontend;

using mozilla::ArrayLength;
using mozilla::DebugOnly;

char const * const js::aopNames[] = {
    "=",    /* AOP_ASSIGN */
    "+=",   /* AOP_PLUS */
    "-=",   /* AOP_MINUS */
    "*=",   /* AOP_STAR */
    "/=",   /* AOP_DIV */
    "%=",   /* AOP_MOD */
    "<<=",  /* AOP_LSH */
    ">>=",  /* AOP_RSH */
    ">>>=", /* AOP_URSH */
    "|=",   /* AOP_BITOR */
    "^=",   /* AOP_BITXOR */
    "&="    /* AOP_BITAND */
};

char const * const js::binopNames[] = {
    "==",         /* BINOP_EQ */
    "!=",         /* BINOP_NE */
    "===",        /* BINOP_STRICTEQ */
    "!==",        /* BINOP_STRICTNE */
    "<",          /* BINOP_LT */
    "<=",         /* BINOP_LE */
    ">",          /* BINOP_GT */
    ">=",         /* BINOP_GE */
    "<<",         /* BINOP_LSH */
    ">>",         /* BINOP_RSH */
    ">>>",        /* BINOP_URSH */
    "+",          /* BINOP_PLUS */
    "-",          /* BINOP_MINUS */
    "*",          /* BINOP_STAR */
    "/",          /* BINOP_DIV */
    "%",          /* BINOP_MOD */
    "|",          /* BINOP_BITOR */
    "^",          /* BINOP_BITXOR */
    "&",          /* BINOP_BITAND */
    "in",         /* BINOP_IN */
    "instanceof", /* BINOP_INSTANCEOF */
};

char const * const js::unopNames[] = {
    "delete",  /* UNOP_DELETE */
	".!",
	".&",
	".~",
	".@",
    "-",       /* UNOP_NEG */
    "+",       /* UNOP_POS */
    "!",       /* UNOP_NOT */
    "~",       /* UNOP_BITNOT */
    "typeof",  /* UNOP_TYPEOF */
    "void"     /* UNOP_VOID */
};

char const * const js::nodeTypeNames[] = {
#define ASTDEF(ast, str, method) str,
#include "jsast.tbl"
#undef ASTDEF
    NULL
};

static char const * const callbackNames[] = {
#define ASTDEF(ast, str, method) method,
#include "jsast.tbl"
#undef ASTDEF
    NULL
};

typedef AutoValueVector NodeVector;

/*
 * ParseNode is a somewhat intricate data structure, and its invariants have
 * evolved, making it more likely that there could be a disconnect between the
 * parser and the AST serializer. We use these macros to check invariants on a
 * parse node and raise a dynamic error on failure.
 */
#define LOCAL_ASSERT(expr)                                                             \
    JS_BEGIN_MACRO                                                                     \
        JS_ASSERT(expr);                                                               \
        if (!(expr)) {                                                                 \
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_PARSE_NODE);  \
            return false;                                                              \
        }                                                                              \
    JS_END_MACRO

#define LOCAL_NOT_REACHED(expr)                                                        \
    JS_BEGIN_MACRO                                                                     \
        JS_NOT_REACHED(expr);                                                          \
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_PARSE_NODE);      \
        return false;                                                                  \
    JS_END_MACRO


//typedef bool (*nodeBuilderToSrcProgramFiveArgs) (
//				 ASTType type, TokenPos *pos,
//                 const char *childName, HandleValue child,
//                 MutableHandleValue dst);
//nodeBuilderToSrcProgramFiveArgs nodeBuilderToSrcProgramFiveArgsFuncs[] = 
//{
//	toSrcProgram //AST_PROGRAM
//};


/*
 * Builder class that constructs JavaScript AST node objects. See:
 *
 *     https://developer.mozilla.org/en/SpiderMonkey/Parser_API
 *
 * Bug 569487: generalize builder interface
 */

class NodeBuilder
{
    JSContext   *cx;
    TokenStream *tokenStream;
    bool        saveLoc;               /* save source location information?     */
    char const  *src;                  /* source filename or null               */
    RootedValue srcval;                /* source filename JS value or null      */
    Value       callbacks[AST_LIMIT];  /* user-specified callbacks              */
    AutoValueArray callbacksRoots;     /* for rooting |callbacks|               */
    RootedValue userv;                 /* user-specified builder object or null */
    RootedValue undefinedVal;          /* a rooted undefined val, used by opt() */
  public:
    //NodeBuilder(JSContext *c, bool l, char const *s, NodeBuilderMode bm)
    //    : cx(c), tokenStream(NULL), saveLoc(l), src(s), srcval(c), builderMode(bm),
    //      callbacksRoots(c, callbacks, AST_LIMIT), userv(c), undefinedVal(c, UndefinedValue())
    //{
    //    //MakeRangeGCSafe(callbacks, mozilla::ArrayLength(callbacks));
    //}

    NodeBuilder(JSContext *c, bool l, char const *s)
        : cx(c), tokenStream(NULL), saveLoc(l), src(s), srcval(c),
          callbacksRoots(c, callbacks, AST_LIMIT), userv(c), undefinedVal(c, UndefinedValue())
    {
		MakeRangeGCSafe(callbacks, mozilla::ArrayLength(callbacks));
    }

    bool init(HandleObject userobj) {
        if (src) {
            if (!atomValue(src, &srcval))
                return false;
        } else {
            srcval.setNull();
        }

        if (!userobj) {
            userv.setNull();
            for (unsigned i = 0; i < AST_LIMIT; i++) {
                callbacks[i].setNull();
            }
            return true;
        }

        userv.setObject(*userobj);

        RootedValue nullVal(cx, NullValue());
        RootedValue funv(cx);
        for (unsigned i = 0; i < AST_LIMIT; i++) {
            const char *name = callbackNames[i];
            RootedAtom atom(cx, Atomize(cx, name, strlen(name)));
            if (!atom)
                return false;
            RootedId id(cx, AtomToId(atom));
            if (!baseops::GetPropertyDefault(cx, userobj, id, nullVal, &funv))
                return false;

            if (funv.isNullOrUndefined()) {
                callbacks[i].setNull();
                continue;
            }

            if (!funv.isObject() || !funv.toObject().is<JSFunction>()) {
                js_ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_NOT_FUNCTION,
                                         JSDVG_SEARCH_STACK, funv, NullPtr(), NULL, NULL);
                return false;
            }

            callbacks[i] = funv;
        }

        return true;
    }

    void setTokenStream(TokenStream *ts) {
        tokenStream = ts;
    }

  private:
    bool callback(HandleValue fun, TokenPos *pos, MutableHandleValue dst) {
        if (saveLoc) {
            RootedValue loc(cx);
            if (!newNodeLoc(pos, &loc))
                return false;
            Value argv[] = { loc };
            AutoValueArray ava(cx, argv, 1);
            return Invoke(cx, userv, fun, ArrayLength(argv), argv, dst.address());
        }

        Value argv[] = { NullValue() }; /* no zero-length arrays allowed! */
        AutoValueArray ava(cx, argv, 1);
        return Invoke(cx, userv, fun, 0, argv, dst.address());
    }

    bool callback(HandleValue fun, HandleValue v1, TokenPos *pos, MutableHandleValue dst) {
        if (saveLoc) {
            RootedValue loc(cx);
            if (!newNodeLoc(pos, &loc))
                return false;
            Value argv[] = { v1, loc };
            AutoValueArray ava(cx, argv, 2);
            return Invoke(cx, userv, fun, ArrayLength(argv), argv, dst.address());
        }

        Value argv[] = { v1 };
        AutoValueArray ava(cx, argv, 1);
        return Invoke(cx, userv, fun, ArrayLength(argv), argv, dst.address());
    }

    bool callback(HandleValue fun, HandleValue v1, HandleValue v2, TokenPos *pos,
                  MutableHandleValue dst) {
        if (saveLoc) {
            RootedValue loc(cx);
            if (!newNodeLoc(pos, &loc))
                return false;
            Value argv[] = { v1, v2, loc };
            AutoValueArray ava(cx, argv, 3);
            return Invoke(cx, userv, fun, ArrayLength(argv), argv, dst.address());
        }

        Value argv[] = { v1, v2 };
        AutoValueArray ava(cx, argv, 2);
        return Invoke(cx, userv, fun, ArrayLength(argv), argv, dst.address());
    }

    bool callback(HandleValue fun, HandleValue v1, HandleValue v2, HandleValue v3, TokenPos *pos,
                  MutableHandleValue dst) {
        if (saveLoc) {
            RootedValue loc(cx);
            if (!newNodeLoc(pos, &loc))
                return false;
            Value argv[] = { v1, v2, v3, loc };
            AutoValueArray ava(cx, argv, 4);
            return Invoke(cx, userv, fun, ArrayLength(argv), argv, dst.address());
        }

        Value argv[] = { v1, v2, v3 };
        AutoValueArray ava(cx, argv, 3);
        return Invoke(cx, userv, fun, ArrayLength(argv), argv, dst.address());
    }

    bool callback(HandleValue fun, HandleValue v1, HandleValue v2, HandleValue v3, HandleValue v4,
                  TokenPos *pos, MutableHandleValue dst) {
        if (saveLoc) {
            RootedValue loc(cx);
            if (!newNodeLoc(pos, &loc))
                return false;
            Value argv[] = { v1, v2, v3, v4, loc };
            AutoValueArray ava(cx, argv, 5);
            return Invoke(cx, userv, fun, ArrayLength(argv), argv, dst.address());
        }

        Value argv[] = { v1, v2, v3, v4 };
        AutoValueArray ava(cx, argv, 4);
        return Invoke(cx, userv, fun, ArrayLength(argv), argv, dst.address());
    }

    bool callback(HandleValue fun, HandleValue v1, HandleValue v2, HandleValue v3, HandleValue v4,
                  HandleValue v5, TokenPos *pos, MutableHandleValue dst) {
        if (saveLoc) {
            RootedValue loc(cx);
            if (!newNodeLoc(pos, &loc))
                return false;
            Value argv[] = { v1, v2, v3, v4, v5, loc };
            AutoValueArray ava(cx, argv, 6);
            return Invoke(cx, userv, fun, ArrayLength(argv), argv, dst.address());
        }

        Value argv[] = { v1, v2, v3, v4, v5 };
        AutoValueArray ava(cx, argv, 5);
        return Invoke(cx, userv, fun, ArrayLength(argv), argv, dst.address());
    }

    // WARNING: Returning a Handle is non-standard, but it works in this case
    // because both |v| and |undefinedVal| are definitely rooted on a previous
    // stack frame (i.e. we're just choosing between two already-rooted
    // values).
    HandleValue opt(HandleValue v) {
        JS_ASSERT_IF(v.isMagic(), v.whyMagic() == JS_SERIALIZE_NO_NODE);
        return v.isMagic(JS_SERIALIZE_NO_NODE) ? undefinedVal : v;
    }

    bool atomValue(const char *s, MutableHandleValue dst) {
        /*
         * Bug 575416: instead of Atomize, lookup constant atoms in tbl file
         */
        RootedAtom atom(cx, Atomize(cx, s, strlen(s)));
        if (!atom)
            return false;

        dst.setString(atom);
        return true;
    }

    bool newObject(MutableHandleObject dst) {
        RootedObject nobj(cx, NewBuiltinClassInstance(cx, &ObjectClass));
        if (!nobj)
            return false;

        dst.set(nobj);
        return true;
    }

    bool newArray(NodeVector &elts, MutableHandleValue dst);

    bool newNode(ASTType type, TokenPos *pos, MutableHandleObject dst);

    bool newNode(ASTType type, TokenPos *pos, MutableHandleValue dst) {
        RootedObject node(cx);
        return newNode(type, pos, &node) &&
               setResult(node, dst);
    }

    bool newNode(ASTType type, TokenPos *pos,
                 const char *childName, HandleValue child,
                 MutableHandleValue dst) {
        RootedObject node(cx);
        return newNode(type, pos, &node) &&
               setProperty(node, childName, child) &&
               setResult(node, dst);
    }

    bool newNode(ASTType type, TokenPos *pos,
                 const char *childName1, HandleValue child1,
                 const char *childName2, HandleValue child2,
                 MutableHandleValue dst) {
        RootedObject node(cx);
        return newNode(type, pos, &node) &&
               setProperty(node, childName1, child1) &&
               setProperty(node, childName2, child2) &&
               setResult(node, dst);
    }

    bool newNode(ASTType type, TokenPos *pos,
                 const char *childName1, HandleValue child1,
                 const char *childName2, HandleValue child2,
                 const char *childName3, HandleValue child3,
                 MutableHandleValue dst) {
        RootedObject node(cx);
        return newNode(type, pos, &node) &&
               setProperty(node, childName1, child1) &&
               setProperty(node, childName2, child2) &&
               setProperty(node, childName3, child3) &&
               setResult(node, dst);
    }

    bool newNode(ASTType type, TokenPos *pos,
                 const char *childName1, HandleValue child1,
                 const char *childName2, HandleValue child2,
                 const char *childName3, HandleValue child3,
                 const char *childName4, HandleValue child4,
                 MutableHandleValue dst) {
        RootedObject node(cx);
        return newNode(type, pos, &node) &&
               setProperty(node, childName1, child1) &&
               setProperty(node, childName2, child2) &&
               setProperty(node, childName3, child3) &&
               setProperty(node, childName4, child4) &&
               setResult(node, dst);
    }

    bool newNode(ASTType type, TokenPos *pos,
                 const char *childName1, HandleValue child1,
                 const char *childName2, HandleValue child2,
                 const char *childName3, HandleValue child3,
                 const char *childName4, HandleValue child4,
                 const char *childName5, HandleValue child5,
                 MutableHandleValue dst) {
        RootedObject node(cx);
        return newNode(type, pos, &node) &&
               setProperty(node, childName1, child1) &&
               setProperty(node, childName2, child2) &&
               setProperty(node, childName3, child3) &&
               setProperty(node, childName4, child4) &&
               setProperty(node, childName5, child5) &&
               setResult(node, dst);
    }

    bool newNode(ASTType type, TokenPos *pos,
                 const char *childName1, HandleValue child1,
                 const char *childName2, HandleValue child2,
                 const char *childName3, HandleValue child3,
                 const char *childName4, HandleValue child4,
                 const char *childName5, HandleValue child5,
                 const char *childName6, HandleValue child6,
                 const char *childName7, HandleValue child7,
                 MutableHandleValue dst) {
        RootedObject node(cx);
        return newNode(type, pos, &node) &&
               setProperty(node, childName1, child1) &&
               setProperty(node, childName2, child2) &&
               setProperty(node, childName3, child3) &&
               setProperty(node, childName4, child4) &&
               setProperty(node, childName5, child5) &&
               setProperty(node, childName6, child6) &&
               setProperty(node, childName7, child7) &&
               setResult(node, dst);
    }

    bool newNode(ASTType type, TokenPos *pos,
                 const char *childName1, HandleValue child1,
                 const char *childName2, HandleValue child2,
                 const char *childName3, HandleValue child3,
                 const char *childName4, HandleValue child4,
                 const char *childName5, HandleValue child5,
                 const char *childName6, HandleValue child6,
                 const char *childName7, HandleValue child7,
				 const char *childName8, HandleValue child8,
                 MutableHandleValue dst) {
        RootedObject node(cx);
        return newNode(type, pos, &node) &&
               setProperty(node, childName1, child1) &&
               setProperty(node, childName2, child2) &&
               setProperty(node, childName3, child3) &&
               setProperty(node, childName4, child4) &&
               setProperty(node, childName5, child5) &&
               setProperty(node, childName6, child6) &&
               setProperty(node, childName7, child7) &&
			   setProperty(node, childName8, child8) &&
               setResult(node, dst);
    }

    bool listNode(ASTType type, const char *propName, NodeVector &elts, TokenPos *pos,
                  MutableHandleValue dst) {
        RootedValue array(cx);
        if (!newArray(elts, &array))
            return false;

        RootedValue cb(cx, callbacks[type]);
        if (!cb.isNull())
            return callback(cb, array, pos, dst);

        return newNode(type, pos, propName, array, dst);
    }

    bool setProperty(HandleObject obj, const char *name, HandleValue val) {
        JS_ASSERT_IF(val.isMagic(), val.whyMagic() == JS_SERIALIZE_NO_NODE);

        /*
         * Bug 575416: instead of Atomize, lookup constant atoms in tbl file
         */
        RootedAtom atom(cx, Atomize(cx, name, strlen(name)));
        if (!atom)
            return false;

        /* Represent "no node" as null and ensure users are not exposed to magic values. */
        RootedValue optVal(cx, val.isMagic(JS_SERIALIZE_NO_NODE) ? NullValue() : val);
        return JSObject::defineProperty(cx, obj, atom->asPropertyName(), optVal);
    }

    bool newNodeLoc(TokenPos *pos, MutableHandleValue dst);

    bool setNodeLoc(HandleObject node, TokenPos *pos);

    bool setResult(HandleObject obj, MutableHandleValue dst) {
        JS_ASSERT(obj);
        dst.setObject(*obj);
        return true;
    }
	//metadev
  public:

	bool program(NodeVector &elts, TokenPos *pos, MutableHandleValue dst)
	{
		return listNode(AST_PROGRAM, "body", elts, pos, dst);
	}


	bool blockStatement(NodeVector &elts, TokenPos *pos, MutableHandleValue dst)
	{
		return listNode(AST_BLOCK_STMT, "body", elts, pos, dst);
	}

	bool metaQuaziStatement(NodeVector &elts, TokenPos *pos, MutableHandleValue dst)
	{
		return listNode(AST_METAQUAZI_STMT, "body", elts, pos, dst);
	}

	bool metaExecStatement(HandleValue stmt, TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_METAEXEC_STMT]);
		if (!cb.isNull())
			return callback(cb, stmt, pos, dst);
		return newNode(AST_METAEXEC_STMT, pos, "body", stmt, dst);
	}

	bool expressionStatement(HandleValue expr, TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_EXPR_STMT]);
		if (!cb.isNull())
			return callback(cb, expr, pos, dst);

		return newNode(AST_EXPR_STMT, pos, "expression", expr, dst);
	}

	bool emptyStatement(TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_EMPTY_STMT]);
		if (!cb.isNull())
			return callback(cb, pos, dst);

		return newNode(AST_EMPTY_STMT, pos, dst);
	}

	bool ifStatement(HandleValue test, HandleValue cons, HandleValue alt, TokenPos *pos,
							 MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_IF_STMT]);
		if (!cb.isNull())
			return callback(cb, test, cons, opt(alt), pos, dst);

		return newNode(AST_IF_STMT, pos,
					   "test", test,
					   "consequent", cons,
					   "alternate", alt,
					   dst);
	}

	bool breakStatement(HandleValue label, TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_BREAK_STMT]);
		if (!cb.isNull())
			return callback(cb, opt(label), pos, dst);

		return newNode(AST_BREAK_STMT, pos, "label", label, dst);
	}

	bool continueStatement(HandleValue label, TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_CONTINUE_STMT]);
		if (!cb.isNull())
			return callback(cb, opt(label), pos, dst);

		return newNode(AST_CONTINUE_STMT, pos, "label", label, dst);
	}

	bool labeledStatement(HandleValue label, HandleValue stmt, TokenPos *pos,
								  MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_LAB_STMT]);
		if (!cb.isNull())
			return callback(cb, label, stmt, pos, dst);

		return newNode(AST_LAB_STMT, pos,
					   "label", label,
					   "body", stmt,
					   dst);
	}

	bool throwStatement(HandleValue arg, TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_THROW_STMT]);
		if (!cb.isNull())
			return callback(cb, arg, pos, dst);

		return newNode(AST_THROW_STMT, pos, "argument", arg, dst);
	}

	bool returnStatement(HandleValue arg, TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_RETURN_STMT]);
		if (!cb.isNull())
			return callback(cb, opt(arg), pos, dst);

		return newNode(AST_RETURN_STMT, pos, "argument", arg, dst);
	}

	bool forStatement(HandleValue init, HandleValue test, HandleValue update, HandleValue stmt,
							  TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_FOR_STMT]);
		if (!cb.isNull())
			return callback(cb, opt(init), opt(test), opt(update), stmt, pos, dst);

		return newNode(AST_FOR_STMT, pos,
					   "init", init,
					   "test", test,
					   "update", update,
					   "body", stmt,
					   dst);
	}

	bool forInStatement(HandleValue var, HandleValue expr, HandleValue stmt, bool isForEach,
								TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue isForEachVal(cx, BooleanValue(isForEach));

		RootedValue cb(cx, callbacks[AST_FOR_IN_STMT]);
		if (!cb.isNull())
			return callback(cb, var, expr, stmt, isForEachVal, pos, dst);

		return newNode(AST_FOR_IN_STMT, pos,
					   "left", var,
					   "right", expr,
					   "body", stmt,
					   "each", isForEachVal,
					   dst);
	}

	bool forOfStatement(HandleValue var, HandleValue expr, HandleValue stmt, TokenPos *pos,
								MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_FOR_OF_STMT]);
		if (!cb.isNull())
			return callback(cb, var, expr, stmt, pos, dst);

		return newNode(AST_FOR_OF_STMT, pos,
					   "left", var,
					   "right", expr,
					   "body", stmt,
					   dst);
	}

	bool withStatement(HandleValue expr, HandleValue stmt, TokenPos *pos,
							   MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_WITH_STMT]);
		if (!cb.isNull())
			return callback(cb, expr, stmt, pos, dst);

		return newNode(AST_WITH_STMT, pos,
					   "object", expr,
					   "body", stmt,
					   dst);
	}

	bool whileStatement(HandleValue test, HandleValue stmt, TokenPos *pos,
								MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_WHILE_STMT]);
		if (!cb.isNull())
			return callback(cb, test, stmt, pos, dst);

		return newNode(AST_WHILE_STMT, pos,
					   "test", test,
					   "body", stmt,
					   dst);
	}

	bool doWhileStatement(HandleValue stmt, HandleValue test, TokenPos *pos,
								  MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_DO_STMT]);
		if (!cb.isNull())
			return callback(cb, stmt, test, pos, dst);

		return newNode(AST_DO_STMT, pos,
					   "body", stmt,
					   "test", test,
					   dst);
	}

	bool switchStatement(HandleValue disc, NodeVector &elts, bool lexical, TokenPos *pos,
								 MutableHandleValue dst)
	{
		RootedValue array(cx);
		if (!newArray(elts, &array))
			return false;

		RootedValue lexicalVal(cx, BooleanValue(lexical));

		RootedValue cb(cx, callbacks[AST_SWITCH_STMT]);
		if (!cb.isNull())
			return callback(cb, disc, array, lexicalVal, pos, dst);

		return newNode(AST_SWITCH_STMT, pos,
					   "discriminant", disc,
					   "cases", array,
					   "lexical", lexicalVal,
					   dst);
	}

	bool tryStatement(HandleValue body, NodeVector &guarded, HandleValue unguarded,
							  HandleValue finally, TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue guardedHandlers(cx);
		if (!newArray(guarded, &guardedHandlers))
			return false;

		RootedValue cb(cx, callbacks[AST_TRY_STMT]);
		if (!cb.isNull())
			return callback(cb, body, guardedHandlers, unguarded, opt(finally), pos, dst);

		return newNode(AST_TRY_STMT, pos,
					   "block", body,
					   "guardedHandlers", guardedHandlers,
					   "handler", unguarded,
					   "finalizer", finally,
					   dst);
	}

	bool debuggerStatement(TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_DEBUGGER_STMT]);
		if (!cb.isNull())
			return callback(cb, pos, dst);

		return newNode(AST_DEBUGGER_STMT, pos, dst);
	}

	bool binaryExpression(BinaryOperator op, HandleValue left, HandleValue right, TokenPos *pos,
								  MutableHandleValue dst)
	{
		JS_ASSERT(op > BINOP_ERR && op < BINOP_LIMIT);

		RootedValue opName(cx);
		if (!atomValue(binopNames[op], &opName))
			return false;

		RootedValue cb(cx, callbacks[AST_BINARY_EXPR]);
		if (!cb.isNull())
			return callback(cb, opName, left, right, pos, dst);

		return newNode(AST_BINARY_EXPR, pos,
					   "operator", opName,
					   "left", left,
					   "right", right,
					   dst);
	}

	bool unaryExpression(UnaryOperator unop, HandleValue expr, TokenPos *pos,
								 MutableHandleValue dst)
	{
		JS_ASSERT(unop > UNOP_ERR && unop < UNOP_LIMIT);

		RootedValue opName(cx);
		if (!atomValue(unopNames[unop], &opName))
			return false;

		RootedValue cb(cx, callbacks[AST_UNARY_EXPR]);
		if (!cb.isNull())
			return callback(cb, opName, expr, pos, dst);

		RootedValue trueVal(cx, BooleanValue(true));
		return newNode(AST_UNARY_EXPR, pos,
					   "operator", opName,
					   "argument", expr,
					   "prefix", trueVal,
					   dst);
	}

	bool assignmentExpression(AssignmentOperator aop, HandleValue lhs, HandleValue rhs,
									  TokenPos *pos, MutableHandleValue dst)
	{
		JS_ASSERT(aop > AOP_ERR && aop < AOP_LIMIT);

		RootedValue opName(cx);
		if (!atomValue(aopNames[aop], &opName))
			return false;

		RootedValue cb(cx, callbacks[AST_ASSIGN_EXPR]);
		if (!cb.isNull())
			return callback(cb, opName, lhs, rhs, pos, dst);

		return newNode(AST_ASSIGN_EXPR, pos,
					   "operator", opName,
					   "left", lhs,
					   "right", rhs,
					   dst);
	}

	bool updateExpression(HandleValue expr, bool incr, bool prefix, TokenPos *pos,
								  MutableHandleValue dst)
	{
		RootedValue opName(cx);
		if (!atomValue(incr ? "++" : "--", &opName))
			return false;

		RootedValue prefixVal(cx, BooleanValue(prefix));

		RootedValue cb(cx, callbacks[AST_UPDATE_EXPR]);
		if (!cb.isNull())
			return callback(cb, expr, opName, prefixVal, pos, dst);

		return newNode(AST_UPDATE_EXPR, pos,
					   "operator", opName,
					   "argument", expr,
					   "prefix", prefixVal,
					   dst);
	}

	bool logicalExpression(bool lor, HandleValue left, HandleValue right, TokenPos *pos,
								   MutableHandleValue dst)
	{
		RootedValue opName(cx);
		if (!atomValue(lor ? "||" : "&&", &opName))
			return false;

		RootedValue cb(cx, callbacks[AST_LOGICAL_EXPR]);
		if (!cb.isNull())
			return callback(cb, opName, left, right, pos, dst);

		return newNode(AST_LOGICAL_EXPR, pos,
					   "operator", opName,
					   "left", left,
					   "right", right,
					   dst);
	}

	bool conditionalExpression(HandleValue test, HandleValue cons, HandleValue alt,
									   TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_COND_EXPR]);
		if (!cb.isNull())
			return callback(cb, test, cons, alt, pos, dst);

		return newNode(AST_COND_EXPR, pos,
					   "test", test,
					   "consequent", cons,
					   "alternate", alt,
					   dst);
	}

	bool sequenceExpression(NodeVector &elts, TokenPos *pos, MutableHandleValue dst)
	{
		return listNode(AST_LIST_EXPR, "expressions", elts, pos, dst);
	}

	bool callExpression(HandleValue callee, NodeVector &args, TokenPos *pos,
								MutableHandleValue dst)
	{
		RootedValue array(cx);
		if (!newArray(args, &array))
			return false;

		RootedValue cb(cx, callbacks[AST_CALL_EXPR]);
		if (!cb.isNull())
			return callback(cb, callee, array, pos, dst);

		return newNode(AST_CALL_EXPR, pos,
					   "callee", callee,
					   "arguments", array,
					   dst);
	}

	bool newExpression(HandleValue callee, NodeVector &args, TokenPos *pos,
							   MutableHandleValue dst)
	{
		RootedValue array(cx);
		if (!newArray(args, &array))
			return false;

		RootedValue cb(cx, callbacks[AST_NEW_EXPR]);
		if (!cb.isNull())
			return callback(cb, callee, array, pos, dst);

		return newNode(AST_NEW_EXPR, pos,
					   "callee", callee,
					   "arguments", array,
					   dst);
	}

	bool memberExpression(bool computed, HandleValue expr, HandleValue member, TokenPos *pos,
								  MutableHandleValue dst)
	{
		RootedValue computedVal(cx, BooleanValue(computed));

		RootedValue cb(cx, callbacks[AST_MEMBER_EXPR]);
		if (!cb.isNull())
			return callback(cb, computedVal, expr, member, pos, dst);

		return newNode(AST_MEMBER_EXPR, pos,
					   "object", expr,
					   "property", member,
					   "computed", computedVal,
					   dst);
	}

	bool arrayExpression(NodeVector &elts, TokenPos *pos, MutableHandleValue dst)
	{
		return listNode(AST_ARRAY_EXPR, "elements", elts, pos, dst);
	}

	bool spreadExpression(HandleValue expr, TokenPos *pos, MutableHandleValue dst)
	{
		return newNode(AST_SPREAD_EXPR, pos,
					   "expression", expr,
					   dst);
	}

	bool propertyPattern(HandleValue key, HandleValue patt, TokenPos *pos,
								 MutableHandleValue dst)
	{
		RootedValue kindName(cx);
		if (!atomValue("init", &kindName))
			return false;

		RootedValue cb(cx, callbacks[AST_PROP_PATT]);
		if (!cb.isNull())
			return callback(cb, key, patt, pos, dst);

		return newNode(AST_PROP_PATT, pos,
					   "key", key,
					   "value", patt,
					   "kind", kindName,
					   dst);
	}

	bool propertyInitializer(HandleValue key, HandleValue val, PropKind kind, TokenPos *pos,
									 MutableHandleValue dst)
	{
		RootedValue kindName(cx);
		if (!atomValue(kind == PROP_INIT
					   ? "init"
					   : kind == PROP_GETTER
					   ? "get"
					   : "set", &kindName)) {
			return false;
		}

		RootedValue cb(cx, callbacks[AST_PROPERTY]);
		if (!cb.isNull())
			return callback(cb, kindName, key, val, pos, dst);

		return newNode(AST_PROPERTY, pos,
					   "key", key,
					   "value", val,
					   "kind", kindName,
					   dst);
	}

	bool objectExpression(NodeVector &elts, TokenPos *pos, MutableHandleValue dst)
	{
		return listNode(AST_OBJECT_EXPR, "properties", elts, pos, dst);
	}

	bool thisExpression(TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_THIS_EXPR]);
		if (!cb.isNull())
			return callback(cb, pos, dst);

		return newNode(AST_THIS_EXPR, pos, dst);
	}

	bool yieldExpression(HandleValue arg, TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_YIELD_EXPR]);
		if (!cb.isNull())
			return callback(cb, opt(arg), pos, dst);

		return newNode(AST_YIELD_EXPR, pos, "argument", arg, dst);
	}

	bool comprehensionBlock(HandleValue patt, HandleValue src, bool isForEach, bool isForOf, TokenPos *pos,
									MutableHandleValue dst)
	{
		RootedValue isForEachVal(cx, BooleanValue(isForEach));
		RootedValue isForOfVal(cx, BooleanValue(isForOf));

		RootedValue cb(cx, callbacks[AST_COMP_BLOCK]);
		if (!cb.isNull())
			return callback(cb, patt, src, isForEachVal, isForOfVal, pos, dst);

		return newNode(AST_COMP_BLOCK, pos,
					   "left", patt,
					   "right", src,
					   "each", isForEachVal,
					   "of", isForOfVal,
					   dst);
	}

	bool comprehensionExpression(HandleValue body, NodeVector &blocks, HandleValue filter,
										 TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue array(cx);
		if (!newArray(blocks, &array))
			return false;

		RootedValue cb(cx, callbacks[AST_COMP_EXPR]);
		if (!cb.isNull())
			return callback(cb, body, array, opt(filter), pos, dst);

		return newNode(AST_COMP_EXPR, pos,
					   "body", body,
					   "blocks", array,
					   "filter", filter,
					   dst);
	}

	bool generatorExpression(HandleValue body, NodeVector &blocks, HandleValue filter,
									 TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue array(cx);
		if (!newArray(blocks, &array))
			return false;

		RootedValue cb(cx, callbacks[AST_GENERATOR_EXPR]);
		if (!cb.isNull())
			return callback(cb, body, array, opt(filter), pos, dst);

		return newNode(AST_GENERATOR_EXPR, pos,
					   "body", body,
					   "blocks", array,
					   "filter", filter,
					   dst);
	}

	bool letExpression(NodeVector &head, HandleValue expr, TokenPos *pos,
							   MutableHandleValue dst)
	{
		RootedValue array(cx);
		if (!newArray(head, &array))
			return false;

		RootedValue cb(cx, callbacks[AST_LET_EXPR]);
		if (!cb.isNull())
			return callback(cb, array, expr, pos, dst);

		return newNode(AST_LET_EXPR, pos,
					   "head", array,
					   "body", expr,
					   dst);
	}

	bool letStatement(NodeVector &head, HandleValue stmt, TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue array(cx);
		if (!newArray(head, &array))
			return false;

		RootedValue cb(cx, callbacks[AST_LET_STMT]);
		if (!cb.isNull())
			return callback(cb, array, stmt, pos, dst);

		return newNode(AST_LET_STMT, pos,
					   "head", array,
					   "body", stmt,
					   dst);
	}

	bool variableDeclaration(NodeVector &elts, VarDeclKind kind, TokenPos *pos,
									 MutableHandleValue dst)
	{
		JS_ASSERT(kind > VARDECL_ERR && kind < VARDECL_LIMIT);

		RootedValue array(cx), kindName(cx);
		if (!newArray(elts, &array) ||
			!atomValue(kind == VARDECL_CONST
					   ? "const"
					   : kind == VARDECL_LET
					   ? "let"
					   : "var", &kindName)) {
			return false;
		}

		RootedValue cb(cx, callbacks[AST_VAR_DECL]);
		if (!cb.isNull())
			return callback(cb, kindName, array, pos, dst);

		return newNode(AST_VAR_DECL, pos,
					   "kind", kindName,
					   "declarations", array,
					   dst);
	}

	bool variableDeclarator(HandleValue id, HandleValue init, TokenPos *pos,
									MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_VAR_DTOR]);
		if (!cb.isNull())
			return callback(cb, id, opt(init), pos, dst);

		return newNode(AST_VAR_DTOR, pos, "id", id, "init", init, dst);
	}

	bool switchCase(HandleValue expr, NodeVector &elts, TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue array(cx);
		if (!newArray(elts, &array))
			return false;

		RootedValue cb(cx, callbacks[AST_CASE]);
		if (!cb.isNull())
			return callback(cb, opt(expr), array, pos, dst);

		return newNode(AST_CASE, pos,
					   "test", expr,
					   "consequent", array,
					   dst);
	}

	bool catchClause(HandleValue var, HandleValue guard, HandleValue body, TokenPos *pos,
							 MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_CATCH]);
		if (!cb.isNull())
			return callback(cb, var, opt(guard), body, pos, dst);

		return newNode(AST_CATCH, pos,
					   "param", var,
					   "guard", guard,
					   "body", body,
					   dst);
	}

	bool literal(HandleValue val, TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_LITERAL]);
		if (!cb.isNull())
			return callback(cb, val, pos, dst);

		return newNode(AST_LITERAL, pos, "value", val, dst);
	}

	bool identifier(HandleValue name, TokenPos *pos, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_IDENTIFIER]);
		if (!cb.isNull())
			return callback(cb, name, pos, dst);

		return newNode(AST_IDENTIFIER, pos, "name", name, dst);
	}

	bool objectPattern(NodeVector &elts, TokenPos *pos, MutableHandleValue dst)
	{
		return listNode(AST_OBJECT_PATT, "properties", elts, pos, dst);
	}

	bool arrayPattern(NodeVector &elts, TokenPos *pos, MutableHandleValue dst)
	{
		return listNode(AST_ARRAY_PATT, "elements", elts, pos, dst);
	}

	bool module(TokenPos *pos, HandleValue name, HandleValue body, MutableHandleValue dst)
	{
		RootedValue cb(cx, callbacks[AST_MODULE_DECL]);
		if (!cb.isNull()) {
			return callback(cb, name, body, pos, dst);
		}

		return newNode(AST_MODULE_DECL, pos,
					   "name", name,
					   "body", body,
					   dst);
	}
	//metadev funcdecl
	bool function(ASTType type, TokenPos *pos,
						  HandleValue escid, HandleValue id, 
						  NodeVector &args, NodeVector &defaults,
						  HandleValue body, HandleValue rest,
						  bool isGenerator, bool isExpression,
						  MutableHandleValue dst)
	{
		RootedValue array(cx), defarray(cx);
		if (!newArray(args, &array))
			return false;
		if (!newArray(defaults, &defarray))
			return false;

		RootedValue isGeneratorVal(cx, BooleanValue(isGenerator));
		RootedValue isExpressionVal(cx, BooleanValue(isExpression));

		RootedValue cb(cx, callbacks[type]);
		if (!cb.isNull()) {
			return callback(cb, opt(id), array, body, isGeneratorVal, isExpressionVal, pos, dst);
		}
		return newNode(type, pos,
			"id", id.isUndefined() ? escid: id,
					   "params", array,
					   "defaults", defarray,
					   "body", body,
					   "rest", rest,
					   "generator", isGeneratorVal,
					   "expression", isExpressionVal,
					   dst);
	}


};


bool
NodeBuilder::newNode(ASTType type, TokenPos *pos, MutableHandleObject dst)
{
    JS_ASSERT(type > AST_ERROR && type < AST_LIMIT); 

    RootedValue tv(cx);
    RootedObject node(cx, NewBuiltinClassInstance(cx, &ObjectClass));
    if (!node ||
        !setNodeLoc(node, pos) ||
        !atomValue(nodeTypeNames[type], &tv) ||
        !setProperty(node, "type", tv)) {
        return false;
    }

    dst.set(node);
    return true;
}

bool
NodeBuilder::newArray(NodeVector &elts, MutableHandleValue dst)
{
    const size_t len = elts.length();
    if (len > UINT32_MAX) {
        js_ReportAllocationOverflow(cx);
        return false;
    }
    RootedObject array(cx, NewDenseAllocatedArray(cx, uint32_t(len)));
    if (!array)
        return false;

    for (size_t i = 0; i < len; i++) {
        RootedValue val(cx, elts[i]);

        JS_ASSERT_IF(val.isMagic(), val.whyMagic() == JS_SERIALIZE_NO_NODE);

        /* Represent "no node" as an array hole by not adding the value. */
        if (val.isMagic(JS_SERIALIZE_NO_NODE))
            continue;

        if (!JSObject::setElement(cx, array, array, i, &val, false))
            return false;
    }

    dst.setObject(*array);
    return true;
}

bool
NodeBuilder::newNodeLoc(TokenPos *pos, MutableHandleValue dst)
{
    if (!pos) {
        dst.setNull();
        return true;
    }

    RootedObject loc(cx);
    RootedObject to(cx);
    RootedValue val(cx);

    if (!newObject(&loc))
        return false;

    dst.setObject(*loc);

    uint32_t startLineNum, startColumnIndex;
    uint32_t endLineNum, endColumnIndex;
    tokenStream->srcCoords.lineNumAndColumnIndex(pos->begin, &startLineNum, &startColumnIndex);
    tokenStream->srcCoords.lineNumAndColumnIndex(pos->end, &endLineNum, &endColumnIndex);

    if (!newObject(&to))
        return false;
    val.setObject(*to);
    if (!setProperty(loc, "start", val))
        return false;
    val.setNumber(startLineNum);
    if (!setProperty(to, "line", val))
        return false;
    val.setNumber(startColumnIndex);
    if (!setProperty(to, "column", val))
        return false;

    if (!newObject(&to))
        return false;
    val.setObject(*to);
    if (!setProperty(loc, "end", val))
        return false;
    val.setNumber(endLineNum);
    if (!setProperty(to, "line", val))
        return false;
    val.setNumber(endColumnIndex);
    if (!setProperty(to, "column", val))
        return false;

    if (!setProperty(loc, "source", srcval))
        return false;

    return true;
}

bool
NodeBuilder::setNodeLoc(HandleObject node, TokenPos *pos)
{
    if (!saveLoc) {
        RootedValue nullVal(cx, NullValue());
        setProperty(node, "loc", nullVal);
        return true;
    }

    RootedValue loc(cx);
    return newNodeLoc(pos, &loc) &&
           setProperty(node, "loc", loc);
}




/*
 * Serialization of parse nodes to JavaScript objects.
 *
 * All serialization methods take a non-nullable ParseNode pointer.
 */
class ASTSerializer
{
    JSContext           *cx;
    Parser<FullParseHandler> *parser;
    NodeBuilder         nodeHandler;
    DebugOnly<uint32_t> lineno;

    Value unrootedAtomContents(JSAtom *atom) {
        return StringValue(atom ? atom : cx->names().empty);
    }

    BinaryOperator binop(ParseNodeKind kind, JSOp op);
    UnaryOperator unop(ParseNodeKind kind, JSOp op);
    AssignmentOperator aop(JSOp op);

    bool statements(ParseNode *pn, NodeVector &elts);
    bool expressions(ParseNode *pn, NodeVector &elts);
    bool leftAssociate(ParseNode *pn, MutableHandleValue dst);
    bool functionArgs(ParseNode *pn, ParseNode *pnargs, ParseNode *pndestruct, ParseNode *pnbody,
                      NodeVector &args, NodeVector &defaults, MutableHandleValue rest);

    bool sourceElement(ParseNode *pn, MutableHandleValue dst);

    bool declaration(ParseNode *pn, MutableHandleValue dst);
    bool variableDeclaration(ParseNode *pn, bool let, MutableHandleValue dst);
    bool variableDeclarator(ParseNode *pn, VarDeclKind *pkind, MutableHandleValue dst);
    bool let(ParseNode *pn, bool expr, MutableHandleValue dst);

    bool optStatement(ParseNode *pn, MutableHandleValue dst) {
        if (!pn) {
            dst.setMagic(JS_SERIALIZE_NO_NODE);
            return true;
        }
        return statement(pn, dst);
    }

    bool forInit(ParseNode *pn, MutableHandleValue dst);
    bool forOfOrIn(ParseNode *loop, ParseNode *head, HandleValue var, HandleValue stmt,
                   MutableHandleValue dst);
    bool statement(ParseNode *pn, MutableHandleValue dst);
    bool blockStatement(ParseNode *pn, MutableHandleValue dst);
	//metadev
	bool metaQuaziStatement(ParseNode *pn, MutableHandleValue dst);
	bool metaExecStatement(ParseNode *pn, MutableHandleValue dst);
    bool switchStatement(ParseNode *pn, MutableHandleValue dst);
    bool switchCase(ParseNode *pn, MutableHandleValue dst);
    bool tryStatement(ParseNode *pn, MutableHandleValue dst);
    bool catchClause(ParseNode *pn, bool *isGuarded, MutableHandleValue dst);

    bool optExpression(ParseNode *pn, MutableHandleValue dst) {
        if (!pn) {
            dst.setMagic(JS_SERIALIZE_NO_NODE);
            return true;
        }
        return expression(pn, dst);
    }

    bool expression(ParseNode *pn, MutableHandleValue dst);

    bool propertyName(ParseNode *pn, MutableHandleValue dst);
    bool property(ParseNode *pn, MutableHandleValue dst);

    bool optIdentifier(HandleAtom atom, TokenPos *pos, MutableHandleValue dst) {
        if (!atom) {
            dst.setMagic(JS_SERIALIZE_NO_NODE);
            return true;
        }
        return identifier(atom, pos, dst);
    }

    bool identifier(HandleAtom atom, TokenPos *pos, MutableHandleValue dst);
    bool identifier(ParseNode *pn, MutableHandleValue dst);
    bool literal(ParseNode *pn, MutableHandleValue dst);

    bool pattern(ParseNode *pn, VarDeclKind *pkind, MutableHandleValue dst);
    bool arrayPattern(ParseNode *pn, VarDeclKind *pkind, MutableHandleValue dst);
    bool objectPattern(ParseNode *pn, VarDeclKind *pkind, MutableHandleValue dst);

    bool module(ParseNode *pn, MutableHandleValue dst);
    bool function(ParseNode *pn, ASTType type, MutableHandleValue dst);
    bool functionArgsAndBody(ParseNode *pn, ParseNode *escpn, NodeVector &args, NodeVector &defaults,
                             MutableHandleValue body, MutableHandleValue rest, MutableHandleValue escid);
    bool moduleOrFunctionBody(ParseNode *pn, TokenPos *pos, MutableHandleValue dst);

    bool comprehensionBlock(ParseNode *pn, MutableHandleValue dst);
    bool comprehension(ParseNode *pn, MutableHandleValue dst);
    bool generatorExpression(ParseNode *pn, MutableHandleValue dst);

  public:
    ASTSerializer(JSContext *c, bool l, char const *src, uint32_t ln)
        : cx(c)
#ifdef DEBUG
        , lineno(ln)
#endif
		, nodeHandler(c, l, src)
    {}

    bool init(HandleObject userobj) {
        return nodeHandler.init(userobj);
    }

    void setParser(Parser<FullParseHandler> *p) {
        parser = p;
        nodeHandler.setTokenStream(&p->tokenStream);
    }

    bool program(ParseNode *pn, MutableHandleValue dst);
};

AssignmentOperator
ASTSerializer::aop(JSOp op)
{
    switch (op) {
      case JSOP_NOP:
        return AOP_ASSIGN;
      case JSOP_ADD:
        return AOP_PLUS;
      case JSOP_SUB:
        return AOP_MINUS;
      case JSOP_MUL:
        return AOP_STAR;
      case JSOP_DIV:
        return AOP_DIV;
      case JSOP_MOD:
        return AOP_MOD;
      case JSOP_LSH:
        return AOP_LSH;
      case JSOP_RSH:
        return AOP_RSH;
      case JSOP_URSH:
        return AOP_URSH;
      case JSOP_BITOR:
        return AOP_BITOR;
      case JSOP_BITXOR:
        return AOP_BITXOR;
      case JSOP_BITAND:
        return AOP_BITAND;
      default:
        return AOP_ERR;
    }
}

UnaryOperator
ASTSerializer::unop(ParseNodeKind kind, JSOp op)
{
	// metadev
	switch (kind) {
	  case PNK_DELETE:
		  return UNOP_DELETE;
	  case PNK_METAINLINE:
		  return UNOP_META_INLINE;
	  case PNK_METAEXEC:
		  return UNOP_META_EXEC;
	  case PNK_METAESC:
		  return UNOP_META_ESC;
	  case PNK_METADUCK:
		  return UNOP_META_DUCK;
	}

    switch (op) {
      case JSOP_NEG:
        return UNOP_NEG;
      case JSOP_POS:
        return UNOP_POS;
      case JSOP_NOT:
        return UNOP_NOT;
      case JSOP_BITNOT:
        return UNOP_BITNOT;
      case JSOP_TYPEOF:
      case JSOP_TYPEOFEXPR:
        return UNOP_TYPEOF;
      case JSOP_VOID:
        return UNOP_VOID;
      default:
        return UNOP_ERR;
    }
}

BinaryOperator
ASTSerializer::binop(ParseNodeKind kind, JSOp op)
{
    switch (kind) {
      case PNK_LSH:
        return BINOP_LSH;
      case PNK_RSH:
        return BINOP_RSH;
      case PNK_URSH:
        return BINOP_URSH;
      case PNK_LT:
        return BINOP_LT;
      case PNK_LE:
        return BINOP_LE;
      case PNK_GT:
        return BINOP_GT;
      case PNK_GE:
        return BINOP_GE;
      case PNK_EQ:
        return BINOP_EQ;
      case PNK_NE:
        return BINOP_NE;
      case PNK_STRICTEQ:
        return BINOP_STRICTEQ;
      case PNK_STRICTNE:
        return BINOP_STRICTNE;
      case PNK_ADD:
        return BINOP_ADD;
      case PNK_SUB:
        return BINOP_SUB;
      case PNK_STAR:
        return BINOP_STAR;
      case PNK_DIV:
        return BINOP_DIV;
      case PNK_MOD:
        return BINOP_MOD;
      case PNK_BITOR:
        return BINOP_BITOR;
      case PNK_BITXOR:
        return BINOP_BITXOR;
      case PNK_BITAND:
        return BINOP_BITAND;
      case PNK_IN:
        return BINOP_IN;
      case PNK_INSTANCEOF:
        return BINOP_INSTANCEOF;
      default:
        return BINOP_ERR;
    }
}

bool
ASTSerializer::statements(ParseNode *pn, NodeVector &elts)
{
    JS_ASSERT(pn->isKind(PNK_STATEMENTLIST));
    JS_ASSERT(pn->isArity(PN_LIST));

    if (!elts.reserve(pn->pn_count))
        return false;

    for (ParseNode *next = pn->pn_head; next; next = next->pn_next) {
        JS_ASSERT(pn->pn_pos.encloses(next->pn_pos));

        RootedValue elt(cx);
        if (!sourceElement(next, &elt))
            return false;
        elts.infallibleAppend(elt);
    }

    return true;
}

bool
ASTSerializer::expressions(ParseNode *pn, NodeVector &elts)
{
    if (!elts.reserve(pn->pn_count))
        return false;

    for (ParseNode *next = pn->pn_head; next; next = next->pn_next) {
        JS_ASSERT(pn->pn_pos.encloses(next->pn_pos));

        RootedValue elt(cx);
        if (!expression(next, &elt))
            return false;
        elts.infallibleAppend(elt);
    }

    return true;
}

bool
ASTSerializer::blockStatement(ParseNode *pn, MutableHandleValue dst)
{
    JS_ASSERT(pn->isKind(PNK_STATEMENTLIST));

    NodeVector stmts(cx);
    return statements(pn, stmts) &&
           nodeHandler.blockStatement(stmts, &pn->pn_pos, dst);
}

// metadev
bool
ASTSerializer::metaQuaziStatement(ParseNode *pn, MutableHandleValue dst)
{
    JS_ASSERT(pn->isKind(PNK_METAQUAZI));

	ParseNode *next = pn->pn_head;
    NodeVector stmts(cx);
    return statements(next, stmts) &&
           nodeHandler.metaQuaziStatement(stmts, &next->pn_pos, dst);
}

bool 
ASTSerializer::metaExecStatement(ParseNode *pn, MutableHandleValue dst)
{
	JS_ASSERT(pn->isKind(PNK_METAEXEC));
	ParseNode *next = pn->pn_head;
    RootedValue stmt(cx);
    return next && 
		   statement(next, &stmt) &&
           nodeHandler.metaExecStatement(stmt, &next->pn_pos, dst);
}

bool
ASTSerializer::program(ParseNode *pn, MutableHandleValue dst)
{
    JS_ASSERT(parser->tokenStream.srcCoords.lineNum(pn->pn_pos.begin) == lineno);

    NodeVector stmts(cx);
    return statements(pn, stmts) &&
           nodeHandler.program(stmts, &pn->pn_pos, dst);
}

bool
ASTSerializer::sourceElement(ParseNode *pn, MutableHandleValue dst)
{
    /* SpiderMonkey allows declarations even in pure statement contexts. */
    return statement(pn, dst);
}

bool
ASTSerializer::declaration(ParseNode *pn, MutableHandleValue dst)
{
    JS_ASSERT(pn->isKind(PNK_FUNCTION) ||
              pn->isKind(PNK_VAR) ||
              pn->isKind(PNK_LET) ||
              pn->isKind(PNK_CONST));

    switch (pn->getKind()) {
      case PNK_FUNCTION:
        return function(pn, AST_FUNC_DECL, dst);

      case PNK_VAR:
      case PNK_CONST:
        return variableDeclaration(pn, false, dst);

      default:
        JS_ASSERT(pn->isKind(PNK_LET));
        return variableDeclaration(pn, true, dst);
    }
}

bool
ASTSerializer::variableDeclaration(ParseNode *pn, bool let, MutableHandleValue dst)
{
    JS_ASSERT(let ? pn->isKind(PNK_LET) : (pn->isKind(PNK_VAR) || pn->isKind(PNK_CONST)));

    /* Later updated to VARDECL_CONST if we find a PND_CONST declarator. */
    VarDeclKind kind = let ? VARDECL_LET : VARDECL_VAR;

    NodeVector dtors(cx);
    if (!dtors.reserve(pn->pn_count))
        return false;
    for (ParseNode *next = pn->pn_head; next; next = next->pn_next) {
        RootedValue child(cx);
        if (!variableDeclarator(next, &kind, &child))
            return false;
        dtors.infallibleAppend(child);
    }
    return nodeHandler.variableDeclaration(dtors, kind, &pn->pn_pos, dst);
}

bool
ASTSerializer::variableDeclarator(ParseNode *pn, VarDeclKind *pkind, MutableHandleValue dst)
{
    ParseNode *pnleft;
    ParseNode *pnright;

    if (pn->isKind(PNK_NAME)) {
        pnleft = pn;
        pnright = pn->isUsed() ? NULL : pn->pn_expr;
        JS_ASSERT_IF(pnright, pn->pn_pos.encloses(pnright->pn_pos));
    } else if (pn->isKind(PNK_ASSIGN)) {
        pnleft = pn->pn_left;
        pnright = pn->pn_right;
        JS_ASSERT(pn->pn_pos.encloses(pnleft->pn_pos));
        JS_ASSERT(pn->pn_pos.encloses(pnright->pn_pos));
    } else {
        /* This happens for a destructuring declarator in a for-in/of loop. */
        pnleft = pn;
        pnright = NULL;
    }

    RootedValue left(cx), right(cx);
    return pattern(pnleft, pkind, &left) &&
           optExpression(pnright, &right) &&
           nodeHandler.variableDeclarator(left, right, &pn->pn_pos, dst);
}

bool
ASTSerializer::let(ParseNode *pn, bool expr, MutableHandleValue dst)
{
    JS_ASSERT(pn->pn_pos.encloses(pn->pn_left->pn_pos));
    JS_ASSERT(pn->pn_pos.encloses(pn->pn_right->pn_pos));

    ParseNode *letHead = pn->pn_left;
    LOCAL_ASSERT(letHead->isArity(PN_LIST));

    ParseNode *letBody = pn->pn_right;
    LOCAL_ASSERT(letBody->isKind(PNK_LEXICALSCOPE));

    NodeVector dtors(cx);
    if (!dtors.reserve(letHead->pn_count))
        return false;

    VarDeclKind kind = VARDECL_LET_HEAD;

    for (ParseNode *next = letHead->pn_head; next; next = next->pn_next) {
        RootedValue child(cx);
        /*
         * Unlike in |variableDeclaration|, this does not update |kind|; since let-heads do
         * not contain const declarations, declarators should never have PND_CONST set.
         */
        if (!variableDeclarator(next, &kind, &child))
            return false;
        dtors.infallibleAppend(child);
    }

    RootedValue v(cx);
    return expr
           ? expression(letBody->pn_expr, &v) &&
             nodeHandler.letExpression(dtors, v, &pn->pn_pos, dst)
           : statement(letBody->pn_expr, &v) &&
             nodeHandler.letStatement(dtors, v, &pn->pn_pos, dst);
}

bool
ASTSerializer::switchCase(ParseNode *pn, MutableHandleValue dst)
{
    JS_ASSERT_IF(pn->pn_left, pn->pn_pos.encloses(pn->pn_left->pn_pos));
    JS_ASSERT(pn->pn_pos.encloses(pn->pn_right->pn_pos));

    NodeVector stmts(cx);

    RootedValue expr(cx);

    return optExpression(pn->pn_left, &expr) &&
           statements(pn->pn_right, stmts) &&
           nodeHandler.switchCase(expr, stmts, &pn->pn_pos, dst);
}

bool
ASTSerializer::switchStatement(ParseNode *pn, MutableHandleValue dst)
{
    JS_ASSERT(pn->pn_pos.encloses(pn->pn_left->pn_pos));
    JS_ASSERT(pn->pn_pos.encloses(pn->pn_right->pn_pos));

    RootedValue disc(cx);

    if (!expression(pn->pn_left, &disc))
        return false;

    ParseNode *listNode;
    bool lexical;

    if (pn->pn_right->isKind(PNK_LEXICALSCOPE)) {
        listNode = pn->pn_right->pn_expr;
        lexical = true;
    } else {
        listNode = pn->pn_right;
        lexical = false;
    }

    NodeVector cases(cx);
    if (!cases.reserve(listNode->pn_count))
        return false;

    for (ParseNode *next = listNode->pn_head; next; next = next->pn_next) {
        RootedValue child(cx);
        if (!switchCase(next, &child))
            return false;
        cases.infallibleAppend(child);
    }

    return nodeHandler.switchStatement(disc, cases, lexical, &pn->pn_pos, dst);
}

bool
ASTSerializer::catchClause(ParseNode *pn, bool *isGuarded, MutableHandleValue dst)
{
    JS_ASSERT(pn->pn_pos.encloses(pn->pn_kid1->pn_pos));
    JS_ASSERT_IF(pn->pn_kid2, pn->pn_pos.encloses(pn->pn_kid2->pn_pos));
    JS_ASSERT(pn->pn_pos.encloses(pn->pn_kid3->pn_pos));

    RootedValue var(cx), guard(cx), body(cx);

    if (!pattern(pn->pn_kid1, NULL, &var) ||
        !optExpression(pn->pn_kid2, &guard)) {
        return false;
    }

    *isGuarded = !guard.isMagic(JS_SERIALIZE_NO_NODE);

    return statement(pn->pn_kid3, &body) &&
           nodeHandler.catchClause(var, guard, body, &pn->pn_pos, dst);
}

bool
ASTSerializer::tryStatement(ParseNode *pn, MutableHandleValue dst)
{
    JS_ASSERT(pn->pn_pos.encloses(pn->pn_kid1->pn_pos));
    JS_ASSERT_IF(pn->pn_kid2, pn->pn_pos.encloses(pn->pn_kid2->pn_pos));
    JS_ASSERT_IF(pn->pn_kid3, pn->pn_pos.encloses(pn->pn_kid3->pn_pos));

    RootedValue body(cx);
    if (!statement(pn->pn_kid1, &body))
        return false;

    NodeVector guarded(cx);
    RootedValue unguarded(cx, NullValue());

    if (pn->pn_kid2) {
        if (!guarded.reserve(pn->pn_kid2->pn_count))
            return false;

        for (ParseNode *next = pn->pn_kid2->pn_head; next; next = next->pn_next) {
            RootedValue clause(cx);
            bool isGuarded;
            if (!catchClause(next->pn_expr, &isGuarded, &clause))
                return false;
            if (isGuarded)
                guarded.infallibleAppend(clause);
            else
                unguarded = clause;
        }
    }

    RootedValue finally(cx);
    return optStatement(pn->pn_kid3, &finally) &&
           nodeHandler.tryStatement(body, guarded, unguarded, finally, &pn->pn_pos, dst);
}

bool
ASTSerializer::forInit(ParseNode *pn, MutableHandleValue dst)
{
    if (!pn) {
        dst.setMagic(JS_SERIALIZE_NO_NODE);
        return true;
    }

    return (pn->isKind(PNK_VAR) || pn->isKind(PNK_CONST))
           ? variableDeclaration(pn, false, dst)
           : expression(pn, dst);
}

bool
ASTSerializer::forOfOrIn(ParseNode *loop, ParseNode *head, HandleValue var, HandleValue stmt,
                         MutableHandleValue dst)
{
    RootedValue expr(cx);
    bool isForEach = loop->pn_iflags & JSITER_FOREACH;
    bool isForOf = loop->pn_iflags & JSITER_FOR_OF;
    JS_ASSERT(!isForOf || !isForEach);

    return expression(head->pn_kid3, &expr) &&
        (isForOf ? nodeHandler.forOfStatement(var, expr, stmt, &loop->pn_pos, dst) :
         nodeHandler.forInStatement(var, expr, stmt, isForEach, &loop->pn_pos, dst));
}

bool
ASTSerializer::statement(ParseNode *pn, MutableHandleValue dst)
{
    JS_CHECK_RECURSION(cx, return false);
    switch (pn->getKind()) {
      case PNK_MODULE:
        return module(pn, dst);

      case PNK_FUNCTION:
      case PNK_VAR:
      case PNK_CONST:
        return declaration(pn, dst);

      case PNK_LET:
        return pn->isArity(PN_BINARY)
               ? let(pn, false, dst)
               : declaration(pn, dst);

      case PNK_NAME:
        LOCAL_ASSERT(pn->isUsed());
        return statement(pn->pn_lexdef, dst);

      case PNK_SEMI:
        if (pn->pn_kid) {
            RootedValue expr(cx);
            return expression(pn->pn_kid, &expr) &&
                   nodeHandler.expressionStatement(expr, &pn->pn_pos, dst);
        }
        return nodeHandler.emptyStatement(&pn->pn_pos, dst);

      case PNK_LEXICALSCOPE:
        pn = pn->pn_expr;
        if (!pn->isKind(PNK_STATEMENTLIST))
            return statement(pn, dst);
        /* FALL THROUGH */

      case PNK_STATEMENTLIST:
        return blockStatement(pn, dst);

      case PNK_IF:
      {
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_kid1->pn_pos));
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_kid2->pn_pos));
        JS_ASSERT_IF(pn->pn_kid3, pn->pn_pos.encloses(pn->pn_kid3->pn_pos));

        RootedValue test(cx), cons(cx), alt(cx);

        return expression(pn->pn_kid1, &test) &&
               statement(pn->pn_kid2, &cons) &&
               optStatement(pn->pn_kid3, &alt) &&
               nodeHandler.ifStatement(test, cons, alt, &pn->pn_pos, dst);
      }

      case PNK_SWITCH:
        return switchStatement(pn, dst);

      case PNK_TRY:
        return tryStatement(pn, dst);

      case PNK_WITH:
      case PNK_WHILE:
      {
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_left->pn_pos));
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_right->pn_pos));

        RootedValue expr(cx), stmt(cx);

        return expression(pn->pn_left, &expr) &&
               statement(pn->pn_right, &stmt) &&
               (pn->isKind(PNK_WITH)
                ? nodeHandler.withStatement(expr, stmt, &pn->pn_pos, dst)
                : nodeHandler.whileStatement(expr, stmt, &pn->pn_pos, dst));
      }

      case PNK_DOWHILE:
      {
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_left->pn_pos));
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_right->pn_pos));

        RootedValue stmt(cx), test(cx);

        return statement(pn->pn_left, &stmt) &&
               expression(pn->pn_right, &test) &&
               nodeHandler.doWhileStatement(stmt, test, &pn->pn_pos, dst);
      }

      case PNK_FOR:
      {
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_left->pn_pos));
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_right->pn_pos));

        ParseNode *head = pn->pn_left;

        JS_ASSERT_IF(head->pn_kid1, head->pn_pos.encloses(head->pn_kid1->pn_pos));
        JS_ASSERT_IF(head->pn_kid2, head->pn_pos.encloses(head->pn_kid2->pn_pos));
        JS_ASSERT_IF(head->pn_kid3, head->pn_pos.encloses(head->pn_kid3->pn_pos));

        RootedValue stmt(cx);
        if (!statement(pn->pn_right, &stmt))
            return false;

        if (head->isKind(PNK_FORIN)) {
            RootedValue var(cx);
            return (!head->pn_kid1
                    ? pattern(head->pn_kid2, NULL, &var)
                    : head->pn_kid1->isKind(PNK_LEXICALSCOPE)
                    ? variableDeclaration(head->pn_kid1->pn_expr, true, &var)
                    : variableDeclaration(head->pn_kid1, false, &var)) &&
                forOfOrIn(pn, head, var, stmt, dst);
        }

        RootedValue init(cx), test(cx), update(cx);

        return forInit(head->pn_kid1, &init) &&
               optExpression(head->pn_kid2, &test) &&
               optExpression(head->pn_kid3, &update) &&
               nodeHandler.forStatement(init, test, update, stmt, &pn->pn_pos, dst);
      }

      /* Synthesized by the parser when a for-in loop contains a variable initializer. */
      case PNK_SEQ:
      {
        LOCAL_ASSERT(pn->pn_count == 2);

        ParseNode *prelude = pn->pn_head;
        ParseNode *loop = prelude->pn_next;

        LOCAL_ASSERT(prelude->isKind(PNK_VAR) && loop->isKind(PNK_FOR));

        RootedValue var(cx);
        if (!variableDeclaration(prelude, false, &var))
            return false;

        ParseNode *head = loop->pn_left;
        JS_ASSERT(head->isKind(PNK_FORIN));

        RootedValue stmt(cx);

        return statement(loop->pn_right, &stmt) && forOfOrIn(loop, head, var, stmt, dst);
      }

      case PNK_BREAK:
      case PNK_CONTINUE:
      {
        RootedValue label(cx);
        RootedAtom pnAtom(cx, pn->pn_atom);
        return optIdentifier(pnAtom, NULL, &label) &&
               (pn->isKind(PNK_BREAK)
                ? nodeHandler.breakStatement(label, &pn->pn_pos, dst)
                : nodeHandler.continueStatement(label, &pn->pn_pos, dst));
      }

      case PNK_LABEL:
      {
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_expr->pn_pos));

        RootedValue label(cx), stmt(cx);
        RootedAtom pnAtom(cx, pn->as<LabeledStatement>().label());
        return identifier(pnAtom, NULL, &label) &&
               statement(pn->pn_expr, &stmt) &&
               nodeHandler.labeledStatement(label, stmt, &pn->pn_pos, dst);
      }

      case PNK_THROW:
      case PNK_RETURN:
      {
        JS_ASSERT_IF(pn->pn_kid, pn->pn_pos.encloses(pn->pn_kid->pn_pos));

        RootedValue arg(cx);

        return optExpression(pn->pn_kid, &arg) &&
               (pn->isKind(PNK_THROW)
                ? nodeHandler.throwStatement(arg, &pn->pn_pos, dst)
                : nodeHandler.returnStatement(arg, &pn->pn_pos, dst));
      }

      case PNK_DEBUGGER:
        return nodeHandler.debuggerStatement(&pn->pn_pos, dst);

      case PNK_NOP:
        return nodeHandler.emptyStatement(&pn->pn_pos, dst);

      default:
        LOCAL_NOT_REACHED("unexpected statement type");
    }
}

bool
ASTSerializer::leftAssociate(ParseNode *pn, MutableHandleValue dst)
{
    JS_ASSERT(pn->isArity(PN_LIST));
    JS_ASSERT(pn->pn_count >= 1);

    ParseNodeKind kind = pn->getKind();
    bool lor = kind == PNK_OR;
    bool logop = lor || (kind == PNK_AND);

    ParseNode *head = pn->pn_head;
    RootedValue left(cx);
    if (!expression(head, &left))
        return false;
    for (ParseNode *next = head->pn_next; next; next = next->pn_next) {
        RootedValue right(cx);
        if (!expression(next, &right))
            return false;

        TokenPos subpos(pn->pn_pos.begin, next->pn_pos.end);

        if (logop) {
            if (!nodeHandler.logicalExpression(lor, left, right, &subpos, &left))
                return false;
        } else {
            BinaryOperator op = binop(pn->getKind(), pn->getOp());
            LOCAL_ASSERT(op > BINOP_ERR && op < BINOP_LIMIT);

            if (!nodeHandler.binaryExpression(op, left, right, &subpos, &left))
                return false;
        }
    }

    dst.set(left);
    return true;
}

bool
ASTSerializer::comprehensionBlock(ParseNode *pn, MutableHandleValue dst)
{
    LOCAL_ASSERT(pn->isArity(PN_BINARY));

    ParseNode *in = pn->pn_left;

    LOCAL_ASSERT(in && in->isKind(PNK_FORIN));

    bool isForEach = pn->pn_iflags & JSITER_FOREACH;
    bool isForOf = pn->pn_iflags & JSITER_FOR_OF;

    RootedValue patt(cx), src(cx);
    return pattern(in->pn_kid2, NULL, &patt) &&
           expression(in->pn_kid3, &src) &&
           nodeHandler.comprehensionBlock(patt, src, isForEach, isForOf, &in->pn_pos, dst);
}

bool
ASTSerializer::comprehension(ParseNode *pn, MutableHandleValue dst)
{
    LOCAL_ASSERT(pn->isKind(PNK_FOR));

    NodeVector blocks(cx);

    ParseNode *next = pn;
    while (next->isKind(PNK_FOR)) {
        RootedValue block(cx);
        if (!comprehensionBlock(next, &block) || !blocks.append(block))
            return false;
        next = next->pn_right;
    }

    RootedValue filter(cx, MagicValue(JS_SERIALIZE_NO_NODE));

    if (next->isKind(PNK_IF)) {
        if (!optExpression(next->pn_kid1, &filter))
            return false;
        next = next->pn_kid2;
    } else if (next->isKind(PNK_STATEMENTLIST) && next->pn_count == 0) {
        /* FoldConstants optimized away the push. */
        NodeVector empty(cx);
        return nodeHandler.arrayExpression(empty, &pn->pn_pos, dst);
    }

    LOCAL_ASSERT(next->isKind(PNK_ARRAYPUSH));

    RootedValue body(cx);

    return expression(next->pn_kid, &body) &&
           nodeHandler.comprehensionExpression(body, blocks, filter, &pn->pn_pos, dst);
}

bool
ASTSerializer::generatorExpression(ParseNode *pn, MutableHandleValue dst)
{
    LOCAL_ASSERT(pn->isKind(PNK_FOR));

    NodeVector blocks(cx);

    ParseNode *next = pn;
    while (next->isKind(PNK_FOR)) {
        RootedValue block(cx);
        if (!comprehensionBlock(next, &block) || !blocks.append(block))
            return false;
        next = next->pn_right;
    }

    RootedValue filter(cx, MagicValue(JS_SERIALIZE_NO_NODE));

    if (next->isKind(PNK_IF)) {
        if (!optExpression(next->pn_kid1, &filter))
            return false;
        next = next->pn_kid2;
    }

    LOCAL_ASSERT(next->isKind(PNK_SEMI) &&
                 next->pn_kid->isKind(PNK_YIELD) &&
                 next->pn_kid->pn_kid);

    RootedValue body(cx);

    return expression(next->pn_kid->pn_kid, &body) &&
           nodeHandler.generatorExpression(body, blocks, filter, &pn->pn_pos, dst);
}

bool
ASTSerializer::expression(ParseNode *pn, MutableHandleValue dst)
{
    JS_CHECK_RECURSION(cx, return false);
    switch (pn->getKind()) {
      case PNK_FUNCTION:
      {
        ASTType type = pn->pn_funbox->function()->isArrow() ? AST_ARROW_EXPR : AST_FUNC_EXPR;
        return function(pn, type, dst);
      }

      case PNK_COMMA:
      {
        NodeVector exprs(cx);
        return expressions(pn, exprs) &&
               nodeHandler.sequenceExpression(exprs, &pn->pn_pos, dst);
      }

      case PNK_CONDITIONAL:
      {
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_kid1->pn_pos));
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_kid2->pn_pos));
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_kid3->pn_pos));

        RootedValue test(cx), cons(cx), alt(cx);

        return expression(pn->pn_kid1, &test) &&
               expression(pn->pn_kid2, &cons) &&
               expression(pn->pn_kid3, &alt) &&
               nodeHandler.conditionalExpression(test, cons, alt, &pn->pn_pos, dst);
      }

      case PNK_OR:
      case PNK_AND:
      {
        if (pn->isArity(PN_BINARY)) {
            JS_ASSERT(pn->pn_pos.encloses(pn->pn_left->pn_pos));
            JS_ASSERT(pn->pn_pos.encloses(pn->pn_right->pn_pos));

            RootedValue left(cx), right(cx);
            return expression(pn->pn_left, &left) &&
                   expression(pn->pn_right, &right) &&
                   nodeHandler.logicalExpression(pn->isKind(PNK_OR), left, right, &pn->pn_pos, dst);
        }
        return leftAssociate(pn, dst);
      }

      case PNK_PREINCREMENT:
      case PNK_PREDECREMENT:
      {
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_kid->pn_pos));

        bool inc = pn->isKind(PNK_PREINCREMENT);
        RootedValue expr(cx);
        return expression(pn->pn_kid, &expr) &&
               nodeHandler.updateExpression(expr, inc, true, &pn->pn_pos, dst);
      }

      case PNK_POSTINCREMENT:
      case PNK_POSTDECREMENT:
      {
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_kid->pn_pos));

        bool inc = pn->isKind(PNK_POSTINCREMENT);
        RootedValue expr(cx);
        return expression(pn->pn_kid, &expr) &&
               nodeHandler.updateExpression(expr, inc, false, &pn->pn_pos, dst);
      }

      case PNK_ASSIGN:
      case PNK_ADDASSIGN:
      case PNK_SUBASSIGN:
      case PNK_BITORASSIGN:
      case PNK_BITXORASSIGN:
      case PNK_BITANDASSIGN:
      case PNK_LSHASSIGN:
      case PNK_RSHASSIGN:
      case PNK_URSHASSIGN:
      case PNK_MULASSIGN:
      case PNK_DIVASSIGN:
      case PNK_MODASSIGN:
      {
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_left->pn_pos));
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_right->pn_pos));

        AssignmentOperator op = aop(pn->getOp());
        LOCAL_ASSERT(op > AOP_ERR && op < AOP_LIMIT);

        RootedValue lhs(cx), rhs(cx);
        return pattern(pn->pn_left, NULL, &lhs) &&
               expression(pn->pn_right, &rhs) &&
               nodeHandler.assignmentExpression(op, lhs, rhs, &pn->pn_pos, dst);
      }

      case PNK_ADD:
      case PNK_SUB:
      case PNK_STRICTEQ:
      case PNK_EQ:
      case PNK_STRICTNE:
      case PNK_NE:
      case PNK_LT:
      case PNK_LE:
      case PNK_GT:
      case PNK_GE:
      case PNK_LSH:
      case PNK_RSH:
      case PNK_URSH:
      case PNK_STAR:
      case PNK_DIV:
      case PNK_MOD:
      case PNK_BITOR:
      case PNK_BITXOR:
      case PNK_BITAND:
      case PNK_IN:
      case PNK_INSTANCEOF:
        if (pn->isArity(PN_BINARY)) {
            JS_ASSERT(pn->pn_pos.encloses(pn->pn_left->pn_pos));
            JS_ASSERT(pn->pn_pos.encloses(pn->pn_right->pn_pos));

            BinaryOperator op = binop(pn->getKind(), pn->getOp());
            LOCAL_ASSERT(op > BINOP_ERR && op < BINOP_LIMIT);

            RootedValue left(cx), right(cx);
            return expression(pn->pn_left, &left) &&
                   expression(pn->pn_right, &right) &&
                   nodeHandler.binaryExpression(op, left, right, &pn->pn_pos, dst);
        }
        return leftAssociate(pn, dst);

	  // metadev
	  case PNK_METAQUAZI: {
		  return metaQuaziStatement(pn, dst);
	  }

	  case PNK_METAEXEC: {
		  return metaExecStatement(pn, dst);
	  }

      case PNK_DELETE:
	  case PNK_METAINLINE:
	  case PNK_METAESC:
	  case PNK_METADUCK:
      case PNK_TYPEOF:
      case PNK_VOID:
      case PNK_NOT:
      case PNK_BITNOT:
      case PNK_POS:
      case PNK_NEG: {
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_kid->pn_pos));

        UnaryOperator op = unop(pn->getKind(), pn->getOp());
        LOCAL_ASSERT(op > UNOP_ERR && op < UNOP_LIMIT);

        RootedValue expr(cx);
        return expression(pn->pn_kid, &expr) &&
               nodeHandler.unaryExpression(op, expr, &pn->pn_pos, dst);
      }

#if JS_HAS_GENERATOR_EXPRS
      case PNK_GENEXP:
        return generatorExpression(pn->generatorExpr(), dst);
#endif

      case PNK_NEW:
      case PNK_CALL:
      {
        ParseNode *next = pn->pn_head;
        JS_ASSERT(pn->pn_pos.encloses(next->pn_pos));

        RootedValue callee(cx);
        if (!expression(next, &callee))
            return false;

        NodeVector args(cx);
        if (!args.reserve(pn->pn_count - 1))
            return false;

        for (next = next->pn_next; next; next = next->pn_next) {
            JS_ASSERT(pn->pn_pos.encloses(next->pn_pos));

            RootedValue arg(cx);
            if (!expression(next, &arg))
                return false;
            args.infallibleAppend(arg);
        }

        return pn->isKind(PNK_NEW)
               ? nodeHandler.newExpression(callee, args, &pn->pn_pos, dst)

            : nodeHandler.callExpression(callee, args, &pn->pn_pos, dst);
      }

      case PNK_DOT:
      {
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_expr->pn_pos));

        RootedValue expr(cx), id(cx);
        RootedAtom pnAtom(cx, pn->pn_atom);
        return expression(pn->pn_expr, &expr) &&
               identifier(pnAtom, NULL, &id) &&
               nodeHandler.memberExpression(false, expr, id, &pn->pn_pos, dst);
      }

      case PNK_ELEM:
      {
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_left->pn_pos));
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_right->pn_pos));

        RootedValue left(cx), right(cx);
        return expression(pn->pn_left, &left) &&
               expression(pn->pn_right, &right) &&
               nodeHandler.memberExpression(true, left, right, &pn->pn_pos, dst);
      }

      case PNK_ARRAY:
      {
        NodeVector elts(cx);
        if (!elts.reserve(pn->pn_count))
            return false;

        for (ParseNode *next = pn->pn_head; next; next = next->pn_next) {
            JS_ASSERT(pn->pn_pos.encloses(next->pn_pos));

            if (next->isKind(PNK_ELISION)) {
                elts.infallibleAppend(NullValue());
            } else {
                RootedValue expr(cx);
                if (!expression(next, &expr))
                    return false;
                elts.infallibleAppend(expr);
            }
        }

        return nodeHandler.arrayExpression(elts, &pn->pn_pos, dst);
      }

      case PNK_SPREAD:
      {
          RootedValue expr(cx);
          return expression(pn->pn_kid, &expr) &&
                 nodeHandler.spreadExpression(expr, &pn->pn_pos, dst);
      }

      case PNK_OBJECT:
      {
        /* The parser notes any uninitialized properties by setting the PNX_DESTRUCT flag. */
        if (pn->pn_xflags & PNX_DESTRUCT) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_OBJECT_INIT);
            return false;
        }
        NodeVector elts(cx);
        if (!elts.reserve(pn->pn_count))
            return false;

        for (ParseNode *next = pn->pn_head; next; next = next->pn_next) {
            JS_ASSERT(pn->pn_pos.encloses(next->pn_pos));

            RootedValue prop(cx);
            if (!property(next, &prop))
                return false;
            elts.infallibleAppend(prop);
        }

        return nodeHandler.objectExpression(elts, &pn->pn_pos, dst);
      }

      case PNK_NAME:
        return identifier(pn, dst);

      case PNK_THIS:
        return nodeHandler.thisExpression(&pn->pn_pos, dst);

      case PNK_STRING:
      case PNK_REGEXP:
      case PNK_NUMBER:
      case PNK_TRUE:
      case PNK_FALSE:
      case PNK_NULL:
        return literal(pn, dst);

      case PNK_YIELD:
      {
        JS_ASSERT_IF(pn->pn_kid, pn->pn_pos.encloses(pn->pn_kid->pn_pos));

        RootedValue arg(cx);
        return optExpression(pn->pn_kid, &arg) &&
               nodeHandler.yieldExpression(arg, &pn->pn_pos, dst);
      }

      case PNK_ARRAYCOMP:
        JS_ASSERT(pn->pn_pos.encloses(pn->pn_head->pn_pos));

        /* NB: it's no longer the case that pn_count could be 2. */
        LOCAL_ASSERT(pn->pn_count == 1);
        LOCAL_ASSERT(pn->pn_head->isKind(PNK_LEXICALSCOPE));

        return comprehension(pn->pn_head->pn_expr, dst);

      case PNK_LET:
        return let(pn, true, dst);

      default:
        LOCAL_NOT_REACHED("unexpected expression type");
    }
}

bool
ASTSerializer::propertyName(ParseNode *pn, MutableHandleValue dst)
{
    if (pn->isKind(PNK_NAME))
        return identifier(pn, dst);

    LOCAL_ASSERT(pn->isKind(PNK_STRING) || pn->isKind(PNK_NUMBER));

    return literal(pn, dst);
}

bool
ASTSerializer::property(ParseNode *pn, MutableHandleValue dst)
{
    PropKind kind;
    switch (pn->getOp()) {
      case JSOP_INITPROP:
        kind = PROP_INIT;
        break;

      case JSOP_INITPROP_GETTER:
        kind = PROP_GETTER;
        break;

      case JSOP_INITPROP_SETTER:
        kind = PROP_SETTER;
        break;

      default:
        LOCAL_NOT_REACHED("unexpected object-literal property");
    }

    RootedValue key(cx), val(cx);
    return propertyName(pn->pn_left, &key) &&
           expression(pn->pn_right, &val) &&
           nodeHandler.propertyInitializer(key, val, kind, &pn->pn_pos, dst);
}

bool
ASTSerializer::literal(ParseNode *pn, MutableHandleValue dst)
{
    RootedValue val(cx);
    switch (pn->getKind()) {
      case PNK_STRING:
        val.setString(pn->pn_atom);
        break;

      case PNK_REGEXP:
      {
        RootedObject re1(cx, pn->as<RegExpLiteral>().objbox()->object);
        LOCAL_ASSERT(re1 && re1->is<RegExpObject>());

        RootedObject proto(cx);
        if (!js_GetClassPrototype(cx, JSProto_RegExp, &proto))
            return false;

        RootedObject re2(cx, CloneRegExpObject(cx, re1, proto));
        if (!re2)
            return false;

        val.setObject(*re2);
        break;
      }

      case PNK_NUMBER:
        val.setNumber(pn->pn_dval);
        break;

      case PNK_NULL:
        val.setNull();
        break;

      case PNK_TRUE:
        val.setBoolean(true);
        break;

      case PNK_FALSE:
        val.setBoolean(false);
        break;

      default:
        LOCAL_NOT_REACHED("unexpected literal type");
    }

    return nodeHandler.literal(val, &pn->pn_pos, dst);
}

bool
ASTSerializer::arrayPattern(ParseNode *pn, VarDeclKind *pkind, MutableHandleValue dst)
{
    JS_ASSERT(pn->isKind(PNK_ARRAY));

    NodeVector elts(cx);
    if (!elts.reserve(pn->pn_count))
        return false;

    for (ParseNode *next = pn->pn_head; next; next = next->pn_next) {
        if (next->isKind(PNK_ELISION)) {
            elts.infallibleAppend(NullValue());
        } else {
            RootedValue patt(cx);
            if (!pattern(next, pkind, &patt))
                return false;
            elts.infallibleAppend(patt);
        }
    }

    return nodeHandler.arrayPattern(elts, &pn->pn_pos, dst);
}

bool
ASTSerializer::objectPattern(ParseNode *pn, VarDeclKind *pkind, MutableHandleValue dst)
{
    JS_ASSERT(pn->isKind(PNK_OBJECT));

    NodeVector elts(cx);
    if (!elts.reserve(pn->pn_count))
        return false;

    for (ParseNode *next = pn->pn_head; next; next = next->pn_next) {
        LOCAL_ASSERT(next->isOp(JSOP_INITPROP));

        RootedValue key(cx), patt(cx), prop(cx);
        if (!propertyName(next->pn_left, &key) ||
            !pattern(next->pn_right, pkind, &patt) ||
            !nodeHandler.propertyPattern(key, patt, &next->pn_pos, &prop)) {
            return false;
        }

        elts.infallibleAppend(prop);
    }

    return nodeHandler.objectPattern(elts, &pn->pn_pos, dst);
}

bool
ASTSerializer::pattern(ParseNode *pn, VarDeclKind *pkind, MutableHandleValue dst)
{
    JS_CHECK_RECURSION(cx, return false);
    switch (pn->getKind()) {
      case PNK_OBJECT:
        return objectPattern(pn, pkind, dst);

      case PNK_ARRAY:
        return arrayPattern(pn, pkind, dst);

      case PNK_NAME:
        if (pkind && (pn->pn_dflags & PND_CONST))
            *pkind = VARDECL_CONST;
        /* FALL THROUGH */

      default:
        return expression(pn, dst);
    }
}

bool
ASTSerializer::identifier(HandleAtom atom, TokenPos *pos, MutableHandleValue dst)
{
    RootedValue atomContentsVal(cx, unrootedAtomContents(atom));
    return nodeHandler.identifier(atomContentsVal, pos, dst);
}

bool
ASTSerializer::identifier(ParseNode *pn, MutableHandleValue dst)
{
    LOCAL_ASSERT(pn->isArity(PN_NAME) || pn->isArity(PN_NULLARY));
    LOCAL_ASSERT(pn->pn_atom);

    RootedAtom pnAtom(cx, pn->pn_atom);
    return identifier(pnAtom, &pn->pn_pos, dst);
}

bool
ASTSerializer::module(ParseNode *pn, MutableHandleValue dst)
{
    RootedValue name(cx, StringValue(pn->atom()));
    RootedValue body(cx);
    return moduleOrFunctionBody(pn->pn_body->pn_head, &pn->pn_body->pn_pos, &body) &&
           nodeHandler.module(&pn->pn_pos, name, body, dst);
}

bool
ASTSerializer::function(ParseNode *pn, ASTType type, MutableHandleValue dst)
{
    RootedFunction func(cx, pn->pn_funbox->function());

    bool isGenerator =
#if JS_HAS_GENERATORS
        pn->pn_funbox->isGenerator();
#else
        false;
#endif

    bool isExpression =
#if JS_HAS_EXPR_CLOSURES
        func->isExprClosure();
#else
        false;
#endif

    RootedValue id(cx);
	if(!pn->pn_metaesc){
		RootedAtom funcAtom(cx, func->atom());
		if (!optIdentifier(funcAtom, NULL, &id))
			return false;
	}

    NodeVector args(cx);
    NodeVector defaults(cx);

    RootedValue body(cx), rest(cx), metaesc(cx);
    if (func->hasRest())
        rest.setUndefined();
    else
        rest.setNull();
    return functionArgsAndBody(pn->pn_body, pn->pn_metaesc, args, defaults, &body, &rest, &metaesc) &&
        nodeHandler.function(type, &pn->pn_pos, metaesc, id, args, defaults, body,
                         rest, isGenerator, isExpression, dst);
}

bool
ASTSerializer::functionArgsAndBody(ParseNode *pn, ParseNode *escpn, NodeVector &args, NodeVector &defaults,
                                   MutableHandleValue body, MutableHandleValue rest, MutableHandleValue escid)
{
    ParseNode *pnargs;
    ParseNode *pnbody;

    /* Extract the args and body separately. */
    if (pn->isKind(PNK_ARGSBODY)) {
        pnargs = pn;
        pnbody = pn->last();
    } else {
        pnargs = NULL;
        pnbody = pn;
    }

    ParseNode *pndestruct;
	if(escpn) {
		if(!expression(escpn, escid))
			return false;
	}

    /* Extract the destructuring assignments. */
    if (pnbody->isArity(PN_LIST) && (pnbody->pn_xflags & PNX_DESTRUCT)) {
        ParseNode *head = pnbody->pn_head;
        LOCAL_ASSERT(head && head->isKind(PNK_SEMI));

        pndestruct = head->pn_kid;
        LOCAL_ASSERT(pndestruct);
        LOCAL_ASSERT(pndestruct->isKind(PNK_VAR));
    } else {
        pndestruct = NULL;
    }

    /* Serialize the arguments and body. */
    switch (pnbody->getKind()) {
      case PNK_RETURN: /* expression closure, no destructured args */
        return functionArgs(pn, pnargs, NULL, pnbody, args, defaults, rest) &&
               expression(pnbody->pn_kid, body);

      case PNK_SEQ:    /* expression closure with destructured args */
      {
        ParseNode *pnstart = pnbody->pn_head->pn_next;
        LOCAL_ASSERT(pnstart && pnstart->isKind(PNK_RETURN));

        return functionArgs(pn, pnargs, pndestruct, pnbody, args, defaults, rest) &&
               expression(pnstart->pn_kid, body);
      }

      case PNK_STATEMENTLIST:     /* statement closure */
      {
        ParseNode *pnstart = (pnbody->pn_xflags & PNX_DESTRUCT)
                               ? pnbody->pn_head->pn_next
                               : pnbody->pn_head;

        return functionArgs(pn, pnargs, pndestruct, pnbody, args, defaults, rest) &&
               moduleOrFunctionBody(pnstart, &pnbody->pn_pos, body);
      }

      default:
        LOCAL_NOT_REACHED("unexpected function contents");
    }
}

bool
ASTSerializer::functionArgs(ParseNode *pn, ParseNode *pnargs, ParseNode *pndestruct,
                            ParseNode *pnbody, NodeVector &args, NodeVector &defaults,
                            MutableHandleValue rest)
{
    uint32_t i = 0;
    ParseNode *arg = pnargs ? pnargs->pn_head : NULL;
    ParseNode *destruct = pndestruct ? pndestruct->pn_head : NULL;
    RootedValue node(cx);

    /*
     * Arguments are found in potentially two different places: 1) the
     * argsbody sequence (which ends with the body node), or 2) a
     * destructuring initialization at the beginning of the body. Loop
     * |arg| through the argsbody and |destruct| through the initial
     * destructuring assignments, stopping only when we've exhausted
     * both.
     */
    while ((arg && arg != pnbody) || destruct) {
        if (destruct && destruct->pn_right->frameSlot() == i) {
            if (!pattern(destruct->pn_left, NULL, &node) || !args.append(node))
                return false;
            destruct = destruct->pn_next;
        } else if (arg && arg != pnbody) {
            /*
             * We don't check that arg->frameSlot() == i since we
             * can't call that method if the arg def has been turned
             * into a use, e.g.:
             *
             *     function(a) { function a() { } }
             *
             * There's no other way to ask a non-destructuring arg its
             * index in the formals list, so we rely on the ability to
             * ask destructuring args their index above.
             */
			JS_ASSERT(arg->isKind(PNK_METAESC) ||arg->isKind(PNK_NAME) || arg->isKind(PNK_ASSIGN));
			//metadev funcdecl
			if(arg->isKind(PNK_METAESC)) {
				if (!expression(arg, &node))
					return false;
				if (!args.append(node))
					return false;
			} else {
			
				ParseNode *argName = arg->isKind(PNK_NAME) ? arg : arg->pn_left;
				if (!identifier(argName, &node))
					return false;
				if (rest.isUndefined() && arg->pn_next == pnbody)
					rest.setObject(node.toObject());
				else if (!args.append(node))
					return false;
				if (arg->pn_dflags & PND_DEFAULT) {
					ParseNode *expr = arg->isDefn() ? arg->expr() : arg->pn_kid->pn_right;
					RootedValue def(cx);
					if (!expression(expr, &def) || !defaults.append(def))
						return false;
				}
			}

            arg = arg->pn_next;
        } else {
            LOCAL_NOT_REACHED("missing function argument");
        }
        ++i;
    }
    JS_ASSERT(!rest.isUndefined());

    return true;
}

bool
ASTSerializer::moduleOrFunctionBody(ParseNode *pn, TokenPos *pos, MutableHandleValue dst)
{
    NodeVector elts(cx);

    /* We aren't sure how many elements there are up front, so we'll check each append. */
    for (ParseNode *next = pn; next; next = next->pn_next) {
        RootedValue child(cx);
        if (!sourceElement(next, &child) || !elts.append(child))
            return false;
    }

    return nodeHandler.blockStatement(elts, pos, dst);
}

static JSBool
reflect_parse(JSContext *cx, uint32_t argc, jsval *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             "Reflect.parse", "0", "s");
        return JS_FALSE;
    }

    RootedString src(cx, ToString<CanGC>(cx, args.handleAt(0)));
    if (!src)
        return JS_FALSE;

    ScopedJSFreePtr<char> filename;
    uint32_t lineno = 1;
    bool loc = true;

    RootedObject builder(cx);

    RootedValue arg(cx, args.get(1));

    if (!arg.isNullOrUndefined()) {
        if (!arg.isObject()) {
            js_ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_UNEXPECTED_TYPE,
                                     JSDVG_SEARCH_STACK, arg, NullPtr(), "not an object", NULL);
            return JS_FALSE;
        }

        RootedObject config(cx, &arg.toObject());

        RootedValue prop(cx);

        /* config.loc */
        RootedId locId(cx, NameToId(cx->names().loc));
        RootedValue trueVal(cx, BooleanValue(true));
        if (!baseops::GetPropertyDefault(cx, config, locId, trueVal, &prop))
            return JS_FALSE;

        loc = ToBoolean(prop);
		// metadev TODO: call jsreflect with loc false
		loc = false;
        if (loc) {
            /* config.source */
            RootedId sourceId(cx, NameToId(cx->names().source));
            RootedValue nullVal(cx, NullValue());
            if (!baseops::GetPropertyDefault(cx, config, sourceId, nullVal, &prop))
                return JS_FALSE;

            if (!prop.isNullOrUndefined()) {
                RootedString str(cx, ToString<CanGC>(cx, prop));
                if (!str)
                    return JS_FALSE;

                size_t length = str->length();
                const jschar *chars = str->getChars(cx);
                if (!chars)
                    return JS_FALSE;

                TwoByteChars tbchars(chars, length);
                filename = LossyTwoByteCharsToNewLatin1CharsZ(cx, tbchars).c_str();
                if (!filename)
                    return JS_FALSE;
            }

            /* config.line */
            RootedId lineId(cx, NameToId(cx->names().line));
            RootedValue oneValue(cx, Int32Value(1));
            if (!baseops::GetPropertyDefault(cx, config, lineId, oneValue, &prop) ||
                !ToUint32(cx, prop, &lineno)) {
                return JS_FALSE;
            }
        }

        /* config.builder */
        RootedId builderId(cx, NameToId(cx->names().builder));
        RootedValue nullVal(cx, NullValue());
        if (!baseops::GetPropertyDefault(cx, config, builderId, nullVal, &prop))
            return JS_FALSE;

        if (!prop.isNullOrUndefined()) {
            if (!prop.isObject()) {
                js_ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_UNEXPECTED_TYPE,
                                         JSDVG_SEARCH_STACK, prop, NullPtr(), "not an object", NULL);
                return JS_FALSE;
            }
            builder = &prop.toObject();
        }
    }

    /* Extract the builder methods first to report errors before parsing. */
	ASTSerializer serialize(cx, loc, filename, lineno);
    if (!serialize.init(builder))
        return JS_FALSE;

    JSStableString *stable = src->ensureStable(cx);
    if (!stable)
        return JS_FALSE;

    const StableCharPtr chars = stable->chars();
    size_t length = stable->length();
    CompileOptions options(cx);
    options.setFileAndLine(filename, lineno);
    options.setCanLazilyParse(false);
    Parser<FullParseHandler> parser(cx, options, chars.get(), length,
                                    /* foldConstants = */ false, NULL, NULL);

    serialize.setParser(&parser);

    ParseNode *pn = parser.parse(NULL);
    if (!pn)
        return JS_FALSE;

    RootedValue val(cx);
    if (!serialize.program(pn, &val)) {
        args.rval().setNull();
        return JS_FALSE;
    }

    args.rval().set(val);
    return JS_TRUE;
}

//metadev
JS_PUBLIC_API(bool)
reflect_parse_from_string(JSContext *cx, jschar *jsQuaziSnippet, uint32_t quaziSnippetLength, jsval *vp)
{
    ScopedJSFreePtr<char> filename;
    uint32_t lineno = 1;
    CompileOptions options(cx);
    options.setFileAndLine("<string>", 1)
			.setCompileAndGo(false);
    Parser<FullParseHandler> parser(cx, options, jsQuaziSnippet, quaziSnippetLength,
                                    /* foldConstants = */ false, NULL, NULL);
	RootedObject builder(cx);
	bool loc = true;
	ASTSerializer serialize(cx, loc, filename, lineno);
    if (!serialize.init(builder)){
        return JS_FALSE;
	}
    serialize.setParser(&parser);
	
    ParseNode *pn = parser.parse(NULL);
    if (!pn){
		JS_ReportException(cx);
        return JS_FALSE;
	}

    RootedValue retVal(cx);
    if (!serialize.program(pn, &retVal)) {
        return JS_FALSE;
    }
	
	vp->setObject( retVal.toObject() );
	
    return JS_TRUE;
} 


JS_PUBLIC_API(bool)
reflect_unparse(JSContext *cx, JSObject *obj)
{

    return JS_TRUE;
} 

////////////////////////////////////////////

static const JSFunctionSpec static_methods[] = {
    JS_FN("parse", reflect_parse, 1, 0),
    JS_FS_END
};

JS_PUBLIC_API(JSObject *)
JS_InitReflect(JSContext *cx, JSObject *objArg)
{
    RootedObject obj(cx, objArg);
    RootedObject Reflect(cx, NewObjectWithClassProto(cx, &ObjectClass, NULL, obj, SingletonObject));
    if (!Reflect)
        return NULL;

    if (!JS_DefineProperty(cx, obj, "Reflect", OBJECT_TO_JSVAL(Reflect),
                           JS_PropertyStub, JS_StrictPropertyStub, 0)) {
        return NULL;
    }

    if (!JS_DefineFunctions(cx, Reflect, static_methods))
        return NULL;

    return Reflect;
}
