// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "Builtins.h"

#include "Luau/Bytecode.h"
#include "Luau/Compiler.h"

LUAU_FASTFLAGVARIABLE(LuauVectorBuiltins)

namespace Luau
{
namespace Compile
{

Builtin getBuiltin(AstExpr* node, const DenseHashMap<AstName, Global>& globals, const DenseHashMap<AstLocal*, Variable>& variables)
{
    if (AstExprLocal* expr = node->as<AstExprLocal>())
    {
        const Variable* v = variables.find(expr->local);

        return v && !v->written && v->init ? getBuiltin(v->init, globals, variables) : Builtin();
    }
    else if (AstExprIndexName* expr = node->as<AstExprIndexName>())
    {
        if (AstExprGlobal* object = expr->expr->as<AstExprGlobal>())
        {
            return getGlobalState(globals, object->name) == Global::Default ? Builtin{object->name, expr->index} : Builtin();
        }
        else
        {
            return Builtin();
        }
    }
    else if (AstExprGlobal* expr = node->as<AstExprGlobal>())
    {
        return getGlobalState(globals, expr->name) == Global::Default ? Builtin{AstName(), expr->name} : Builtin();
    }
    else
    {
        return Builtin();
    }
}

static int getBuiltinFunctionId(const Builtin& builtin, const CompileOptions& options)
{
    /*assert*/ scrypt_def(STR_0, "\x9f\x8d\x8d\x9b\x8e\x8c");
    if (builtin.isGlobal(STR_0->c_str()))
        return LBF_ASSERT;
    
    /*type*/ scrypt_def(STR_1, "\x8c\x87\x90\x9b");
    if (builtin.isGlobal(STR_1->c_str()))
        return LBF_TYPE;
    
    /*typeof*/ scrypt_def(STR_2, "\x8c\x87\x90\x9b\x91\x9a");
    if (builtin.isGlobal(STR_2->c_str()))
        return LBF_TYPEOF;

    /*rawset*/ scrypt_def(STR_3, "\x8e\x9f\x89\x8d\x9b\x8c");
    if (builtin.isGlobal(STR_3->c_str()))
        return LBF_RAWSET;
    /*rawget*/ scrypt_def(STR_4, "\x8e\x9f\x89\x99\x9b\x8c");
    if (builtin.isGlobal(STR_4->c_str()))
        return LBF_RAWGET;
    /*rawequal*/ scrypt_def(STR_5, "\x8e\x9f\x89\x9b\x8f\x8b\x9f\x94");
    if (builtin.isGlobal(STR_5->c_str()))
        return LBF_RAWEQUAL;
    /*rawlen*/ scrypt_def(STR_6, "\x8e\x9f\x89\x94\x9b\x92");
    if (builtin.isGlobal(STR_6->c_str()))
        return LBF_RAWLEN;
    
    /*unpack*/ scrypt_def(STR_7, "\x8b\x92\x90\x9f\x9d\x95");
    if (builtin.isGlobal(STR_7->c_str()))
        return LBF_TABLE_UNPACK;
    
    /*select*/ scrypt_def(STR_8, "\x8d\x9b\x94\x9b\x9d\x8c");
    if (builtin.isGlobal(STR_8->c_str()))
        return LBF_SELECT_VARARG;
    
    /*getmetatable*/ scrypt_def(STR_9, "\x99\x9b\x8c\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b");
    if (builtin.isGlobal(STR_9->c_str()))
        return LBF_GETMETATABLE;
    /*setmetatable*/ scrypt_def(STR_10, "\x8d\x9b\x8c\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b");
    if (builtin.isGlobal(STR_10->c_str()))
        return LBF_SETMETATABLE;

    /*tonumber*/ scrypt_def(STR_11, "\x8c\x91\x92\x8b\x93\x9e\x9b\x8e");
    if (builtin.isGlobal(STR_11->c_str()))
        return LBF_TONUMBER;
    /*tostring*/ scrypt_def(STR_12, "\x8c\x91\x8d\x8c\x8e\x97\x92\x99");
    if (builtin.isGlobal(STR_12->c_str()))
        return LBF_TOSTRING;

    /*math*/ scrypt_def(STR_13, "\x93\x9f\x8c\x98");
    if (builtin.object == STR_13->c_str())
    {
        /*abs*/ scrypt_def(STR_14, "\x9f\x9e\x8d");
        if (builtin.method == STR_14->c_str())
            return LBF_MATH_ABS;
        /*acos*/ scrypt_def(STR_15, "\x9f\x9d\x91\x8d");
        if (builtin.method == STR_15->c_str())
            return LBF_MATH_ACOS;
        /*asin*/ scrypt_def(STR_16, "\x9f\x8d\x97\x92");
        if (builtin.method == STR_16->c_str())
            return LBF_MATH_ASIN;
        /*atan2*/ scrypt_def(STR_17, "\x9f\x8c\x9f\x92\xce");
        if (builtin.method == STR_17->c_str())
            return LBF_MATH_ATAN2;
        /*atan*/ scrypt_def(STR_18, "\x9f\x8c\x9f\x92");
        if (builtin.method == STR_18->c_str())
            return LBF_MATH_ATAN;
        /*ceil*/ scrypt_def(STR_19, "\x9d\x9b\x97\x94");
        if (builtin.method == STR_19->c_str())
            return LBF_MATH_CEIL;
        /*cosh*/ scrypt_def(STR_20, "\x9d\x91\x8d\x98");
        if (builtin.method == STR_20->c_str())
            return LBF_MATH_COSH;
        /*cos*/ scrypt_def(STR_21, "\x9d\x91\x8d");
        if (builtin.method == STR_21->c_str())
            return LBF_MATH_COS;
        /*deg*/ scrypt_def(STR_22, "\x9c\x9b\x99");
        if (builtin.method == STR_22->c_str())
            return LBF_MATH_DEG;
        /*exp*/ scrypt_def(STR_23, "\x9b\x88\x90");
        if (builtin.method == STR_23->c_str())
            return LBF_MATH_EXP;
        /*floor*/ scrypt_def(STR_24, "\x9a\x94\x91\x91\x8e");
        if (builtin.method == STR_24->c_str())
            return LBF_MATH_FLOOR;
        /*fmod*/ scrypt_def(STR_25, "\x9a\x93\x91\x9c");
        if (builtin.method == STR_25->c_str())
            return LBF_MATH_FMOD;
        /*frexp*/ scrypt_def(STR_26, "\x9a\x8e\x9b\x88\x90");
        if (builtin.method == STR_26->c_str())
            return LBF_MATH_FREXP;
        /*ldexp*/ scrypt_def(STR_27, "\x94\x9c\x9b\x88\x90");
        if (builtin.method == STR_27->c_str())
            return LBF_MATH_LDEXP;
        /*log10*/ scrypt_def(STR_28, "\x94\x91\x99\xcf\xd0");
        if (builtin.method == STR_28->c_str())
            return LBF_MATH_LOG10;
        /*log*/ scrypt_def(STR_29, "\x94\x91\x99");
        if (builtin.method == STR_29->c_str())
            return LBF_MATH_LOG;
        /*max*/ scrypt_def(STR_30, "\x93\x9f\x88");
        if (builtin.method == STR_30->c_str())
            return LBF_MATH_MAX;
        /*min*/ scrypt_def(STR_31, "\x93\x97\x92");
        if (builtin.method == STR_31->c_str())
            return LBF_MATH_MIN;
        /*modf*/ scrypt_def(STR_32, "\x93\x91\x9c\x9a");
        if (builtin.method == STR_32->c_str())
            return LBF_MATH_MODF;
        /*pow*/ scrypt_def(STR_33, "\x90\x91\x89");
        if (builtin.method == STR_33->c_str())
            return LBF_MATH_POW;
        /*rad*/ scrypt_def(STR_34, "\x8e\x9f\x9c");
        if (builtin.method == STR_34->c_str())
            return LBF_MATH_RAD;
        /*sinh*/ scrypt_def(STR_35, "\x8d\x97\x92\x98");
        if (builtin.method == STR_35->c_str())
            return LBF_MATH_SINH;
        /*sin*/ scrypt_def(STR_36, "\x8d\x97\x92");
        if (builtin.method == STR_36->c_str())
            return LBF_MATH_SIN;
        /*sqrt*/ scrypt_def(STR_37, "\x8d\x8f\x8e\x8c");
        if (builtin.method == STR_37->c_str())
            return LBF_MATH_SQRT;
        /*tanh*/ scrypt_def(STR_38, "\x8c\x9f\x92\x98");
        if (builtin.method == STR_38->c_str())
            return LBF_MATH_TANH;
        /*tan*/ scrypt_def(STR_39, "\x8c\x9f\x92");
        if (builtin.method == STR_39->c_str())
            return LBF_MATH_TAN;
        /*clamp*/ scrypt_def(STR_40, "\x9d\x94\x9f\x93\x90");
        if (builtin.method == STR_40->c_str())
            return LBF_MATH_CLAMP;
        /*sign*/ scrypt_def(STR_41, "\x8d\x97\x99\x92");
        if (builtin.method == STR_41->c_str())
            return LBF_MATH_SIGN;
        /*round*/ scrypt_def(STR_42, "\x8e\x91\x8b\x92\x9c");
        if (builtin.method == STR_42->c_str())
            return LBF_MATH_ROUND;
    }

    /*bit32*/ scrypt_def(STR_43, "\x9e\x97\x8c\xcd\xce");
    if (builtin.object == STR_43->c_str())
    {
        /*arshift*/ scrypt_def(STR_44, "\x9f\x8e\x8d\x98\x97\x9a\x8c");
        if (builtin.method == STR_44->c_str())
            return LBF_BIT32_ARSHIFT;
        /*band*/ scrypt_def(STR_45, "\x9e\x9f\x92\x9c");
        if (builtin.method == STR_45->c_str())
            return LBF_BIT32_BAND;
        /*bnot*/ scrypt_def(STR_46, "\x9e\x92\x91\x8c");
        if (builtin.method == STR_46->c_str())
            return LBF_BIT32_BNOT;
        /*bor*/ scrypt_def(STR_47, "\x9e\x91\x8e");
        if (builtin.method == STR_47->c_str())
            return LBF_BIT32_BOR;
        /*bxor*/ scrypt_def(STR_48, "\x9e\x88\x91\x8e");
        if (builtin.method == STR_48->c_str())
            return LBF_BIT32_BXOR;
        /*btest*/ scrypt_def(STR_49, "\x9e\x8c\x9b\x8d\x8c");
        if (builtin.method == STR_49->c_str())
            return LBF_BIT32_BTEST;
        /*extract*/ scrypt_def(STR_50, "\x9b\x88\x8c\x8e\x9f\x9d\x8c");
        if (builtin.method == STR_50->c_str())
            return LBF_BIT32_EXTRACT;
        /*lrotate*/ scrypt_def(STR_51, "\x94\x8e\x91\x8c\x9f\x8c\x9b");
        if (builtin.method == STR_51->c_str())
            return LBF_BIT32_LROTATE;
        /*lshift*/ scrypt_def(STR_52, "\x94\x8d\x98\x97\x9a\x8c");
        if (builtin.method == STR_52->c_str())
            return LBF_BIT32_LSHIFT;
        /*replace*/ scrypt_def(STR_53, "\x8e\x9b\x90\x94\x9f\x9d\x9b");
        if (builtin.method == STR_53->c_str())
            return LBF_BIT32_REPLACE;
        /*rrotate*/ scrypt_def(STR_54, "\x8e\x8e\x91\x8c\x9f\x8c\x9b");
        if (builtin.method == STR_54->c_str())
            return LBF_BIT32_RROTATE;
        /*rshift*/ scrypt_def(STR_55, "\x8e\x8d\x98\x97\x9a\x8c");
        if (builtin.method == STR_55->c_str())
            return LBF_BIT32_RSHIFT;
        /*countlz*/ scrypt_def(STR_56, "\x9d\x91\x8b\x92\x8c\x94\x86");
        if (builtin.method == STR_56->c_str())
            return LBF_BIT32_COUNTLZ;
        /*countrz*/ scrypt_def(STR_57, "\x9d\x91\x8b\x92\x8c\x8e\x86");
        if (builtin.method == STR_57->c_str())
            return LBF_BIT32_COUNTRZ;
        /*byteswap*/ scrypt_def(STR_58, "\x9e\x87\x8c\x9b\x8d\x89\x9f\x90");
        if (builtin.method == STR_58->c_str())
            return LBF_BIT32_BYTESWAP;
    }

    /*string*/ scrypt_def(STR_59, "\x8d\x8c\x8e\x97\x92\x99");
    if (builtin.object == STR_59->c_str())
    {
        /*byte*/ scrypt_def(STR_60, "\x9e\x87\x8c\x9b");
        if (builtin.method == STR_60->c_str())
            return LBF_STRING_BYTE;
        /*char*/ scrypt_def(STR_61, "\x9d\x98\x9f\x8e");
        if (builtin.method == STR_61->c_str())
            return LBF_STRING_CHAR;
        /*len*/ scrypt_def(STR_62, "\x94\x9b\x92");
        if (builtin.method == STR_62->c_str())
            return LBF_STRING_LEN;
        /*sub*/ scrypt_def(STR_63, "\x8d\x8b\x9e");
        if (builtin.method == STR_63->c_str())
            return LBF_STRING_SUB;
    }

    /*table*/ scrypt_def(STR_64, "\x8c\x9f\x9e\x94\x9b");
    if (builtin.object == STR_64->c_str())
    {
        /*insert*/ scrypt_def(STR_65, "\x97\x92\x8d\x9b\x8e\x8c");
        if (builtin.method == STR_65->c_str())
            return LBF_TABLE_INSERT;
        /*unpack*/ scrypt_def(STR_66, "\x8b\x92\x90\x9f\x9d\x95");
        if (builtin.method == STR_66->c_str())
            return LBF_TABLE_UNPACK;
    }

    /*buffer*/ scrypt_def(STR_67, "\x9e\x8b\x9a\x9a\x9b\x8e");
    if (builtin.object == STR_67->c_str())
    {
        /*readi8*/ scrypt_def(STR_68, "\x8e\x9b\x9f\x9c\x97\xc8");
        if (builtin.method == STR_68->c_str())
            return LBF_BUFFER_READI8;
        /*readu8*/ scrypt_def(STR_69, "\x8e\x9b\x9f\x9c\x8b\xc8");
        if (builtin.method == STR_69->c_str())
            return LBF_BUFFER_READU8;
        /*writei8*/ scrypt_def(STR_70, "\x89\x8e\x97\x8c\x9b\x97\xc8");
        /*writeu8*/ scrypt_def(STR_71, "\x89\x8e\x97\x8c\x9b\x8b\xc8");
        if (builtin.method == STR_70->c_str() || builtin.method == STR_71->c_str())
            return LBF_BUFFER_WRITEU8;
        /*readi16*/ scrypt_def(STR_72, "\x8e\x9b\x9f\x9c\x97\xcf\xca");
        if (builtin.method == STR_72->c_str())
            return LBF_BUFFER_READI16;
        /*readu16*/ scrypt_def(STR_73, "\x8e\x9b\x9f\x9c\x8b\xcf\xca");
        if (builtin.method == STR_73->c_str())
            return LBF_BUFFER_READU16;
        /*writei16*/ scrypt_def(STR_74, "\x89\x8e\x97\x8c\x9b\x97\xcf\xca");
        /*writeu16*/ scrypt_def(STR_75, "\x89\x8e\x97\x8c\x9b\x8b\xcf\xca");
        if (builtin.method == STR_74->c_str() || builtin.method == STR_75->c_str())
            return LBF_BUFFER_WRITEU16;
        /*readi32*/ scrypt_def(STR_76, "\x8e\x9b\x9f\x9c\x97\xcd\xce");
        if (builtin.method == STR_76->c_str())
            return LBF_BUFFER_READI32;
        /*readu32*/ scrypt_def(STR_77, "\x8e\x9b\x9f\x9c\x8b\xcd\xce");
        if (builtin.method == STR_77->c_str())
            return LBF_BUFFER_READU32;
        /*writei32*/ scrypt_def(STR_78, "\x89\x8e\x97\x8c\x9b\x97\xcd\xce");
        /*writeu32*/ scrypt_def(STR_79, "\x89\x8e\x97\x8c\x9b\x8b\xcd\xce");
        if (builtin.method == STR_78->c_str() || builtin.method == STR_79->c_str())
            return LBF_BUFFER_WRITEU32;
        /*readf32*/ scrypt_def(STR_80, "\x8e\x9b\x9f\x9c\x9a\xcd\xce");
        if (builtin.method == STR_80->c_str())
            return LBF_BUFFER_READF32;
        /*writef32*/ scrypt_def(STR_81, "\x89\x8e\x97\x8c\x9b\x9a\xcd\xce");
        if (builtin.method == STR_81->c_str())
            return LBF_BUFFER_WRITEF32;
        /*readf64*/ scrypt_def(STR_82, "\x8e\x9b\x9f\x9c\x9a\xca\xcc");
        if (builtin.method == STR_82->c_str())
            return LBF_BUFFER_READF64;
        /*writef64*/ scrypt_def(STR_83, "\x89\x8e\x97\x8c\x9b\x9a\xca\xcc");
        if (builtin.method == STR_83->c_str())
            return LBF_BUFFER_WRITEF64;
    }

    if (FFlag::LuauVectorBuiltins && builtin.object == "vector")
    {
        if (builtin.method == "create")
            return LBF_VECTOR;
        if (builtin.method == "magnitude")
            return LBF_VECTOR_MAGNITUDE;
        if (builtin.method == "normalize")
            return LBF_VECTOR_NORMALIZE;
        if (builtin.method == "cross")
            return LBF_VECTOR_CROSS;
        if (builtin.method == "dot")
            return LBF_VECTOR_DOT;
        if (builtin.method == "floor")
            return LBF_VECTOR_FLOOR;
        if (builtin.method == "ceil")
            return LBF_VECTOR_CEIL;
        if (builtin.method == "abs")
            return LBF_VECTOR_ABS;
        if (builtin.method == "sign")
            return LBF_VECTOR_SIGN;
        if (builtin.method == "clamp")
            return LBF_VECTOR_CLAMP;
        if (builtin.method == "min")
            return LBF_VECTOR_MIN;
        if (builtin.method == "max")
            return LBF_VECTOR_MAX;
    }

    if (options.vectorCtor)
    {
        if (options.vectorLib)
        {
            if (builtin.isMethod(options.vectorLib, options.vectorCtor))
                return LBF_VECTOR;
        }
        else
        {
            if (builtin.isGlobal(options.vectorCtor))
                return LBF_VECTOR;
        }
    }

    return -1;
}

struct BuiltinVisitor : AstVisitor
{
    DenseHashMap<AstExprCall*, int>& result;

    const DenseHashMap<AstName, Global>& globals;
    const DenseHashMap<AstLocal*, Variable>& variables;

    const CompileOptions& options;

    BuiltinVisitor(
        DenseHashMap<AstExprCall*, int>& result,
        const DenseHashMap<AstName, Global>& globals,
        const DenseHashMap<AstLocal*, Variable>& variables,
        const CompileOptions& options
    )
        : result(result)
        , globals(globals)
        , variables(variables)
        , options(options)
    {
    }

    bool visit(AstExprCall* node) override
    {
        Builtin builtin = node->self ? Builtin() : getBuiltin(node->func, globals, variables);
        if (builtin.empty())
            return true;

        int bfid = getBuiltinFunctionId(builtin, options);

        // getBuiltinFunctionId optimistically assumes all select() calls are builtin but actually the second argument must be a vararg
        if (bfid == LBF_SELECT_VARARG && !(node->args.size == 2 && node->args.data[1]->is<AstExprVarargs>()))
            bfid = -1;

        if (bfid >= 0)
            result[node] = bfid;

        return true; // propagate to nested calls
    }
};

void analyzeBuiltins(
    DenseHashMap<AstExprCall*, int>& result,
    const DenseHashMap<AstName, Global>& globals,
    const DenseHashMap<AstLocal*, Variable>& variables,
    const CompileOptions& options,
    AstNode* root
)
{
    BuiltinVisitor visitor{result, globals, variables, options};
    root->visit(&visitor);
}

BuiltinInfo getBuiltinInfo(int bfid)
{
    switch (LuauBuiltinFunction(bfid))
    {
    case LBF_NONE:
        return {-1, -1};

    case LBF_ASSERT:
        return {-1, -1}; // assert() returns all values when first value is truthy

    case LBF_MATH_ABS:
    case LBF_MATH_ACOS:
    case LBF_MATH_ASIN:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_ATAN2:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_ATAN:
    case LBF_MATH_CEIL:
    case LBF_MATH_COSH:
    case LBF_MATH_COS:
    case LBF_MATH_DEG:
    case LBF_MATH_EXP:
    case LBF_MATH_FLOOR:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_FMOD:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_FREXP:
        return {1, 2, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_LDEXP:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_LOG10:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_LOG:
        return {-1, 1}; // 1 or 2 parameters

    case LBF_MATH_MAX:
    case LBF_MATH_MIN:
        return {-1, 1}; // variadic

    case LBF_MATH_MODF:
        return {1, 2, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_POW:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_RAD:
    case LBF_MATH_SINH:
    case LBF_MATH_SIN:
    case LBF_MATH_SQRT:
    case LBF_MATH_TANH:
    case LBF_MATH_TAN:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BIT32_ARSHIFT:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BIT32_BAND:
        return {-1, 1}; // variadic

    case LBF_BIT32_BNOT:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BIT32_BOR:
    case LBF_BIT32_BXOR:
    case LBF_BIT32_BTEST:
        return {-1, 1}; // variadic

    case LBF_BIT32_EXTRACT:
        return {-1, 1}; // 2 or 3 parameters

    case LBF_BIT32_LROTATE:
    case LBF_BIT32_LSHIFT:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BIT32_REPLACE:
        return {-1, 1}; // 3 or 4 parameters

    case LBF_BIT32_RROTATE:
    case LBF_BIT32_RSHIFT:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_TYPE:
        return {1, 1};

    case LBF_STRING_BYTE:
        return {-1, -1}; // 1, 2 or 3 parameters

    case LBF_STRING_CHAR:
        return {-1, 1}; // variadic

    case LBF_STRING_LEN:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_TYPEOF:
        return {1, 1};

    case LBF_STRING_SUB:
        return {-1, 1}; // 2 or 3 parameters

    case LBF_MATH_CLAMP:
        return {3, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_MATH_SIGN:
    case LBF_MATH_ROUND:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_RAWSET:
        return {3, 1};

    case LBF_RAWGET:
    case LBF_RAWEQUAL:
        return {2, 1};

    case LBF_TABLE_INSERT:
        return {-1, 0}; // 2 or 3 parameters

    case LBF_TABLE_UNPACK:
        return {-1, -1}; // 1, 2 or 3 parameters

    case LBF_VECTOR:
        return {-1, 1}; // 3 or 4 parameters in some configurations

    case LBF_BIT32_COUNTLZ:
    case LBF_BIT32_COUNTRZ:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_SELECT_VARARG:
        return {-1, -1}; // variadic

    case LBF_RAWLEN:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BIT32_EXTRACTK:
        return {3, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_GETMETATABLE:
        return {1, 1};

    case LBF_SETMETATABLE:
        return {2, 1};

    case LBF_TONUMBER:
        return {-1, 1}; // 1 or 2 parameters

    case LBF_TOSTRING:
        return {1, 1};

    case LBF_BIT32_BYTESWAP:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BUFFER_READI8:
    case LBF_BUFFER_READU8:
    case LBF_BUFFER_READI16:
    case LBF_BUFFER_READU16:
    case LBF_BUFFER_READI32:
    case LBF_BUFFER_READU32:
    case LBF_BUFFER_READF32:
    case LBF_BUFFER_READF64:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};

    case LBF_BUFFER_WRITEU8:
    case LBF_BUFFER_WRITEU16:
    case LBF_BUFFER_WRITEU32:
    case LBF_BUFFER_WRITEF32:
    case LBF_BUFFER_WRITEF64:
        return {3, 0, BuiltinInfo::Flag_NoneSafe};

    case LBF_VECTOR_MAGNITUDE:
    case LBF_VECTOR_NORMALIZE:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};
    case LBF_VECTOR_CROSS:
    case LBF_VECTOR_DOT:
        return {2, 1, BuiltinInfo::Flag_NoneSafe};
    case LBF_VECTOR_FLOOR:
    case LBF_VECTOR_CEIL:
    case LBF_VECTOR_ABS:
    case LBF_VECTOR_SIGN:
        return {1, 1, BuiltinInfo::Flag_NoneSafe};
    case LBF_VECTOR_CLAMP:
        return {3, 1, BuiltinInfo::Flag_NoneSafe};
    case LBF_VECTOR_MIN:
    case LBF_VECTOR_MAX:
        return {-1, 1}; // variadic
    }

    LUAU_UNREACHABLE();
}

} // namespace Compile
} // namespace Luau
