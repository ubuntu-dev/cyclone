// This is a C header file to be used by the output of the Cyclone
// to C translator.  The corresponding definitions are in
// the file lib/runtime_cyc.c

#ifndef _CYC_INCLUDE_H_
#define _CYC_INCLUDE_H_

#include <setjmp.h>

#ifdef NO_CYC_PREFIX
#define ADD_PREFIX(x) x
#else
#define ADD_PREFIX(x) Cyc_##x
#endif

#ifndef offsetof
// should be size_t, but int is fine.
#define offsetof(t,n) ((int)(&(((t *)0)->n)))
#endif

//// Tagged arrays
struct _tagged_arr { 
  unsigned char *curr; 
  unsigned char *base; 
  unsigned char *last_plus_one; 
};
struct _tagged_string {  // delete after bootstrapping
  unsigned char *curr; 
  unsigned char *base; 
  unsigned char *last_plus_one; 
};

//// Discriminated Unions
struct _xtunion_struct { char *tag; };

// The runtime maintains a stack that contains either _handler_cons
// structs or _RegionHandle structs.  The tag is 0 for a handler_cons
// and 1 for a region handle.  
struct _RuntimeStack {
  int tag; // 0 for an exception handler, 1 for a region handle
  struct _RuntimeStack *next;
};

//// Regions
struct _RegionPage {
  struct _RegionPage *next;
#ifdef CYC_REGION_PROFILE
  unsigned int total_bytes;
  unsigned int free_bytes;
#endif
  char data[0];
};

struct _RegionHandle {
  struct _RuntimeStack s;
  struct _RegionPage *curr;
  char               *offset;
  char               *last_plus_one;
};

extern struct _RegionHandle _new_region();
extern void * _region_malloc(struct _RegionHandle *, unsigned int);
extern void   _free_region(struct _RegionHandle *);

//// Exceptions 
struct _handler_cons {
  struct _RuntimeStack s;
  jmp_buf handler;
};
extern void _push_handler(struct _handler_cons *);
extern void _push_region(struct _RegionHandle *);
extern void _npop_handler(int);
extern void _pop_handler();
extern void _pop_region();

#ifndef _throw
extern int _throw_null();
extern int _throw_arraybounds();
extern int _throw_badalloc();
extern int _throw(void * e);
#endif

extern struct _xtunion_struct *_exn_thrown;

//// Built-in Exceptions
extern struct _xtunion_struct ADD_PREFIX(Null_Exception_struct);
extern struct _xtunion_struct * ADD_PREFIX(Null_Exception);
extern struct _xtunion_struct ADD_PREFIX(Array_bounds_struct);
extern struct _xtunion_struct * ADD_PREFIX(Array_bounds);
extern struct _xtunion_struct ADD_PREFIX(Match_Exception_struct);
extern struct _xtunion_struct * ADD_PREFIX(Match_Exception);
extern struct _xtunion_struct ADD_PREFIX(Bad_alloc_struct);
extern struct _xtunion_struct * ADD_PREFIX(Bad_alloc);

//// Built-in Run-time Checks and company
static inline 
void * _check_null(void * ptr) {
  if(!ptr)
    _throw_null();
  return ptr;
}
static inline 
char * _check_known_subscript_null(void * ptr, unsigned bound, 
				   unsigned elt_sz, unsigned index) {
  if(!ptr || index >= bound)
    _throw_null();
  return ((char *)ptr) + elt_sz*index;
}
static inline 
unsigned _check_known_subscript_notnull(unsigned bound, unsigned index) {
  if(index >= bound)
    _throw_arraybounds();
  return index;
}
static inline 
char * _check_unknown_subscript(struct _tagged_arr arr,
				unsigned elt_sz, unsigned index) {
  // caller casts first argument and result
  // multiplication looks inefficient, but C compiler has to insert it otherwise
  // by inlining, it should be able to avoid actual multiplication
  unsigned char * ans = arr.curr + elt_sz * index;
  // might be faster not to distinguish these two cases. definitely would be
  // smaller.
  if(!arr.base) 
    _throw_null();
  if(ans < arr.base || ans >= arr.last_plus_one)
    _throw_arraybounds();
  return ans;
}
static inline 
struct _tagged_arr _tag_arr(const void * curr, 
			    unsigned elt_sz, unsigned num_elts) {
  // beware the gcc bug, can happen with *temp = _tag_arr(...) in weird places!
  struct _tagged_arr ans;
  ans.base = (void *)curr;
  ans.curr = (void *)curr;
  ans.last_plus_one = ((char *)curr) + elt_sz * num_elts;
  return ans;
}
static inline
struct _tagged_arr * _init_tag_arr(struct _tagged_arr * arr_ptr, void * arr, 
				   unsigned elt_sz, unsigned num_elts) {
  // we use this to (hopefully) avoid the gcc bug
  arr_ptr->base = arr_ptr->curr = arr;
  arr_ptr->last_plus_one = ((char *)arr) + elt_sz * num_elts;
  return arr_ptr;
}

static inline
char * _untag_arr(struct _tagged_arr arr, unsigned elt_sz, unsigned num_elts) {
  // Note: if arr is "null" base and curr should both be null, so this
  //       is correct (caller checks for null if untagging to @ type)
  // base may not be null if you use t ? pointer subtraction to get 0 -- oh well
  if(arr.curr < arr.base || arr.curr + elt_sz * num_elts > arr.last_plus_one)
    _throw_arraybounds();
  return arr.curr;
}
static inline 
unsigned _get_arr_size(struct _tagged_arr arr, unsigned elt_sz) {
  return (arr.last_plus_one - arr.curr) / elt_sz;
}
static inline
struct _tagged_arr _tagged_arr_plus(struct _tagged_arr arr, unsigned elt_sz,
				    int change) {
  struct _tagged_arr ans = arr;
  ans.curr += ((int)elt_sz)*change;
  return ans;
}
static inline
struct _tagged_arr _tagged_arr_inplace_plus(struct _tagged_arr * arr_ptr,
					    unsigned elt_sz, int change) {
  arr_ptr->curr += ((int)elt_sz)*change;
  return *arr_ptr;
}
static inline
struct _tagged_arr _tagged_arr_inplace_plus_post(struct _tagged_arr * arr_ptr,
						 unsigned elt_sz, int change) {
  struct _tagged_arr ans = *arr_ptr;
  arr_ptr->curr += ((int)elt_sz)*change;
  return ans;
}
				  
//// Allocation
extern void * GC_malloc(int);
extern void * GC_malloc_atomic(int);

static inline void * _cycalloc(int n) {
  void * ans = GC_malloc(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static inline void * _cycalloc_atomic(int n) {
  void * ans = GC_malloc(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
#define MAX_MALLOC_SIZE (1 << 28)
static inline unsigned int _check_times(unsigned int x, unsigned int y) {
  unsigned long long whole_ans = 
    ((unsigned long long)x)*((unsigned long long)y);
  unsigned int word_ans = (unsigned int)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#ifdef CYC_REGION_PROFILE
extern void * _profile_GC_malloc(int,char *file,int lineno);
extern void * _profile_GC_malloc_atomic(int,char *file,int lineno);
extern void * _profile_region_malloc(struct _RegionHandle *, unsigned int,
                                     char *file,int lineno);
#define _cycalloc(n) _profile_cycalloc(n,__FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_cycalloc_atomic(n,__FUNCTION__,__LINE__)
#define _region_malloc(rh,n) _profile_region_malloc(rh,n,__FUNCTION__,__LINE__)
#endif

#endif
 extern void exit( int); extern void* abort(); struct Cyc_Std__types_fd_set{ int
fds_bits[ 2u]; } ; struct Cyc_Core_Opt{ void* v; } ; extern unsigned char Cyc_Core_Invalid_argument[
21u]; struct Cyc_Core_Invalid_argument_struct{ unsigned char* tag; struct
_tagged_arr f1; } ; extern unsigned char Cyc_Core_Failure[ 12u]; struct Cyc_Core_Failure_struct{
unsigned char* tag; struct _tagged_arr f1; } ; extern unsigned char Cyc_Core_Impossible[
15u]; struct Cyc_Core_Impossible_struct{ unsigned char* tag; struct _tagged_arr
f1; } ; extern unsigned char Cyc_Core_Not_found[ 14u]; extern unsigned char Cyc_Core_Unreachable[
16u]; struct Cyc_Core_Unreachable_struct{ unsigned char* tag; struct _tagged_arr
f1; } ; extern unsigned char* string_to_Cstring( struct _tagged_arr); extern
unsigned char* underlying_Cstring( struct _tagged_arr); extern struct
_tagged_arr Cstring_to_string( unsigned char*); extern struct _tagged_arr
wrap_Cstring_as_string( unsigned char*, unsigned int); extern struct _tagged_arr
ntCsl_to_ntsl( unsigned char**); struct Cyc_List_List{ void* hd; struct Cyc_List_List*
tl; } ; extern int Cyc_List_length( struct Cyc_List_List* x); extern struct Cyc_List_List*
Cyc_List_map_c( void*(* f)( void*, void*), void* env, struct Cyc_List_List* x);
extern unsigned char Cyc_List_List_mismatch[ 18u]; extern struct Cyc_List_List*
Cyc_List_rev( struct Cyc_List_List* x); extern struct Cyc_List_List* Cyc_List_imp_rev(
struct Cyc_List_List* x); extern unsigned char Cyc_List_Nth[ 8u]; struct Cyc_Lineno_Pos{
struct _tagged_arr logical_file; struct _tagged_arr line; int line_no; int col;
} ; extern unsigned char Cyc_Position_Exit[ 9u]; struct Cyc_Position_Segment;
static const int Cyc_Position_Lex= 0; static const int Cyc_Position_Parse= 1;
static const int Cyc_Position_Elab= 2; struct Cyc_Position_Error{ struct
_tagged_arr source; struct Cyc_Position_Segment* seg; void* kind; struct
_tagged_arr desc; } ; extern unsigned char Cyc_Position_Nocontext[ 14u]; static
const int Cyc_Absyn_Loc_n= 0; static const int Cyc_Absyn_Rel_n= 0; struct Cyc_Absyn_Rel_n_struct{
int tag; struct Cyc_List_List* f1; } ; static const int Cyc_Absyn_Abs_n= 1;
struct Cyc_Absyn_Abs_n_struct{ int tag; struct Cyc_List_List* f1; } ; struct
_tuple0{ void* f1; struct _tagged_arr* f2; } ; struct Cyc_Absyn_Conref; static
const int Cyc_Absyn_Static= 0; static const int Cyc_Absyn_Abstract= 1; static
const int Cyc_Absyn_Public= 2; static const int Cyc_Absyn_Extern= 3; static
const int Cyc_Absyn_ExternC= 4; struct Cyc_Absyn_Tqual{ int q_const: 1; int
q_volatile: 1; int q_restrict: 1; } ; static const int Cyc_Absyn_B1= 0; static
const int Cyc_Absyn_B2= 1; static const int Cyc_Absyn_B4= 2; static const int
Cyc_Absyn_B8= 3; static const int Cyc_Absyn_AnyKind= 0; static const int Cyc_Absyn_MemKind=
1; static const int Cyc_Absyn_BoxKind= 2; static const int Cyc_Absyn_RgnKind= 3;
static const int Cyc_Absyn_EffKind= 4; static const int Cyc_Absyn_Signed= 0;
static const int Cyc_Absyn_Unsigned= 1; struct Cyc_Absyn_Conref{ void* v; } ;
static const int Cyc_Absyn_Eq_constr= 0; struct Cyc_Absyn_Eq_constr_struct{ int
tag; void* f1; } ; static const int Cyc_Absyn_Forward_constr= 1; struct Cyc_Absyn_Forward_constr_struct{
int tag; struct Cyc_Absyn_Conref* f1; } ; static const int Cyc_Absyn_No_constr=
0; struct Cyc_Absyn_Tvar{ struct _tagged_arr* name; int* identity; struct Cyc_Absyn_Conref*
kind; } ; static const int Cyc_Absyn_Unknown_b= 0; static const int Cyc_Absyn_Upper_b=
0; struct Cyc_Absyn_Upper_b_struct{ int tag; struct Cyc_Absyn_Exp* f1; } ;
struct Cyc_Absyn_PtrInfo{ void* elt_typ; void* rgn_typ; struct Cyc_Absyn_Conref*
nullable; struct Cyc_Absyn_Tqual tq; struct Cyc_Absyn_Conref* bounds; } ; struct
Cyc_Absyn_VarargInfo{ struct Cyc_Core_Opt* name; struct Cyc_Absyn_Tqual tq; void*
type; int inject; } ; struct Cyc_Absyn_FnInfo{ struct Cyc_List_List* tvars;
struct Cyc_Core_Opt* effect; void* ret_typ; struct Cyc_List_List* args; int
c_varargs; struct Cyc_Absyn_VarargInfo* cyc_varargs; struct Cyc_List_List*
rgn_po; struct Cyc_List_List* attributes; } ; struct Cyc_Absyn_UnknownTunionInfo{
struct _tuple0* name; int is_xtunion; } ; static const int Cyc_Absyn_UnknownTunion=
0; struct Cyc_Absyn_UnknownTunion_struct{ int tag; struct Cyc_Absyn_UnknownTunionInfo
f1; } ; static const int Cyc_Absyn_KnownTunion= 1; struct Cyc_Absyn_KnownTunion_struct{
int tag; struct Cyc_Absyn_Tuniondecl** f1; } ; struct Cyc_Absyn_TunionInfo{ void*
tunion_info; struct Cyc_List_List* targs; void* rgn; } ; struct Cyc_Absyn_UnknownTunionFieldInfo{
struct _tuple0* tunion_name; struct _tuple0* field_name; int is_xtunion; } ;
static const int Cyc_Absyn_UnknownTunionfield= 0; struct Cyc_Absyn_UnknownTunionfield_struct{
int tag; struct Cyc_Absyn_UnknownTunionFieldInfo f1; } ; static const int Cyc_Absyn_KnownTunionfield=
1; struct Cyc_Absyn_KnownTunionfield_struct{ int tag; struct Cyc_Absyn_Tuniondecl*
f1; struct Cyc_Absyn_Tunionfield* f2; } ; struct Cyc_Absyn_TunionFieldInfo{ void*
field_info; struct Cyc_List_List* targs; } ; static const int Cyc_Absyn_VoidType=
0; static const int Cyc_Absyn_Evar= 0; struct Cyc_Absyn_Evar_struct{ int tag;
struct Cyc_Core_Opt* f1; struct Cyc_Core_Opt* f2; int f3; struct Cyc_Core_Opt*
f4; } ; static const int Cyc_Absyn_VarType= 1; struct Cyc_Absyn_VarType_struct{
int tag; struct Cyc_Absyn_Tvar* f1; } ; static const int Cyc_Absyn_TunionType= 2;
struct Cyc_Absyn_TunionType_struct{ int tag; struct Cyc_Absyn_TunionInfo f1; } ;
static const int Cyc_Absyn_TunionFieldType= 3; struct Cyc_Absyn_TunionFieldType_struct{
int tag; struct Cyc_Absyn_TunionFieldInfo f1; } ; static const int Cyc_Absyn_PointerType=
4; struct Cyc_Absyn_PointerType_struct{ int tag; struct Cyc_Absyn_PtrInfo f1; }
; static const int Cyc_Absyn_IntType= 5; struct Cyc_Absyn_IntType_struct{ int
tag; void* f1; void* f2; } ; static const int Cyc_Absyn_FloatType= 1; static
const int Cyc_Absyn_DoubleType= 6; struct Cyc_Absyn_DoubleType_struct{ int tag;
int f1; } ; static const int Cyc_Absyn_ArrayType= 7; struct Cyc_Absyn_ArrayType_struct{
int tag; void* f1; struct Cyc_Absyn_Tqual f2; struct Cyc_Absyn_Exp* f3; } ;
static const int Cyc_Absyn_FnType= 8; struct Cyc_Absyn_FnType_struct{ int tag;
struct Cyc_Absyn_FnInfo f1; } ; static const int Cyc_Absyn_TupleType= 9; struct
Cyc_Absyn_TupleType_struct{ int tag; struct Cyc_List_List* f1; } ; static const
int Cyc_Absyn_StructType= 10; struct Cyc_Absyn_StructType_struct{ int tag;
struct _tuple0* f1; struct Cyc_List_List* f2; struct Cyc_Absyn_Structdecl** f3;
} ; static const int Cyc_Absyn_UnionType= 11; struct Cyc_Absyn_UnionType_struct{
int tag; struct _tuple0* f1; struct Cyc_List_List* f2; struct Cyc_Absyn_Uniondecl**
f3; } ; static const int Cyc_Absyn_EnumType= 12; struct Cyc_Absyn_EnumType_struct{
int tag; struct _tuple0* f1; struct Cyc_Absyn_Enumdecl* f2; } ; static const int
Cyc_Absyn_AnonStructType= 13; struct Cyc_Absyn_AnonStructType_struct{ int tag;
struct Cyc_List_List* f1; } ; static const int Cyc_Absyn_AnonUnionType= 14;
struct Cyc_Absyn_AnonUnionType_struct{ int tag; struct Cyc_List_List* f1; } ;
static const int Cyc_Absyn_AnonEnumType= 15; struct Cyc_Absyn_AnonEnumType_struct{
int tag; struct Cyc_List_List* f1; } ; static const int Cyc_Absyn_RgnHandleType=
16; struct Cyc_Absyn_RgnHandleType_struct{ int tag; void* f1; } ; static const
int Cyc_Absyn_TypedefType= 17; struct Cyc_Absyn_TypedefType_struct{ int tag;
struct _tuple0* f1; struct Cyc_List_List* f2; struct Cyc_Core_Opt* f3; } ;
static const int Cyc_Absyn_HeapRgn= 2; static const int Cyc_Absyn_AccessEff= 18;
struct Cyc_Absyn_AccessEff_struct{ int tag; void* f1; } ; static const int Cyc_Absyn_JoinEff=
19; struct Cyc_Absyn_JoinEff_struct{ int tag; struct Cyc_List_List* f1; } ;
static const int Cyc_Absyn_RgnsEff= 20; struct Cyc_Absyn_RgnsEff_struct{ int tag;
void* f1; } ; static const int Cyc_Absyn_NoTypes= 0; struct Cyc_Absyn_NoTypes_struct{
int tag; struct Cyc_List_List* f1; struct Cyc_Position_Segment* f2; } ; static
const int Cyc_Absyn_WithTypes= 1; struct Cyc_Absyn_WithTypes_struct{ int tag;
struct Cyc_List_List* f1; int f2; struct Cyc_Absyn_VarargInfo* f3; struct Cyc_Core_Opt*
f4; struct Cyc_List_List* f5; } ; static const int Cyc_Absyn_NonNullable_ps= 0;
struct Cyc_Absyn_NonNullable_ps_struct{ int tag; struct Cyc_Absyn_Exp* f1; } ;
static const int Cyc_Absyn_Nullable_ps= 1; struct Cyc_Absyn_Nullable_ps_struct{
int tag; struct Cyc_Absyn_Exp* f1; } ; static const int Cyc_Absyn_TaggedArray_ps=
0; static const int Cyc_Absyn_Printf_ft= 0; static const int Cyc_Absyn_Scanf_ft=
1; static const int Cyc_Absyn_Regparm_att= 0; struct Cyc_Absyn_Regparm_att_struct{
int tag; int f1; } ; static const int Cyc_Absyn_Stdcall_att= 0; static const int
Cyc_Absyn_Cdecl_att= 1; static const int Cyc_Absyn_Fastcall_att= 2; static const
int Cyc_Absyn_Noreturn_att= 3; static const int Cyc_Absyn_Const_att= 4; static
const int Cyc_Absyn_Aligned_att= 1; struct Cyc_Absyn_Aligned_att_struct{ int tag;
int f1; } ; static const int Cyc_Absyn_Packed_att= 5; static const int Cyc_Absyn_Section_att=
2; struct Cyc_Absyn_Section_att_struct{ int tag; struct _tagged_arr f1; } ;
static const int Cyc_Absyn_Nocommon_att= 6; static const int Cyc_Absyn_Shared_att=
7; static const int Cyc_Absyn_Unused_att= 8; static const int Cyc_Absyn_Weak_att=
9; static const int Cyc_Absyn_Dllimport_att= 10; static const int Cyc_Absyn_Dllexport_att=
11; static const int Cyc_Absyn_No_instrument_function_att= 12; static const int
Cyc_Absyn_Constructor_att= 13; static const int Cyc_Absyn_Destructor_att= 14;
static const int Cyc_Absyn_No_check_memory_usage_att= 15; static const int Cyc_Absyn_Format_att=
3; struct Cyc_Absyn_Format_att_struct{ int tag; void* f1; int f2; int f3; } ;
static const int Cyc_Absyn_Carray_mod= 0; static const int Cyc_Absyn_ConstArray_mod=
0; struct Cyc_Absyn_ConstArray_mod_struct{ int tag; struct Cyc_Absyn_Exp* f1; }
; static const int Cyc_Absyn_Pointer_mod= 1; struct Cyc_Absyn_Pointer_mod_struct{
int tag; void* f1; void* f2; struct Cyc_Absyn_Tqual f3; } ; static const int Cyc_Absyn_Function_mod=
2; struct Cyc_Absyn_Function_mod_struct{ int tag; void* f1; } ; static const int
Cyc_Absyn_TypeParams_mod= 3; struct Cyc_Absyn_TypeParams_mod_struct{ int tag;
struct Cyc_List_List* f1; struct Cyc_Position_Segment* f2; int f3; } ; static
const int Cyc_Absyn_Attributes_mod= 4; struct Cyc_Absyn_Attributes_mod_struct{
int tag; struct Cyc_Position_Segment* f1; struct Cyc_List_List* f2; } ; static
const int Cyc_Absyn_Char_c= 0; struct Cyc_Absyn_Char_c_struct{ int tag; void* f1;
unsigned char f2; } ; static const int Cyc_Absyn_Short_c= 1; struct Cyc_Absyn_Short_c_struct{
int tag; void* f1; short f2; } ; static const int Cyc_Absyn_Int_c= 2; struct Cyc_Absyn_Int_c_struct{
int tag; void* f1; int f2; } ; static const int Cyc_Absyn_LongLong_c= 3; struct
Cyc_Absyn_LongLong_c_struct{ int tag; void* f1; long long f2; } ; static const
int Cyc_Absyn_Float_c= 4; struct Cyc_Absyn_Float_c_struct{ int tag; struct
_tagged_arr f1; } ; static const int Cyc_Absyn_String_c= 5; struct Cyc_Absyn_String_c_struct{
int tag; struct _tagged_arr f1; } ; static const int Cyc_Absyn_Null_c= 0; static
const int Cyc_Absyn_Plus= 0; static const int Cyc_Absyn_Times= 1; static const
int Cyc_Absyn_Minus= 2; static const int Cyc_Absyn_Div= 3; static const int Cyc_Absyn_Mod=
4; static const int Cyc_Absyn_Eq= 5; static const int Cyc_Absyn_Neq= 6; static
const int Cyc_Absyn_Gt= 7; static const int Cyc_Absyn_Lt= 8; static const int
Cyc_Absyn_Gte= 9; static const int Cyc_Absyn_Lte= 10; static const int Cyc_Absyn_Not=
11; static const int Cyc_Absyn_Bitnot= 12; static const int Cyc_Absyn_Bitand= 13;
static const int Cyc_Absyn_Bitor= 14; static const int Cyc_Absyn_Bitxor= 15;
static const int Cyc_Absyn_Bitlshift= 16; static const int Cyc_Absyn_Bitlrshift=
17; static const int Cyc_Absyn_Bitarshift= 18; static const int Cyc_Absyn_Size=
19; static const int Cyc_Absyn_PreInc= 0; static const int Cyc_Absyn_PostInc= 1;
static const int Cyc_Absyn_PreDec= 2; static const int Cyc_Absyn_PostDec= 3;
struct Cyc_Absyn_VarargCallInfo{ int num_varargs; struct Cyc_List_List*
injectors; struct Cyc_Absyn_VarargInfo* vai; } ; static const int Cyc_Absyn_StructField=
0; struct Cyc_Absyn_StructField_struct{ int tag; struct _tagged_arr* f1; } ;
static const int Cyc_Absyn_TupleIndex= 1; struct Cyc_Absyn_TupleIndex_struct{
int tag; unsigned int f1; } ; static const int Cyc_Absyn_Const_e= 0; struct Cyc_Absyn_Const_e_struct{
int tag; void* f1; } ; static const int Cyc_Absyn_Var_e= 1; struct Cyc_Absyn_Var_e_struct{
int tag; struct _tuple0* f1; void* f2; } ; static const int Cyc_Absyn_UnknownId_e=
2; struct Cyc_Absyn_UnknownId_e_struct{ int tag; struct _tuple0* f1; } ; static
const int Cyc_Absyn_Primop_e= 3; struct Cyc_Absyn_Primop_e_struct{ int tag; void*
f1; struct Cyc_List_List* f2; } ; static const int Cyc_Absyn_AssignOp_e= 4;
struct Cyc_Absyn_AssignOp_e_struct{ int tag; struct Cyc_Absyn_Exp* f1; struct
Cyc_Core_Opt* f2; struct Cyc_Absyn_Exp* f3; } ; static const int Cyc_Absyn_Increment_e=
5; struct Cyc_Absyn_Increment_e_struct{ int tag; struct Cyc_Absyn_Exp* f1; void*
f2; } ; static const int Cyc_Absyn_Conditional_e= 6; struct Cyc_Absyn_Conditional_e_struct{
int tag; struct Cyc_Absyn_Exp* f1; struct Cyc_Absyn_Exp* f2; struct Cyc_Absyn_Exp*
f3; } ; static const int Cyc_Absyn_SeqExp_e= 7; struct Cyc_Absyn_SeqExp_e_struct{
int tag; struct Cyc_Absyn_Exp* f1; struct Cyc_Absyn_Exp* f2; } ; static const
int Cyc_Absyn_UnknownCall_e= 8; struct Cyc_Absyn_UnknownCall_e_struct{ int tag;
struct Cyc_Absyn_Exp* f1; struct Cyc_List_List* f2; } ; static const int Cyc_Absyn_FnCall_e=
9; struct Cyc_Absyn_FnCall_e_struct{ int tag; struct Cyc_Absyn_Exp* f1; struct
Cyc_List_List* f2; struct Cyc_Absyn_VarargCallInfo* f3; } ; static const int Cyc_Absyn_Throw_e=
10; struct Cyc_Absyn_Throw_e_struct{ int tag; struct Cyc_Absyn_Exp* f1; } ;
static const int Cyc_Absyn_NoInstantiate_e= 11; struct Cyc_Absyn_NoInstantiate_e_struct{
int tag; struct Cyc_Absyn_Exp* f1; } ; static const int Cyc_Absyn_Instantiate_e=
12; struct Cyc_Absyn_Instantiate_e_struct{ int tag; struct Cyc_Absyn_Exp* f1;
struct Cyc_List_List* f2; } ; static const int Cyc_Absyn_Cast_e= 13; struct Cyc_Absyn_Cast_e_struct{
int tag; void* f1; struct Cyc_Absyn_Exp* f2; } ; static const int Cyc_Absyn_Address_e=
14; struct Cyc_Absyn_Address_e_struct{ int tag; struct Cyc_Absyn_Exp* f1; } ;
static const int Cyc_Absyn_New_e= 15; struct Cyc_Absyn_New_e_struct{ int tag;
struct Cyc_Absyn_Exp* f1; struct Cyc_Absyn_Exp* f2; } ; static const int Cyc_Absyn_Sizeoftyp_e=
16; struct Cyc_Absyn_Sizeoftyp_e_struct{ int tag; void* f1; } ; static const int
Cyc_Absyn_Sizeofexp_e= 17; struct Cyc_Absyn_Sizeofexp_e_struct{ int tag; struct
Cyc_Absyn_Exp* f1; } ; static const int Cyc_Absyn_Offsetof_e= 18; struct Cyc_Absyn_Offsetof_e_struct{
int tag; void* f1; void* f2; } ; static const int Cyc_Absyn_Gentyp_e= 19; struct
Cyc_Absyn_Gentyp_e_struct{ int tag; struct Cyc_List_List* f1; void* f2; } ;
static const int Cyc_Absyn_Deref_e= 20; struct Cyc_Absyn_Deref_e_struct{ int tag;
struct Cyc_Absyn_Exp* f1; } ; static const int Cyc_Absyn_StructMember_e= 21;
struct Cyc_Absyn_StructMember_e_struct{ int tag; struct Cyc_Absyn_Exp* f1;
struct _tagged_arr* f2; } ; static const int Cyc_Absyn_StructArrow_e= 22; struct
Cyc_Absyn_StructArrow_e_struct{ int tag; struct Cyc_Absyn_Exp* f1; struct
_tagged_arr* f2; } ; static const int Cyc_Absyn_Subscript_e= 23; struct Cyc_Absyn_Subscript_e_struct{
int tag; struct Cyc_Absyn_Exp* f1; struct Cyc_Absyn_Exp* f2; } ; static const
int Cyc_Absyn_Tuple_e= 24; struct Cyc_Absyn_Tuple_e_struct{ int tag; struct Cyc_List_List*
f1; } ; static const int Cyc_Absyn_CompoundLit_e= 25; struct _tuple1{ struct Cyc_Core_Opt*
f1; struct Cyc_Absyn_Tqual f2; void* f3; } ; struct Cyc_Absyn_CompoundLit_e_struct{
int tag; struct _tuple1* f1; struct Cyc_List_List* f2; } ; static const int Cyc_Absyn_Array_e=
26; struct Cyc_Absyn_Array_e_struct{ int tag; struct Cyc_List_List* f1; } ;
static const int Cyc_Absyn_Comprehension_e= 27; struct Cyc_Absyn_Comprehension_e_struct{
int tag; struct Cyc_Absyn_Vardecl* f1; struct Cyc_Absyn_Exp* f2; struct Cyc_Absyn_Exp*
f3; } ; static const int Cyc_Absyn_Struct_e= 28; struct Cyc_Absyn_Struct_e_struct{
int tag; struct _tuple0* f1; struct Cyc_Core_Opt* f2; struct Cyc_List_List* f3;
struct Cyc_Absyn_Structdecl* f4; } ; static const int Cyc_Absyn_AnonStruct_e= 29;
struct Cyc_Absyn_AnonStruct_e_struct{ int tag; void* f1; struct Cyc_List_List*
f2; } ; static const int Cyc_Absyn_Tunion_e= 30; struct Cyc_Absyn_Tunion_e_struct{
int tag; struct Cyc_Core_Opt* f1; struct Cyc_Core_Opt* f2; struct Cyc_List_List*
f3; struct Cyc_Absyn_Tuniondecl* f4; struct Cyc_Absyn_Tunionfield* f5; } ;
static const int Cyc_Absyn_Enum_e= 31; struct Cyc_Absyn_Enum_e_struct{ int tag;
struct _tuple0* f1; struct Cyc_Absyn_Enumdecl* f2; struct Cyc_Absyn_Enumfield*
f3; } ; static const int Cyc_Absyn_AnonEnum_e= 32; struct Cyc_Absyn_AnonEnum_e_struct{
int tag; struct _tuple0* f1; void* f2; struct Cyc_Absyn_Enumfield* f3; } ;
static const int Cyc_Absyn_Malloc_e= 33; struct Cyc_Absyn_Malloc_e_struct{ int
tag; struct Cyc_Absyn_Exp* f1; void* f2; } ; static const int Cyc_Absyn_UnresolvedMem_e=
34; struct Cyc_Absyn_UnresolvedMem_e_struct{ int tag; struct Cyc_Core_Opt* f1;
struct Cyc_List_List* f2; } ; static const int Cyc_Absyn_StmtExp_e= 35; struct
Cyc_Absyn_StmtExp_e_struct{ int tag; struct Cyc_Absyn_Stmt* f1; } ; static const
int Cyc_Absyn_Codegen_e= 36; struct Cyc_Absyn_Codegen_e_struct{ int tag; struct
Cyc_Absyn_Fndecl* f1; } ; static const int Cyc_Absyn_Fill_e= 37; struct Cyc_Absyn_Fill_e_struct{
int tag; struct Cyc_Absyn_Exp* f1; } ; struct Cyc_Absyn_Exp{ struct Cyc_Core_Opt*
topt; void* r; struct Cyc_Position_Segment* loc; void* annot; } ; static const
int Cyc_Absyn_Skip_s= 0; static const int Cyc_Absyn_Exp_s= 0; struct Cyc_Absyn_Exp_s_struct{
int tag; struct Cyc_Absyn_Exp* f1; } ; static const int Cyc_Absyn_Seq_s= 1;
struct Cyc_Absyn_Seq_s_struct{ int tag; struct Cyc_Absyn_Stmt* f1; struct Cyc_Absyn_Stmt*
f2; } ; static const int Cyc_Absyn_Return_s= 2; struct Cyc_Absyn_Return_s_struct{
int tag; struct Cyc_Absyn_Exp* f1; } ; static const int Cyc_Absyn_IfThenElse_s=
3; struct Cyc_Absyn_IfThenElse_s_struct{ int tag; struct Cyc_Absyn_Exp* f1;
struct Cyc_Absyn_Stmt* f2; struct Cyc_Absyn_Stmt* f3; } ; static const int Cyc_Absyn_While_s=
4; struct _tuple2{ struct Cyc_Absyn_Exp* f1; struct Cyc_Absyn_Stmt* f2; } ;
struct Cyc_Absyn_While_s_struct{ int tag; struct _tuple2 f1; struct Cyc_Absyn_Stmt*
f2; } ; static const int Cyc_Absyn_Break_s= 5; struct Cyc_Absyn_Break_s_struct{
int tag; struct Cyc_Absyn_Stmt* f1; } ; static const int Cyc_Absyn_Continue_s= 6;
struct Cyc_Absyn_Continue_s_struct{ int tag; struct Cyc_Absyn_Stmt* f1; } ;
static const int Cyc_Absyn_Goto_s= 7; struct Cyc_Absyn_Goto_s_struct{ int tag;
struct _tagged_arr* f1; struct Cyc_Absyn_Stmt* f2; } ; static const int Cyc_Absyn_For_s=
8; struct Cyc_Absyn_For_s_struct{ int tag; struct Cyc_Absyn_Exp* f1; struct
_tuple2 f2; struct _tuple2 f3; struct Cyc_Absyn_Stmt* f4; } ; static const int
Cyc_Absyn_Switch_s= 9; struct Cyc_Absyn_Switch_s_struct{ int tag; struct Cyc_Absyn_Exp*
f1; struct Cyc_List_List* f2; } ; static const int Cyc_Absyn_SwitchC_s= 10;
struct Cyc_Absyn_SwitchC_s_struct{ int tag; struct Cyc_Absyn_Exp* f1; struct Cyc_List_List*
f2; } ; static const int Cyc_Absyn_Fallthru_s= 11; struct Cyc_Absyn_Fallthru_s_struct{
int tag; struct Cyc_List_List* f1; struct Cyc_Absyn_Switch_clause** f2; } ;
static const int Cyc_Absyn_Decl_s= 12; struct Cyc_Absyn_Decl_s_struct{ int tag;
struct Cyc_Absyn_Decl* f1; struct Cyc_Absyn_Stmt* f2; } ; static const int Cyc_Absyn_Cut_s=
13; struct Cyc_Absyn_Cut_s_struct{ int tag; struct Cyc_Absyn_Stmt* f1; } ;
static const int Cyc_Absyn_Splice_s= 14; struct Cyc_Absyn_Splice_s_struct{ int
tag; struct Cyc_Absyn_Stmt* f1; } ; static const int Cyc_Absyn_Label_s= 15;
struct Cyc_Absyn_Label_s_struct{ int tag; struct _tagged_arr* f1; struct Cyc_Absyn_Stmt*
f2; } ; static const int Cyc_Absyn_Do_s= 16; struct Cyc_Absyn_Do_s_struct{ int
tag; struct Cyc_Absyn_Stmt* f1; struct _tuple2 f2; } ; static const int Cyc_Absyn_TryCatch_s=
17; struct Cyc_Absyn_TryCatch_s_struct{ int tag; struct Cyc_Absyn_Stmt* f1;
struct Cyc_List_List* f2; } ; static const int Cyc_Absyn_Region_s= 18; struct
Cyc_Absyn_Region_s_struct{ int tag; struct Cyc_Absyn_Tvar* f1; struct Cyc_Absyn_Vardecl*
f2; struct Cyc_Absyn_Stmt* f3; } ; struct Cyc_Absyn_Stmt{ void* r; struct Cyc_Position_Segment*
loc; struct Cyc_List_List* non_local_preds; int try_depth; void* annot; } ;
static const int Cyc_Absyn_Wild_p= 0; static const int Cyc_Absyn_Var_p= 0;
struct Cyc_Absyn_Var_p_struct{ int tag; struct Cyc_Absyn_Vardecl* f1; } ; static
const int Cyc_Absyn_Null_p= 1; static const int Cyc_Absyn_Int_p= 1; struct Cyc_Absyn_Int_p_struct{
int tag; void* f1; int f2; } ; static const int Cyc_Absyn_Char_p= 2; struct Cyc_Absyn_Char_p_struct{
int tag; unsigned char f1; } ; static const int Cyc_Absyn_Float_p= 3; struct Cyc_Absyn_Float_p_struct{
int tag; struct _tagged_arr f1; } ; static const int Cyc_Absyn_Tuple_p= 4;
struct Cyc_Absyn_Tuple_p_struct{ int tag; struct Cyc_List_List* f1; } ; static
const int Cyc_Absyn_Pointer_p= 5; struct Cyc_Absyn_Pointer_p_struct{ int tag;
struct Cyc_Absyn_Pat* f1; } ; static const int Cyc_Absyn_Reference_p= 6; struct
Cyc_Absyn_Reference_p_struct{ int tag; struct Cyc_Absyn_Vardecl* f1; } ; static
const int Cyc_Absyn_Struct_p= 7; struct Cyc_Absyn_Struct_p_struct{ int tag;
struct Cyc_Absyn_Structdecl* f1; struct Cyc_Core_Opt* f2; struct Cyc_List_List*
f3; struct Cyc_List_List* f4; } ; static const int Cyc_Absyn_Tunion_p= 8; struct
Cyc_Absyn_Tunion_p_struct{ int tag; struct Cyc_Absyn_Tuniondecl* f1; struct Cyc_Absyn_Tunionfield*
f2; struct Cyc_List_List* f3; struct Cyc_List_List* f4; } ; static const int Cyc_Absyn_Enum_p=
9; struct Cyc_Absyn_Enum_p_struct{ int tag; struct Cyc_Absyn_Enumdecl* f1;
struct Cyc_Absyn_Enumfield* f2; } ; static const int Cyc_Absyn_AnonEnum_p= 10;
struct Cyc_Absyn_AnonEnum_p_struct{ int tag; void* f1; struct Cyc_Absyn_Enumfield*
f2; } ; static const int Cyc_Absyn_UnknownId_p= 11; struct Cyc_Absyn_UnknownId_p_struct{
int tag; struct _tuple0* f1; } ; static const int Cyc_Absyn_UnknownCall_p= 12;
struct Cyc_Absyn_UnknownCall_p_struct{ int tag; struct _tuple0* f1; struct Cyc_List_List*
f2; struct Cyc_List_List* f3; } ; static const int Cyc_Absyn_UnknownFields_p= 13;
struct Cyc_Absyn_UnknownFields_p_struct{ int tag; struct _tuple0* f1; struct Cyc_List_List*
f2; struct Cyc_List_List* f3; } ; struct Cyc_Absyn_Pat{ void* r; struct Cyc_Core_Opt*
topt; struct Cyc_Position_Segment* loc; } ; struct Cyc_Absyn_Switch_clause{
struct Cyc_Absyn_Pat* pattern; struct Cyc_Core_Opt* pat_vars; struct Cyc_Absyn_Exp*
where_clause; struct Cyc_Absyn_Stmt* body; struct Cyc_Position_Segment* loc; } ;
struct Cyc_Absyn_SwitchC_clause{ struct Cyc_Absyn_Exp* cnst_exp; struct Cyc_Absyn_Stmt*
body; struct Cyc_Position_Segment* loc; } ; static const int Cyc_Absyn_Unresolved_b=
0; static const int Cyc_Absyn_Global_b= 0; struct Cyc_Absyn_Global_b_struct{ int
tag; struct Cyc_Absyn_Vardecl* f1; } ; static const int Cyc_Absyn_Funname_b= 1;
struct Cyc_Absyn_Funname_b_struct{ int tag; struct Cyc_Absyn_Fndecl* f1; } ;
static const int Cyc_Absyn_Param_b= 2; struct Cyc_Absyn_Param_b_struct{ int tag;
struct Cyc_Absyn_Vardecl* f1; } ; static const int Cyc_Absyn_Local_b= 3; struct
Cyc_Absyn_Local_b_struct{ int tag; struct Cyc_Absyn_Vardecl* f1; } ; static
const int Cyc_Absyn_Pat_b= 4; struct Cyc_Absyn_Pat_b_struct{ int tag; struct Cyc_Absyn_Vardecl*
f1; } ; struct Cyc_Absyn_Vardecl{ void* sc; struct _tuple0* name; struct Cyc_Absyn_Tqual
tq; void* type; struct Cyc_Absyn_Exp* initializer; struct Cyc_Core_Opt* rgn;
struct Cyc_List_List* attributes; } ; struct Cyc_Absyn_Fndecl{ void* sc; int
is_inline; struct _tuple0* name; struct Cyc_List_List* tvs; struct Cyc_Core_Opt*
effect; void* ret_type; struct Cyc_List_List* args; int c_varargs; struct Cyc_Absyn_VarargInfo*
cyc_varargs; struct Cyc_List_List* rgn_po; struct Cyc_Absyn_Stmt* body; struct
Cyc_Core_Opt* cached_typ; struct Cyc_Core_Opt* param_vardecls; struct Cyc_List_List*
attributes; } ; struct Cyc_Absyn_Structfield{ struct _tagged_arr* name; struct
Cyc_Absyn_Tqual tq; void* type; struct Cyc_Absyn_Exp* width; struct Cyc_List_List*
attributes; } ; struct Cyc_Absyn_Structdecl{ void* sc; struct Cyc_Core_Opt* name;
struct Cyc_List_List* tvs; struct Cyc_Core_Opt* fields; struct Cyc_List_List*
attributes; } ; struct Cyc_Absyn_Uniondecl{ void* sc; struct Cyc_Core_Opt* name;
struct Cyc_List_List* tvs; struct Cyc_Core_Opt* fields; struct Cyc_List_List*
attributes; } ; struct Cyc_Absyn_Tunionfield{ struct _tuple0* name; struct Cyc_List_List*
tvs; struct Cyc_List_List* typs; struct Cyc_Position_Segment* loc; void* sc; } ;
struct Cyc_Absyn_Tuniondecl{ void* sc; struct _tuple0* name; struct Cyc_List_List*
tvs; struct Cyc_Core_Opt* fields; int is_xtunion; } ; struct Cyc_Absyn_Enumfield{
struct _tuple0* name; struct Cyc_Absyn_Exp* tag; struct Cyc_Position_Segment*
loc; } ; struct Cyc_Absyn_Enumdecl{ void* sc; struct _tuple0* name; struct Cyc_Core_Opt*
fields; } ; struct Cyc_Absyn_Typedefdecl{ struct _tuple0* name; struct Cyc_List_List*
tvs; void* defn; } ; static const int Cyc_Absyn_Var_d= 0; struct Cyc_Absyn_Var_d_struct{
int tag; struct Cyc_Absyn_Vardecl* f1; } ; static const int Cyc_Absyn_Fn_d= 1;
struct Cyc_Absyn_Fn_d_struct{ int tag; struct Cyc_Absyn_Fndecl* f1; } ; static
const int Cyc_Absyn_Let_d= 2; struct Cyc_Absyn_Let_d_struct{ int tag; struct Cyc_Absyn_Pat*
f1; struct Cyc_Core_Opt* f2; struct Cyc_Core_Opt* f3; struct Cyc_Absyn_Exp* f4;
int f5; } ; static const int Cyc_Absyn_Letv_d= 3; struct Cyc_Absyn_Letv_d_struct{
int tag; struct Cyc_List_List* f1; } ; static const int Cyc_Absyn_Struct_d= 4;
struct Cyc_Absyn_Struct_d_struct{ int tag; struct Cyc_Absyn_Structdecl* f1; } ;
static const int Cyc_Absyn_Union_d= 5; struct Cyc_Absyn_Union_d_struct{ int tag;
struct Cyc_Absyn_Uniondecl* f1; } ; static const int Cyc_Absyn_Tunion_d= 6;
struct Cyc_Absyn_Tunion_d_struct{ int tag; struct Cyc_Absyn_Tuniondecl* f1; } ;
static const int Cyc_Absyn_Enum_d= 7; struct Cyc_Absyn_Enum_d_struct{ int tag;
struct Cyc_Absyn_Enumdecl* f1; } ; static const int Cyc_Absyn_Typedef_d= 8;
struct Cyc_Absyn_Typedef_d_struct{ int tag; struct Cyc_Absyn_Typedefdecl* f1; }
; static const int Cyc_Absyn_Namespace_d= 9; struct Cyc_Absyn_Namespace_d_struct{
int tag; struct _tagged_arr* f1; struct Cyc_List_List* f2; } ; static const int
Cyc_Absyn_Using_d= 10; struct Cyc_Absyn_Using_d_struct{ int tag; struct _tuple0*
f1; struct Cyc_List_List* f2; } ; static const int Cyc_Absyn_ExternC_d= 11;
struct Cyc_Absyn_ExternC_d_struct{ int tag; struct Cyc_List_List* f1; } ; struct
Cyc_Absyn_Decl{ void* r; struct Cyc_Position_Segment* loc; } ; static const int
Cyc_Absyn_ArrayElement= 0; struct Cyc_Absyn_ArrayElement_struct{ int tag; struct
Cyc_Absyn_Exp* f1; } ; static const int Cyc_Absyn_FieldName= 1; struct Cyc_Absyn_FieldName_struct{
int tag; struct _tagged_arr* f1; } ; extern unsigned char Cyc_Absyn_EmptyAnnot[
15u]; extern struct Cyc_Absyn_Conref* Cyc_Absyn_new_conref( void* x); extern
void* Cyc_Absyn_exn_typ; extern struct Cyc_Absyn_Exp* Cyc_Absyn_uint_exp(
unsigned int, struct Cyc_Position_Segment*); extern struct Cyc_Absyn_Stmt* Cyc_Absyn_new_stmt(
void* s, struct Cyc_Position_Segment* loc); extern struct Cyc_Absyn_Stmt* Cyc_Absyn_skip_stmt(
struct Cyc_Position_Segment* loc); extern struct Cyc_Absyn_Stmt* Cyc_Absyn_seq_stmt(
struct Cyc_Absyn_Stmt* s1, struct Cyc_Absyn_Stmt* s2, struct Cyc_Position_Segment*
loc); extern struct Cyc_Absyn_Stmt* Cyc_Absyn_switch_stmt( struct Cyc_Absyn_Exp*
e, struct Cyc_List_List*, struct Cyc_Position_Segment* loc); extern struct Cyc_Absyn_Pat*
Cyc_Absyn_new_pat( void* p, struct Cyc_Position_Segment* s); struct Cyc_Std___sFILE;
extern unsigned char Cyc_Std_FileCloseError[ 19u]; extern unsigned char Cyc_Std_FileOpenError[
18u]; struct Cyc_Std_FileOpenError_struct{ unsigned char* tag; struct
_tagged_arr f1; } ; static const int Cyc_Std_String_pa= 0; struct Cyc_Std_String_pa_struct{
int tag; struct _tagged_arr f1; } ; static const int Cyc_Std_Int_pa= 1; struct
Cyc_Std_Int_pa_struct{ int tag; unsigned int f1; } ; static const int Cyc_Std_Double_pa=
2; struct Cyc_Std_Double_pa_struct{ int tag; double f1; } ; static const int Cyc_Std_ShortPtr_pa=
3; struct Cyc_Std_ShortPtr_pa_struct{ int tag; short* f1; } ; static const int
Cyc_Std_IntPtr_pa= 4; struct Cyc_Std_IntPtr_pa_struct{ int tag; unsigned int* f1;
} ; extern struct _tagged_arr Cyc_Std_aprintf( struct _tagged_arr fmt, struct
_tagged_arr); static const int Cyc_Std_ShortPtr_sa= 0; struct Cyc_Std_ShortPtr_sa_struct{
int tag; short* f1; } ; static const int Cyc_Std_UShortPtr_sa= 1; struct Cyc_Std_UShortPtr_sa_struct{
int tag; unsigned short* f1; } ; static const int Cyc_Std_IntPtr_sa= 2; struct
Cyc_Std_IntPtr_sa_struct{ int tag; int* f1; } ; static const int Cyc_Std_UIntPtr_sa=
3; struct Cyc_Std_UIntPtr_sa_struct{ int tag; unsigned int* f1; } ; static const
int Cyc_Std_StringPtr_sa= 4; struct Cyc_Std_StringPtr_sa_struct{ int tag; struct
_tagged_arr f1; } ; static const int Cyc_Std_DoublePtr_sa= 5; struct Cyc_Std_DoublePtr_sa_struct{
int tag; double* f1; } ; static const int Cyc_Std_FloatPtr_sa= 6; struct Cyc_Std_FloatPtr_sa_struct{
int tag; float* f1; } ; struct Cyc_PP_Ppstate; struct Cyc_PP_Out; struct Cyc_PP_Doc;
struct Cyc_Absynpp_Params{ int expand_typedefs: 1; int qvar_to_Cids: 1; int
add_cyc_prefix: 1; int to_VC: 1; int decls_first: 1; int rewrite_temp_tvars: 1;
int print_all_tvars: 1; int print_all_kinds: 1; int print_using_stmts: 1; int
print_externC_stmts: 1; int print_full_evars: 1; int use_curr_namespace: 1;
struct Cyc_List_List* curr_namespace; } ; extern struct _tagged_arr Cyc_Absynpp_typ2string(
void*); struct Cyc_Set_Set; extern unsigned char Cyc_Set_Absent[ 11u]; struct
Cyc_Dict_Dict; extern unsigned char Cyc_Dict_Present[ 12u]; extern unsigned char
Cyc_Dict_Absent[ 11u]; static const int Cyc_Tcenv_VarRes= 0; struct Cyc_Tcenv_VarRes_struct{
int tag; void* f1; } ; static const int Cyc_Tcenv_StructRes= 1; struct Cyc_Tcenv_StructRes_struct{
int tag; struct Cyc_Absyn_Structdecl* f1; } ; static const int Cyc_Tcenv_TunionRes=
2; struct Cyc_Tcenv_TunionRes_struct{ int tag; struct Cyc_Absyn_Tuniondecl* f1;
struct Cyc_Absyn_Tunionfield* f2; } ; static const int Cyc_Tcenv_EnumRes= 3;
struct Cyc_Tcenv_EnumRes_struct{ int tag; struct Cyc_Absyn_Enumdecl* f1; struct
Cyc_Absyn_Enumfield* f2; } ; static const int Cyc_Tcenv_AnonEnumRes= 4; struct
Cyc_Tcenv_AnonEnumRes_struct{ int tag; void* f1; struct Cyc_Absyn_Enumfield* f2;
} ; struct Cyc_Tcenv_Genv{ struct Cyc_Set_Set* namespaces; struct Cyc_Dict_Dict*
structdecls; struct Cyc_Dict_Dict* uniondecls; struct Cyc_Dict_Dict* tuniondecls;
struct Cyc_Dict_Dict* enumdecls; struct Cyc_Dict_Dict* typedefs; struct Cyc_Dict_Dict*
ordinaries; struct Cyc_List_List* availables; } ; struct Cyc_Tcenv_Fenv; static
const int Cyc_Tcenv_NotLoop_j= 0; static const int Cyc_Tcenv_CaseEnd_j= 1;
static const int Cyc_Tcenv_FnEnd_j= 2; static const int Cyc_Tcenv_Stmt_j= 0;
struct Cyc_Tcenv_Stmt_j_struct{ int tag; struct Cyc_Absyn_Stmt* f1; } ; static
const int Cyc_Tcenv_Outermost= 0; struct Cyc_Tcenv_Outermost_struct{ int tag;
void* f1; } ; static const int Cyc_Tcenv_Frame= 1; struct Cyc_Tcenv_Frame_struct{
int tag; void* f1; void* f2; } ; static const int Cyc_Tcenv_Hidden= 2; struct
Cyc_Tcenv_Hidden_struct{ int tag; void* f1; void* f2; } ; struct Cyc_Tcenv_Tenv{
struct Cyc_List_List* ns; struct Cyc_Dict_Dict* ae; struct Cyc_Core_Opt* le; } ;
extern void* Cyc_Tcenv_return_typ( struct Cyc_Tcenv_Tenv*); extern struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_add_local_var( struct Cyc_Position_Segment*, struct Cyc_Tcenv_Tenv*,
struct Cyc_Absyn_Vardecl*); extern struct Cyc_Tcenv_Tenv* Cyc_Tcenv_add_pat_var(
struct Cyc_Position_Segment*, struct Cyc_Tcenv_Tenv*, struct Cyc_Absyn_Vardecl*);
extern struct Cyc_List_List* Cyc_Tcenv_lookup_type_vars( struct Cyc_Tcenv_Tenv*);
extern struct Cyc_Tcenv_Tenv* Cyc_Tcenv_add_type_vars( struct Cyc_Position_Segment*,
struct Cyc_Tcenv_Tenv*, struct Cyc_List_List*); extern struct Cyc_Tcenv_Tenv*
Cyc_Tcenv_set_in_loop( struct Cyc_Tcenv_Tenv* te, struct Cyc_Absyn_Stmt*
continue_dest); extern struct Cyc_Tcenv_Tenv* Cyc_Tcenv_set_in_switch( struct
Cyc_Tcenv_Tenv*); extern struct Cyc_Tcenv_Tenv* Cyc_Tcenv_set_fallthru( struct
Cyc_Tcenv_Tenv* te, struct Cyc_List_List* new_tvs, struct Cyc_List_List* vds,
struct Cyc_Absyn_Switch_clause* clause); extern struct Cyc_Tcenv_Tenv* Cyc_Tcenv_clear_fallthru(
struct Cyc_Tcenv_Tenv*); extern struct Cyc_Tcenv_Tenv* Cyc_Tcenv_set_next(
struct Cyc_Tcenv_Tenv*, void*); extern struct Cyc_Tcenv_Tenv* Cyc_Tcenv_enter_try(
struct Cyc_Tcenv_Tenv* te); extern int Cyc_Tcenv_get_try_depth( struct Cyc_Tcenv_Tenv*
te); extern void Cyc_Tcenv_process_continue( struct Cyc_Tcenv_Tenv*, struct Cyc_Absyn_Stmt*,
struct Cyc_Absyn_Stmt**); extern void Cyc_Tcenv_process_break( struct Cyc_Tcenv_Tenv*,
struct Cyc_Absyn_Stmt*, struct Cyc_Absyn_Stmt**); extern void Cyc_Tcenv_process_goto(
struct Cyc_Tcenv_Tenv*, struct Cyc_Absyn_Stmt*, struct _tagged_arr*, struct Cyc_Absyn_Stmt**);
struct _tuple3{ struct Cyc_Absyn_Switch_clause* f1; struct Cyc_List_List* f2;
struct Cyc_List_List* f3; } ; extern struct _tuple3* Cyc_Tcenv_process_fallthru(
struct Cyc_Tcenv_Tenv*, struct Cyc_Absyn_Stmt*, struct Cyc_Absyn_Switch_clause***);
extern struct Cyc_Absyn_Stmt* Cyc_Tcenv_get_encloser( struct Cyc_Tcenv_Tenv*);
extern struct Cyc_Tcenv_Tenv* Cyc_Tcenv_set_encloser( struct Cyc_Tcenv_Tenv*,
struct Cyc_Absyn_Stmt*); extern struct Cyc_Tcenv_Tenv* Cyc_Tcenv_add_label(
struct Cyc_Tcenv_Tenv*, struct _tagged_arr*, struct Cyc_Absyn_Stmt*); extern
struct Cyc_Tcenv_Tenv* Cyc_Tcenv_new_block( struct Cyc_Position_Segment*, struct
Cyc_Tcenv_Tenv*); extern struct Cyc_Tcenv_Tenv* Cyc_Tcenv_new_named_block(
struct Cyc_Position_Segment*, struct Cyc_Tcenv_Tenv*, struct Cyc_Absyn_Tvar*
name); extern void* Cyc_Tcenv_curr_rgn( struct Cyc_Tcenv_Tenv*); extern struct
Cyc_Tcenv_Tenv* Cyc_Tcenv_add_region( struct Cyc_Tcenv_Tenv* te, void* r);
extern unsigned char Cyc_Tcutil_TypeErr[ 12u]; extern void* Cyc_Tcutil_impos(
struct _tagged_arr fmt, struct _tagged_arr ap); extern void Cyc_Tcutil_terr(
struct Cyc_Position_Segment*, struct _tagged_arr fmt, struct _tagged_arr ap);
extern void* Cyc_Tcutil_compress( void* t); extern int Cyc_Tcutil_coerce_arg(
struct Cyc_Tcenv_Tenv*, struct Cyc_Absyn_Exp*, void*); extern int Cyc_Tcutil_coerce_assign(
struct Cyc_Tcenv_Tenv*, struct Cyc_Absyn_Exp*, void*); extern void Cyc_Tcutil_explain_failure();
extern int Cyc_Tcutil_unify( void*, void*); extern void* Cyc_Tcutil_substitute(
struct Cyc_List_List*, void*); struct _tuple4{ struct Cyc_Absyn_Tvar* f1; void*
f2; } ; extern struct _tuple4* Cyc_Tcutil_make_inst_var( struct Cyc_List_List*,
struct Cyc_Absyn_Tvar*); extern void Cyc_Tcutil_check_type( struct Cyc_Position_Segment*,
struct Cyc_Tcenv_Tenv*, struct Cyc_List_List* bound_tvars, void* k, int
allow_evars, void*); extern int* Cyc_Tcutil_new_tvar_id(); extern int Cyc_Tcutil_is_const_exp(
struct Cyc_Tcenv_Tenv* te, struct Cyc_Absyn_Exp* e); extern void* Cyc_Tcexp_tcExp(
struct Cyc_Tcenv_Tenv*, void**, struct Cyc_Absyn_Exp*); extern void* Cyc_Tcexp_tcExpInitializer(
struct Cyc_Tcenv_Tenv*, void**, struct Cyc_Absyn_Exp*); extern void Cyc_Tcexp_tcTest(
struct Cyc_Tcenv_Tenv* te, struct Cyc_Absyn_Exp* e, struct _tagged_arr msg_part);
struct _tuple5{ struct Cyc_List_List* f1; struct Cyc_List_List* f2; } ; extern
struct _tuple5 Cyc_Tcpat_tcPat( struct Cyc_Tcenv_Tenv* te, struct Cyc_Absyn_Pat*
p, void** topt, void** region_opt); extern void Cyc_Tcpat_check_pat_regions(
struct Cyc_Tcenv_Tenv* te, struct Cyc_Absyn_Pat* p); extern void Cyc_Tcpat_check_switch_exhaustive(
struct Cyc_Position_Segment*, struct Cyc_List_List*); extern int Cyc_Tcpat_check_let_pat_exhaustive(
struct Cyc_Position_Segment*, struct Cyc_Absyn_Pat* p); extern void Cyc_Tcpat_check_catch_overlap(
struct Cyc_Position_Segment*, struct Cyc_List_List*); static const int Cyc_CfFlowInfo_VarRoot=
0; struct Cyc_CfFlowInfo_VarRoot_struct{ int tag; struct Cyc_Absyn_Vardecl* f1;
} ; static const int Cyc_CfFlowInfo_MallocPt= 1; struct Cyc_CfFlowInfo_MallocPt_struct{
int tag; struct Cyc_Absyn_Exp* f1; } ; struct Cyc_CfFlowInfo_Place{ void* root;
struct Cyc_List_List* fields; } ; static const int Cyc_CfFlowInfo_NoneIL= 0;
static const int Cyc_CfFlowInfo_ThisIL= 1; static const int Cyc_CfFlowInfo_AllIL=
2; extern unsigned char Cyc_CfFlowInfo_IsZero[ 11u]; extern unsigned char Cyc_CfFlowInfo_NotZero[
12u]; extern unsigned char Cyc_CfFlowInfo_UnknownZ[ 13u]; static const int Cyc_CfFlowInfo_PlaceL=
0; struct Cyc_CfFlowInfo_PlaceL_struct{ int tag; struct Cyc_CfFlowInfo_Place* f1;
} ; static const int Cyc_CfFlowInfo_UnknownL= 0; static const int Cyc_CfFlowInfo_Zero=
0; static const int Cyc_CfFlowInfo_NotZeroAll= 1; static const int Cyc_CfFlowInfo_NotZeroThis=
2; static const int Cyc_CfFlowInfo_UnknownR= 0; struct Cyc_CfFlowInfo_UnknownR_struct{
int tag; void* f1; } ; static const int Cyc_CfFlowInfo_Esc= 1; struct Cyc_CfFlowInfo_Esc_struct{
int tag; void* f1; } ; static const int Cyc_CfFlowInfo_AddressOf= 2; struct Cyc_CfFlowInfo_AddressOf_struct{
int tag; struct Cyc_CfFlowInfo_Place* f1; } ; static const int Cyc_CfFlowInfo_Aggregate=
3; struct Cyc_CfFlowInfo_Aggregate_struct{ int tag; struct Cyc_Dict_Dict* f1; }
; static const int Cyc_CfFlowInfo_BottomFL= 0; static const int Cyc_CfFlowInfo_ReachableFL=
0; struct Cyc_CfFlowInfo_ReachableFL_struct{ int tag; struct Cyc_Dict_Dict* f1;
} ; extern void Cyc_NewControlFlow_set_encloser( struct Cyc_Absyn_Stmt* enclosee,
struct Cyc_Absyn_Stmt* encloser); extern void Cyc_Tcstmt_tcStmt( struct Cyc_Tcenv_Tenv*
te, struct Cyc_Absyn_Stmt* s, int new_block); extern unsigned int Cyc_Evexp_eval_const_uint_exp(
struct Cyc_Absyn_Exp* e); static void Cyc_Tcstmt_decorate_stmt( struct Cyc_Tcenv_Tenv*
te, struct Cyc_Absyn_Stmt* s){ Cyc_NewControlFlow_set_encloser( s, Cyc_Tcenv_get_encloser(
te)); s->try_depth= Cyc_Tcenv_get_try_depth( te);} static void Cyc_Tcstmt_pattern_synth(
struct Cyc_Position_Segment* loc, struct Cyc_Tcenv_Tenv* te, struct Cyc_List_List*
tvs, struct Cyc_List_List* vs, struct Cyc_Absyn_Stmt* s, struct Cyc_Absyn_Exp*
where_opt, int new_block){ struct Cyc_Tcenv_Tenv* te2= Cyc_Tcenv_add_type_vars(
loc, te, tvs); if( new_block){ te2= Cyc_Tcenv_new_block( loc, te2);}{ struct Cyc_Core_Opt*
_temp0=({ struct Cyc_Core_Opt* _temp2=( struct Cyc_Core_Opt*) _cycalloc( sizeof(
struct Cyc_Core_Opt)); _temp2->v=( void*) Cyc_Tcenv_curr_rgn( te2); _temp2;});{
struct Cyc_List_List* _temp1= vs; for( 0; _temp1 !=  0; _temp1= _temp1->tl){ te2=
Cyc_Tcenv_add_pat_var( loc, te2,( struct Cyc_Absyn_Vardecl*) _temp1->hd);((
struct Cyc_Absyn_Vardecl*) _temp1->hd)->rgn= _temp0;}} if( where_opt !=  0){ Cyc_Tcexp_tcTest(
te2,( struct Cyc_Absyn_Exp*) _check_null( where_opt), _tag_arr("switch clause guard",
sizeof( unsigned char), 20u));} if( vs !=  0){ te2= Cyc_Tcenv_set_encloser( te2,
s);} Cyc_Tcstmt_tcStmt( te2, s, 0); if( vs !=  0){ Cyc_NewControlFlow_set_encloser(
s, Cyc_Tcenv_get_encloser( te));}}} void Cyc_Tcstmt_tcStmt( struct Cyc_Tcenv_Tenv*
te, struct Cyc_Absyn_Stmt* s0, int new_block){ Cyc_Tcstmt_decorate_stmt( te, s0);{
void* _temp3=( void*) s0->r; struct Cyc_Absyn_Exp* _temp45; struct Cyc_Absyn_Stmt*
_temp47; struct Cyc_Absyn_Stmt* _temp49; struct Cyc_Absyn_Exp* _temp51; struct
Cyc_Absyn_Stmt* _temp53; struct Cyc_Absyn_Stmt* _temp55; struct Cyc_Absyn_Exp*
_temp57; struct Cyc_Absyn_Stmt* _temp59; struct _tuple2 _temp61; struct Cyc_Absyn_Stmt*
_temp63; struct Cyc_Absyn_Exp* _temp65; struct Cyc_Absyn_Stmt* _temp67; struct
_tuple2 _temp69; struct Cyc_Absyn_Stmt* _temp71; struct Cyc_Absyn_Exp* _temp73;
struct _tuple2 _temp75; struct Cyc_Absyn_Stmt* _temp77; struct Cyc_Absyn_Exp*
_temp79; struct Cyc_Absyn_Exp* _temp81; struct _tuple2 _temp83; struct Cyc_Absyn_Stmt*
_temp85; struct Cyc_Absyn_Exp* _temp87; struct Cyc_Absyn_Stmt* _temp89; struct
Cyc_Absyn_Stmt* _temp91; struct Cyc_Absyn_Stmt** _temp93; struct Cyc_Absyn_Stmt*
_temp94; struct Cyc_Absyn_Stmt** _temp96; struct Cyc_Absyn_Stmt* _temp97; struct
Cyc_Absyn_Stmt** _temp99; struct _tagged_arr* _temp100; struct Cyc_Absyn_Switch_clause**
_temp102; struct Cyc_Absyn_Switch_clause*** _temp104; struct Cyc_List_List*
_temp105; struct Cyc_Absyn_Stmt* _temp107; struct _tagged_arr* _temp109; struct
Cyc_List_List* _temp111; struct Cyc_Absyn_Exp* _temp113; struct Cyc_List_List*
_temp115; struct Cyc_Absyn_Exp* _temp117; struct Cyc_List_List* _temp119; struct
Cyc_Absyn_Stmt* _temp121; struct Cyc_Absyn_Stmt* _temp123; struct Cyc_Absyn_Decl*
_temp125; struct Cyc_Absyn_Stmt* _temp127; struct Cyc_Absyn_Vardecl* _temp129;
struct Cyc_Absyn_Tvar* _temp131; struct Cyc_Absyn_Stmt* _temp133; struct Cyc_Absyn_Stmt*
_temp135; _LL5: if( _temp3 == ( void*) Cyc_Absyn_Skip_s){ goto _LL6;} else{ goto
_LL7;} _LL7: if(( unsigned int) _temp3 >  1u?*(( int*) _temp3) ==  Cyc_Absyn_Exp_s:
0){ _LL46: _temp45=(( struct Cyc_Absyn_Exp_s_struct*) _temp3)->f1; goto _LL8;}
else{ goto _LL9;} _LL9: if(( unsigned int) _temp3 >  1u?*(( int*) _temp3) == 
Cyc_Absyn_Seq_s: 0){ _LL50: _temp49=(( struct Cyc_Absyn_Seq_s_struct*) _temp3)->f1;
goto _LL48; _LL48: _temp47=(( struct Cyc_Absyn_Seq_s_struct*) _temp3)->f2; goto
_LL10;} else{ goto _LL11;} _LL11: if(( unsigned int) _temp3 >  1u?*(( int*)
_temp3) ==  Cyc_Absyn_Return_s: 0){ _LL52: _temp51=(( struct Cyc_Absyn_Return_s_struct*)
_temp3)->f1; goto _LL12;} else{ goto _LL13;} _LL13: if(( unsigned int) _temp3 > 
1u?*(( int*) _temp3) ==  Cyc_Absyn_IfThenElse_s: 0){ _LL58: _temp57=(( struct
Cyc_Absyn_IfThenElse_s_struct*) _temp3)->f1; goto _LL56; _LL56: _temp55=((
struct Cyc_Absyn_IfThenElse_s_struct*) _temp3)->f2; goto _LL54; _LL54: _temp53=((
struct Cyc_Absyn_IfThenElse_s_struct*) _temp3)->f3; goto _LL14;} else{ goto
_LL15;} _LL15: if(( unsigned int) _temp3 >  1u?*(( int*) _temp3) ==  Cyc_Absyn_While_s:
0){ _LL62: _temp61=(( struct Cyc_Absyn_While_s_struct*) _temp3)->f1; _LL66:
_temp65= _temp61.f1; goto _LL64; _LL64: _temp63= _temp61.f2; goto _LL60; _LL60:
_temp59=(( struct Cyc_Absyn_While_s_struct*) _temp3)->f2; goto _LL16;} else{
goto _LL17;} _LL17: if(( unsigned int) _temp3 >  1u?*(( int*) _temp3) ==  Cyc_Absyn_For_s:
0){ _LL82: _temp81=(( struct Cyc_Absyn_For_s_struct*) _temp3)->f1; goto _LL76;
_LL76: _temp75=(( struct Cyc_Absyn_For_s_struct*) _temp3)->f2; _LL80: _temp79=
_temp75.f1; goto _LL78; _LL78: _temp77= _temp75.f2; goto _LL70; _LL70: _temp69=((
struct Cyc_Absyn_For_s_struct*) _temp3)->f3; _LL74: _temp73= _temp69.f1; goto
_LL72; _LL72: _temp71= _temp69.f2; goto _LL68; _LL68: _temp67=(( struct Cyc_Absyn_For_s_struct*)
_temp3)->f4; goto _LL18;} else{ goto _LL19;} _LL19: if(( unsigned int) _temp3 > 
1u?*(( int*) _temp3) ==  Cyc_Absyn_Do_s: 0){ _LL90: _temp89=(( struct Cyc_Absyn_Do_s_struct*)
_temp3)->f1; goto _LL84; _LL84: _temp83=(( struct Cyc_Absyn_Do_s_struct*) _temp3)->f2;
_LL88: _temp87= _temp83.f1; goto _LL86; _LL86: _temp85= _temp83.f2; goto _LL20;}
else{ goto _LL21;} _LL21: if(( unsigned int) _temp3 >  1u?*(( int*) _temp3) == 
Cyc_Absyn_Break_s: 0){ _LL92: _temp91=(( struct Cyc_Absyn_Break_s_struct*)
_temp3)->f1; _temp93=( struct Cyc_Absyn_Stmt**)&(( struct Cyc_Absyn_Break_s_struct*)
_temp3)->f1; goto _LL22;} else{ goto _LL23;} _LL23: if(( unsigned int) _temp3 > 
1u?*(( int*) _temp3) ==  Cyc_Absyn_Continue_s: 0){ _LL95: _temp94=(( struct Cyc_Absyn_Continue_s_struct*)
_temp3)->f1; _temp96=( struct Cyc_Absyn_Stmt**)&(( struct Cyc_Absyn_Continue_s_struct*)
_temp3)->f1; goto _LL24;} else{ goto _LL25;} _LL25: if(( unsigned int) _temp3 > 
1u?*(( int*) _temp3) ==  Cyc_Absyn_Goto_s: 0){ _LL101: _temp100=(( struct Cyc_Absyn_Goto_s_struct*)
_temp3)->f1; goto _LL98; _LL98: _temp97=(( struct Cyc_Absyn_Goto_s_struct*)
_temp3)->f2; _temp99=( struct Cyc_Absyn_Stmt**)&(( struct Cyc_Absyn_Goto_s_struct*)
_temp3)->f2; goto _LL26;} else{ goto _LL27;} _LL27: if(( unsigned int) _temp3 > 
1u?*(( int*) _temp3) ==  Cyc_Absyn_Fallthru_s: 0){ _LL106: _temp105=(( struct
Cyc_Absyn_Fallthru_s_struct*) _temp3)->f1; goto _LL103; _LL103: _temp102=((
struct Cyc_Absyn_Fallthru_s_struct*) _temp3)->f2; _temp104=( struct Cyc_Absyn_Switch_clause***)&((
struct Cyc_Absyn_Fallthru_s_struct*) _temp3)->f2; goto _LL28;} else{ goto _LL29;}
_LL29: if(( unsigned int) _temp3 >  1u?*(( int*) _temp3) ==  Cyc_Absyn_Label_s:
0){ _LL110: _temp109=(( struct Cyc_Absyn_Label_s_struct*) _temp3)->f1; goto
_LL108; _LL108: _temp107=(( struct Cyc_Absyn_Label_s_struct*) _temp3)->f2; goto
_LL30;} else{ goto _LL31;} _LL31: if(( unsigned int) _temp3 >  1u?*(( int*)
_temp3) ==  Cyc_Absyn_Switch_s: 0){ _LL114: _temp113=(( struct Cyc_Absyn_Switch_s_struct*)
_temp3)->f1; goto _LL112; _LL112: _temp111=(( struct Cyc_Absyn_Switch_s_struct*)
_temp3)->f2; goto _LL32;} else{ goto _LL33;} _LL33: if(( unsigned int) _temp3 > 
1u?*(( int*) _temp3) ==  Cyc_Absyn_SwitchC_s: 0){ _LL118: _temp117=(( struct Cyc_Absyn_SwitchC_s_struct*)
_temp3)->f1; goto _LL116; _LL116: _temp115=(( struct Cyc_Absyn_SwitchC_s_struct*)
_temp3)->f2; goto _LL34;} else{ goto _LL35;} _LL35: if(( unsigned int) _temp3 > 
1u?*(( int*) _temp3) ==  Cyc_Absyn_TryCatch_s: 0){ _LL122: _temp121=(( struct
Cyc_Absyn_TryCatch_s_struct*) _temp3)->f1; goto _LL120; _LL120: _temp119=((
struct Cyc_Absyn_TryCatch_s_struct*) _temp3)->f2; goto _LL36;} else{ goto _LL37;}
_LL37: if(( unsigned int) _temp3 >  1u?*(( int*) _temp3) ==  Cyc_Absyn_Decl_s: 0){
_LL126: _temp125=(( struct Cyc_Absyn_Decl_s_struct*) _temp3)->f1; goto _LL124;
_LL124: _temp123=(( struct Cyc_Absyn_Decl_s_struct*) _temp3)->f2; goto _LL38;}
else{ goto _LL39;} _LL39: if(( unsigned int) _temp3 >  1u?*(( int*) _temp3) == 
Cyc_Absyn_Region_s: 0){ _LL132: _temp131=(( struct Cyc_Absyn_Region_s_struct*)
_temp3)->f1; goto _LL130; _LL130: _temp129=(( struct Cyc_Absyn_Region_s_struct*)
_temp3)->f2; goto _LL128; _LL128: _temp127=(( struct Cyc_Absyn_Region_s_struct*)
_temp3)->f3; goto _LL40;} else{ goto _LL41;} _LL41: if(( unsigned int) _temp3 > 
1u?*(( int*) _temp3) ==  Cyc_Absyn_Cut_s: 0){ _LL134: _temp133=(( struct Cyc_Absyn_Cut_s_struct*)
_temp3)->f1; goto _LL42;} else{ goto _LL43;} _LL43: if(( unsigned int) _temp3 > 
1u?*(( int*) _temp3) ==  Cyc_Absyn_Splice_s: 0){ _LL136: _temp135=(( struct Cyc_Absyn_Splice_s_struct*)
_temp3)->f1; goto _LL44;} else{ goto _LL4;} _LL6: return; _LL8: Cyc_Tcexp_tcExp(
te, 0, _temp45); return; _LL10: { struct Cyc_Tcenv_Tenv* _temp137= Cyc_Tcenv_set_next(
te,( void*)({ struct Cyc_Tcenv_Stmt_j_struct* _temp150=( struct Cyc_Tcenv_Stmt_j_struct*)
_cycalloc( sizeof( struct Cyc_Tcenv_Stmt_j_struct)); _temp150[ 0]=({ struct Cyc_Tcenv_Stmt_j_struct
_temp151; _temp151.tag= Cyc_Tcenv_Stmt_j; _temp151.f1= _temp47; _temp151;});
_temp150;})); Cyc_Tcstmt_tcStmt( _temp137, _temp49, 1); while( 1) { void*
_temp138=( void*) _temp47->r; struct Cyc_Absyn_Stmt* _temp144; struct Cyc_Absyn_Stmt*
_temp146; _LL140: if(( unsigned int) _temp138 >  1u?*(( int*) _temp138) ==  Cyc_Absyn_Seq_s:
0){ _LL147: _temp146=(( struct Cyc_Absyn_Seq_s_struct*) _temp138)->f1; goto
_LL145; _LL145: _temp144=(( struct Cyc_Absyn_Seq_s_struct*) _temp138)->f2; goto
_LL141;} else{ goto _LL142;} _LL142: goto _LL143; _LL141: Cyc_Tcstmt_decorate_stmt(
te, _temp47); _temp137= Cyc_Tcenv_set_next( te,( void*)({ struct Cyc_Tcenv_Stmt_j_struct*
_temp148=( struct Cyc_Tcenv_Stmt_j_struct*) _cycalloc( sizeof( struct Cyc_Tcenv_Stmt_j_struct));
_temp148[ 0]=({ struct Cyc_Tcenv_Stmt_j_struct _temp149; _temp149.tag= Cyc_Tcenv_Stmt_j;
_temp149.f1= _temp144; _temp149;}); _temp148;})); Cyc_Tcstmt_tcStmt( _temp137,
_temp146, 1); _temp47= _temp144; continue; _LL143: goto seq_end; _LL139:;}
seq_end: Cyc_Tcstmt_tcStmt( te, _temp47, 1); return;} _LL12: { void* t= Cyc_Tcenv_return_typ(
te); if( _temp51 ==  0){ void* _temp152= Cyc_Tcutil_compress( t); _LL154: if(
_temp152 == ( void*) Cyc_Absyn_VoidType){ goto _LL155;} else{ goto _LL156;}
_LL156: goto _LL157; _LL155: goto _LL153; _LL157:({ struct Cyc_Std_String_pa_struct
_temp159; _temp159.tag= Cyc_Std_String_pa; _temp159.f1=( struct _tagged_arr) Cyc_Absynpp_typ2string(
t);{ void* _temp158[ 1u]={& _temp159}; Cyc_Tcutil_terr( s0->loc, _tag_arr("must return a value of type %s",
sizeof( unsigned char), 31u), _tag_arr( _temp158, sizeof( void*), 1u));}}); goto
_LL153; _LL153:;} else{ struct Cyc_Absyn_Exp* e=( struct Cyc_Absyn_Exp*)
_check_null( _temp51); Cyc_Tcexp_tcExp( te,( void**)& t, e); if( ! Cyc_Tcutil_coerce_arg(
te, e, t)){({ struct Cyc_Std_String_pa_struct _temp162; _temp162.tag= Cyc_Std_String_pa;
_temp162.f1=( struct _tagged_arr) Cyc_Absynpp_typ2string( t);{ struct Cyc_Std_String_pa_struct
_temp161; _temp161.tag= Cyc_Std_String_pa; _temp161.f1=( struct _tagged_arr) Cyc_Absynpp_typ2string((
void*)(( struct Cyc_Core_Opt*) _check_null( e->topt))->v);{ void* _temp160[ 2u]={&
_temp161,& _temp162}; Cyc_Tcutil_terr( s0->loc, _tag_arr("returns value of type %s but requires %s",
sizeof( unsigned char), 41u), _tag_arr( _temp160, sizeof( void*), 2u));}}}); Cyc_Tcutil_explain_failure();}}
return;} _LL14: Cyc_Tcexp_tcTest( te, _temp57, _tag_arr("if statement", sizeof(
unsigned char), 13u)); Cyc_Tcstmt_tcStmt( te, _temp55, 1); Cyc_Tcstmt_tcStmt( te,
_temp53, 1); return; _LL16: Cyc_Tcstmt_decorate_stmt( te, _temp63); Cyc_Tcexp_tcTest(
te, _temp65, _tag_arr("while loop", sizeof( unsigned char), 11u)); Cyc_Tcstmt_tcStmt(
Cyc_Tcenv_set_in_loop( te, _temp63), _temp59, 1); return; _LL18: Cyc_Tcstmt_decorate_stmt(
te, _temp77); Cyc_Tcstmt_decorate_stmt( te, _temp71); Cyc_Tcexp_tcExp( te, 0,
_temp81); Cyc_Tcexp_tcTest( te, _temp79, _tag_arr("for loop", sizeof(
unsigned char), 9u)); te= Cyc_Tcenv_set_in_loop( te, _temp71); Cyc_Tcstmt_tcStmt(
te, _temp67, 1); Cyc_Tcexp_tcExp( te, 0, _temp73); return; _LL20: Cyc_Tcstmt_decorate_stmt(
te, _temp85); Cyc_Tcstmt_tcStmt( Cyc_Tcenv_set_in_loop( te, _temp85), _temp89, 1);
Cyc_Tcexp_tcTest( te, _temp87, _tag_arr("do loop", sizeof( unsigned char), 8u));
return; _LL22: Cyc_Tcenv_process_break( te, s0, _temp93); return; _LL24: Cyc_Tcenv_process_continue(
te, s0, _temp96); return; _LL26: Cyc_Tcenv_process_goto( te, s0, _temp100,
_temp99); return; _LL28: { struct _tuple3* _temp163= Cyc_Tcenv_process_fallthru(
te, s0, _temp104); if( _temp163 ==  0){({ void* _temp164[ 0u]={}; Cyc_Tcutil_terr(
s0->loc, _tag_arr("fallthru not in a non-last case", sizeof( unsigned char), 32u),
_tag_arr( _temp164, sizeof( void*), 0u));}); return;}{ struct _tuple3 _temp167;
struct Cyc_List_List* _temp168; struct Cyc_List_List* _temp170; struct _tuple3*
_temp165=( struct _tuple3*) _check_null( _temp163); _temp167=* _temp165; _LL171:
_temp170= _temp167.f2; goto _LL169; _LL169: _temp168= _temp167.f3; goto _LL166;
_LL166: { struct Cyc_List_List* instantiation=(( struct Cyc_List_List*(*)(
struct _tuple4*(* f)( struct Cyc_List_List*, struct Cyc_Absyn_Tvar*), struct Cyc_List_List*
env, struct Cyc_List_List* x)) Cyc_List_map_c)( Cyc_Tcutil_make_inst_var, Cyc_Tcenv_lookup_type_vars(
te), _temp170); _temp168=(( struct Cyc_List_List*(*)( void*(* f)( struct Cyc_List_List*,
void*), struct Cyc_List_List* env, struct Cyc_List_List* x)) Cyc_List_map_c)(
Cyc_Tcutil_substitute, instantiation, _temp168); for( 0; _temp168 !=  0?
_temp105 !=  0: 0;( _temp168= _temp168->tl, _temp105= _temp105->tl)){ Cyc_Tcexp_tcExp(
te, 0,( struct Cyc_Absyn_Exp*) _temp105->hd); if( ! Cyc_Tcutil_coerce_arg( te,(
struct Cyc_Absyn_Exp*) _temp105->hd,( void*) _temp168->hd)){({ struct Cyc_Std_String_pa_struct
_temp174; _temp174.tag= Cyc_Std_String_pa; _temp174.f1=( struct _tagged_arr) Cyc_Absynpp_typ2string((
void*) _temp168->hd);{ struct Cyc_Std_String_pa_struct _temp173; _temp173.tag=
Cyc_Std_String_pa; _temp173.f1=( struct _tagged_arr) Cyc_Absynpp_typ2string((
void*)(( struct Cyc_Core_Opt*) _check_null((( struct Cyc_Absyn_Exp*) _temp105->hd)->topt))->v);{
void* _temp172[ 2u]={& _temp173,& _temp174}; Cyc_Tcutil_terr( s0->loc, _tag_arr("fallthru argument has type %s but pattern variable has type %s",
sizeof( unsigned char), 63u), _tag_arr( _temp172, sizeof( void*), 2u));}}}); Cyc_Tcutil_explain_failure();}}
if( _temp105 !=  0){({ void* _temp175[ 0u]={}; Cyc_Tcutil_terr( s0->loc,
_tag_arr("too many arguments to explicit fallthru", sizeof( unsigned char), 40u),
_tag_arr( _temp175, sizeof( void*), 0u));});} if( _temp168 !=  0){({ void*
_temp176[ 0u]={}; Cyc_Tcutil_terr( s0->loc, _tag_arr("too few arguments to explicit fallthru",
sizeof( unsigned char), 39u), _tag_arr( _temp176, sizeof( void*), 0u));});}
return;}}} _LL30: Cyc_Tcstmt_tcStmt( Cyc_Tcenv_new_named_block( s0->loc, Cyc_Tcenv_add_label(
te, _temp109, _temp107),({ struct Cyc_Absyn_Tvar* _temp177=( struct Cyc_Absyn_Tvar*)
_cycalloc( sizeof( struct Cyc_Absyn_Tvar)); _temp177->name=({ struct _tagged_arr*
_temp178=( struct _tagged_arr*) _cycalloc( sizeof( struct _tagged_arr));
_temp178[ 0]=( struct _tagged_arr)({ struct Cyc_Std_String_pa_struct _temp180;
_temp180.tag= Cyc_Std_String_pa; _temp180.f1=( struct _tagged_arr)* _temp109;{
void* _temp179[ 1u]={& _temp180}; Cyc_Std_aprintf( _tag_arr("`%s", sizeof(
unsigned char), 4u), _tag_arr( _temp179, sizeof( void*), 1u));}}); _temp178;});
_temp177->identity= Cyc_Tcutil_new_tvar_id(); _temp177->kind= Cyc_Absyn_new_conref((
void*) Cyc_Absyn_RgnKind); _temp177;})), _temp107, 0); return; _LL32: Cyc_Tcexp_tcExp(
te, 0, _temp113);{ void* _temp181=( void*)(( struct Cyc_Core_Opt*) _check_null(
_temp113->topt))->v; te= Cyc_Tcenv_set_in_switch( te); te= Cyc_Tcenv_clear_fallthru(
te);{ struct Cyc_List_List* scs=(( struct Cyc_List_List*(*)( struct Cyc_List_List*
x)) Cyc_List_rev)( _temp111); for( 0; scs !=  0; scs= scs->tl){ struct Cyc_Absyn_Pat*
_temp182=(( struct Cyc_Absyn_Switch_clause*) scs->hd)->pattern; struct Cyc_List_List*
_temp185; struct Cyc_List_List* _temp187; struct _tuple5 _temp183= Cyc_Tcpat_tcPat(
te, _temp182,( void**)& _temp181, 0); _LL188: _temp187= _temp183.f1; goto _LL186;
_LL186: _temp185= _temp183.f2; goto _LL184; _LL184: if( ! Cyc_Tcutil_unify((
void*)(( struct Cyc_Core_Opt*) _check_null( _temp182->topt))->v, _temp181)){({
struct Cyc_Std_String_pa_struct _temp191; _temp191.tag= Cyc_Std_String_pa;
_temp191.f1=( struct _tagged_arr) Cyc_Absynpp_typ2string(( void*)(( struct Cyc_Core_Opt*)
_check_null( _temp182->topt))->v);{ struct Cyc_Std_String_pa_struct _temp190;
_temp190.tag= Cyc_Std_String_pa; _temp190.f1=( struct _tagged_arr) Cyc_Absynpp_typ2string(
_temp181);{ void* _temp189[ 2u]={& _temp190,& _temp191}; Cyc_Tcutil_terr(((
struct Cyc_Absyn_Switch_clause*) scs->hd)->loc, _tag_arr("switch on type %s, but case expects type %s",
sizeof( unsigned char), 44u), _tag_arr( _temp189, sizeof( void*), 2u));}}}); Cyc_Tcutil_explain_failure();}
else{ Cyc_Tcpat_check_pat_regions( te, _temp182);}(( struct Cyc_Absyn_Switch_clause*)
scs->hd)->pat_vars=({ struct Cyc_Core_Opt* _temp192=( struct Cyc_Core_Opt*)
_cycalloc( sizeof( struct Cyc_Core_Opt)); _temp192->v=( void*) _temp185;
_temp192;}); Cyc_Tcstmt_pattern_synth((( struct Cyc_Absyn_Switch_clause*) scs->hd)->loc,
te, _temp187, _temp185,(( struct Cyc_Absyn_Switch_clause*) scs->hd)->body,((
struct Cyc_Absyn_Switch_clause*) scs->hd)->where_clause, 1); te= Cyc_Tcenv_set_fallthru(
te, _temp187, _temp185,( struct Cyc_Absyn_Switch_clause*) scs->hd);}} Cyc_Tcpat_check_switch_exhaustive(
s0->loc, _temp111); return;} _LL34: { struct Cyc_List_List* _temp193= 0; for( 0;
_temp115 !=  0; _temp115= _temp115->tl){ struct Cyc_Absyn_SwitchC_clause
_temp196; struct Cyc_Position_Segment* _temp197; struct Cyc_Absyn_Stmt* _temp199;
struct Cyc_Absyn_Exp* _temp201; struct Cyc_Absyn_SwitchC_clause* _temp194=(
struct Cyc_Absyn_SwitchC_clause*) _temp115->hd; _temp196=* _temp194; _LL202:
_temp201= _temp196.cnst_exp; goto _LL200; _LL200: _temp199= _temp196.body; goto
_LL198; _LL198: _temp197= _temp196.loc; goto _LL195; _LL195: { struct Cyc_Absyn_Pat*
pat; if( _temp201 !=  0){ Cyc_Tcexp_tcExp( te, 0,( struct Cyc_Absyn_Exp*)
_check_null( _temp201)); pat= Cyc_Absyn_new_pat(( void*)({ struct Cyc_Absyn_Int_p_struct*
_temp203=( struct Cyc_Absyn_Int_p_struct*) _cycalloc( sizeof( struct Cyc_Absyn_Int_p_struct));
_temp203[ 0]=({ struct Cyc_Absyn_Int_p_struct _temp204; _temp204.tag= Cyc_Absyn_Int_p;
_temp204.f1=( void*)(( void*) Cyc_Absyn_Signed); _temp204.f2=( int) Cyc_Evexp_eval_const_uint_exp((
struct Cyc_Absyn_Exp*) _check_null( _temp201)); _temp204;}); _temp203;}),
_temp201->loc);} else{ pat= Cyc_Absyn_new_pat(( void*) Cyc_Absyn_Wild_p,
_temp197);} _temp193=({ struct Cyc_List_List* _temp205=( struct Cyc_List_List*)
_cycalloc( sizeof( struct Cyc_List_List)); _temp205->hd=( void*)({ struct Cyc_Absyn_Switch_clause*
_temp206=( struct Cyc_Absyn_Switch_clause*) _cycalloc( sizeof( struct Cyc_Absyn_Switch_clause));
_temp206->pattern= pat; _temp206->pat_vars= 0; _temp206->where_clause= 0;
_temp206->body= _temp199; _temp206->loc= _temp197; _temp206;}); _temp205->tl=
_temp193; _temp205;});}}( void*)( s0->r=( void*)(( void*)( Cyc_Absyn_switch_stmt(
_temp117,(( struct Cyc_List_List*(*)( struct Cyc_List_List* x)) Cyc_List_imp_rev)(
_temp193), 0))->r)); Cyc_Tcstmt_tcStmt( te, s0, new_block); return;} _LL36:(
void*)( _temp121->r=( void*)(( void*)( Cyc_Absyn_seq_stmt( Cyc_Absyn_new_stmt((
void*) _temp121->r, _temp121->loc), Cyc_Absyn_skip_stmt( _temp121->loc),
_temp121->loc))->r)); Cyc_Tcstmt_tcStmt( Cyc_Tcenv_set_encloser( Cyc_Tcenv_enter_try(
te), s0), _temp121, 1); te= Cyc_Tcenv_set_in_switch( te); te= Cyc_Tcenv_clear_fallthru(
te);{ struct Cyc_List_List* _temp207=(( struct Cyc_List_List*(*)( struct Cyc_List_List*
x)) Cyc_List_rev)( _temp119); for( 0; _temp207 !=  0; _temp207= _temp207->tl){
struct Cyc_Absyn_Pat* _temp208=(( struct Cyc_Absyn_Switch_clause*) _temp207->hd)->pattern;
struct Cyc_List_List* _temp211; struct Cyc_List_List* _temp213; struct _tuple5
_temp209= Cyc_Tcpat_tcPat( te, _temp208,( void**)& Cyc_Absyn_exn_typ, 0); _LL214:
_temp213= _temp209.f1; goto _LL212; _LL212: _temp211= _temp209.f2; goto _LL210;
_LL210: if( ! Cyc_Tcutil_unify(( void*)(( struct Cyc_Core_Opt*) _check_null(
_temp208->topt))->v, Cyc_Absyn_exn_typ)){({ struct Cyc_Std_String_pa_struct
_temp216; _temp216.tag= Cyc_Std_String_pa; _temp216.f1=( struct _tagged_arr) Cyc_Absynpp_typ2string((
void*)(( struct Cyc_Core_Opt*) _check_null( _temp208->topt))->v);{ void*
_temp215[ 1u]={& _temp216}; Cyc_Tcutil_terr((( struct Cyc_Absyn_Switch_clause*)
_temp207->hd)->loc, _tag_arr("expected xtunion exn but found %s", sizeof(
unsigned char), 34u), _tag_arr( _temp215, sizeof( void*), 1u));}});} else{ Cyc_Tcpat_check_pat_regions(
te, _temp208);}(( struct Cyc_Absyn_Switch_clause*) _temp207->hd)->pat_vars=({
struct Cyc_Core_Opt* _temp217=( struct Cyc_Core_Opt*) _cycalloc( sizeof( struct
Cyc_Core_Opt)); _temp217->v=( void*) _temp211; _temp217;}); Cyc_Tcstmt_pattern_synth(((
struct Cyc_Absyn_Switch_clause*) _temp207->hd)->loc, te, _temp213, _temp211,((
struct Cyc_Absyn_Switch_clause*) _temp207->hd)->body,(( struct Cyc_Absyn_Switch_clause*)
_temp207->hd)->where_clause, 1); te= Cyc_Tcenv_set_fallthru( te, _temp213,
_temp211,( struct Cyc_Absyn_Switch_clause*) _temp207->hd);}} Cyc_Tcpat_check_catch_overlap(
s0->loc, _temp119); return; _LL38: { struct _tagged_arr unimp_msg_part; if(
new_block){ te= Cyc_Tcenv_new_block( s0->loc, te);}{ void* _temp218=( void*)
_temp125->r; struct Cyc_Absyn_Vardecl* _temp244; int _temp246; int* _temp248;
struct Cyc_Absyn_Exp* _temp249; struct Cyc_Core_Opt* _temp251; struct Cyc_Core_Opt**
_temp253; struct Cyc_Core_Opt* _temp254; struct Cyc_Core_Opt** _temp256; struct
Cyc_Absyn_Pat* _temp257; struct Cyc_List_List* _temp259; struct Cyc_List_List*
_temp261; struct _tagged_arr* _temp263; struct Cyc_List_List* _temp265; struct
_tuple0* _temp267; _LL220: if(*(( int*) _temp218) ==  Cyc_Absyn_Var_d){ _LL245:
_temp244=(( struct Cyc_Absyn_Var_d_struct*) _temp218)->f1; goto _LL221;} else{
goto _LL222;} _LL222: if(*(( int*) _temp218) ==  Cyc_Absyn_Let_d){ _LL258:
_temp257=(( struct Cyc_Absyn_Let_d_struct*) _temp218)->f1; goto _LL255; _LL255:
_temp254=(( struct Cyc_Absyn_Let_d_struct*) _temp218)->f2; _temp256=( struct Cyc_Core_Opt**)&((
struct Cyc_Absyn_Let_d_struct*) _temp218)->f2; goto _LL252; _LL252: _temp251=((
struct Cyc_Absyn_Let_d_struct*) _temp218)->f3; _temp253=( struct Cyc_Core_Opt**)&((
struct Cyc_Absyn_Let_d_struct*) _temp218)->f3; goto _LL250; _LL250: _temp249=((
struct Cyc_Absyn_Let_d_struct*) _temp218)->f4; goto _LL247; _LL247: _temp246=((
struct Cyc_Absyn_Let_d_struct*) _temp218)->f5; _temp248=( int*)&(( struct Cyc_Absyn_Let_d_struct*)
_temp218)->f5; goto _LL223;} else{ goto _LL224;} _LL224: if(*(( int*) _temp218)
==  Cyc_Absyn_Letv_d){ _LL260: _temp259=(( struct Cyc_Absyn_Letv_d_struct*)
_temp218)->f1; goto _LL225;} else{ goto _LL226;} _LL226: if(*(( int*) _temp218)
==  Cyc_Absyn_Namespace_d){ _LL264: _temp263=(( struct Cyc_Absyn_Namespace_d_struct*)
_temp218)->f1; goto _LL262; _LL262: _temp261=(( struct Cyc_Absyn_Namespace_d_struct*)
_temp218)->f2; goto _LL227;} else{ goto _LL228;} _LL228: if(*(( int*) _temp218)
==  Cyc_Absyn_Using_d){ _LL268: _temp267=(( struct Cyc_Absyn_Using_d_struct*)
_temp218)->f1; goto _LL266; _LL266: _temp265=(( struct Cyc_Absyn_Using_d_struct*)
_temp218)->f2; goto _LL229;} else{ goto _LL230;} _LL230: if(*(( int*) _temp218)
==  Cyc_Absyn_Fn_d){ goto _LL231;} else{ goto _LL232;} _LL232: if(*(( int*)
_temp218) ==  Cyc_Absyn_Struct_d){ goto _LL233;} else{ goto _LL234;} _LL234: if(*((
int*) _temp218) ==  Cyc_Absyn_Union_d){ goto _LL235;} else{ goto _LL236;} _LL236:
if(*(( int*) _temp218) ==  Cyc_Absyn_Tunion_d){ goto _LL237;} else{ goto _LL238;}
_LL238: if(*(( int*) _temp218) ==  Cyc_Absyn_Enum_d){ goto _LL239;} else{ goto
_LL240;} _LL240: if(*(( int*) _temp218) ==  Cyc_Absyn_Typedef_d){ goto _LL241;}
else{ goto _LL242;} _LL242: if(*(( int*) _temp218) ==  Cyc_Absyn_ExternC_d){
goto _LL243;} else{ goto _LL219;} _LL221: { struct Cyc_Absyn_Vardecl _temp271;
struct Cyc_List_List* _temp272; struct Cyc_Core_Opt* _temp274; struct Cyc_Core_Opt**
_temp276; struct Cyc_Absyn_Exp* _temp277; void* _temp279; struct Cyc_Absyn_Tqual
_temp281; struct _tuple0* _temp283; struct _tuple0 _temp285; struct _tagged_arr*
_temp286; void* _temp288; void* _temp290; struct Cyc_Absyn_Vardecl* _temp269=
_temp244; _temp271=* _temp269; _LL291: _temp290=( void*) _temp271.sc; goto
_LL284; _LL284: _temp283= _temp271.name; _temp285=* _temp283; _LL289: _temp288=
_temp285.f1; goto _LL287; _LL287: _temp286= _temp285.f2; goto _LL282; _LL282:
_temp281= _temp271.tq; goto _LL280; _LL280: _temp279=( void*) _temp271.type;
goto _LL278; _LL278: _temp277= _temp271.initializer; goto _LL275; _LL275:
_temp274= _temp271.rgn; _temp276=( struct Cyc_Core_Opt**)&(* _temp269).rgn; goto
_LL273; _LL273: _temp272= _temp271.attributes; goto _LL270; _LL270: { void*
_temp292= Cyc_Tcenv_curr_rgn( te); int is_local;{ void* _temp293= _temp290;
_LL295: if( _temp293 == ( void*) Cyc_Absyn_Static){ goto _LL296;} else{ goto
_LL297;} _LL297: if( _temp293 == ( void*) Cyc_Absyn_Extern){ goto _LL298;} else{
goto _LL299;} _LL299: if( _temp293 == ( void*) Cyc_Absyn_ExternC){ goto _LL300;}
else{ goto _LL301;} _LL301: if( _temp293 == ( void*) Cyc_Absyn_Abstract){ goto
_LL302;} else{ goto _LL303;} _LL303: if( _temp293 == ( void*) Cyc_Absyn_Public){
goto _LL304;} else{ goto _LL294;} _LL296: goto _LL298; _LL298: goto _LL300;
_LL300: is_local= 0; goto _LL294; _LL302:({ void* _temp305[ 0u]={}; Cyc_Tcutil_terr(
_temp125->loc, _tag_arr("bad abstract scope for local variable", sizeof(
unsigned char), 38u), _tag_arr( _temp305, sizeof( void*), 0u));}); goto _LL304;
_LL304: is_local= 1; goto _LL294; _LL294:;}* _temp276= is_local?({ struct Cyc_Core_Opt*
_temp306=( struct Cyc_Core_Opt*) _cycalloc( sizeof( struct Cyc_Core_Opt));
_temp306->v=( void*) _temp292; _temp306;}):({ struct Cyc_Core_Opt* _temp307=(
struct Cyc_Core_Opt*) _cycalloc( sizeof( struct Cyc_Core_Opt)); _temp307->v=(
void*)(( void*) Cyc_Absyn_HeapRgn); _temp307;});{ void* _temp308= _temp288;
struct Cyc_List_List* _temp318; struct Cyc_List_List* _temp320; _LL310: if(
_temp308 == ( void*) Cyc_Absyn_Loc_n){ goto _LL311;} else{ goto _LL312;} _LL312:
if(( unsigned int) _temp308 >  1u?*(( int*) _temp308) ==  Cyc_Absyn_Rel_n: 0){
_LL319: _temp318=(( struct Cyc_Absyn_Rel_n_struct*) _temp308)->f1; if( _temp318
==  0){ goto _LL313;} else{ goto _LL314;}} else{ goto _LL314;} _LL314: if((
unsigned int) _temp308 >  1u?*(( int*) _temp308) ==  Cyc_Absyn_Abs_n: 0){ _LL321:
_temp320=(( struct Cyc_Absyn_Abs_n_struct*) _temp308)->f1; goto _LL315;} else{
goto _LL316;} _LL316: goto _LL317; _LL311: goto _LL309; _LL313:(* _temp244->name).f1=(
void*) Cyc_Absyn_Loc_n; goto _LL309; _LL315:( int) _throw(({ void* _temp322[ 0u]={};
Cyc_Tcutil_impos( _tag_arr("tcstmt: Abs_n declaration", sizeof( unsigned char),
26u), _tag_arr( _temp322, sizeof( void*), 0u));})); _LL317:({ void* _temp323[ 0u]={};
Cyc_Tcutil_terr( _temp125->loc, _tag_arr("cannot declare a qualified local variable",
sizeof( unsigned char), 42u), _tag_arr( _temp323, sizeof( void*), 0u));}); goto
_LL309; _LL309:;}{ void* _temp324= Cyc_Tcutil_compress( _temp279); struct Cyc_Absyn_Exp*
_temp331; struct Cyc_Absyn_Tqual _temp333; void* _temp335; _LL326: if((
unsigned int) _temp324 >  3u?*(( int*) _temp324) ==  Cyc_Absyn_ArrayType: 0){
_LL336: _temp335=( void*)(( struct Cyc_Absyn_ArrayType_struct*) _temp324)->f1;
goto _LL334; _LL334: _temp333=(( struct Cyc_Absyn_ArrayType_struct*) _temp324)->f2;
goto _LL332; _LL332: _temp331=(( struct Cyc_Absyn_ArrayType_struct*) _temp324)->f3;
if( _temp331 ==  0){ goto _LL330;} else{ goto _LL328;}} else{ goto _LL328;}
_LL330: if( _temp244->initializer !=  0){ goto _LL327;} else{ goto _LL328;}
_LL328: goto _LL329; _LL327:{ void* _temp337=( void*)(( struct Cyc_Absyn_Exp*)
_check_null( _temp244->initializer))->r; void* _temp349; struct _tagged_arr
_temp351; struct Cyc_Absyn_Exp* _temp353; struct Cyc_List_List* _temp355; struct
Cyc_List_List* _temp357; _LL339: if(*(( int*) _temp337) ==  Cyc_Absyn_Const_e){
_LL350: _temp349=( void*)(( struct Cyc_Absyn_Const_e_struct*) _temp337)->f1; if((
unsigned int) _temp349 >  1u?*(( int*) _temp349) ==  Cyc_Absyn_String_c: 0){
_LL352: _temp351=(( struct Cyc_Absyn_String_c_struct*) _temp349)->f1; goto
_LL340;} else{ goto _LL341;}} else{ goto _LL341;} _LL341: if(*(( int*) _temp337)
==  Cyc_Absyn_Comprehension_e){ _LL354: _temp353=(( struct Cyc_Absyn_Comprehension_e_struct*)
_temp337)->f2; goto _LL342;} else{ goto _LL343;} _LL343: if(*(( int*) _temp337)
==  Cyc_Absyn_UnresolvedMem_e){ _LL356: _temp355=(( struct Cyc_Absyn_UnresolvedMem_e_struct*)
_temp337)->f2; goto _LL344;} else{ goto _LL345;} _LL345: if(*(( int*) _temp337)
==  Cyc_Absyn_Array_e){ _LL358: _temp357=(( struct Cyc_Absyn_Array_e_struct*)
_temp337)->f1; goto _LL346;} else{ goto _LL347;} _LL347: goto _LL348; _LL340:
_temp279=( void*)( _temp244->type=( void*)(( void*)({ struct Cyc_Absyn_ArrayType_struct*
_temp359=( struct Cyc_Absyn_ArrayType_struct*) _cycalloc( sizeof( struct Cyc_Absyn_ArrayType_struct));
_temp359[ 0]=({ struct Cyc_Absyn_ArrayType_struct _temp360; _temp360.tag= Cyc_Absyn_ArrayType;
_temp360.f1=( void*) _temp335; _temp360.f2= _temp333; _temp360.f3=( struct Cyc_Absyn_Exp*)
Cyc_Absyn_uint_exp( _get_arr_size( _temp351, sizeof( unsigned char)), 0);
_temp360;}); _temp359;}))); goto _LL338; _LL342: _temp279=( void*)( _temp244->type=(
void*)(( void*)({ struct Cyc_Absyn_ArrayType_struct* _temp361=( struct Cyc_Absyn_ArrayType_struct*)
_cycalloc( sizeof( struct Cyc_Absyn_ArrayType_struct)); _temp361[ 0]=({ struct
Cyc_Absyn_ArrayType_struct _temp362; _temp362.tag= Cyc_Absyn_ArrayType; _temp362.f1=(
void*) _temp335; _temp362.f2= _temp333; _temp362.f3=( struct Cyc_Absyn_Exp*)
_temp353; _temp362;}); _temp361;}))); goto _LL338; _LL344: _temp357= _temp355;
goto _LL346; _LL346: _temp279=( void*)( _temp244->type=( void*)(( void*)({
struct Cyc_Absyn_ArrayType_struct* _temp363=( struct Cyc_Absyn_ArrayType_struct*)
_cycalloc( sizeof( struct Cyc_Absyn_ArrayType_struct)); _temp363[ 0]=({ struct
Cyc_Absyn_ArrayType_struct _temp364; _temp364.tag= Cyc_Absyn_ArrayType; _temp364.f1=(
void*) _temp335; _temp364.f2= _temp333; _temp364.f3=( struct Cyc_Absyn_Exp*) Cyc_Absyn_uint_exp((
unsigned int)(( int(*)( struct Cyc_List_List* x)) Cyc_List_length)( _temp357), 0);
_temp364;}); _temp363;}))); goto _LL338; _LL348: goto _LL338; _LL338:;} goto
_LL325; _LL329: goto _LL325; _LL325:;}{ struct Cyc_List_List* _temp365= !
is_local? 0: Cyc_Tcenv_lookup_type_vars( te); int _temp366= ! is_local? 0: 1;
Cyc_Tcutil_check_type( s0->loc, te, _temp365,( void*) Cyc_Absyn_MemKind,
_temp366, _temp279);{ struct Cyc_Tcenv_Tenv* _temp367= Cyc_Tcenv_add_local_var(
_temp123->loc, te, _temp244); if( _temp290 == ( void*) Cyc_Absyn_Extern? 1:
_temp290 == ( void*) Cyc_Absyn_ExternC){({ void* _temp368[ 0u]={}; Cyc_Tcutil_terr(
_temp125->loc, _tag_arr("local extern declarations not yet supported", sizeof(
unsigned char), 44u), _tag_arr( _temp368, sizeof( void*), 0u));});} if( _temp277
!=  0){ Cyc_Tcexp_tcExpInitializer( _temp367,( void**)& _temp279,( struct Cyc_Absyn_Exp*)
_check_null( _temp277)); if( ! is_local? ! Cyc_Tcutil_is_const_exp( te,( struct
Cyc_Absyn_Exp*) _check_null( _temp277)): 0){({ void* _temp369[ 0u]={}; Cyc_Tcutil_terr(
_temp125->loc, _tag_arr("initializer needs to be a constant expression", sizeof(
unsigned char), 46u), _tag_arr( _temp369, sizeof( void*), 0u));});} if( ! Cyc_Tcutil_coerce_assign(
_temp367,( struct Cyc_Absyn_Exp*) _check_null( _temp277), _temp279)){({ struct
Cyc_Std_String_pa_struct _temp373; _temp373.tag= Cyc_Std_String_pa; _temp373.f1=(
struct _tagged_arr) Cyc_Absynpp_typ2string(( void*)(( struct Cyc_Core_Opt*)
_check_null( _temp277->topt))->v);{ struct Cyc_Std_String_pa_struct _temp372;
_temp372.tag= Cyc_Std_String_pa; _temp372.f1=( struct _tagged_arr) Cyc_Absynpp_typ2string(
_temp279);{ struct Cyc_Std_String_pa_struct _temp371; _temp371.tag= Cyc_Std_String_pa;
_temp371.f1=( struct _tagged_arr)* _temp286;{ void* _temp370[ 3u]={& _temp371,&
_temp372,& _temp373}; Cyc_Tcutil_terr( _temp125->loc, _tag_arr("%s declared with type \n%s\n but initialized with type \n%s",
sizeof( unsigned char), 57u), _tag_arr( _temp370, sizeof( void*), 3u));}}}});
Cyc_Tcutil_explain_failure();}} Cyc_Tcstmt_tcStmt( Cyc_Tcenv_set_encloser(
_temp367, s0), _temp123, 0); return;}}}} _LL223: { struct Cyc_Core_Opt* _temp374=*
_temp253; Cyc_Tcexp_tcExpInitializer( te, 0, _temp249);* _temp253= _temp249->topt;{
struct Cyc_List_List* _temp377; struct Cyc_List_List* _temp379; struct _tuple5
_temp375= Cyc_Tcpat_tcPat( te, _temp257,( void**)(( void**)(( void*)&(( struct
Cyc_Core_Opt*) _check_null( _temp249->topt))->v)), 0); _LL380: _temp379=
_temp375.f1; goto _LL378; _LL378: _temp377= _temp375.f2; goto _LL376; _LL376:*
_temp256=({ struct Cyc_Core_Opt* _temp381=( struct Cyc_Core_Opt*) _cycalloc(
sizeof( struct Cyc_Core_Opt)); _temp381->v=( void*) _temp377; _temp381;}); if(
_temp374 !=  0? ! Cyc_Tcutil_unify(( void*)(( struct Cyc_Core_Opt*) _check_null(
_temp257->topt))->v,( void*) _temp374->v): 0){({ void* _temp382[ 0u]={}; Cyc_Tcutil_terr(
_temp125->loc, _tag_arr("type of pattern does not match declared type", sizeof(
unsigned char), 45u), _tag_arr( _temp382, sizeof( void*), 0u));});} if( ! Cyc_Tcutil_unify((
void*)(( struct Cyc_Core_Opt*) _check_null( _temp257->topt))->v,( void*)((
struct Cyc_Core_Opt*) _check_null( _temp249->topt))->v)){({ struct Cyc_Std_String_pa_struct
_temp385; _temp385.tag= Cyc_Std_String_pa; _temp385.f1=( struct _tagged_arr) Cyc_Absynpp_typ2string((
void*)(( struct Cyc_Core_Opt*) _check_null( _temp249->topt))->v);{ struct Cyc_Std_String_pa_struct
_temp384; _temp384.tag= Cyc_Std_String_pa; _temp384.f1=( struct _tagged_arr) Cyc_Absynpp_typ2string((
void*)(( struct Cyc_Core_Opt*) _check_null( _temp257->topt))->v);{ void*
_temp383[ 2u]={& _temp384,& _temp385}; Cyc_Tcutil_terr( _temp125->loc, _tag_arr("pattern type %s does not match definition type %s",
sizeof( unsigned char), 50u), _tag_arr( _temp383, sizeof( void*), 2u));}}}); Cyc_Tcutil_explain_failure();}
else{ Cyc_Tcpat_check_pat_regions( te, _temp257);}* _temp248= Cyc_Tcpat_check_let_pat_exhaustive(
_temp257->loc, _temp257); Cyc_Tcstmt_pattern_synth( s0->loc, te, _temp379,
_temp377, _temp123, 0, 0); return;}} _LL225: { void* _temp386= Cyc_Tcenv_curr_rgn(
te); struct Cyc_Tcenv_Tenv* _temp387= te; for( 0; _temp259 !=  0; _temp259=
_temp259->tl){ struct Cyc_Absyn_Vardecl* _temp388=( struct Cyc_Absyn_Vardecl*)
_temp259->hd; struct Cyc_Absyn_Vardecl _temp391; struct Cyc_Core_Opt* _temp392;
struct Cyc_Core_Opt** _temp394; void* _temp395; struct _tuple0* _temp397; struct
_tuple0 _temp399; void* _temp400; struct Cyc_Absyn_Vardecl* _temp389= _temp388;
_temp391=* _temp389; _LL398: _temp397= _temp391.name; _temp399=* _temp397;
_LL401: _temp400= _temp399.f1; goto _LL396; _LL396: _temp395=( void*) _temp391.type;
goto _LL393; _LL393: _temp392= _temp391.rgn; _temp394=( struct Cyc_Core_Opt**)&(*
_temp389).rgn; goto _LL390; _LL390:* _temp394=({ struct Cyc_Core_Opt* _temp402=(
struct Cyc_Core_Opt*) _cycalloc( sizeof( struct Cyc_Core_Opt)); _temp402->v=(
void*) _temp386; _temp402;});{ void* _temp403= _temp400; struct Cyc_List_List*
_temp413; struct Cyc_List_List* _temp415; _LL405: if( _temp403 == ( void*) Cyc_Absyn_Loc_n){
goto _LL406;} else{ goto _LL407;} _LL407: if(( unsigned int) _temp403 >  1u?*((
int*) _temp403) ==  Cyc_Absyn_Rel_n: 0){ _LL414: _temp413=(( struct Cyc_Absyn_Rel_n_struct*)
_temp403)->f1; if( _temp413 ==  0){ goto _LL408;} else{ goto _LL409;}} else{
goto _LL409;} _LL409: if(( unsigned int) _temp403 >  1u?*(( int*) _temp403) == 
Cyc_Absyn_Abs_n: 0){ _LL416: _temp415=(( struct Cyc_Absyn_Abs_n_struct*)
_temp403)->f1; goto _LL410;} else{ goto _LL411;} _LL411: goto _LL412; _LL406:
goto _LL404; _LL408:(* _temp388->name).f1=( void*) Cyc_Absyn_Loc_n; goto _LL404;
_LL410:( int) _throw(({ void* _temp417[ 0u]={}; Cyc_Tcutil_impos( _tag_arr("tcstmt: Abs_n declaration",
sizeof( unsigned char), 26u), _tag_arr( _temp417, sizeof( void*), 0u));}));
_LL412:({ void* _temp418[ 0u]={}; Cyc_Tcutil_terr( _temp125->loc, _tag_arr("cannot declare a qualified local variable",
sizeof( unsigned char), 42u), _tag_arr( _temp418, sizeof( void*), 0u));}); goto
_LL404; _LL404:;} Cyc_Tcutil_check_type( s0->loc, _temp387, Cyc_Tcenv_lookup_type_vars(
_temp387),( void*) Cyc_Absyn_MemKind, 1, _temp395); _temp387= Cyc_Tcenv_add_local_var(
_temp123->loc, _temp387, _temp388);} Cyc_Tcstmt_tcStmt( Cyc_Tcenv_set_encloser(
_temp387, s0), _temp123, 0); return;} _LL227: unimp_msg_part= _tag_arr("namespace",
sizeof( unsigned char), 10u); goto _LL219; _LL229: unimp_msg_part= _tag_arr("using",
sizeof( unsigned char), 6u); goto _LL219; _LL231: unimp_msg_part= _tag_arr("function",
sizeof( unsigned char), 9u); goto _LL219; _LL233: unimp_msg_part= _tag_arr("struct",
sizeof( unsigned char), 7u); goto _LL219; _LL235: unimp_msg_part= _tag_arr("union",
sizeof( unsigned char), 6u); goto _LL219; _LL237: unimp_msg_part= _tag_arr("[x]tunion",
sizeof( unsigned char), 10u); goto _LL219; _LL239: unimp_msg_part= _tag_arr("enum",
sizeof( unsigned char), 5u); goto _LL219; _LL241: unimp_msg_part= _tag_arr("typedef",
sizeof( unsigned char), 8u); goto _LL219; _LL243: unimp_msg_part= _tag_arr("extern \"C\"",
sizeof( unsigned char), 11u); goto _LL219; _LL219:;}( int) _throw(({ struct Cyc_Std_String_pa_struct
_temp420; _temp420.tag= Cyc_Std_String_pa; _temp420.f1=( struct _tagged_arr)
unimp_msg_part;{ void* _temp419[ 1u]={& _temp420}; Cyc_Tcutil_impos( _tag_arr("tcStmt: nested %s declarations unimplemented",
sizeof( unsigned char), 45u), _tag_arr( _temp419, sizeof( void*), 1u));}}));}
_LL40:( void*)( _temp127->r=( void*)(( void*)( Cyc_Absyn_seq_stmt( Cyc_Absyn_new_stmt((
void*) _temp127->r, _temp127->loc), Cyc_Absyn_skip_stmt( _temp127->loc),
_temp127->loc))->r)); if( new_block){ te= Cyc_Tcenv_new_block( s0->loc, te);}{
struct Cyc_Absyn_Vardecl _temp423; struct Cyc_Core_Opt* _temp424; struct Cyc_Core_Opt**
_temp426; void* _temp427; struct Cyc_Absyn_Vardecl* _temp421= _temp129; _temp423=*
_temp421; _LL428: _temp427=( void*) _temp423.type; goto _LL425; _LL425: _temp424=
_temp423.rgn; _temp426=( struct Cyc_Core_Opt**)&(* _temp421).rgn; goto _LL422;
_LL422: { void* _temp429= Cyc_Tcenv_curr_rgn( te); void* rgn_typ=( void*)({
struct Cyc_Absyn_VarType_struct* _temp435=( struct Cyc_Absyn_VarType_struct*)
_cycalloc( sizeof( struct Cyc_Absyn_VarType_struct)); _temp435[ 0]=({ struct Cyc_Absyn_VarType_struct
_temp436; _temp436.tag= Cyc_Absyn_VarType; _temp436.f1= _temp131; _temp436;});
_temp435;});* _temp426=({ struct Cyc_Core_Opt* _temp430=( struct Cyc_Core_Opt*)
_cycalloc( sizeof( struct Cyc_Core_Opt)); _temp430->v=( void*) _temp429;
_temp430;}); te= Cyc_Tcenv_add_type_vars( s0->loc, te,({ struct Cyc_List_List*
_temp431=( struct Cyc_List_List*) _cycalloc( sizeof( struct Cyc_List_List));
_temp431->hd=( void*) _temp131; _temp431->tl= 0; _temp431;})); Cyc_Tcutil_check_type(
s0->loc, te, Cyc_Tcenv_lookup_type_vars( te),( void*) Cyc_Absyn_BoxKind, 1,
_temp427); te= Cyc_Tcenv_add_region( te, rgn_typ); if( ! Cyc_Tcutil_unify(
_temp427,( void*)({ struct Cyc_Absyn_RgnHandleType_struct* _temp432=( struct Cyc_Absyn_RgnHandleType_struct*)
_cycalloc( sizeof( struct Cyc_Absyn_RgnHandleType_struct)); _temp432[ 0]=({
struct Cyc_Absyn_RgnHandleType_struct _temp433; _temp433.tag= Cyc_Absyn_RgnHandleType;
_temp433.f1=( void*) rgn_typ; _temp433;}); _temp432;}))){({ void* _temp434[ 0u]={};((
int(*)( struct _tagged_arr fmt, struct _tagged_arr ap)) Cyc_Tcutil_impos)(
_tag_arr("region stmt: type of region handle is wrong!", sizeof( unsigned char),
45u), _tag_arr( _temp434, sizeof( void*), 0u));});} Cyc_Tcstmt_tcStmt( Cyc_Tcenv_set_encloser(
Cyc_Tcenv_add_local_var( _temp127->loc, Cyc_Tcenv_enter_try( te), _temp129), s0),
_temp127, 0); return;}} _LL42:( int) _throw(({ void* _temp437[ 0u]={}; Cyc_Tcutil_impos(
_tag_arr("tcStmt: cut is not implemented", sizeof( unsigned char), 31u),
_tag_arr( _temp437, sizeof( void*), 0u));})); _LL44:( int) _throw(({ void*
_temp438[ 0u]={}; Cyc_Tcutil_impos( _tag_arr("tcStmt: splice is not implemented",
sizeof( unsigned char), 34u), _tag_arr( _temp438, sizeof( void*), 0u));})); _LL4:;}}