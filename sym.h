

#pragma once


enum SymTagEnum {
   SymTagNull,
   SymTagExe,
   SymTagCompiland,
   SymTagCompilandDetails,
   SymTagCompilandEnv,
   SymTagFunction,				//函数
   SymTagBlock,
   SymTagData,					//变量
   SymTagAnnotation,            //注释
   SymTagLabel,
   SymTagPublicSymbol,
   SymTagUDT,					//用户定义类型，例如struct，class和union
   SymTagEnumType,				//枚举类型
   SymTagFunctionType,			//函数类型
   SymTagPointerType,			//指针类型
   SymTagArrayType,				//数组类型
   SymTagBaseType,				//基本类型
   SymTagTypedef,				//typedef类型
   SymTagBaseClass,				//基类
   SymTagFriend,				//友元类型
   SymTagFunctionArgType,		//函数参数类型
   SymTagFuncDebugStart,
   SymTagFuncDebugEnd,
   SymTagUsingNamespace,
   SymTagVTableShape,
   SymTagVTable,
   SymTagCustom,
   SymTagThunk,
   SymTagCustomType,
   SymTagManagedType,
   SymTagDimension
};

enum BaseTypeEnum {
   btNoType = 0,
   btVoid = 1,
   btChar = 2,
   btWChar = 3,
   btInt = 6,
   btUInt = 7,
   btFloat = 8,
   btBCD = 9,
   btBool = 10,
   btLong = 13,
   btULong = 14,
   btCurrency = 25,
   btDate = 26,
   btVariant = 27,
   btComplex = 28,
   btBit = 29,
   btBSTR = 30,
   btHresult = 31
};
enum VarTypeEnum {
	vtNone,
	vtVoid,
	vtBool,
	vtChar,
	vtByte,
	vtWChar,
	vtShort,
	vtUShort,
	vtInt,
	vtUInt,
	vtLong,
	vtULong,
	vtLongLong,
	vtULongLong,
	vtFloat,
	vtDouble,
	vtEnd,
};

enum SymTypeEnum {
    ST_SYMTAG,
    ST_SYMNAME,
    ST_LENGTH,
    ST_TYPE,
    ST_TYPEID,
    ST_BASETYPE,
    ST_ARRAY,
    ST_FIND_CHILDREN,
    ST_DATAKIND,
    ST_ADDRESS_OFFSET,
    ST_OFFSET,
    ST_VALUE,
    ST_COUNT,
    ST_CHILDREN_COUNT,
    ST_BIT_POS,
    ST_VT_BASECLASS,
    ST_VT_TABLE_SHAPEID,
    ST_VT_BASE_POINTER_OFFSET,
    ST_VT_CLASS_PARENTID,
    ST_NESTED,
    ST_SYMINDEX,
    ST_LEXICAL_PARENT,
    ST_ADDRESS,
    ST_THIS_ADJUST,
    ST_UDTKIND,
    ST_IS_EQUIV_TO,
    ST_CALLING_CONVENTION,
    ST_IS_CLOSE_EQUIV_TO,
    ST_GTIEX_REQS_VALID,
    ST_VT_BASE_OFFSET,
    ST_VT_BASE_DISPINDEX,
    ST_IS_REFERENCE,
    SYM_TYPE_MAX,
} ;

extern const char * w2char(const wchar_t* wp,char* buff);

class Sym {
	public:

	HANDLE process;
	DWORD  mod;
	DWORD  radix;
    Sym(HANDLE hp,DWORD _mod):process(hp),mod(_mod),radix(16){}
	const char* toTypeName(VarTypeEnum vt);

	const char* toValue(VarTypeEnum vt,char* buff,void* data);

	VarTypeEnum getBaseType(DWORD type_id);
	SymTagEnum  getSymTag(DWORD type_id);
	const char* getSymName(DWORD type_id,char* buff);

	BOOL        getTypeName(DWORD type_id,char* out);

	BOOL        getValue(DWORD type_id,char*out,DWORD addr,void*data);
    void        dump(DWORD type_id);
	private:
    BOOL getSymInfo(ULONG type_id,SymTypeEnum get_type,PVOID pInfo);

};



