// This is a C header file to be used by the output of the Cyclone
// to C translator.  The corresponding definitions are in file lib/runtime_cyc.c
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

//// Discriminated Unions
struct _xtunion_struct { char *tag; };

// Need one of these per thread (we don't have threads)
// The runtime maintains a stack that contains either _handler_cons
// structs or _RegionHandle structs.  The tag is 0 for a handler_cons
// and 1 for a region handle.  
struct _RuntimeStack {
  int tag; // 0 for an exception handler, 1 for a region handle
  struct _RuntimeStack *next;
};

//// Regions
struct _RegionPage {
#ifdef CYC_REGION_PROFILE
  unsigned total_bytes;
  unsigned free_bytes;
#endif
  struct _RegionPage *next;
  char data[0];
};

struct _RegionHandle {
  struct _RuntimeStack s;
  struct _RegionPage *curr;
  char               *offset;
  char               *last_plus_one;
#ifdef CYC_REGION_PROFILE
  const char         *name;
#endif
};

extern struct _RegionHandle _new_region(const char *);
extern void * _region_malloc(struct _RegionHandle *, unsigned);
extern void * _region_calloc(struct _RegionHandle *, unsigned t, unsigned n);
extern void   _free_region(struct _RegionHandle *);
extern void   _reset_region(struct _RegionHandle *);

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
extern int _throw(void* e);
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
#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#define _check_null(ptr) \
  ({ void*_check_null_temp = (void*)(ptr); \
     if (!_check_null_temp) _throw_null(); \
     _check_null_temp; })
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  ((char *)ptr) + (elt_sz)*(index); })
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  void*_cks_ptr = (void*)(ptr); \
  unsigned _cks_bound = (bound); \
  unsigned _cks_elt_sz = (elt_sz); \
  unsigned _cks_index = (index); \
  if (!_cks_ptr) _throw_null(); \
  if (!_cks_index >= _cks_bound) _throw_arraybounds(); \
  ((char *)_cks_ptr) + _cks_elt_sz*_cks_index; })
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_notnull(bound,index) (index)
#else
#define _check_known_subscript_notnull(bound,index) ({ \
  unsigned _cksnn_bound = (bound); \
  unsigned _cksnn_index = (index); \
  if (_cksnn_index >= _cksnn_bound) _throw_arraybounds(); \
  _cksnn_index; })
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_unknown_subscript(arr,elt_sz,index) ({ \
  struct _tagged_arr _cus_arr = (arr); \
  unsigned _cus_elt_sz = (elt_sz); \
  unsigned _cus_index = (index); \
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index; \
  _cus_ans; })
#else
#define _check_unknown_subscript(arr,elt_sz,index) ({ \
  struct _tagged_arr _cus_arr = (arr); \
  unsigned _cus_elt_sz = (elt_sz); \
  unsigned _cus_index = (index); \
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index; \
  if (!_cus_arr.base) _throw_null(); \
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#endif

#define _tag_arr(tcurr,elt_sz,num_elts) ({ \
  struct _tagged_arr _tag_arr_ans; \
  _tag_arr_ans.base = _tag_arr_ans.curr = (void*)(tcurr); \
  _tag_arr_ans.last_plus_one = _tag_arr_ans.base + (elt_sz) * (num_elts); \
  _tag_arr_ans; })

#define _init_tag_arr(arr_ptr,arr,elt_sz,num_elts) ({ \
  struct _tagged_arr *_itarr_ptr = (arr_ptr); \
  void* _itarr = (arr); \
  _itarr_ptr->base = _itarr_ptr->curr = _itarr; \
  _itarr_ptr->last_plus_one = ((char *)_itarr) + (elt_sz) * (num_elts); \
  _itarr_ptr; })

#ifdef NO_CYC_BOUNDS_CHECKS
#define _untag_arr(arr,elt_sz,num_elts) ((arr).curr)
#else
#define _untag_arr(arr,elt_sz,num_elts) ({ \
  struct _tagged_arr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if (_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one)\
    _throw_arraybounds(); \
  _curr; })
#endif

#define _get_arr_size(arr,elt_sz) \
  ({struct _tagged_arr _get_arr_size_temp = (arr); \
    unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr; \
    unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one; \
    (_get_arr_size_curr < _get_arr_size_temp.base || \
     _get_arr_size_curr >= _get_arr_size_last) ? 0 : \
    ((_get_arr_size_last - _get_arr_size_curr) / (elt_sz));})

#define _tagged_arr_plus(arr,elt_sz,change) ({ \
  struct _tagged_arr _ans = (arr); \
  _ans.curr += ((int)(elt_sz))*(change); \
  _ans; })

#define _tagged_arr_inplace_plus(arr_ptr,elt_sz,change) ({ \
  struct _tagged_arr * _arr_ptr = (arr_ptr); \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  *_arr_ptr; })

#define _tagged_arr_inplace_plus_post(arr_ptr,elt_sz,change) ({ \
  struct _tagged_arr * _arr_ptr = (arr_ptr); \
  struct _tagged_arr _ans = *_arr_ptr; \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  _ans; })

//// Allocation
extern void* GC_malloc(int);
extern void* GC_malloc_atomic(int);
extern void* GC_calloc(unsigned,unsigned);
extern void* GC_calloc_atomic(unsigned,unsigned);

static inline void* _cycalloc(int n) {
  void * ans = (void *)GC_malloc(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static inline void* _cycalloc_atomic(int n) {
  void * ans = (void *)GC_malloc_atomic(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static inline void* _cyccalloc(unsigned n, unsigned s) {
  void* ans = (void*)GC_calloc(n,s);
  if (!ans)
    _throw_badalloc();
  return ans;
}
static inline void* _cyccalloc_atomic(unsigned n, unsigned s) {
  void* ans = (void*)GC_calloc_atomic(n,s);
  if (!ans)
    _throw_badalloc();
  return ans;
}
#define MAX_MALLOC_SIZE (1 << 28)
static inline unsigned int _check_times(unsigned x, unsigned y) {
  unsigned long long whole_ans = 
    ((unsigned long long)x)*((unsigned long long)y);
  unsigned word_ans = (unsigned)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#if defined(CYC_REGION_PROFILE) 
extern void* _profile_GC_malloc(int,char *file,int lineno);
extern void* _profile_GC_malloc_atomic(int,char *file,int lineno);
extern void* _profile_region_malloc(struct _RegionHandle *, unsigned,
                                     char *file,int lineno);
extern struct _RegionHandle _profile_new_region(const char *rgn_name,
						char *file,int lineno);
extern void _profile_free_region(struct _RegionHandle *,
				 char *file,int lineno);
#  if !defined(RUNTIME_CYC)
#define _new_region(n) _profile_new_region(n,__FILE__ ":" __FUNCTION__,__LINE__)
#define _free_region(r) _profile_free_region(r,__FILE__ ":" __FUNCTION__,__LINE__)
#define _region_malloc(rh,n) _profile_region_malloc(rh,n,__FILE__ ":" __FUNCTION__,__LINE__)
#  endif
#define _cycalloc(n) _profile_GC_malloc(n,__FILE__ ":" __FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_GC_malloc_atomic(n,__FILE__ ":" __FUNCTION__,__LINE__)
#endif
#endif
 struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[21];struct Cyc_Core_Invalid_argument_struct{
char*tag;struct _tagged_arr f1;};extern char Cyc_Core_Failure[12];struct Cyc_Core_Failure_struct{
char*tag;struct _tagged_arr f1;};extern char Cyc_Core_Impossible[15];struct Cyc_Core_Impossible_struct{
char*tag;struct _tagged_arr f1;};extern char Cyc_Core_Not_found[14];extern char Cyc_Core_Unreachable[
16];struct Cyc_Core_Unreachable_struct{char*tag;struct _tagged_arr f1;};extern
struct _RegionHandle*Cyc_Core_heap_region;struct Cyc_Cstdio___abstractFILE;struct
Cyc_Std___cycFILE;extern struct Cyc_Std___cycFILE*Cyc_Std_stderr;extern char Cyc_Std_FileCloseError[
19];extern char Cyc_Std_FileOpenError[18];struct Cyc_Std_FileOpenError_struct{char*
tag;struct _tagged_arr f1;};struct Cyc_Std_String_pa_struct{int tag;struct
_tagged_arr f1;};struct Cyc_Std_Int_pa_struct{int tag;unsigned int f1;};struct Cyc_Std_Double_pa_struct{
int tag;double f1;};struct Cyc_Std_ShortPtr_pa_struct{int tag;short*f1;};struct Cyc_Std_IntPtr_pa_struct{
int tag;unsigned int*f1;};int Cyc_Std_fprintf(struct Cyc_Std___cycFILE*,struct
_tagged_arr fmt,struct _tagged_arr);struct _tagged_arr Cyc_Std_aprintf(struct
_tagged_arr fmt,struct _tagged_arr);struct _tagged_arr Cyc_Std_vrprintf(struct
_RegionHandle*r1,struct _tagged_arr fmt,struct _tagged_arr ap);struct Cyc_Std_ShortPtr_sa_struct{
int tag;short*f1;};struct Cyc_Std_UShortPtr_sa_struct{int tag;unsigned short*f1;};
struct Cyc_Std_IntPtr_sa_struct{int tag;int*f1;};struct Cyc_Std_UIntPtr_sa_struct{
int tag;unsigned int*f1;};struct Cyc_Std_StringPtr_sa_struct{int tag;struct
_tagged_arr f1;};struct Cyc_Std_DoublePtr_sa_struct{int tag;double*f1;};struct Cyc_Std_FloatPtr_sa_struct{
int tag;float*f1;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};int Cyc_List_length(
struct Cyc_List_List*x);struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),
void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[18];struct Cyc_List_List*
Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[
8];struct Cyc_Lineno_Pos{struct _tagged_arr logical_file;struct _tagged_arr line;int
line_no;int col;};extern char Cyc_Position_Exit[9];struct Cyc_Position_Segment;
struct Cyc_Position_Error{struct _tagged_arr source;struct Cyc_Position_Segment*seg;
void*kind;struct _tagged_arr desc;};struct Cyc_Position_Error*Cyc_Position_mk_err_elab(
struct Cyc_Position_Segment*,struct _tagged_arr);extern char Cyc_Position_Nocontext[
14];void Cyc_Position_post_error(struct Cyc_Position_Error*);struct Cyc_Absyn_Rel_n_struct{
int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Abs_n_struct{int tag;struct Cyc_List_List*
f1;};struct _tuple0{void*f1;struct _tagged_arr*f2;};struct Cyc_Absyn_Conref;struct
Cyc_Absyn_Tqual{int q_const: 1;int q_volatile: 1;int q_restrict: 1;};struct Cyc_Absyn_Conref{
void*v;};struct Cyc_Absyn_Eq_constr_struct{int tag;void*f1;};struct Cyc_Absyn_Forward_constr_struct{
int tag;struct Cyc_Absyn_Conref*f1;};struct Cyc_Absyn_Eq_kb_struct{int tag;void*f1;}
;struct Cyc_Absyn_Unknown_kb_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_struct{
int tag;struct Cyc_Core_Opt*f1;void*f2;};struct Cyc_Absyn_Tvar{struct _tagged_arr*
name;int*identity;void*kind;};struct Cyc_Absyn_Upper_b_struct{int tag;struct Cyc_Absyn_Exp*
f1;};struct Cyc_Absyn_PtrInfo{void*elt_typ;void*rgn_typ;struct Cyc_Absyn_Conref*
nullable;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Conref*bounds;};struct Cyc_Absyn_VarargInfo{
struct Cyc_Core_Opt*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{
struct Cyc_List_List*tvars;struct Cyc_Core_Opt*effect;void*ret_typ;struct Cyc_List_List*
args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*
rgn_po;struct Cyc_List_List*attributes;};struct Cyc_Absyn_UnknownTunionInfo{struct
_tuple0*name;int is_xtunion;};struct Cyc_Absyn_UnknownTunion_struct{int tag;struct
Cyc_Absyn_UnknownTunionInfo f1;};struct Cyc_Absyn_KnownTunion_struct{int tag;struct
Cyc_Absyn_Tuniondecl**f1;};struct Cyc_Absyn_TunionInfo{void*tunion_info;struct Cyc_List_List*
targs;void*rgn;};struct Cyc_Absyn_UnknownTunionFieldInfo{struct _tuple0*
tunion_name;struct _tuple0*field_name;int is_xtunion;};struct Cyc_Absyn_UnknownTunionfield_struct{
int tag;struct Cyc_Absyn_UnknownTunionFieldInfo f1;};struct Cyc_Absyn_KnownTunionfield_struct{
int tag;struct Cyc_Absyn_Tuniondecl*f1;struct Cyc_Absyn_Tunionfield*f2;};struct Cyc_Absyn_TunionFieldInfo{
void*field_info;struct Cyc_List_List*targs;};struct Cyc_Absyn_UnknownAggr_struct{
int tag;void*f1;struct _tuple0*f2;};struct Cyc_Absyn_KnownAggr_struct{int tag;struct
Cyc_Absyn_Aggrdecl**f1;};struct Cyc_Absyn_AggrInfo{void*aggr_info;struct Cyc_List_List*
targs;};struct Cyc_Absyn_Evar_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Core_Opt*
f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_struct{int tag;struct Cyc_Absyn_Tvar*
f1;};struct Cyc_Absyn_TunionType_struct{int tag;struct Cyc_Absyn_TunionInfo f1;};
struct Cyc_Absyn_TunionFieldType_struct{int tag;struct Cyc_Absyn_TunionFieldInfo f1;
};struct Cyc_Absyn_PointerType_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct
Cyc_Absyn_IntType_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_DoubleType_struct{
int tag;int f1;};struct Cyc_Absyn_ArrayType_struct{int tag;void*f1;struct Cyc_Absyn_Tqual
f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_FnType_struct{int tag;struct Cyc_Absyn_FnInfo
f1;};struct Cyc_Absyn_TupleType_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AggrType_struct{
int tag;struct Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AnonAggrType_struct{int tag;
void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_EnumType_struct{int tag;struct
_tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumType_struct{int
tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_SizeofType_struct{int tag;void*f1;};
struct Cyc_Absyn_RgnHandleType_struct{int tag;void*f1;};struct Cyc_Absyn_TypedefType_struct{
int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;
struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_TagType_struct{int tag;void*f1;};struct
Cyc_Absyn_TypeInt_struct{int tag;int f1;};struct Cyc_Absyn_AccessEff_struct{int tag;
void*f1;};struct Cyc_Absyn_JoinEff_struct{int tag;struct Cyc_List_List*f1;};struct
Cyc_Absyn_RgnsEff_struct{int tag;void*f1;};struct Cyc_Absyn_NoTypes_struct{int tag;
struct Cyc_List_List*f1;struct Cyc_Position_Segment*f2;};struct Cyc_Absyn_WithTypes_struct{
int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;struct Cyc_Core_Opt*
f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_NonNullable_ps_struct{int tag;struct
Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Nullable_ps_struct{int tag;struct Cyc_Absyn_Exp*
f1;};struct Cyc_Absyn_Regparm_att_struct{int tag;int f1;};struct Cyc_Absyn_Aligned_att_struct{
int tag;int f1;};struct Cyc_Absyn_Section_att_struct{int tag;struct _tagged_arr f1;};
struct Cyc_Absyn_Format_att_struct{int tag;void*f1;int f2;int f3;};struct Cyc_Absyn_ConstArray_mod_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pointer_mod_struct{int tag;void*
f1;void*f2;struct Cyc_Absyn_Tqual f3;};struct Cyc_Absyn_Function_mod_struct{int tag;
void*f1;};struct Cyc_Absyn_TypeParams_mod_struct{int tag;struct Cyc_List_List*f1;
struct Cyc_Position_Segment*f2;int f3;};struct Cyc_Absyn_Attributes_mod_struct{int
tag;struct Cyc_Position_Segment*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Char_c_struct{
int tag;void*f1;char f2;};struct Cyc_Absyn_Short_c_struct{int tag;void*f1;short f2;};
struct Cyc_Absyn_Int_c_struct{int tag;void*f1;int f2;};struct Cyc_Absyn_LongLong_c_struct{
int tag;void*f1;long long f2;};struct Cyc_Absyn_Float_c_struct{int tag;struct
_tagged_arr f1;};struct Cyc_Absyn_String_c_struct{int tag;struct _tagged_arr f1;};
struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;
struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_struct{int tag;
struct _tagged_arr*f1;};struct Cyc_Absyn_TupleIndex_struct{int tag;unsigned int f1;}
;struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;
struct Cyc_Absyn_Exp*num_elts;int fat_result;};struct Cyc_Absyn_Const_e_struct{int
tag;void*f1;};struct Cyc_Absyn_Var_e_struct{int tag;struct _tuple0*f1;void*f2;};
struct Cyc_Absyn_UnknownId_e_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Primop_e_struct{
int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_struct{int tag;
struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;void*f2;};struct Cyc_Absyn_Conditional_e_struct{int
tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};
struct Cyc_Absyn_SeqExp_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*
f2;};struct Cyc_Absyn_UnknownCall_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct
Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_struct{int tag;struct Cyc_Absyn_Exp*f1;
struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;};struct Cyc_Absyn_Throw_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoInstantiate_e_struct{int tag;
struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_struct{int tag;void*f1;struct
Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Address_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;};struct Cyc_Absyn_New_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*
f2;};struct Cyc_Absyn_Sizeoftyp_e_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_struct{int tag;void*f1;
void*f2;};struct Cyc_Absyn_Gentyp_e_struct{int tag;struct Cyc_List_List*f1;void*f2;
};struct Cyc_Absyn_Deref_e_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct _tagged_arr*f2;};struct Cyc_Absyn_AggrArrow_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct _tagged_arr*f2;};struct Cyc_Absyn_Subscript_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_struct{
int tag;struct Cyc_List_List*f1;};struct _tuple1{struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Tqual
f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_struct{int tag;struct _tuple1*f1;struct
Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_struct{int tag;struct Cyc_List_List*f1;
};struct Cyc_Absyn_Comprehension_e_struct{int tag;struct Cyc_Absyn_Vardecl*f1;
struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Struct_e_struct{
int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*
f4;};struct Cyc_Absyn_AnonStruct_e_struct{int tag;void*f1;struct Cyc_List_List*f2;}
;struct Cyc_Absyn_Tunion_e_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Tuniondecl*
f2;struct Cyc_Absyn_Tunionfield*f3;};struct Cyc_Absyn_Enum_e_struct{int tag;struct
_tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_AnonEnum_e_struct{
int tag;struct _tuple0*f1;void*f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_Malloc_e_struct{
int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_UnresolvedMem_e_struct{int
tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Codegen_e_struct{int tag;struct
Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Fill_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;};struct Cyc_Absyn_Exp{struct Cyc_Core_Opt*topt;void*r;struct Cyc_Position_Segment*
loc;void*annot;};struct _tuple2{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};
struct Cyc_Absyn_ForArrayInfo{struct Cyc_List_List*defns;struct _tuple2 condition;
struct _tuple2 delta;struct Cyc_Absyn_Stmt*body;};struct Cyc_Absyn_Exp_s_struct{int
tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_struct{int tag;struct Cyc_Absyn_Stmt*
f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_struct{int tag;struct Cyc_Absyn_Exp*
f1;};struct Cyc_Absyn_IfThenElse_s_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*
f2;struct Cyc_Absyn_Stmt*f3;};struct Cyc_Absyn_While_s_struct{int tag;struct _tuple2
f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_struct{int tag;struct Cyc_Absyn_Stmt*
f1;};struct Cyc_Absyn_Continue_s_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct
Cyc_Absyn_Goto_s_struct{int tag;struct _tagged_arr*f1;struct Cyc_Absyn_Stmt*f2;};
struct Cyc_Absyn_For_s_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple2 f2;
struct _tuple2 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_struct{int tag;
struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_SwitchC_s_struct{
int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Fallthru_s_struct{
int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_struct{
int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Cut_s_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Splice_s_struct{int tag;struct
Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Label_s_struct{int tag;struct _tagged_arr*f1;
struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_struct{int tag;struct Cyc_Absyn_Stmt*
f1;struct _tuple2 f2;};struct Cyc_Absyn_TryCatch_s_struct{int tag;struct Cyc_Absyn_Stmt*
f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Region_s_struct{int tag;struct Cyc_Absyn_Tvar*
f1;struct Cyc_Absyn_Vardecl*f2;int f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_ForArray_s_struct{
int tag;struct Cyc_Absyn_ForArrayInfo f1;};struct Cyc_Absyn_ResetRegion_s_struct{int
tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Stmt{void*r;struct Cyc_Position_Segment*
loc;struct Cyc_List_List*non_local_preds;int try_depth;void*annot;};struct Cyc_Absyn_Var_p_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Int_p_struct{int tag;void*f1;
int f2;};struct Cyc_Absyn_Char_p_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_struct{
int tag;struct _tagged_arr f1;};struct Cyc_Absyn_Tuple_p_struct{int tag;struct Cyc_List_List*
f1;};struct Cyc_Absyn_Pointer_p_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Reference_p_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Aggr_p_struct{int tag;struct
Cyc_Absyn_AggrInfo f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;};struct Cyc_Absyn_Tunion_p_struct{
int tag;struct Cyc_Absyn_Tuniondecl*f1;struct Cyc_Absyn_Tunionfield*f2;struct Cyc_List_List*
f3;};struct Cyc_Absyn_Enum_p_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*
f2;};struct Cyc_Absyn_AnonEnum_p_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*
f2;};struct Cyc_Absyn_UnknownId_p_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_struct{
int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Pat{void*r;
struct Cyc_Core_Opt*topt;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Switch_clause{
struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*
where_clause;struct Cyc_Absyn_Stmt*body;struct Cyc_Position_Segment*loc;};struct
Cyc_Absyn_SwitchC_clause{struct Cyc_Absyn_Exp*cnst_exp;struct Cyc_Absyn_Stmt*body;
struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Global_b_struct{int tag;struct
Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_struct{int tag;struct Cyc_Absyn_Fndecl*
f1;};struct Cyc_Absyn_Param_b_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct
Cyc_Absyn_Local_b_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{void*sc;struct
_tuple0*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;
struct Cyc_Core_Opt*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{
void*sc;int is_inline;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*
effect;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*
cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_Absyn_Stmt*body;struct Cyc_Core_Opt*
cached_typ;struct Cyc_Core_Opt*param_vardecls;struct Cyc_List_List*attributes;};
struct Cyc_Absyn_Aggrfield{struct _tagged_arr*name;struct Cyc_Absyn_Tqual tq;void*
type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;};struct Cyc_Absyn_AggrdeclImpl{
struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*
fields;};struct Cyc_Absyn_Aggrdecl{void*kind;void*sc;struct _tuple0*name;struct Cyc_List_List*
tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;};struct Cyc_Absyn_Tunionfield{
struct _tuple0*name;struct Cyc_List_List*typs;struct Cyc_Position_Segment*loc;void*
sc;};struct Cyc_Absyn_Tuniondecl{void*sc;struct _tuple0*name;struct Cyc_List_List*
tvs;struct Cyc_Core_Opt*fields;int is_xtunion;};struct Cyc_Absyn_Enumfield{struct
_tuple0*name;struct Cyc_Absyn_Exp*tag;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Enumdecl{
void*sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{
struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;struct Cyc_Core_Opt*
defn;};struct Cyc_Absyn_Var_d_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct
Cyc_Absyn_Fn_d_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_struct{
int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};
struct Cyc_Absyn_Letv_d_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Aggr_d_struct{
int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Tunion_d_struct{int tag;
struct Cyc_Absyn_Tuniondecl*f1;};struct Cyc_Absyn_Enum_d_struct{int tag;struct Cyc_Absyn_Enumdecl*
f1;};struct Cyc_Absyn_Typedef_d_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};
struct Cyc_Absyn_Namespace_d_struct{int tag;struct _tagged_arr*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_Using_d_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_ExternC_d_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Decl{
void*r;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_ArrayElement_struct{int
tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_struct{int tag;struct
_tagged_arr*f1;};extern char Cyc_Absyn_EmptyAnnot[15];int Cyc_Absyn_qvar_cmp(struct
_tuple0*,struct _tuple0*);struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual();void*Cyc_Absyn_force_kb(
void*kb);struct _tagged_arr Cyc_Absyn_attribute2string(void*);extern char Cyc_Tcdecl_Incompatible[
17];struct Cyc_Tcdecl_Xtunionfielddecl{struct Cyc_Absyn_Tuniondecl*base;struct Cyc_Absyn_Tunionfield*
field;};void Cyc_Tcdecl_merr(struct Cyc_Position_Segment*loc,struct _tagged_arr*
msg1,struct _tagged_arr fmt,struct _tagged_arr ap);struct _tuple3{void*f1;int f2;};
struct _tuple3 Cyc_Tcdecl_merge_scope(void*s0,void*s1,struct _tagged_arr t,struct
_tagged_arr v,struct Cyc_Position_Segment*loc,struct _tagged_arr*msg);struct Cyc_Absyn_Aggrdecl*
Cyc_Tcdecl_merge_aggrdecl(struct Cyc_Absyn_Aggrdecl*d0,struct Cyc_Absyn_Aggrdecl*
d1,struct Cyc_Position_Segment*loc,struct _tagged_arr*msg);struct Cyc_Absyn_Tuniondecl*
Cyc_Tcdecl_merge_tuniondecl(struct Cyc_Absyn_Tuniondecl*d0,struct Cyc_Absyn_Tuniondecl*
d1,struct Cyc_Position_Segment*loc,struct _tagged_arr*msg);struct Cyc_Absyn_Enumdecl*
Cyc_Tcdecl_merge_enumdecl(struct Cyc_Absyn_Enumdecl*d0,struct Cyc_Absyn_Enumdecl*
d1,struct Cyc_Position_Segment*loc,struct _tagged_arr*msg);struct Cyc_Absyn_Vardecl*
Cyc_Tcdecl_merge_vardecl(struct Cyc_Absyn_Vardecl*d0,struct Cyc_Absyn_Vardecl*d1,
struct Cyc_Position_Segment*loc,struct _tagged_arr*msg);struct Cyc_Absyn_Typedefdecl*
Cyc_Tcdecl_merge_typedefdecl(struct Cyc_Absyn_Typedefdecl*d0,struct Cyc_Absyn_Typedefdecl*
d1,struct Cyc_Position_Segment*loc,struct _tagged_arr*msg);void*Cyc_Tcdecl_merge_binding(
void*d0,void*d1,struct Cyc_Position_Segment*loc,struct _tagged_arr*msg);struct Cyc_Tcdecl_Xtunionfielddecl*
Cyc_Tcdecl_merge_xtunionfielddecl(struct Cyc_Tcdecl_Xtunionfielddecl*d0,struct Cyc_Tcdecl_Xtunionfielddecl*
d1,struct Cyc_Position_Segment*loc,struct _tagged_arr*msg);struct Cyc_List_List*Cyc_Tcdecl_sort_xtunion_fields(
struct Cyc_List_List*f,int*res,struct _tagged_arr*v,struct Cyc_Position_Segment*loc,
struct _tagged_arr*msg);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*
dest);};int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc_Dict_Dict;extern
char Cyc_Dict_Present[12];extern char Cyc_Dict_Absent[11];struct _tuple4{void*f1;
void*f2;};struct _tuple4*Cyc_Dict_rchoose(struct _RegionHandle*r,struct Cyc_Dict_Dict*
d);struct _tuple4*Cyc_Dict_rchoose(struct _RegionHandle*,struct Cyc_Dict_Dict*d);
int Cyc_Std_zstrptrcmp(struct _tagged_arr*,struct _tagged_arr*);struct _tagged_arr
Cyc_Std_strconcat(struct _tagged_arr,struct _tagged_arr);struct Cyc_Set_Set;extern
char Cyc_Set_Absent[11];struct Cyc_RgnOrder_RgnPO;struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_initial_fn_po(
struct Cyc_List_List*tvs,struct Cyc_List_List*po,void*effect,struct Cyc_Absyn_Tvar*
fst_rgn);struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_outlives_constraint(struct Cyc_RgnOrder_RgnPO*
po,void*eff,void*rgn);struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_youngest(struct
Cyc_RgnOrder_RgnPO*po,struct Cyc_Absyn_Tvar*rgn,int resetable);int Cyc_RgnOrder_is_region_resetable(
struct Cyc_RgnOrder_RgnPO*po,struct Cyc_Absyn_Tvar*r);int Cyc_RgnOrder_effect_outlives(
struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn);int Cyc_RgnOrder_satisfies_constraints(
struct Cyc_RgnOrder_RgnPO*po,struct Cyc_List_List*constraints,void*default_bound,
int do_pin);int Cyc_RgnOrder_eff_outlives_eff(struct Cyc_RgnOrder_RgnPO*po,void*
eff1,void*eff2);struct Cyc_Tcenv_VarRes_struct{int tag;void*f1;};struct Cyc_Tcenv_AggrRes_struct{
int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Tcenv_TunionRes_struct{int tag;
struct Cyc_Absyn_Tuniondecl*f1;struct Cyc_Absyn_Tunionfield*f2;};struct Cyc_Tcenv_EnumRes_struct{
int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcenv_AnonEnumRes_struct{
int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcenv_Genv{struct Cyc_Set_Set*
namespaces;struct Cyc_Dict_Dict*aggrdecls;struct Cyc_Dict_Dict*tuniondecls;struct
Cyc_Dict_Dict*enumdecls;struct Cyc_Dict_Dict*typedefs;struct Cyc_Dict_Dict*
ordinaries;struct Cyc_List_List*availables;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Stmt_j_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Tcenv_Outermost_struct{int tag;void*f1;
};struct Cyc_Tcenv_Frame_struct{int tag;void*f1;void*f2;};struct Cyc_Tcenv_Hidden_struct{
int tag;void*f1;void*f2;};struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Dict_Dict*
ae;struct Cyc_Core_Opt*le;};void Cyc_Tcutil_explain_failure();int Cyc_Tcutil_unify(
void*,void*);void*Cyc_Tcutil_substitute(struct Cyc_List_List*,void*);int Cyc_Tcutil_equal_tqual(
struct Cyc_Absyn_Tqual tq1,struct Cyc_Absyn_Tqual tq2);int Cyc_Tcutil_same_atts(
struct Cyc_List_List*,struct Cyc_List_List*);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;
struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs: 1;int qvar_to_Cids: 1;
int add_cyc_prefix: 1;int to_VC: 1;int decls_first: 1;int rewrite_temp_tvars: 1;int
print_all_tvars: 1;int print_all_kinds: 1;int print_using_stmts: 1;int
print_externC_stmts: 1;int print_full_evars: 1;int generate_line_directives: 1;int
use_curr_namespace: 1;struct Cyc_List_List*curr_namespace;};struct _tagged_arr Cyc_Absynpp_typ2string(
void*);struct _tagged_arr Cyc_Absynpp_kind2string(void*);struct _tagged_arr Cyc_Absynpp_qvar2string(
struct _tuple0*);struct _tagged_arr Cyc_Absynpp_scope2string(void*sc);char Cyc_Tcdecl_Incompatible[
17]="\000\000\000\000Incompatible";void Cyc_Tcdecl_merr(struct Cyc_Position_Segment*
loc,struct _tagged_arr*msg1,struct _tagged_arr fmt,struct _tagged_arr ap){if(msg1 == 0)(
int)_throw((void*)Cyc_Tcdecl_Incompatible);{struct _tagged_arr fmt2=(struct
_tagged_arr)Cyc_Std_strconcat(_tag_arr("%s ",sizeof(char),4),fmt);struct
_tagged_arr ap2=({unsigned int _tmp0=_get_arr_size(ap,sizeof(void*))+ 1;void**
_tmp1=(void**)_cycalloc(_check_times(sizeof(void*),_tmp0));struct _tagged_arr
_tmp5=_tag_arr(_tmp1,sizeof(void*),_get_arr_size(ap,sizeof(void*))+ 1);{
unsigned int _tmp2=_tmp0;unsigned int i;for(i=0;i < _tmp2;i ++){_tmp1[i]=i == 0?(void*)({
struct Cyc_Std_String_pa_struct*_tmp3=_cycalloc(sizeof(*_tmp3));_tmp3[0]=({struct
Cyc_Std_String_pa_struct _tmp4;_tmp4.tag=0;_tmp4.f1=(struct _tagged_arr)*msg1;
_tmp4;});_tmp3;}):*((void**)_check_unknown_subscript(ap,sizeof(void*),(int)(i - 1)));}}
_tmp5;});Cyc_Position_post_error(Cyc_Position_mk_err_elab(loc,(struct _tagged_arr)
Cyc_Std_vrprintf(Cyc_Core_heap_region,fmt2,ap2)));}}static void Cyc_Tcdecl_merge_scope_err(
void*s0,void*s1,struct _tagged_arr t,struct _tagged_arr v,struct Cyc_Position_Segment*
loc,struct _tagged_arr*msg){({struct Cyc_Std_String_pa_struct _tmpA;_tmpA.tag=0;
_tmpA.f1=(struct _tagged_arr)Cyc_Absynpp_scope2string(s0);{struct Cyc_Std_String_pa_struct
_tmp9;_tmp9.tag=0;_tmp9.f1=(struct _tagged_arr)Cyc_Absynpp_scope2string(s1);{
struct Cyc_Std_String_pa_struct _tmp8;_tmp8.tag=0;_tmp8.f1=(struct _tagged_arr)v;{
struct Cyc_Std_String_pa_struct _tmp7;_tmp7.tag=0;_tmp7.f1=(struct _tagged_arr)t;{
void*_tmp6[4]={& _tmp7,& _tmp8,& _tmp9,& _tmpA};Cyc_Tcdecl_merr(loc,msg,_tag_arr("%s %s is %s whereas expected scope is %s",
sizeof(char),41),_tag_arr(_tmp6,sizeof(void*),4));}}}}});}struct _tuple3 Cyc_Tcdecl_merge_scope(
void*s0,void*s1,struct _tagged_arr t,struct _tagged_arr v,struct Cyc_Position_Segment*
loc,struct _tagged_arr*msg){{struct _tuple4 _tmpC=({struct _tuple4 _tmpB;_tmpB.f1=s0;
_tmpB.f2=s1;_tmpB;});void*_tmpD;void*_tmpE;void*_tmpF;void*_tmp10;void*_tmp11;
void*_tmp12;void*_tmp13;void*_tmp14;void*_tmp15;void*_tmp16;void*_tmp17;void*
_tmp18;_LL1: _tmpD=_tmpC.f1;if((int)_tmpD != 4)goto _LL3;_tmpE=_tmpC.f2;if((int)
_tmpE != 4)goto _LL3;_LL2: goto _LL0;_LL3: _tmpF=_tmpC.f1;if((int)_tmpF != 4)goto _LL5;
_LL4: goto _LL6;_LL5: _tmp10=_tmpC.f2;if((int)_tmp10 != 4)goto _LL7;_LL6: Cyc_Tcdecl_merge_scope_err(
s0,s1,t,v,loc,msg);return({struct _tuple3 _tmp19;_tmp19.f1=s1;_tmp19.f2=0;_tmp19;});
_LL7: _tmp11=_tmpC.f2;if((int)_tmp11 != 3)goto _LL9;_LL8: s1=s0;goto _LL0;_LL9: _tmp12=
_tmpC.f1;if((int)_tmp12 != 3)goto _LLB;_LLA: goto _LL0;_LLB: _tmp13=_tmpC.f1;if((int)
_tmp13 != 0)goto _LLD;_tmp14=_tmpC.f2;if((int)_tmp14 != 0)goto _LLD;_LLC: goto _LLE;
_LLD: _tmp15=_tmpC.f1;if((int)_tmp15 != 2)goto _LLF;_tmp16=_tmpC.f2;if((int)_tmp16
!= 2)goto _LLF;_LLE: goto _LL10;_LLF: _tmp17=_tmpC.f1;if((int)_tmp17 != 1)goto _LL11;
_tmp18=_tmpC.f2;if((int)_tmp18 != 1)goto _LL11;_LL10: goto _LL0;_LL11:;_LL12: Cyc_Tcdecl_merge_scope_err(
s0,s1,t,v,loc,msg);return({struct _tuple3 _tmp1A;_tmp1A.f1=s1;_tmp1A.f2=0;_tmp1A;});
_LL0:;}return({struct _tuple3 _tmp1B;_tmp1B.f1=s1;_tmp1B.f2=1;_tmp1B;});}static int
Cyc_Tcdecl_check_type(void*t0,void*t1){return Cyc_Tcutil_unify(t0,t1);}static
unsigned int Cyc_Tcdecl_get_uint_const_value(struct Cyc_Absyn_Exp*e){void*_tmp1C=(
void*)e->r;void*_tmp1D;int _tmp1E;_LL14: if(*((int*)_tmp1C)!= 0)goto _LL16;_tmp1D=(
void*)((struct Cyc_Absyn_Const_e_struct*)_tmp1C)->f1;if(_tmp1D <= (void*)1?1:*((
int*)_tmp1D)!= 2)goto _LL16;_tmp1E=((struct Cyc_Absyn_Int_c_struct*)_tmp1D)->f2;
_LL15: return(unsigned int)_tmp1E;_LL16:;_LL17:(int)_throw((void*)({struct Cyc_Core_Invalid_argument_struct*
_tmp1F=_cycalloc(sizeof(*_tmp1F));_tmp1F[0]=({struct Cyc_Core_Invalid_argument_struct
_tmp20;_tmp20.tag=Cyc_Core_Invalid_argument;_tmp20.f1=_tag_arr("Tcdecl::get_uint_const_value",
sizeof(char),29);_tmp20;});_tmp1F;}));_LL13:;}inline static int Cyc_Tcdecl_check_tvs(
struct Cyc_List_List*tvs0,struct Cyc_List_List*tvs1,struct _tagged_arr t,struct
_tagged_arr v,struct Cyc_Position_Segment*loc,struct _tagged_arr*msg){if(((int(*)(
struct Cyc_List_List*x))Cyc_List_length)(tvs0)!= ((int(*)(struct Cyc_List_List*x))
Cyc_List_length)(tvs1)){({struct Cyc_Std_String_pa_struct _tmp23;_tmp23.tag=0;
_tmp23.f1=(struct _tagged_arr)v;{struct Cyc_Std_String_pa_struct _tmp22;_tmp22.tag=
0;_tmp22.f1=(struct _tagged_arr)t;{void*_tmp21[2]={& _tmp22,& _tmp23};Cyc_Tcdecl_merr(
loc,msg,_tag_arr("%s %s has a different number of type parameters",sizeof(char),
48),_tag_arr(_tmp21,sizeof(void*),2));}}});return 0;}{struct Cyc_List_List*_tmp24=
tvs0;struct Cyc_List_List*_tmp25=tvs1;for(0;_tmp24 != 0;(_tmp24=_tmp24->tl,_tmp25=
_tmp25->tl)){void*_tmp26=Cyc_Absyn_force_kb((void*)((struct Cyc_Absyn_Tvar*)
_tmp24->hd)->kind);void*_tmp27=Cyc_Absyn_force_kb((void*)((struct Cyc_Absyn_Tvar*)((
struct Cyc_List_List*)_check_null(_tmp25))->hd)->kind);if(_tmp26 == _tmp27)
continue;({struct Cyc_Std_String_pa_struct _tmp2D;_tmp2D.tag=0;_tmp2D.f1=(struct
_tagged_arr)Cyc_Absynpp_kind2string(_tmp27);{struct Cyc_Std_String_pa_struct
_tmp2C;_tmp2C.tag=0;_tmp2C.f1=(struct _tagged_arr)*((struct Cyc_Absyn_Tvar*)_tmp24->hd)->name;{
struct Cyc_Std_String_pa_struct _tmp2B;_tmp2B.tag=0;_tmp2B.f1=(struct _tagged_arr)
Cyc_Absynpp_kind2string(_tmp26);{struct Cyc_Std_String_pa_struct _tmp2A;_tmp2A.tag=
0;_tmp2A.f1=(struct _tagged_arr)v;{struct Cyc_Std_String_pa_struct _tmp29;_tmp29.tag=
0;_tmp29.f1=(struct _tagged_arr)t;{void*_tmp28[5]={& _tmp29,& _tmp2A,& _tmp2B,&
_tmp2C,& _tmp2D};Cyc_Tcdecl_merr(loc,msg,_tag_arr("%s %s has a different kind (%s) for type parameter %s (%s)",
sizeof(char),59),_tag_arr(_tmp28,sizeof(void*),5));}}}}}});return 0;}return 1;}}
static int Cyc_Tcdecl_check_atts(struct Cyc_List_List*atts0,struct Cyc_List_List*
atts1,struct _tagged_arr t,struct _tagged_arr v,struct Cyc_Position_Segment*loc,
struct _tagged_arr*msg){if(!Cyc_Tcutil_same_atts(atts0,atts1)){({struct Cyc_Std_String_pa_struct
_tmp30;_tmp30.tag=0;_tmp30.f1=(struct _tagged_arr)v;{struct Cyc_Std_String_pa_struct
_tmp2F;_tmp2F.tag=0;_tmp2F.f1=(struct _tagged_arr)t;{void*_tmp2E[2]={& _tmp2F,&
_tmp30};Cyc_Tcdecl_merr(loc,msg,_tag_arr("%s %s has different attributes",
sizeof(char),31),_tag_arr(_tmp2E,sizeof(void*),2));}}});return 0;}return 1;}struct
_tuple5{struct Cyc_Absyn_Tvar*f1;void*f2;};static struct Cyc_List_List*Cyc_Tcdecl_build_tvs_map(
struct Cyc_List_List*tvs0,struct Cyc_List_List*tvs1){struct Cyc_List_List*inst=0;
for(0;tvs0 != 0;(tvs0=tvs0->tl,tvs1=tvs1->tl)){inst=({struct Cyc_List_List*_tmp31=
_cycalloc(sizeof(*_tmp31));_tmp31->hd=({struct _tuple5*_tmp32=_cycalloc(sizeof(*
_tmp32));_tmp32->f1=(struct Cyc_Absyn_Tvar*)((struct Cyc_List_List*)_check_null(
tvs1))->hd;_tmp32->f2=(void*)({struct Cyc_Absyn_VarType_struct*_tmp33=_cycalloc(
sizeof(*_tmp33));_tmp33[0]=({struct Cyc_Absyn_VarType_struct _tmp34;_tmp34.tag=1;
_tmp34.f1=(struct Cyc_Absyn_Tvar*)tvs0->hd;_tmp34;});_tmp33;});_tmp32;});_tmp31->tl=
inst;_tmp31;});}return inst;}struct _tuple6{struct Cyc_Absyn_AggrdeclImpl*f1;struct
Cyc_Absyn_AggrdeclImpl*f2;};struct Cyc_Absyn_Aggrdecl*Cyc_Tcdecl_merge_aggrdecl(
struct Cyc_Absyn_Aggrdecl*d0,struct Cyc_Absyn_Aggrdecl*d1,struct Cyc_Position_Segment*
loc,struct _tagged_arr*msg){struct _tagged_arr _tmp35=Cyc_Absynpp_qvar2string(d0->name);
int _tmp36=1;if(!((void*)d0->kind == (void*)d1->kind))return 0;if(!Cyc_Tcdecl_check_tvs(
d0->tvs,d1->tvs,_tag_arr("type",sizeof(char),5),_tmp35,loc,msg))return 0;{void*
_tmp38;int _tmp39;struct _tuple3 _tmp37=Cyc_Tcdecl_merge_scope((void*)d0->sc,(void*)
d1->sc,_tag_arr("type",sizeof(char),5),_tmp35,loc,msg);_tmp38=_tmp37.f1;_tmp39=
_tmp37.f2;if(!_tmp39)_tmp36=0;if(!Cyc_Tcdecl_check_atts(d0->attributes,d1->attributes,
_tag_arr("type",sizeof(char),5),_tmp35,loc,msg))_tmp36=0;{struct Cyc_Absyn_Aggrdecl*
d2;{struct _tuple6 _tmp3B=({struct _tuple6 _tmp3A;_tmp3A.f1=d0->impl;_tmp3A.f2=d1->impl;
_tmp3A;});struct Cyc_Absyn_AggrdeclImpl*_tmp3C;struct Cyc_Absyn_AggrdeclImpl*
_tmp3D;struct Cyc_Absyn_AggrdeclImpl*_tmp3E;struct Cyc_Absyn_AggrdeclImpl _tmp3F;
struct Cyc_List_List*_tmp40;struct Cyc_List_List*_tmp41;struct Cyc_List_List*_tmp42;
struct Cyc_Absyn_AggrdeclImpl*_tmp43;struct Cyc_Absyn_AggrdeclImpl _tmp44;struct Cyc_List_List*
_tmp45;struct Cyc_List_List*_tmp46;struct Cyc_List_List*_tmp47;_LL19: _tmp3C=_tmp3B.f2;
if(_tmp3C != 0)goto _LL1B;_LL1A: d2=d0;goto _LL18;_LL1B: _tmp3D=_tmp3B.f1;if(_tmp3D != 
0)goto _LL1D;_LL1C: d2=d1;goto _LL18;_LL1D: _tmp3E=_tmp3B.f1;if(_tmp3E == 0)goto _LL18;
_tmp3F=*_tmp3E;_tmp40=_tmp3F.exist_vars;_tmp41=_tmp3F.rgn_po;_tmp42=_tmp3F.fields;
_tmp43=_tmp3B.f2;if(_tmp43 == 0)goto _LL18;_tmp44=*_tmp43;_tmp45=_tmp44.exist_vars;
_tmp46=_tmp44.rgn_po;_tmp47=_tmp44.fields;_LL1E: if(!Cyc_Tcdecl_check_tvs(_tmp40,
_tmp45,_tag_arr("type",sizeof(char),5),_tmp35,loc,msg))return 0;{struct Cyc_List_List*
_tmp48=Cyc_Tcdecl_build_tvs_map(((struct Cyc_List_List*(*)(struct Cyc_List_List*x,
struct Cyc_List_List*y))Cyc_List_append)(d0->tvs,_tmp40),((struct Cyc_List_List*(*)(
struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_append)(d1->tvs,_tmp45));
for(0;_tmp41 != 0?_tmp46 != 0: 0;(_tmp41=_tmp41->tl,_tmp46=_tmp46->tl)){Cyc_Tcdecl_check_type((*((
struct _tuple4*)_tmp41->hd)).f1,(*((struct _tuple4*)_tmp46->hd)).f1);Cyc_Tcdecl_check_type((*((
struct _tuple4*)_tmp41->hd)).f2,(*((struct _tuple4*)_tmp46->hd)).f2);}for(0;_tmp42
!= 0?_tmp47 != 0: 0;(_tmp42=_tmp42->tl,_tmp47=_tmp47->tl)){struct Cyc_Absyn_Aggrfield
_tmp4A;struct _tagged_arr*_tmp4B;struct Cyc_Absyn_Tqual _tmp4C;void*_tmp4D;struct
Cyc_Absyn_Exp*_tmp4E;struct Cyc_List_List*_tmp4F;struct Cyc_Absyn_Aggrfield*_tmp49=(
struct Cyc_Absyn_Aggrfield*)_tmp42->hd;_tmp4A=*_tmp49;_tmp4B=_tmp4A.name;_tmp4C=
_tmp4A.tq;_tmp4D=(void*)_tmp4A.type;_tmp4E=_tmp4A.width;_tmp4F=_tmp4A.attributes;{
struct Cyc_Absyn_Aggrfield _tmp51;struct _tagged_arr*_tmp52;struct Cyc_Absyn_Tqual
_tmp53;void*_tmp54;struct Cyc_Absyn_Exp*_tmp55;struct Cyc_List_List*_tmp56;struct
Cyc_Absyn_Aggrfield*_tmp50=(struct Cyc_Absyn_Aggrfield*)_tmp47->hd;_tmp51=*_tmp50;
_tmp52=_tmp51.name;_tmp53=_tmp51.tq;_tmp54=(void*)_tmp51.type;_tmp55=_tmp51.width;
_tmp56=_tmp51.attributes;if(Cyc_Std_zstrptrcmp(_tmp4B,_tmp52)!= 0){({struct Cyc_Std_String_pa_struct
_tmp5B;_tmp5B.tag=0;_tmp5B.f1=(struct _tagged_arr)*_tmp52;{struct Cyc_Std_String_pa_struct
_tmp5A;_tmp5A.tag=0;_tmp5A.f1=(struct _tagged_arr)*_tmp4B;{struct Cyc_Std_String_pa_struct
_tmp59;_tmp59.tag=0;_tmp59.f1=(struct _tagged_arr)_tmp35;{struct Cyc_Std_String_pa_struct
_tmp58;_tmp58.tag=0;_tmp58.f1=_tag_arr("type",sizeof(char),5);{void*_tmp57[4]={&
_tmp58,& _tmp59,& _tmp5A,& _tmp5B};Cyc_Tcdecl_merr(loc,msg,_tag_arr("%s %s : field name mismatch %s != %s",
sizeof(char),37),_tag_arr(_tmp57,sizeof(void*),4));}}}}});return 0;}if(!Cyc_Tcutil_same_atts(
_tmp4F,_tmp56)){({struct Cyc_Std_String_pa_struct _tmp5F;_tmp5F.tag=0;_tmp5F.f1=(
struct _tagged_arr)*_tmp4B;{struct Cyc_Std_String_pa_struct _tmp5E;_tmp5E.tag=0;
_tmp5E.f1=(struct _tagged_arr)_tmp35;{struct Cyc_Std_String_pa_struct _tmp5D;_tmp5D.tag=
0;_tmp5D.f1=_tag_arr("type",sizeof(char),5);{void*_tmp5C[3]={& _tmp5D,& _tmp5E,&
_tmp5F};Cyc_Tcdecl_merr(loc,msg,_tag_arr("%s %s : attribute mismatch on field %s",
sizeof(char),39),_tag_arr(_tmp5C,sizeof(void*),3));}}}});_tmp36=0;}if(!Cyc_Tcutil_equal_tqual(
_tmp4C,_tmp53)){({struct Cyc_Std_String_pa_struct _tmp63;_tmp63.tag=0;_tmp63.f1=(
struct _tagged_arr)*_tmp4B;{struct Cyc_Std_String_pa_struct _tmp62;_tmp62.tag=0;
_tmp62.f1=(struct _tagged_arr)_tmp35;{struct Cyc_Std_String_pa_struct _tmp61;_tmp61.tag=
0;_tmp61.f1=_tag_arr("type",sizeof(char),5);{void*_tmp60[3]={& _tmp61,& _tmp62,&
_tmp63};Cyc_Tcdecl_merr(loc,msg,_tag_arr("%s %s : qualifier mismatch on field %s",
sizeof(char),39),_tag_arr(_tmp60,sizeof(void*),3));}}}});_tmp36=0;}if((((_tmp4E
!= 0?_tmp55 != 0: 0)?Cyc_Tcdecl_get_uint_const_value((struct Cyc_Absyn_Exp*)
_check_null(_tmp4E))!= Cyc_Tcdecl_get_uint_const_value((struct Cyc_Absyn_Exp*)
_check_null(_tmp55)): 0)?1:(_tmp4E == 0?_tmp55 != 0: 0))?1:(_tmp4E != 0?_tmp55 == 0: 0)){({
struct Cyc_Std_String_pa_struct _tmp67;_tmp67.tag=0;_tmp67.f1=(struct _tagged_arr)*
_tmp4B;{struct Cyc_Std_String_pa_struct _tmp66;_tmp66.tag=0;_tmp66.f1=(struct
_tagged_arr)_tmp35;{struct Cyc_Std_String_pa_struct _tmp65;_tmp65.tag=0;_tmp65.f1=
_tag_arr("type",sizeof(char),5);{void*_tmp64[3]={& _tmp65,& _tmp66,& _tmp67};Cyc_Tcdecl_merr(
loc,msg,_tag_arr("%s %s : bitfield mismatch on field %s",sizeof(char),38),
_tag_arr(_tmp64,sizeof(void*),3));}}}});_tmp36=0;}{void*subst_t1=Cyc_Tcutil_substitute(
_tmp48,_tmp54);if(!Cyc_Tcdecl_check_type(_tmp4D,subst_t1)){({struct Cyc_Std_String_pa_struct
_tmp6C;_tmp6C.tag=0;_tmp6C.f1=(struct _tagged_arr)Cyc_Absynpp_typ2string(subst_t1);{
struct Cyc_Std_String_pa_struct _tmp6B;_tmp6B.tag=0;_tmp6B.f1=(struct _tagged_arr)
Cyc_Absynpp_typ2string(_tmp4D);{struct Cyc_Std_String_pa_struct _tmp6A;_tmp6A.tag=
0;_tmp6A.f1=(struct _tagged_arr)*_tmp4B;{struct Cyc_Std_String_pa_struct _tmp69;
_tmp69.tag=0;_tmp69.f1=(struct _tagged_arr)_tmp35;{void*_tmp68[4]={& _tmp69,&
_tmp6A,& _tmp6B,& _tmp6C};Cyc_Tcdecl_merr(loc,msg,_tag_arr("type %s : type mismatch on field %s: %s != %s",
sizeof(char),46),_tag_arr(_tmp68,sizeof(void*),4));}}}}});Cyc_Tcutil_explain_failure();
_tmp36=0;}}}}if(_tmp42 != 0){({struct Cyc_Std_String_pa_struct _tmp6F;_tmp6F.tag=0;
_tmp6F.f1=(struct _tagged_arr)*((struct Cyc_Absyn_Aggrfield*)_tmp42->hd)->name;{
struct Cyc_Std_String_pa_struct _tmp6E;_tmp6E.tag=0;_tmp6E.f1=(struct _tagged_arr)
_tmp35;{void*_tmp6D[2]={& _tmp6E,& _tmp6F};Cyc_Tcdecl_merr(loc,msg,_tag_arr("type %s is missing field %s",
sizeof(char),28),_tag_arr(_tmp6D,sizeof(void*),2));}}});_tmp36=0;}if(_tmp47 != 0){({
struct Cyc_Std_String_pa_struct _tmp72;_tmp72.tag=0;_tmp72.f1=(struct _tagged_arr)*((
struct Cyc_Absyn_Aggrfield*)_tmp47->hd)->name;{struct Cyc_Std_String_pa_struct
_tmp71;_tmp71.tag=0;_tmp71.f1=(struct _tagged_arr)_tmp35;{void*_tmp70[2]={& _tmp71,&
_tmp72};Cyc_Tcdecl_merr(loc,msg,_tag_arr("type %s has extra field %s",sizeof(
char),27),_tag_arr(_tmp70,sizeof(void*),2));}}});_tmp36=0;}d2=d0;goto _LL18;}
_LL18:;}if(!_tmp36)return 0;if(_tmp38 == (void*)d2->sc)return(struct Cyc_Absyn_Aggrdecl*)
d2;else{d2=({struct Cyc_Absyn_Aggrdecl*_tmp73=_cycalloc(sizeof(*_tmp73));_tmp73[0]=*
d2;_tmp73;});(void*)(d2->sc=(void*)_tmp38);return(struct Cyc_Absyn_Aggrdecl*)d2;}}}}
inline static struct _tagged_arr Cyc_Tcdecl_is_x2string(int is_x){return is_x?_tag_arr("xtunion",
sizeof(char),8): _tag_arr("tunion",sizeof(char),7);}struct _tuple7{struct Cyc_Absyn_Tqual
f1;void*f2;};static struct Cyc_Absyn_Tunionfield*Cyc_Tcdecl_merge_tunionfield(
struct Cyc_Absyn_Tunionfield*f0,struct Cyc_Absyn_Tunionfield*f1,struct Cyc_List_List*
inst,struct _tagged_arr t,struct _tagged_arr v,struct _tagged_arr*msg){struct Cyc_Position_Segment*
loc=f1->loc;if(Cyc_Std_zstrptrcmp((*f0->name).f2,(*f1->name).f2)!= 0){({struct
Cyc_Std_String_pa_struct _tmp78;_tmp78.tag=0;_tmp78.f1=(struct _tagged_arr)*(*f0->name).f2;{
struct Cyc_Std_String_pa_struct _tmp77;_tmp77.tag=0;_tmp77.f1=(struct _tagged_arr)*(*
f1->name).f2;{struct Cyc_Std_String_pa_struct _tmp76;_tmp76.tag=0;_tmp76.f1=(
struct _tagged_arr)v;{struct Cyc_Std_String_pa_struct _tmp75;_tmp75.tag=0;_tmp75.f1=(
struct _tagged_arr)t;{void*_tmp74[4]={& _tmp75,& _tmp76,& _tmp77,& _tmp78};Cyc_Tcdecl_merr(
loc,msg,_tag_arr("%s %s: field name mismatch %s != %s",sizeof(char),36),_tag_arr(
_tmp74,sizeof(void*),4));}}}}});return 0;}{struct _tagged_arr _tmp79=*(*f0->name).f2;
void*_tmp7E;int _tmp7F;struct _tuple3 _tmp7D=Cyc_Tcdecl_merge_scope((void*)f0->sc,(
void*)f1->sc,(struct _tagged_arr)({struct Cyc_Std_String_pa_struct _tmp7C;_tmp7C.tag=
0;_tmp7C.f1=(struct _tagged_arr)v;{struct Cyc_Std_String_pa_struct _tmp7B;_tmp7B.tag=
0;_tmp7B.f1=(struct _tagged_arr)t;{void*_tmp7A[2]={& _tmp7B,& _tmp7C};Cyc_Std_aprintf(
_tag_arr("in %s %s, field",sizeof(char),16),_tag_arr(_tmp7A,sizeof(void*),2));}}}),
_tmp79,loc,msg);_tmp7E=_tmp7D.f1;_tmp7F=_tmp7D.f2;{struct Cyc_List_List*_tmp80=f0->typs;
struct Cyc_List_List*_tmp81=f1->typs;if(((int(*)(struct Cyc_List_List*x))Cyc_List_length)(
_tmp80)!= ((int(*)(struct Cyc_List_List*x))Cyc_List_length)(_tmp81)){({struct Cyc_Std_String_pa_struct
_tmp85;_tmp85.tag=0;_tmp85.f1=(struct _tagged_arr)_tmp79;{struct Cyc_Std_String_pa_struct
_tmp84;_tmp84.tag=0;_tmp84.f1=(struct _tagged_arr)v;{struct Cyc_Std_String_pa_struct
_tmp83;_tmp83.tag=0;_tmp83.f1=(struct _tagged_arr)t;{void*_tmp82[3]={& _tmp83,&
_tmp84,& _tmp85};Cyc_Tcdecl_merr(loc,msg,_tag_arr("%s %s, field %s: parameter number mismatch",
sizeof(char),43),_tag_arr(_tmp82,sizeof(void*),3));}}}});_tmp7F=0;}for(0;_tmp80
!= 0;(_tmp80=_tmp80->tl,_tmp81=_tmp81->tl)){if(!Cyc_Tcutil_equal_tqual((*((
struct _tuple7*)_tmp80->hd)).f1,(*((struct _tuple7*)((struct Cyc_List_List*)
_check_null(_tmp81))->hd)).f1)){({struct Cyc_Std_String_pa_struct _tmp89;_tmp89.tag=
0;_tmp89.f1=(struct _tagged_arr)_tmp79;{struct Cyc_Std_String_pa_struct _tmp88;
_tmp88.tag=0;_tmp88.f1=(struct _tagged_arr)v;{struct Cyc_Std_String_pa_struct
_tmp87;_tmp87.tag=0;_tmp87.f1=(struct _tagged_arr)t;{void*_tmp86[3]={& _tmp87,&
_tmp88,& _tmp89};Cyc_Tcdecl_merr(loc,msg,_tag_arr("%s %s, field %s: parameter qualifier",
sizeof(char),37),_tag_arr(_tmp86,sizeof(void*),3));}}}});_tmp7F=0;}{void*
subst_t1=Cyc_Tcutil_substitute(inst,(*((struct _tuple7*)_tmp81->hd)).f2);if(!Cyc_Tcdecl_check_type((*((
struct _tuple7*)_tmp80->hd)).f2,subst_t1)){({struct Cyc_Std_String_pa_struct _tmp8F;
_tmp8F.tag=0;_tmp8F.f1=(struct _tagged_arr)Cyc_Absynpp_typ2string(subst_t1);{
struct Cyc_Std_String_pa_struct _tmp8E;_tmp8E.tag=0;_tmp8E.f1=(struct _tagged_arr)
Cyc_Absynpp_typ2string((*((struct _tuple7*)_tmp80->hd)).f2);{struct Cyc_Std_String_pa_struct
_tmp8D;_tmp8D.tag=0;_tmp8D.f1=(struct _tagged_arr)_tmp79;{struct Cyc_Std_String_pa_struct
_tmp8C;_tmp8C.tag=0;_tmp8C.f1=(struct _tagged_arr)v;{struct Cyc_Std_String_pa_struct
_tmp8B;_tmp8B.tag=0;_tmp8B.f1=(struct _tagged_arr)t;{void*_tmp8A[5]={& _tmp8B,&
_tmp8C,& _tmp8D,& _tmp8E,& _tmp8F};Cyc_Tcdecl_merr(loc,msg,_tag_arr("%s %s, field %s: parameter type mismatch %s != %s",
sizeof(char),50),_tag_arr(_tmp8A,sizeof(void*),5));}}}}}});Cyc_Tcutil_explain_failure();
_tmp7F=0;}}}if(!_tmp7F)return 0;if((void*)f0->sc != _tmp7E){struct Cyc_Absyn_Tunionfield*
_tmp90=({struct Cyc_Absyn_Tunionfield*_tmp91=_cycalloc(sizeof(*_tmp91));_tmp91[0]=*
f0;_tmp91;});(void*)(((struct Cyc_Absyn_Tunionfield*)_check_null(_tmp90))->sc=(
void*)_tmp7E);return _tmp90;}else{return(struct Cyc_Absyn_Tunionfield*)f0;}}}}
static struct _tuple7*Cyc_Tcdecl_substitute_tunionfield_f2(struct Cyc_List_List*
inst,struct _tuple7*x){struct _tuple7 _tmp93;struct Cyc_Absyn_Tqual _tmp94;void*
_tmp95;struct _tuple7*_tmp92=x;_tmp93=*_tmp92;_tmp94=_tmp93.f1;_tmp95=_tmp93.f2;
return({struct _tuple7*_tmp96=_cycalloc(sizeof(*_tmp96));_tmp96->f1=_tmp94;_tmp96->f2=
Cyc_Tcutil_substitute(inst,_tmp95);_tmp96;});}static struct Cyc_Absyn_Tunionfield*
Cyc_Tcdecl_substitute_tunionfield(struct Cyc_List_List*inst1,struct Cyc_Absyn_Tunionfield*
f1){struct Cyc_Absyn_Tunionfield*_tmp97=({struct Cyc_Absyn_Tunionfield*_tmp98=
_cycalloc(sizeof(*_tmp98));_tmp98[0]=*f1;_tmp98;});_tmp97->typs=((struct Cyc_List_List*(*)(
struct _tuple7*(*f)(struct Cyc_List_List*,struct _tuple7*),struct Cyc_List_List*env,
struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Tcdecl_substitute_tunionfield_f2,
inst1,f1->typs);return _tmp97;}static struct Cyc_List_List*Cyc_Tcdecl_merge_xtunion_fields(
struct Cyc_List_List*f0s,struct Cyc_List_List*f1s,struct Cyc_List_List*inst,struct
Cyc_List_List*tvs0,struct Cyc_List_List*tvs1,int*res,int*incl,struct _tagged_arr t,
struct _tagged_arr v,struct Cyc_Position_Segment*loc,struct _tagged_arr*msg){struct
Cyc_List_List**f2sp=({struct Cyc_List_List**_tmpA1=_cycalloc(sizeof(*_tmpA1));
_tmpA1[0]=0;_tmpA1;});struct Cyc_List_List**_tmp99=f2sp;int cmp=- 1;for(0;f0s != 0?
f1s != 0: 0;f1s=f1s->tl){while(f0s != 0?(cmp=Cyc_Absyn_qvar_cmp(((struct Cyc_Absyn_Tunionfield*)
f0s->hd)->name,((struct Cyc_Absyn_Tunionfield*)f1s->hd)->name))< 0: 0){struct Cyc_List_List*
_tmp9A=({struct Cyc_List_List*_tmp9B=_cycalloc(sizeof(*_tmp9B));_tmp9B->hd=(
struct Cyc_Absyn_Tunionfield*)f0s->hd;_tmp9B->tl=0;_tmp9B;});*_tmp99=_tmp9A;
_tmp99=&((struct Cyc_List_List*)_check_null(_tmp9A))->tl;f0s=f0s->tl;}if(f0s == 0?
1: cmp > 0){*incl=0;{struct Cyc_List_List*_tmp9C=({struct Cyc_List_List*_tmp9D=
_cycalloc(sizeof(*_tmp9D));_tmp9D->hd=Cyc_Tcdecl_substitute_tunionfield(inst,(
struct Cyc_Absyn_Tunionfield*)f1s->hd);_tmp9D->tl=0;_tmp9D;});*_tmp99=_tmp9C;
_tmp99=&((struct Cyc_List_List*)_check_null(_tmp9C))->tl;}}else{struct Cyc_Absyn_Tunionfield*
_tmp9E=Cyc_Tcdecl_merge_tunionfield((struct Cyc_Absyn_Tunionfield*)f0s->hd,(
struct Cyc_Absyn_Tunionfield*)f1s->hd,inst,t,v,msg);if(_tmp9E != 0){if(_tmp9E != (
struct Cyc_Absyn_Tunionfield*)((struct Cyc_Absyn_Tunionfield*)f0s->hd))*incl=0;{
struct Cyc_List_List*_tmp9F=({struct Cyc_List_List*_tmpA0=_cycalloc(sizeof(*_tmpA0));
_tmpA0->hd=(struct Cyc_Absyn_Tunionfield*)_check_null(_tmp9E);_tmpA0->tl=0;_tmpA0;});*
_tmp99=_tmp9F;_tmp99=&((struct Cyc_List_List*)_check_null(_tmp9F))->tl;}}else{*
res=0;}f0s=f0s->tl;}}if(f1s != 0){*incl=0;*_tmp99=f1s;}else{*_tmp99=f0s;}return*
f2sp;}struct _tuple8{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};static struct
_tuple8 Cyc_Tcdecl_split(struct Cyc_List_List*f){if(f == 0)return({struct _tuple8
_tmpA2;_tmpA2.f1=0;_tmpA2.f2=0;_tmpA2;});if(f->tl == 0)return({struct _tuple8
_tmpA3;_tmpA3.f1=f;_tmpA3.f2=0;_tmpA3;});{struct Cyc_List_List*_tmpA5;struct Cyc_List_List*
_tmpA6;struct _tuple8 _tmpA4=Cyc_Tcdecl_split(((struct Cyc_List_List*)_check_null(f->tl))->tl);
_tmpA5=_tmpA4.f1;_tmpA6=_tmpA4.f2;return({struct _tuple8 _tmpA7;_tmpA7.f1=({struct
Cyc_List_List*_tmpA9=_cycalloc(sizeof(*_tmpA9));_tmpA9->hd=(void*)((void*)f->hd);
_tmpA9->tl=_tmpA5;_tmpA9;});_tmpA7.f2=({struct Cyc_List_List*_tmpA8=_cycalloc(
sizeof(*_tmpA8));_tmpA8->hd=(void*)((void*)((struct Cyc_List_List*)_check_null(f->tl))->hd);
_tmpA8->tl=_tmpA6;_tmpA8;});_tmpA7;});}}struct Cyc_List_List*Cyc_Tcdecl_sort_xtunion_fields(
struct Cyc_List_List*f,int*res,struct _tagged_arr*v,struct Cyc_Position_Segment*loc,
struct _tagged_arr*msg){struct Cyc_List_List*_tmpAB;struct Cyc_List_List*_tmpAC;
struct _tuple8 _tmpAA=((struct _tuple8(*)(struct Cyc_List_List*f))Cyc_Tcdecl_split)(
f);_tmpAB=_tmpAA.f1;_tmpAC=_tmpAA.f2;if(_tmpAB != 0?_tmpAB->tl != 0: 0)_tmpAB=Cyc_Tcdecl_sort_xtunion_fields(
_tmpAB,res,v,loc,msg);if(_tmpAC != 0?_tmpAC->tl != 0: 0)_tmpAC=Cyc_Tcdecl_sort_xtunion_fields(
_tmpAC,res,v,loc,msg);return Cyc_Tcdecl_merge_xtunion_fields(_tmpAB,_tmpAC,0,0,0,
res,({int*_tmpAD=_cycalloc_atomic(sizeof(*_tmpAD));_tmpAD[0]=1;_tmpAD;}),
_tag_arr("xtunion",sizeof(char),8),*v,loc,msg);}struct _tuple9{struct Cyc_Core_Opt*
f1;struct Cyc_Core_Opt*f2;};struct Cyc_Absyn_Tuniondecl*Cyc_Tcdecl_merge_tuniondecl(
struct Cyc_Absyn_Tuniondecl*d0,struct Cyc_Absyn_Tuniondecl*d1,struct Cyc_Position_Segment*
loc,struct _tagged_arr*msg){struct _tagged_arr _tmpAE=Cyc_Absynpp_qvar2string(d0->name);
struct _tagged_arr t=_tag_arr("[x]tunion",sizeof(char),10);int _tmpAF=1;if(d0->is_xtunion
!= d1->is_xtunion){({struct Cyc_Std_String_pa_struct _tmpB3;_tmpB3.tag=0;_tmpB3.f1=(
struct _tagged_arr)Cyc_Tcdecl_is_x2string(d1->is_xtunion);{struct Cyc_Std_String_pa_struct
_tmpB2;_tmpB2.tag=0;_tmpB2.f1=(struct _tagged_arr)Cyc_Tcdecl_is_x2string(d0->is_xtunion);{
struct Cyc_Std_String_pa_struct _tmpB1;_tmpB1.tag=0;_tmpB1.f1=(struct _tagged_arr)
_tmpAE;{void*_tmpB0[3]={& _tmpB1,& _tmpB2,& _tmpB3};Cyc_Tcdecl_merr(loc,msg,
_tag_arr("expected %s to be a %s instead of a %s",sizeof(char),39),_tag_arr(
_tmpB0,sizeof(void*),3));}}}});_tmpAF=0;}else{t=Cyc_Tcdecl_is_x2string(d0->is_xtunion);}
if(!Cyc_Tcdecl_check_tvs(d0->tvs,d1->tvs,t,_tmpAE,loc,msg))return 0;{void*_tmpB5;
int _tmpB6;struct _tuple3 _tmpB4=Cyc_Tcdecl_merge_scope((void*)d0->sc,(void*)d1->sc,
t,_tmpAE,loc,msg);_tmpB5=_tmpB4.f1;_tmpB6=_tmpB4.f2;if(!_tmpB6)_tmpAF=0;{struct
Cyc_Absyn_Tuniondecl*d2;{struct _tuple9 _tmpB8=({struct _tuple9 _tmpB7;_tmpB7.f1=d0->fields;
_tmpB7.f2=d1->fields;_tmpB7;});struct Cyc_Core_Opt*_tmpB9;struct Cyc_Core_Opt*
_tmpBA;struct Cyc_Core_Opt*_tmpBB;struct Cyc_Core_Opt _tmpBC;struct Cyc_List_List*
_tmpBD;struct Cyc_Core_Opt*_tmpBE;struct Cyc_Core_Opt _tmpBF;struct Cyc_List_List*
_tmpC0;_LL20: _tmpB9=_tmpB8.f2;if(_tmpB9 != 0)goto _LL22;_LL21: d2=d0;goto _LL1F;
_LL22: _tmpBA=_tmpB8.f1;if(_tmpBA != 0)goto _LL24;_LL23: d2=d1;goto _LL1F;_LL24:
_tmpBB=_tmpB8.f1;if(_tmpBB == 0)goto _LL1F;_tmpBC=*_tmpBB;_tmpBD=(struct Cyc_List_List*)
_tmpBC.v;_tmpBE=_tmpB8.f2;if(_tmpBE == 0)goto _LL1F;_tmpBF=*_tmpBE;_tmpC0=(struct
Cyc_List_List*)_tmpBF.v;_LL25: {struct Cyc_List_List*_tmpC1=Cyc_Tcdecl_build_tvs_map(
d0->tvs,d1->tvs);if(d0->is_xtunion){int _tmpC2=1;struct Cyc_List_List*_tmpC3=Cyc_Tcdecl_merge_xtunion_fields(
_tmpBD,_tmpC0,_tmpC1,d0->tvs,d1->tvs,& _tmpAF,& _tmpC2,t,_tmpAE,loc,msg);if(_tmpC2)
d2=d0;else{d2=({struct Cyc_Absyn_Tuniondecl*_tmpC4=_cycalloc(sizeof(*_tmpC4));
_tmpC4[0]=*d0;_tmpC4;});(void*)(d2->sc=(void*)_tmpB5);d2->fields=({struct Cyc_Core_Opt*
_tmpC5=_cycalloc(sizeof(*_tmpC5));_tmpC5->v=_tmpC3;_tmpC5;});}}else{for(0;_tmpBD
!= 0?_tmpC0 != 0: 0;(_tmpBD=_tmpBD->tl,_tmpC0=_tmpC0->tl)){Cyc_Tcdecl_merge_tunionfield((
struct Cyc_Absyn_Tunionfield*)_tmpBD->hd,(struct Cyc_Absyn_Tunionfield*)_tmpC0->hd,
_tmpC1,t,_tmpAE,msg);}if(_tmpBD != 0){({struct Cyc_Std_String_pa_struct _tmpC9;
_tmpC9.tag=0;_tmpC9.f1=(struct _tagged_arr)*(*((struct Cyc_Absyn_Tunionfield*)((
struct Cyc_List_List*)_check_null(_tmpC0))->hd)->name).f2;{struct Cyc_Std_String_pa_struct
_tmpC8;_tmpC8.tag=0;_tmpC8.f1=(struct _tagged_arr)_tmpAE;{struct Cyc_Std_String_pa_struct
_tmpC7;_tmpC7.tag=0;_tmpC7.f1=(struct _tagged_arr)t;{void*_tmpC6[3]={& _tmpC7,&
_tmpC8,& _tmpC9};Cyc_Tcdecl_merr(loc,msg,_tag_arr("%s %s has extra field %s",
sizeof(char),25),_tag_arr(_tmpC6,sizeof(void*),3));}}}});_tmpAF=0;}if(_tmpC0 != 0){({
struct Cyc_Std_String_pa_struct _tmpCD;_tmpCD.tag=0;_tmpCD.f1=(struct _tagged_arr)*(*((
struct Cyc_Absyn_Tunionfield*)_tmpC0->hd)->name).f2;{struct Cyc_Std_String_pa_struct
_tmpCC;_tmpCC.tag=0;_tmpCC.f1=(struct _tagged_arr)_tmpAE;{struct Cyc_Std_String_pa_struct
_tmpCB;_tmpCB.tag=0;_tmpCB.f1=(struct _tagged_arr)t;{void*_tmpCA[3]={& _tmpCB,&
_tmpCC,& _tmpCD};Cyc_Tcdecl_merr(loc,msg,_tag_arr("%s %s is missing field %s",
sizeof(char),26),_tag_arr(_tmpCA,sizeof(void*),3));}}}});_tmpAF=0;}d2=d0;}goto
_LL1F;}_LL1F:;}if(!_tmpAF)return 0;if(_tmpB5 == (void*)d2->sc)return(struct Cyc_Absyn_Tuniondecl*)
d2;else{d2=({struct Cyc_Absyn_Tuniondecl*_tmpCE=_cycalloc(sizeof(*_tmpCE));_tmpCE[
0]=*d2;_tmpCE;});(void*)(d2->sc=(void*)_tmpB5);return(struct Cyc_Absyn_Tuniondecl*)
d2;}}}}struct Cyc_Absyn_Enumdecl*Cyc_Tcdecl_merge_enumdecl(struct Cyc_Absyn_Enumdecl*
d0,struct Cyc_Absyn_Enumdecl*d1,struct Cyc_Position_Segment*loc,struct _tagged_arr*
msg){struct _tagged_arr _tmpCF=Cyc_Absynpp_qvar2string(d0->name);int _tmpD0=1;void*
_tmpD2;int _tmpD3;struct _tuple3 _tmpD1=Cyc_Tcdecl_merge_scope((void*)d0->sc,(void*)
d1->sc,_tag_arr("enum",sizeof(char),5),_tmpCF,loc,msg);_tmpD2=_tmpD1.f1;_tmpD3=
_tmpD1.f2;if(!_tmpD3)_tmpD0=0;{struct Cyc_Absyn_Enumdecl*d2;{struct _tuple9 _tmpD5=({
struct _tuple9 _tmpD4;_tmpD4.f1=d0->fields;_tmpD4.f2=d1->fields;_tmpD4;});struct
Cyc_Core_Opt*_tmpD6;struct Cyc_Core_Opt*_tmpD7;struct Cyc_Core_Opt*_tmpD8;struct
Cyc_Core_Opt _tmpD9;struct Cyc_List_List*_tmpDA;struct Cyc_Core_Opt*_tmpDB;struct
Cyc_Core_Opt _tmpDC;struct Cyc_List_List*_tmpDD;_LL27: _tmpD6=_tmpD5.f2;if(_tmpD6 != 
0)goto _LL29;_LL28: d2=d0;goto _LL26;_LL29: _tmpD7=_tmpD5.f1;if(_tmpD7 != 0)goto _LL2B;
_LL2A: d2=d1;goto _LL26;_LL2B: _tmpD8=_tmpD5.f1;if(_tmpD8 == 0)goto _LL26;_tmpD9=*
_tmpD8;_tmpDA=(struct Cyc_List_List*)_tmpD9.v;_tmpDB=_tmpD5.f2;if(_tmpDB == 0)goto
_LL26;_tmpDC=*_tmpDB;_tmpDD=(struct Cyc_List_List*)_tmpDC.v;_LL2C: for(0;_tmpDA != 
0?_tmpDD != 0: 0;(_tmpDA=_tmpDA->tl,_tmpDD=_tmpDD->tl)){struct Cyc_Absyn_Enumfield
_tmpDF;struct _tuple0*_tmpE0;struct _tuple0 _tmpE1;struct _tagged_arr*_tmpE2;struct
Cyc_Absyn_Exp*_tmpE3;struct Cyc_Position_Segment*_tmpE4;struct Cyc_Absyn_Enumfield*
_tmpDE=(struct Cyc_Absyn_Enumfield*)_tmpDA->hd;_tmpDF=*_tmpDE;_tmpE0=_tmpDF.name;
_tmpE1=*_tmpE0;_tmpE2=_tmpE1.f2;_tmpE3=_tmpDF.tag;_tmpE4=_tmpDF.loc;{struct Cyc_Absyn_Enumfield
_tmpE6;struct _tuple0*_tmpE7;struct _tuple0 _tmpE8;struct _tagged_arr*_tmpE9;struct
Cyc_Absyn_Exp*_tmpEA;struct Cyc_Position_Segment*_tmpEB;struct Cyc_Absyn_Enumfield*
_tmpE5=(struct Cyc_Absyn_Enumfield*)_tmpDD->hd;_tmpE6=*_tmpE5;_tmpE7=_tmpE6.name;
_tmpE8=*_tmpE7;_tmpE9=_tmpE8.f2;_tmpEA=_tmpE6.tag;_tmpEB=_tmpE6.loc;if(Cyc_Std_zstrptrcmp(
_tmpE2,_tmpE9)!= 0){({struct Cyc_Std_String_pa_struct _tmpEF;_tmpEF.tag=0;_tmpEF.f1=(
struct _tagged_arr)*_tmpE9;{struct Cyc_Std_String_pa_struct _tmpEE;_tmpEE.tag=0;
_tmpEE.f1=(struct _tagged_arr)*_tmpE2;{struct Cyc_Std_String_pa_struct _tmpED;
_tmpED.tag=0;_tmpED.f1=(struct _tagged_arr)_tmpCF;{void*_tmpEC[3]={& _tmpED,&
_tmpEE,& _tmpEF};Cyc_Tcdecl_merr(loc,msg,_tag_arr("enum %s: field name mismatch %s != %s",
sizeof(char),38),_tag_arr(_tmpEC,sizeof(void*),3));}}}});_tmpD0=0;}if(Cyc_Tcdecl_get_uint_const_value((
struct Cyc_Absyn_Exp*)_check_null(_tmpE3))!= Cyc_Tcdecl_get_uint_const_value((
struct Cyc_Absyn_Exp*)_check_null(_tmpEA))){({struct Cyc_Std_String_pa_struct
_tmpF2;_tmpF2.tag=0;_tmpF2.f1=(struct _tagged_arr)*_tmpE9;{struct Cyc_Std_String_pa_struct
_tmpF1;_tmpF1.tag=0;_tmpF1.f1=(struct _tagged_arr)_tmpCF;{void*_tmpF0[2]={& _tmpF1,&
_tmpF2};Cyc_Tcdecl_merr(loc,msg,_tag_arr("enum %s, field %s, value mismatch",
sizeof(char),34),_tag_arr(_tmpF0,sizeof(void*),2));}}});_tmpD0=0;}}}d2=d0;goto
_LL26;_LL26:;}if(!_tmpD0)return 0;if((void*)d2->sc == _tmpD2)return(struct Cyc_Absyn_Enumdecl*)
d2;else{d2=({struct Cyc_Absyn_Enumdecl*_tmpF3=_cycalloc(sizeof(*_tmpF3));_tmpF3[0]=*
d2;_tmpF3;});(void*)(d2->sc=(void*)_tmpD2);return(struct Cyc_Absyn_Enumdecl*)d2;}}}
static struct _tuple3 Cyc_Tcdecl_check_var_or_fn_decl(void*sc0,void*t0,struct Cyc_Absyn_Tqual
tq0,struct Cyc_List_List*atts0,void*sc1,void*t1,struct Cyc_Absyn_Tqual tq1,struct
Cyc_List_List*atts1,struct _tagged_arr t,struct _tagged_arr v,struct Cyc_Position_Segment*
loc,struct _tagged_arr*msg){int _tmpF4=1;void*_tmpF6;int _tmpF7;struct _tuple3 _tmpF5=
Cyc_Tcdecl_merge_scope(sc0,sc1,t,v,loc,msg);_tmpF6=_tmpF5.f1;_tmpF7=_tmpF5.f2;
if(!_tmpF7)_tmpF4=0;if(!Cyc_Tcdecl_check_type(t0,t1)){({struct Cyc_Std_String_pa_struct
_tmpFC;_tmpFC.tag=0;_tmpFC.f1=(struct _tagged_arr)Cyc_Absynpp_typ2string(t0);{
struct Cyc_Std_String_pa_struct _tmpFB;_tmpFB.tag=0;_tmpFB.f1=(struct _tagged_arr)
Cyc_Absynpp_typ2string(t1);{struct Cyc_Std_String_pa_struct _tmpFA;_tmpFA.tag=0;
_tmpFA.f1=(struct _tagged_arr)v;{struct Cyc_Std_String_pa_struct _tmpF9;_tmpF9.tag=
0;_tmpF9.f1=(struct _tagged_arr)t;{void*_tmpF8[4]={& _tmpF9,& _tmpFA,& _tmpFB,&
_tmpFC};Cyc_Tcdecl_merr(loc,msg,_tag_arr("%s %s has type \n%s\n instead of \n%s\n",
sizeof(char),36),_tag_arr(_tmpF8,sizeof(void*),4));}}}}});Cyc_Tcutil_explain_failure();
_tmpF4=0;}if(!Cyc_Tcutil_equal_tqual(tq0,tq1)){({struct Cyc_Std_String_pa_struct
_tmpFF;_tmpFF.tag=0;_tmpFF.f1=(struct _tagged_arr)v;{struct Cyc_Std_String_pa_struct
_tmpFE;_tmpFE.tag=0;_tmpFE.f1=(struct _tagged_arr)t;{void*_tmpFD[2]={& _tmpFE,&
_tmpFF};Cyc_Tcdecl_merr(loc,msg,_tag_arr("%s %s has different type qualifiers",
sizeof(char),36),_tag_arr(_tmpFD,sizeof(void*),2));}}});_tmpF4=0;}if(!Cyc_Tcutil_same_atts(
atts0,atts1)){({struct Cyc_Std_String_pa_struct _tmp102;_tmp102.tag=0;_tmp102.f1=(
struct _tagged_arr)v;{struct Cyc_Std_String_pa_struct _tmp101;_tmp101.tag=0;_tmp101.f1=(
struct _tagged_arr)t;{void*_tmp100[2]={& _tmp101,& _tmp102};Cyc_Tcdecl_merr(loc,msg,
_tag_arr("%s %s has different attributes",sizeof(char),31),_tag_arr(_tmp100,
sizeof(void*),2));}}});({void*_tmp103[0]={};Cyc_Std_fprintf(Cyc_Std_stderr,
_tag_arr("previous attributes: ",sizeof(char),22),_tag_arr(_tmp103,sizeof(void*),
0));});for(0;atts0 != 0;atts0=atts0->tl){({struct Cyc_Std_String_pa_struct _tmp105;
_tmp105.tag=0;_tmp105.f1=(struct _tagged_arr)Cyc_Absyn_attribute2string((void*)
atts0->hd);{void*_tmp104[1]={& _tmp105};Cyc_Std_fprintf(Cyc_Std_stderr,_tag_arr("%s ",
sizeof(char),4),_tag_arr(_tmp104,sizeof(void*),1));}});}({void*_tmp106[0]={};Cyc_Std_fprintf(
Cyc_Std_stderr,_tag_arr("\ncurrent attributes: ",sizeof(char),22),_tag_arr(
_tmp106,sizeof(void*),0));});for(0;atts1 != 0;atts1=atts1->tl){({struct Cyc_Std_String_pa_struct
_tmp108;_tmp108.tag=0;_tmp108.f1=(struct _tagged_arr)Cyc_Absyn_attribute2string((
void*)atts1->hd);{void*_tmp107[1]={& _tmp108};Cyc_Std_fprintf(Cyc_Std_stderr,
_tag_arr("%s ",sizeof(char),4),_tag_arr(_tmp107,sizeof(void*),1));}});}({void*
_tmp109[0]={};Cyc_Std_fprintf(Cyc_Std_stderr,_tag_arr("\n",sizeof(char),2),
_tag_arr(_tmp109,sizeof(void*),0));});_tmpF4=0;}return({struct _tuple3 _tmp10A;
_tmp10A.f1=_tmpF6;_tmp10A.f2=_tmpF4;_tmp10A;});}struct Cyc_Absyn_Vardecl*Cyc_Tcdecl_merge_vardecl(
struct Cyc_Absyn_Vardecl*d0,struct Cyc_Absyn_Vardecl*d1,struct Cyc_Position_Segment*
loc,struct _tagged_arr*msg){struct _tagged_arr _tmp10B=Cyc_Absynpp_qvar2string(d0->name);
void*_tmp10D;int _tmp10E;struct _tuple3 _tmp10C=Cyc_Tcdecl_check_var_or_fn_decl((
void*)d0->sc,(void*)d0->type,d0->tq,d0->attributes,(void*)d1->sc,(void*)d1->type,
d1->tq,d1->attributes,_tag_arr("variable",sizeof(char),9),_tmp10B,loc,msg);
_tmp10D=_tmp10C.f1;_tmp10E=_tmp10C.f2;if(!_tmp10E)return 0;if((void*)d0->sc == 
_tmp10D)return(struct Cyc_Absyn_Vardecl*)d0;else{struct Cyc_Absyn_Vardecl*_tmp10F=({
struct Cyc_Absyn_Vardecl*_tmp110=_cycalloc(sizeof(*_tmp110));_tmp110[0]=*d0;
_tmp110;});(void*)(((struct Cyc_Absyn_Vardecl*)_check_null(_tmp10F))->sc=(void*)
_tmp10D);return _tmp10F;}}struct Cyc_Absyn_Typedefdecl*Cyc_Tcdecl_merge_typedefdecl(
struct Cyc_Absyn_Typedefdecl*d0,struct Cyc_Absyn_Typedefdecl*d1,struct Cyc_Position_Segment*
loc,struct _tagged_arr*msg){struct _tagged_arr _tmp111=Cyc_Absynpp_qvar2string(d0->name);
if(!Cyc_Tcdecl_check_tvs(d0->tvs,d1->tvs,_tag_arr("typedef",sizeof(char),8),
_tmp111,loc,msg))return 0;{struct Cyc_List_List*_tmp112=Cyc_Tcdecl_build_tvs_map(
d0->tvs,d1->tvs);if(d0->defn != 0?d1->defn != 0: 0){void*subst_defn1=Cyc_Tcutil_substitute(
_tmp112,(void*)((struct Cyc_Core_Opt*)_check_null(d1->defn))->v);if(!Cyc_Tcdecl_check_type((
void*)((struct Cyc_Core_Opt*)_check_null(d0->defn))->v,subst_defn1)){({struct Cyc_Std_String_pa_struct
_tmp116;_tmp116.tag=0;_tmp116.f1=(struct _tagged_arr)Cyc_Absynpp_typ2string((void*)((
struct Cyc_Core_Opt*)_check_null(d0->defn))->v);{struct Cyc_Std_String_pa_struct
_tmp115;_tmp115.tag=0;_tmp115.f1=(struct _tagged_arr)Cyc_Absynpp_typ2string(
subst_defn1);{struct Cyc_Std_String_pa_struct _tmp114;_tmp114.tag=0;_tmp114.f1=(
struct _tagged_arr)_tmp111;{void*_tmp113[3]={& _tmp114,& _tmp115,& _tmp116};Cyc_Tcdecl_merr(
loc,msg,_tag_arr("typedef %s does not refer to the same type: %s != %s",sizeof(
char),53),_tag_arr(_tmp113,sizeof(void*),3));}}}});return 0;}}return(struct Cyc_Absyn_Typedefdecl*)
d0;}}void*Cyc_Tcdecl_merge_binding(void*b0,void*b1,struct Cyc_Position_Segment*
loc,struct _tagged_arr*msg){struct _tuple4 _tmp118=({struct _tuple4 _tmp117;_tmp117.f1=
b0;_tmp117.f2=b1;_tmp117;});void*_tmp119;void*_tmp11A;void*_tmp11B;struct Cyc_Absyn_Vardecl*
_tmp11C;void*_tmp11D;struct Cyc_Absyn_Vardecl*_tmp11E;void*_tmp11F;struct Cyc_Absyn_Vardecl*
_tmp120;void*_tmp121;struct Cyc_Absyn_Fndecl*_tmp122;void*_tmp123;void*_tmp124;
struct Cyc_Absyn_Fndecl*_tmp125;void*_tmp126;struct Cyc_Absyn_Fndecl*_tmp127;void*
_tmp128;struct Cyc_Absyn_Vardecl*_tmp129;_LL2E: _tmp119=_tmp118.f1;if((int)_tmp119
!= 0)goto _LL30;_tmp11A=_tmp118.f2;if((int)_tmp11A != 0)goto _LL30;_LL2F: return(
void*)0;_LL30: _tmp11B=_tmp118.f1;if(_tmp11B <= (void*)1?1:*((int*)_tmp11B)!= 0)
goto _LL32;_tmp11C=((struct Cyc_Absyn_Global_b_struct*)_tmp11B)->f1;_tmp11D=
_tmp118.f2;if(_tmp11D <= (void*)1?1:*((int*)_tmp11D)!= 0)goto _LL32;_tmp11E=((
struct Cyc_Absyn_Global_b_struct*)_tmp11D)->f1;_LL31: {struct Cyc_Absyn_Vardecl*
_tmp12A=Cyc_Tcdecl_merge_vardecl(_tmp11C,_tmp11E,loc,msg);if(_tmp12A == 0)return(
void*)0;if(_tmp12A == (struct Cyc_Absyn_Vardecl*)_tmp11C)return b0;if(_tmp12A == (
struct Cyc_Absyn_Vardecl*)_tmp11E)return b1;return(void*)({struct Cyc_Absyn_Global_b_struct*
_tmp12B=_cycalloc(sizeof(*_tmp12B));_tmp12B[0]=({struct Cyc_Absyn_Global_b_struct
_tmp12C;_tmp12C.tag=0;_tmp12C.f1=(struct Cyc_Absyn_Vardecl*)_check_null(_tmp12A);
_tmp12C;});_tmp12B;});}_LL32: _tmp11F=_tmp118.f1;if(_tmp11F <= (void*)1?1:*((int*)
_tmp11F)!= 0)goto _LL34;_tmp120=((struct Cyc_Absyn_Global_b_struct*)_tmp11F)->f1;
_tmp121=_tmp118.f2;if(_tmp121 <= (void*)1?1:*((int*)_tmp121)!= 1)goto _LL34;
_tmp122=((struct Cyc_Absyn_Funname_b_struct*)_tmp121)->f1;_LL33: {int _tmp12E;
struct _tuple3 _tmp12D=Cyc_Tcdecl_check_var_or_fn_decl((void*)_tmp120->sc,(void*)
_tmp120->type,_tmp120->tq,_tmp120->attributes,(void*)_tmp122->sc,(void*)((struct
Cyc_Core_Opt*)_check_null(_tmp122->cached_typ))->v,Cyc_Absyn_empty_tqual(),
_tmp122->attributes,_tag_arr("function",sizeof(char),9),Cyc_Absynpp_qvar2string(
_tmp120->name),loc,msg);_tmp12E=_tmp12D.f2;return _tmp12E?b1:(void*)0;}_LL34:
_tmp123=_tmp118.f1;if(_tmp123 <= (void*)1?1:*((int*)_tmp123)!= 1)goto _LL36;
_tmp124=_tmp118.f2;if(_tmp124 <= (void*)1?1:*((int*)_tmp124)!= 1)goto _LL36;
_tmp125=((struct Cyc_Absyn_Funname_b_struct*)_tmp124)->f1;_LL35:({struct Cyc_Std_String_pa_struct
_tmp130;_tmp130.tag=0;_tmp130.f1=(struct _tagged_arr)Cyc_Absynpp_qvar2string(
_tmp125->name);{void*_tmp12F[1]={& _tmp130};Cyc_Tcdecl_merr(loc,msg,_tag_arr("redefinition of function %s",
sizeof(char),28),_tag_arr(_tmp12F,sizeof(void*),1));}});return(void*)0;_LL36:
_tmp126=_tmp118.f1;if(_tmp126 <= (void*)1?1:*((int*)_tmp126)!= 1)goto _LL38;
_tmp127=((struct Cyc_Absyn_Funname_b_struct*)_tmp126)->f1;_tmp128=_tmp118.f2;if(
_tmp128 <= (void*)1?1:*((int*)_tmp128)!= 0)goto _LL38;_tmp129=((struct Cyc_Absyn_Global_b_struct*)
_tmp128)->f1;_LL37: {int _tmp132;struct _tuple3 _tmp131=Cyc_Tcdecl_check_var_or_fn_decl((
void*)_tmp127->sc,(void*)((struct Cyc_Core_Opt*)_check_null(_tmp127->cached_typ))->v,
Cyc_Absyn_empty_tqual(),_tmp127->attributes,(void*)_tmp129->sc,(void*)_tmp129->type,
_tmp129->tq,_tmp129->attributes,_tag_arr("variable",sizeof(char),9),Cyc_Absynpp_qvar2string(
_tmp127->name),loc,msg);_tmp132=_tmp131.f2;return _tmp132?b0:(void*)0;}_LL38:;
_LL39:(int)_throw((void*)({struct Cyc_Core_Invalid_argument_struct*_tmp133=
_cycalloc(sizeof(*_tmp133));_tmp133[0]=({struct Cyc_Core_Invalid_argument_struct
_tmp134;_tmp134.tag=Cyc_Core_Invalid_argument;_tmp134.f1=_tag_arr("Tcdecl::merge_binding",
sizeof(char),22);_tmp134;});_tmp133;}));_LL2D:;}struct Cyc_Tcdecl_Xtunionfielddecl*
Cyc_Tcdecl_merge_xtunionfielddecl(struct Cyc_Tcdecl_Xtunionfielddecl*d0,struct Cyc_Tcdecl_Xtunionfielddecl*
d1,struct Cyc_Position_Segment*loc,struct _tagged_arr*msg){struct Cyc_Tcdecl_Xtunionfielddecl
_tmp136;struct Cyc_Absyn_Tuniondecl*_tmp137;struct Cyc_Absyn_Tunionfield*_tmp138;
struct Cyc_Tcdecl_Xtunionfielddecl*_tmp135=d0;_tmp136=*_tmp135;_tmp137=_tmp136.base;
_tmp138=_tmp136.field;{struct Cyc_Tcdecl_Xtunionfielddecl _tmp13A;struct Cyc_Absyn_Tuniondecl*
_tmp13B;struct Cyc_Absyn_Tunionfield*_tmp13C;struct Cyc_Tcdecl_Xtunionfielddecl*
_tmp139=d1;_tmp13A=*_tmp139;_tmp13B=_tmp13A.base;_tmp13C=_tmp13A.field;{struct
_tagged_arr _tmp13D=Cyc_Absynpp_qvar2string(_tmp138->name);if(Cyc_Absyn_qvar_cmp(
_tmp137->name,_tmp13B->name)!= 0){({struct Cyc_Std_String_pa_struct _tmp141;
_tmp141.tag=0;_tmp141.f1=(struct _tagged_arr)_tmp13D;{struct Cyc_Std_String_pa_struct
_tmp140;_tmp140.tag=0;_tmp140.f1=(struct _tagged_arr)Cyc_Absynpp_qvar2string(
_tmp13B->name);{struct Cyc_Std_String_pa_struct _tmp13F;_tmp13F.tag=0;_tmp13F.f1=(
struct _tagged_arr)Cyc_Absynpp_qvar2string(_tmp137->name);{void*_tmp13E[3]={&
_tmp13F,& _tmp140,& _tmp141};Cyc_Tcdecl_merr(loc,msg,_tag_arr("xtunions %s and %s have both a field named %s",
sizeof(char),46),_tag_arr(_tmp13E,sizeof(void*),3));}}}});return 0;}if(!Cyc_Tcdecl_check_tvs(
_tmp137->tvs,_tmp13B->tvs,_tag_arr("xtunion",sizeof(char),8),Cyc_Absynpp_qvar2string(
_tmp137->name),loc,msg))return 0;{struct Cyc_List_List*_tmp142=Cyc_Tcdecl_build_tvs_map(
_tmp137->tvs,_tmp13B->tvs);struct Cyc_Absyn_Tunionfield*_tmp143=Cyc_Tcdecl_merge_tunionfield(
_tmp138,_tmp13C,_tmp142,_tag_arr("xtunionfield",sizeof(char),13),_tmp13D,msg);
if(_tmp143 == 0)return 0;if(_tmp143 == (struct Cyc_Absyn_Tunionfield*)_tmp138)return(
struct Cyc_Tcdecl_Xtunionfielddecl*)d0;return({struct Cyc_Tcdecl_Xtunionfielddecl*
_tmp144=_cycalloc(sizeof(*_tmp144));_tmp144->base=_tmp137;_tmp144->field=(struct
Cyc_Absyn_Tunionfield*)_check_null(_tmp143);_tmp144;});}}}}
