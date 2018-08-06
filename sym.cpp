
#include <windows.h>
#include "dbghelp.h"
#include "sym.h"
#include <malloc.h>
#include <stdio.h>

const char* Sym::toTypeName(VarTypeEnum vt){

	switch(vt){

	case vtVoid: return "void";
	case vtBool: return "bool";
	case vtChar: return "char";
	case vtByte: return "byte";
	case vtWChar: return "wchar_t";
	case vtShort: return "short";
	case vtUShort: return "ushort";
	case vtInt: return "int";
	case vtUInt: return "uint";
	case vtLong: return "long";
	case vtULong: return "ulong";
	case vtLongLong: return "longlong";
	case vtULongLong: return "ulonglong";
	case vtFloat: return "float";
	case vtDouble: return "double";
	default :
	  return "none";
    }
	return 0;
}

BOOL Sym::getSymInfo(ULONG type_id,SymTypeEnum get_type,PVOID pInfo){

   return ::SymGetTypeInfo(process,mod,type_id,
          (IMAGEHLP_SYMBOL_TYPE_INFO)get_type,pInfo);
}


VarTypeEnum Sym::getBaseType(DWORD type_id){


    DWORD bt;
    getSymInfo(type_id,ST_BASETYPE,&bt);

    //获取基本类型的长度
	ULONG64 len;
	getSymInfo(type_id,ST_LENGTH,&len);

	switch (bt) {
		case btVoid: return vtVoid;
		case btChar: return vtChar;
		case btWChar:return vtWChar;
		case btBool: return vtBool;
		case btLong: return vtLong;

		case btULong:
			return vtULong;
		case btInt:
			switch (len) {
				case 2:  return vtShort;
				case 4:  return vtInt;
				default: return vtLongLong;
			}

		case btUInt:
			switch (len) {
				case 1:  return vtByte;
				case 2:  return vtUShort;
				case 4:  return vtUInt;
				default: return vtULongLong;
			}

		case btFloat:
			switch (len) {
				case 4:  return vtFloat;
				default: return vtDouble;
			}

		default:
			return vtNone;
	}

}

SymTagEnum Sym::getSymTag(DWORD type_id){

    SymTagEnum tag;
	getSymInfo(type_id,ST_SYMTAG,&tag);
    return tag;
}
const char * w2char(const wchar_t* wp,char* buff)
{
    size_t m_encode = CP_ACP;

	int len = WideCharToMultiByte(m_encode, 0, wp, wcslen(wp), NULL, 0, NULL, NULL);
	char*m_char = (char*)alloca(len + 1);
	WideCharToMultiByte(m_encode, 0, wp, wcslen(wp), m_char, len, NULL, NULL);
	m_char[len] = '\0';
	strcpy(buff,m_char);
	return buff;
}
 // The name of the UDT. return is BSTR so
 //  first convert it to const char*
 const char* Sym::getSymName(DWORD type_id,char* buff){

    wchar_t * name=0;
    getSymInfo(type_id,ST_SYMNAME,&name);
    const char* ret = w2char(name,buff);
    ::LocalFree(name);

    return ret;
 }


 #define FC_PARAMS TI_FINDCHILDREN_PARAMS
 #define DEF_CHILDREN_PARAMS(n,p) FC_PARAMS* p = (FC_PARAMS*)\
        alloca(sizeof(FC_PARAMS)+n*sizeof(DWORD)); \
        p->Start=0;p->Count=n;

 BOOL Sym::getTypeName(DWORD type_id,char* out){

   SymTagEnum tag = getSymTag(type_id);

   VarTypeEnum vt;
   BOOL ret = FALSE;
   BOOL is_ref;
   DWORD id,chid,num;
   char tmp[128]={0};
   switch(tag){

		case SymTagBaseType:
		    vt = getBaseType(type_id);
		    strcpy(out,toTypeName(vt));
			return TRUE;

		case SymTagPointerType:
            //获取指针所指对象的类型的typeID
            getSymInfo(type_id,ST_TYPEID,&id);
            //获取是指针类型还是引用类型
		    getSymInfo(type_id,ST_IS_REFERENCE,&is_ref);

            if(ret=getTypeName(id,out))
               strcat(out,is_ref==TRUE ? "&":"*");

			return ret;

		case SymTagArrayType:
		    //对象的类型的typeID
            getSymInfo(type_id,ST_TYPEID,&id);
            //获取数组元素的COUNT
            getSymInfo(type_id,ST_COUNT,&num);

            if(ret=getTypeName(id,out)){
               strcat(out,"[");
               strcat(out,itoa(num,tmp,10));
               strcat(out,"]");
            }
			return ret;

		case SymTagUDT:
		    {
		        DWORD uk=1;
                getSymInfo(type_id,ST_UDTKIND,&uk);
                if(uk == 0)
                    strcpy(out,"struct ");
                if(uk == 1)
                    strcpy(out,"class ");
                if(uk == 2)
                    strcpy(out,"union ");

                const char * name;
                name = getSymName(type_id,tmp);
                strcat(out,name);
		    }
		    return TRUE;
		case SymTagEnumType:
            {
                const char * name;
                name = getSymName(type_id,tmp);
                strcpy(out,name);
            }
			return TRUE;

		case SymTagFunctionType:
            {
                //获取返回值的名称
                getSymInfo(type_id,ST_TYPEID,&id);
                getTypeName(id,tmp);
                strcpy(out,tmp);

                //获取参数数量
                getSymInfo(type_id,ST_CHILDREN_COUNT,&num);
                TI_FINDCHILDREN_PARAMS*  params =
                (TI_FINDCHILDREN_PARAMS*)alloca(sizeof(*params) + sizeof(DWORD)*num);
                params->Start = 0;
                params->Count = num;
                // 获取参数typeid 集合
                getSymInfo(type_id,ST_FIND_CHILDREN,params);

                strcat(out,"(");
                for(DWORD i=0;i<num;i++){
                    chid = params->ChildId[i];
                    getSymInfo(chid,ST_TYPEID,&id);
                    getTypeName(id,tmp);
                    if(i!=0)
                        strcat(out,",");
                    strcat(out,tmp);

                }
                strcat(out,")");

            }
			return TRUE;

		default:
		    strcpy(out,"??");
			return TRUE;

   }
   return ret;
}



const char* Sym::toValue(VarTypeEnum vt,char* buff,void* data)
{
	switch (vt) {

		case vtBool:  return (*(int*)data == 0 ? "false" : "true");
		case vtByte: return itoa(*((BYTE*)data),buff,radix);
		case vtChar:
		    {
		        char val = *(char*)data;
		        if (val < 0x1E || val > 0x7F)
		          return "?";
                sprintf(buff,"%c",val);
                return buff;
		    }
		case vtWChar:
		    {
		      wchar_t wc[2];
		      wc[0] =  *((wchar_t*)data) ;
		      wc[1] = 0;
		      w2char(wc,buff);
		      return buff;
		    }
		case vtShort: return itoa( *((short*)data),buff,radix);
		case vtUShort:return itoa( *((unsigned short*)data),buff,radix);
		case vtInt:return itoa( *((int*)data),buff,radix);
		case vtUInt:return itoa( *((unsigned int*)data),buff,radix);
		case vtLong:return ltoa(*((long*)data),buff,radix);

		case vtULong:
			sprintf(buff,"%ul", *(unsigned long*)data);
			break;

		case vtLongLong:
			sprintf(buff,"%ll", *(long long*)data);
			break;

		case vtULongLong:
			sprintf(buff,"%ull",*(unsigned long long*)data);
			break;

		case vtFloat:
			sprintf(buff,"%lf",*(float*)data);
			break;

		case vtDouble:
			sprintf(buff,"%lf", *(double*)data);
			break;

        case vtNone:
		case vtVoid:
        default :
            return "??";
      }
      return buff;
}

BOOL equal_variant(VarTypeEnum vt,VARIANT var,void* data)
{

	switch (vt) {
		case vtChar:
			return var.cVal == *((char*)data);
		case vtByte:
			return var.bVal == *((unsigned char*)data);
		case vtShort:
			return var.iVal == *((short*)data);
		case vtWChar:
		case vtUShort:
			return var.uiVal == *((unsigned short*)data);
		case vtUInt:
			return var.uintVal == *((int*)data);
		case vtLong:
			return var.lVal == *((long*)data);
		case vtULong:
			return var.ulVal == *((unsigned long*)data);
		case vtLongLong:
			return var.llVal == *((long long*)data);
		case vtULongLong:
			return var.ullVal == *((unsigned long long*)data);
		case vtInt:
		default:
			return var.intVal == *((int*)data);
	}
}

BOOL Sym::getValue(DWORD type_id,char*out,DWORD addr,void*data)
{
     SymTagEnum tag = getSymTag(type_id);
     DWORD  id,num,chid;
     VarTypeEnum vt;
     char buff[128]={0};
     switch(tag){
        case SymTagBaseType :
            vt = getBaseType(type_id);
		    strcpy(out,toValue(vt,buff,data));
			return TRUE;
		case SymTagPointerType:
            sprintf(out,"%x",*(DWORD*)data);
            return TRUE;
        case SymTagEnumType:
            {

                VARIANT val;
                vt = getBaseType(type_id);

                getSymInfo(type_id,ST_CHILDREN_COUNT,&num);

                DEF_CHILDREN_PARAMS(num,params);
                getSymInfo(type_id,ST_FIND_CHILDREN,params);

                for(DWORD i=0;i<num;i++){
                    chid = params->ChildId[i];
                    getSymInfo(chid,ST_VALUE,&val);
                    if(equal_variant(vt,val,data))
                    {
                        // get enum item name;
                        getSymName(chid,buff);
                        strcpy(out,buff);
                        return TRUE;
                    }
                }

                sprintf(out,"%d",*(int*)data);

                return TRUE;
            }

		case SymTagArrayType:
		    {
                //获取元素个数
                getSymInfo(type_id,ST_COUNT,&num);
                //获取数组元素的TypeID
                getSymInfo(type_id,ST_TYPEID,&id);

                //获取数组元素的长度
                /*ULONG64*/ DWORD len;
                getSymInfo(id,ST_LENGTH,&len);

                strcpy(out,"[");
                for(DWORD i=0;i<num ;i++ ){
                    if(i!=0)
                        strcat(out,",");
                    DWORD offset_i = i*sizeof(DWORD);
                    DWORD offset_d = i*len;
                    buff[0]=0;
                    if(getValue(id,buff,addr+offset_i,data+offset_d))
                    {
                        strcat(out,buff);
                    }
                    // 最多输出32个值
                    if(i == 32 )
                        strcat(out,"...");
                }
                strcat(out,"]");
		    }
			return TRUE;
        case SymTagUDT:
            {
                getSymInfo(type_id,ST_CHILDREN_COUNT,&num);
                DEF_CHILDREN_PARAMS(num,params);
                getSymInfo(type_id,ST_FIND_CHILDREN,params);

                strcpy(out,"{\n");
                for(DWORD i=0;i<num;i++){
                    chid = params->ChildId[i];
                    getSymInfo(chid,ST_SYMTAG,&tag);

                    // //如果不是是数据成员，返回FASE
                    if (tag != SymTagData && tag != SymTagBaseClass) {
		             return FALSE;
                    }
                    //获取偏移地址
                    DWORD offset;
                    getSymInfo(chid,ST_OFFSET,&offset);

                    //获取成员类型
                    getSymInfo(chid,ST_TYPEID,&id);
                    buff[0]=0;
                    getTypeName(id,buff);

                    strcat(out,buff);
                    strcat(out,"  ");
                     //输出名称
                    if(tag == SymTagData){
                      getSymName(chid,buff);
                      strcat(out,buff);
                    }

                    /*ULONG64*/ DWORD len;
                    getSymInfo(id,ST_LENGTH,&len); // 成员长度

                    strcat(out,"=");

                    getValue(id,buff,addr+offset,data+offset);
                    strcat(out,buff);
                    strcat(out,"\n");
               }
                strcat(out,"}\n");
            }
			return TRUE;
		case SymTagTypedef:
			//获取真正类型的ID
			DWORD act_id;
			getSymInfo( type_id, ST_TYPEID,	&act_id);
			return getValue(act_id,out,addr, data);

        default:
            strcpy(out,"??");
            return TRUE;

     }
}


struct sym_tag_t { SymTagEnum tag ; const char* name;};
#define DEF_TAG(name) {SymTag##name,#name},
sym_tag_t _tags[] = {
       DEF_TAG(Null)
       DEF_TAG(Exe)
       DEF_TAG(Compiland)
       DEF_TAG(CompilandDetails)
       DEF_TAG(CompilandEnv)
       DEF_TAG(Function)
       DEF_TAG(Block)
       DEF_TAG(Data)
       DEF_TAG(Annotation)
       DEF_TAG(Label)
       DEF_TAG(PublicSymbol)
       DEF_TAG(UDT)
       DEF_TAG(EnumType)
       DEF_TAG(FunctionType)
       DEF_TAG(PointerType)
       DEF_TAG(ArrayType)
       DEF_TAG(BaseType)
       DEF_TAG(Typedef)
       DEF_TAG(BaseClass)
       DEF_TAG(Friend)
       DEF_TAG(FunctionArgType)
       DEF_TAG(FuncDebugStart)
       DEF_TAG(FuncDebugEnd)
       DEF_TAG(UsingNamespace)
       DEF_TAG(VTableShape)
       DEF_TAG(VTable)
       DEF_TAG(Custom)
       DEF_TAG(Thunk)
       DEF_TAG(CustomType)
       DEF_TAG(ManagedType)
       DEF_TAG(Dimension)
};
struct sym_type_t { SymTypeEnum st ; const char* name;};
#define DEF_TYPE(name) {ST_##name,#name},
sym_type_t _tis[] = {
    DEF_TYPE(SYMTAG)
    DEF_TYPE(SYMNAME)
    DEF_TYPE(LENGTH)
    DEF_TYPE(TYPE)
    DEF_TYPE(TYPEID)
    DEF_TYPE(BASETYPE)
    DEF_TYPE(ARRAY)
    DEF_TYPE(FIND_CHILDREN)
    DEF_TYPE(DATAKIND)
    DEF_TYPE(ADDRESS_OFFSET)
    DEF_TYPE(OFFSET)
    DEF_TYPE(VALUE)
    DEF_TYPE(COUNT)
    DEF_TYPE(CHILDREN_COUNT)
    DEF_TYPE(BIT_POS)
    DEF_TYPE(VT_BASECLASS)
    DEF_TYPE(VT_TABLE_SHAPEID)
    DEF_TYPE(VT_BASE_POINTER_OFFSET)
    DEF_TYPE(VT_CLASS_PARENTID)
    DEF_TYPE(NESTED)
    DEF_TYPE(SYMINDEX)
    DEF_TYPE(LEXICAL_PARENT)
    DEF_TYPE(ADDRESS)
    DEF_TYPE(THIS_ADJUST)
    DEF_TYPE(UDTKIND)
    DEF_TYPE(IS_EQUIV_TO)
    DEF_TYPE(CALLING_CONVENTION)
    DEF_TYPE(IS_CLOSE_EQUIV_TO)
    DEF_TYPE(GTIEX_REQS_VALID)
    DEF_TYPE(VT_BASE_OFFSET)
    DEF_TYPE(VT_BASE_DISPINDEX)
    DEF_TYPE(IS_REFERENCE)
};


void Sym::dump(DWORD type_id)
{
   int i,len = sizeof(_tags) / sizeof(sym_tag_t);
   SymTagEnum tag = getSymTag(type_id);
   for(i=0;i<len;i++){
     if(_tags[i].tag == tag)
        printf( "tag:%s\n" , _tags[i].name );
   }

   char data[128];
   len = sizeof(_tis) / sizeof(sym_type_t);
   for(i=0;i<len;i++){
    if(getSymInfo(type_id,_tis[i].st,(void*)data)){
        printf("   %s", _tis[i].name);
        if(_tis[i].st == ST_TYPEID)
            printf( "->%d:%d" , type_id , *(DWORD*)data);
        printf("\n");
    }
   }
}

