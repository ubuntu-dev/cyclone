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
#ifdef __APPLE__
#define _INLINE_FUNCTIONS
#endif

#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#ifdef _INLINE_FUNCTIONS
static inline void *
_check_null(void *ptr) {
  void*_check_null_temp = (void*)(ptr);
  if (!_check_null_temp) _throw_null();
  return _check_null_temp;
}
#else
#define _check_null(ptr) \
  ({ void*_check_null_temp = (void*)(ptr); \
     if (!_check_null_temp) _throw_null(); \
     _check_null_temp; })
#endif
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  ((char *)ptr) + (elt_sz)*(index); })
#else
#ifdef _INLINE_FUNCTIONS
static inline char *
_check_known_subscript_null(void *ptr, unsigned bound, unsigned elt_sz, unsigned index) {
  void*_cks_ptr = (void*)(ptr);
  unsigned _cks_bound = (bound);
  unsigned _cks_elt_sz = (elt_sz);
  unsigned _cks_index = (index);
  if (!_cks_ptr) _throw_null();
  if (_cks_index >= _cks_bound) _throw_arraybounds();
  return ((char *)_cks_ptr) + _cks_elt_sz*_cks_index;
}
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  void*_cks_ptr = (void*)(ptr); \
  unsigned _cks_bound = (bound); \
  unsigned _cks_elt_sz = (elt_sz); \
  unsigned _cks_index = (index); \
  if (!_cks_ptr) _throw_null(); \
  if (_cks_index >= _cks_bound) _throw_arraybounds(); \
  ((char *)_cks_ptr) + _cks_elt_sz*_cks_index; })
#endif
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_notnull(bound,index) (index)
#else
#ifdef _INLINE_FUNCTIONS
static inline unsigned
_check_known_subscript_notnull(unsigned bound,unsigned index) { 
  unsigned _cksnn_bound = (bound); 
  unsigned _cksnn_index = (index); 
  if (_cksnn_index >= _cksnn_bound) _throw_arraybounds(); 
  return _cksnn_index;
}
#else
#define _check_known_subscript_notnull(bound,index) ({ \
  unsigned _cksnn_bound = (bound); \
  unsigned _cksnn_index = (index); \
  if (_cksnn_index >= _cksnn_bound) _throw_arraybounds(); \
  _cksnn_index; })
#endif
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#ifdef _INLINE_FUNCTIONS
static inline unsigned char *
_check_unknown_subscript(struct _tagged_arr arr,unsigned elt_sz,unsigned index) {
  struct _tagged_arr _cus_arr = (arr);
  unsigned _cus_elt_sz = (elt_sz);
  unsigned _cus_index = (index);
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index;
  return _cus_ans;
}
#else
#define _check_unknown_subscript(arr,elt_sz,index) ({ \
  struct _tagged_arr _cus_arr = (arr); \
  unsigned _cus_elt_sz = (elt_sz); \
  unsigned _cus_index = (index); \
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index; \
  _cus_ans; })
#endif
#else
#ifdef _INLINE_FUNCTIONS
static inline unsigned char *
_check_unknown_subscript(struct _tagged_arr arr,unsigned elt_sz,unsigned index) {
  struct _tagged_arr _cus_arr = (arr);
  unsigned _cus_elt_sz = (elt_sz);
  unsigned _cus_index = (index);
  unsigned char *_cus_ans = _cus_arr.curr + _cus_elt_sz * _cus_index;
  if (!_cus_arr.base) _throw_null();
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one)
    _throw_arraybounds();
  return _cus_ans;
}
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
#endif

#ifdef _INLINE_FUNCTIONS
static inline struct _tagged_arr
_tag_arr(const void *tcurr,unsigned elt_sz,unsigned num_elts) {
  struct _tagged_arr _tag_arr_ans;
  _tag_arr_ans.base = _tag_arr_ans.curr = (void*)(tcurr);
  _tag_arr_ans.last_plus_one = _tag_arr_ans.base + (elt_sz) * (num_elts);
  return _tag_arr_ans;
}
#else
#define _tag_arr(tcurr,elt_sz,num_elts) ({ \
  struct _tagged_arr _tag_arr_ans; \
  _tag_arr_ans.base = _tag_arr_ans.curr = (void*)(tcurr); \
  _tag_arr_ans.last_plus_one = _tag_arr_ans.base + (elt_sz) * (num_elts); \
  _tag_arr_ans; })
#endif

#ifdef _INLINE_FUNCTIONS
static inline struct _tagged_arr *
_init_tag_arr(struct _tagged_arr *arr_ptr,
              void *arr, unsigned elt_sz, unsigned num_elts) {
  struct _tagged_arr *_itarr_ptr = (arr_ptr);
  void* _itarr = (arr);
  _itarr_ptr->base = _itarr_ptr->curr = _itarr;
  _itarr_ptr->last_plus_one = ((char *)_itarr) + (elt_sz) * (num_elts);
  return _itarr_ptr;
}
#else
#define _init_tag_arr(arr_ptr,arr,elt_sz,num_elts) ({ \
  struct _tagged_arr *_itarr_ptr = (arr_ptr); \
  void* _itarr = (arr); \
  _itarr_ptr->base = _itarr_ptr->curr = _itarr; \
  _itarr_ptr->last_plus_one = ((char *)_itarr) + (elt_sz) * (num_elts); \
  _itarr_ptr; })
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _untag_arr(arr,elt_sz,num_elts) ((arr).curr)
#else
#ifdef _INLINE_FUNCTIONS
static inline unsigned char *
_untag_arr(struct _tagged_arr arr, unsigned elt_sz,unsigned num_elts) {
  struct _tagged_arr _arr = (arr);
  unsigned char *_curr = _arr.curr;
  if (_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one)
    _throw_arraybounds();
  return _curr;
}
#else
#define _untag_arr(arr,elt_sz,num_elts) ({ \
  struct _tagged_arr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if (_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one)\
    _throw_arraybounds(); \
  _curr; })
#endif
#endif

#ifdef _INLINE_FUNCTIONS
static inline unsigned
_get_arr_size(struct _tagged_arr arr,unsigned elt_sz) {
  struct _tagged_arr _get_arr_size_temp = (arr);
  unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr;
  unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one;
  return (_get_arr_size_curr < _get_arr_size_temp.base ||
          _get_arr_size_curr >= _get_arr_size_last) ? 0 :
    ((_get_arr_size_last - _get_arr_size_curr) / (elt_sz));
}
#else
#define _get_arr_size(arr,elt_sz) \
  ({struct _tagged_arr _get_arr_size_temp = (arr); \
    unsigned char *_get_arr_size_curr=_get_arr_size_temp.curr; \
    unsigned char *_get_arr_size_last=_get_arr_size_temp.last_plus_one; \
    (_get_arr_size_curr < _get_arr_size_temp.base || \
     _get_arr_size_curr >= _get_arr_size_last) ? 0 : \
    ((_get_arr_size_last - _get_arr_size_curr) / (elt_sz));})
#endif

#ifdef _INLINE_FUNCTIONS
static inline struct _tagged_arr
_tagged_arr_plus(struct _tagged_arr arr,unsigned elt_sz,int change) {
  struct _tagged_arr _ans = (arr);
  _ans.curr += ((int)(elt_sz))*(change);
  return _ans;
}
#else
#define _tagged_arr_plus(arr,elt_sz,change) ({ \
  struct _tagged_arr _ans = (arr); \
  _ans.curr += ((int)(elt_sz))*(change); \
  _ans; })
#endif

#ifdef _INLINE_FUNCTIONS
static inline struct _tagged_arr
_tagged_arr_inplace_plus(struct _tagged_arr *arr_ptr,unsigned elt_sz,int change) {
  struct _tagged_arr * _arr_ptr = (arr_ptr);
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return *_arr_ptr;
}
#else
#define _tagged_arr_inplace_plus(arr_ptr,elt_sz,change) ({ \
  struct _tagged_arr * _arr_ptr = (arr_ptr); \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  *_arr_ptr; })
#endif

#ifdef _INLINE_FUNCTIONS
static inline struct _tagged_arr
_tagged_arr_inplace_plus_post(struct _tagged_arr *arr_ptr,unsigned elt_sz,int change) {
  struct _tagged_arr * _arr_ptr = (arr_ptr);
  struct _tagged_arr _ans = *_arr_ptr;
  _arr_ptr->curr += ((int)(elt_sz))*(change);
  return _ans;
}
#else
#define _tagged_arr_inplace_plus_post(arr_ptr,elt_sz,change) ({ \
  struct _tagged_arr * _arr_ptr = (arr_ptr); \
  struct _tagged_arr _ans = *_arr_ptr; \
  _arr_ptr->curr += ((int)(elt_sz))*(change); \
  _ans; })
#endif

// Decrease the upper bound on a fat pointer by numelts where sz is
// the size of the pointer's type.  Note that this can't be a macro
// if we're to get initializers right.
static struct _tagged_arr _tagged_ptr_decrease_size(struct _tagged_arr x,
                                                    unsigned int sz,
                                                    unsigned int numelts) {
  x.last_plus_one -= sz * numelts; 
  return x; 
}

// Add i to zero-terminated pointer x.  Checks for x being null and
// ensures that x[0..i-1] are not 0.
#ifdef NO_CYC_BOUNDS_CHECK
#define _zero_arr_plus(orig_x,orig_sz,orig_i) ((orig_x)+(orig_i))
#else
#define _zero_arr_plus(orig_x,orig_sz,orig_i) ({ \
  typedef _czs_tx = (*orig_x); \
  _czs_tx *_czs_x = (_czs_tx *)(orig_x); \
  unsigned int _czs_sz = (orig_sz); \
  int _czs_i = (orig_i); \
  unsigned int _czs_temp; \
  if ((_czs_x) == 0) _throw_null(); \
  if (_czs_i < 0) _throw_arraybounds(); \
  for (_czs_temp=_czs_sz; _czs_temp < _czs_i; _czs_temp++) \
    if (_czs_x[_czs_temp] == 0) _throw_arraybounds(); \
  _czs_x+_czs_i; })
#endif

// Calculates the number of elements in a zero-terminated, thin array.
// If non-null, the array is guaranteed to have orig_offset elements.
#define _get_zero_arr_size(orig_x,orig_offset) ({ \
  typedef _gres_tx = (*orig_x); \
  _gres_tx *_gres_x = (_gres_tx *)(orig_x); \
  unsigned int _gres_offset = (orig_offset); \
  unsigned int _gres = 0; \
  if (_gres_x != 0) { \
     _gres = _gres_offset; \
     _gres_x += _gres_offset - 1; \
     while (*_gres_x != 0) { _gres_x++; _gres++; } \
  } _gres; })

// Does in-place addition of a zero-terminated pointer (x += e and ++x).  
// Note that this expands to call _zero_arr_plus.
#define _zero_arr_inplace_plus(x,orig_i) ({ \
  typedef _zap_tx = (*x); \
  _zap_tx **_zap_x = &((_zap_tx*)x); \
  *_zap_x = _zero_arr_plus(*_zap_x,1,(orig_i)); })

// Does in-place increment of a zero-terminated pointer (e.g., x++).
// Note that this expands to call _zero_arr_plus.
#define _zero_arr_inplace_plus_post(x,orig_i) ({ \
  typedef _zap_tx = (*x); \
  _zap_tx **_zap_x = &((_zap_tx*)x); \
  _zap_tx *_zap_res = *_zap_x; \
  *_zap_x = _zero_arr_plus(_zap_res,1,(orig_i)); \
  _zap_res; })
  
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
struct _RegionHandle*Cyc_Core_heap_region;struct Cyc___cycFILE;extern struct Cyc___cycFILE*
Cyc_stderr;struct Cyc_Cstdio___abstractFILE;struct Cyc_String_pa_struct{int tag;
struct _tagged_arr f1;};struct Cyc_Int_pa_struct{int tag;unsigned int f1;};struct Cyc_Double_pa_struct{
int tag;double f1;};struct Cyc_LongDouble_pa_struct{int tag;long double f1;};struct
Cyc_ShortPtr_pa_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_struct{int tag;
unsigned int*f1;};struct _tagged_arr Cyc_aprintf(struct _tagged_arr,struct
_tagged_arr);int Cyc_fprintf(struct Cyc___cycFILE*,struct _tagged_arr,struct
_tagged_arr);struct Cyc_ShortPtr_sa_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_struct{
int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_struct{
int tag;unsigned int*f1;};struct Cyc_StringPtr_sa_struct{int tag;struct _tagged_arr
f1;};struct Cyc_DoublePtr_sa_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_struct{
int tag;float*f1;};struct Cyc_CharPtr_sa_struct{int tag;struct _tagged_arr f1;};
struct _tagged_arr Cyc_vrprintf(struct _RegionHandle*,struct _tagged_arr,struct
_tagged_arr);extern char Cyc_FileCloseError[19];extern char Cyc_FileOpenError[18];
struct Cyc_FileOpenError_struct{char*tag;struct _tagged_arr f1;};extern char Cyc_Lexing_Error[
10];struct Cyc_Lexing_Error_struct{char*tag;struct _tagged_arr f1;};struct Cyc_Lexing_lexbuf{
void(*refill_buff)(struct Cyc_Lexing_lexbuf*);void*refill_state;struct _tagged_arr
lex_buffer;int lex_buffer_len;int lex_abs_pos;int lex_start_pos;int lex_curr_pos;int
lex_last_pos;int lex_last_action;int lex_eof_reached;};struct Cyc_Lexing_function_lexbuf_state{
int(*read_fun)(struct _tagged_arr,int,void*);void*read_fun_state;};struct Cyc_Lexing_lex_tables{
struct _tagged_arr lex_base;struct _tagged_arr lex_backtrk;struct _tagged_arr
lex_default;struct _tagged_arr lex_trans;struct _tagged_arr lex_check;};struct Cyc_Lexing_lexbuf*
Cyc_Lexing_from_file(struct Cyc___cycFILE*);struct Cyc_List_List{void*hd;struct Cyc_List_List*
tl;};int Cyc_List_length(struct Cyc_List_List*x);struct Cyc_List_List*Cyc_List_map(
void*(*f)(void*),struct Cyc_List_List*x);struct Cyc_List_List*Cyc_List_map_c(void*(*
f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[
18];void Cyc_List_iter(void(*f)(void*),struct Cyc_List_List*x);void*Cyc_List_fold_right(
void*(*f)(void*,void*),struct Cyc_List_List*x,void*accum);struct Cyc_List_List*Cyc_List_imp_rev(
struct Cyc_List_List*x);struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,
struct Cyc_List_List*y);struct Cyc_List_List*Cyc_List_imp_append(struct Cyc_List_List*
x,struct Cyc_List_List*y);struct Cyc_List_List*Cyc_List_flatten(struct Cyc_List_List*
x);extern char Cyc_List_Nth[8];struct Cyc_List_List*Cyc_List_zip(struct Cyc_List_List*
x,struct Cyc_List_List*y);struct _tuple0{struct Cyc_List_List*f1;struct Cyc_List_List*
f2;};struct _tuple0 Cyc_List_split(struct Cyc_List_List*x);unsigned int Cyc_strlen(
struct _tagged_arr s);int Cyc_strcmp(struct _tagged_arr s1,struct _tagged_arr s2);int
Cyc_zstrcmp(struct _tagged_arr,struct _tagged_arr);int Cyc_zstrptrcmp(struct
_tagged_arr*,struct _tagged_arr*);struct _tagged_arr Cyc_strcat(struct _tagged_arr
dest,struct _tagged_arr src);struct _tagged_arr Cyc_strcpy(struct _tagged_arr dest,
struct _tagged_arr src);struct _tagged_arr Cyc_substring(struct _tagged_arr,int ofs,
unsigned int n);struct Cyc_Lineno_Pos{struct _tagged_arr logical_file;struct
_tagged_arr line;int line_no;int col;};extern char Cyc_Position_Exit[9];struct Cyc_Position_Segment;
struct Cyc_Position_Segment*Cyc_Position_segment_of_abs(int,int);struct Cyc_Position_Segment*
Cyc_Position_segment_join(struct Cyc_Position_Segment*,struct Cyc_Position_Segment*);
struct _tagged_arr Cyc_Position_string_of_segment(struct Cyc_Position_Segment*);
struct Cyc_Position_Error{struct _tagged_arr source;struct Cyc_Position_Segment*seg;
void*kind;struct _tagged_arr desc;};struct Cyc_Position_Error*Cyc_Position_mk_err_parse(
struct Cyc_Position_Segment*,struct _tagged_arr);extern char Cyc_Position_Nocontext[
14];void Cyc_Position_post_error(struct Cyc_Position_Error*);struct Cyc_Absyn_Rel_n_struct{
int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Abs_n_struct{int tag;struct Cyc_List_List*
f1;};struct _tuple1{void*f1;struct _tagged_arr*f2;};struct Cyc_Absyn_Conref;struct
Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;
};struct Cyc_Absyn_Conref{void*v;};struct Cyc_Absyn_Eq_constr_struct{int tag;void*
f1;};struct Cyc_Absyn_Forward_constr_struct{int tag;struct Cyc_Absyn_Conref*f1;};
struct Cyc_Absyn_Eq_kb_struct{int tag;void*f1;};struct Cyc_Absyn_Unknown_kb_struct{
int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_struct{int tag;struct Cyc_Core_Opt*
f1;void*f2;};struct Cyc_Absyn_Tvar{struct _tagged_arr*name;int*identity;void*kind;
};struct Cyc_Absyn_Upper_b_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AbsUpper_b_struct{
int tag;void*f1;};struct Cyc_Absyn_PtrAtts{void*rgn;struct Cyc_Absyn_Conref*
nullable;struct Cyc_Absyn_Conref*bounds;struct Cyc_Absyn_Conref*zero_term;};struct
Cyc_Absyn_PtrInfo{void*elt_typ;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts
ptr_atts;};struct Cyc_Absyn_VarargInfo{struct Cyc_Core_Opt*name;struct Cyc_Absyn_Tqual
tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;struct
Cyc_Core_Opt*effect;void*ret_typ;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*
cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;};struct
Cyc_Absyn_UnknownTunionInfo{struct _tuple1*name;int is_xtunion;};struct Cyc_Absyn_UnknownTunion_struct{
int tag;struct Cyc_Absyn_UnknownTunionInfo f1;};struct Cyc_Absyn_KnownTunion_struct{
int tag;struct Cyc_Absyn_Tuniondecl**f1;};struct Cyc_Absyn_TunionInfo{void*
tunion_info;struct Cyc_List_List*targs;void*rgn;};struct Cyc_Absyn_UnknownTunionFieldInfo{
struct _tuple1*tunion_name;struct _tuple1*field_name;int is_xtunion;};struct Cyc_Absyn_UnknownTunionfield_struct{
int tag;struct Cyc_Absyn_UnknownTunionFieldInfo f1;};struct Cyc_Absyn_KnownTunionfield_struct{
int tag;struct Cyc_Absyn_Tuniondecl*f1;struct Cyc_Absyn_Tunionfield*f2;};struct Cyc_Absyn_TunionFieldInfo{
void*field_info;struct Cyc_List_List*targs;};struct Cyc_Absyn_UnknownAggr_struct{
int tag;void*f1;struct _tuple1*f2;};struct Cyc_Absyn_KnownAggr_struct{int tag;struct
Cyc_Absyn_Aggrdecl**f1;};struct Cyc_Absyn_AggrInfo{void*aggr_info;struct Cyc_List_List*
targs;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct
Cyc_Absyn_Exp*num_elts;struct Cyc_Absyn_Conref*zero_term;};struct Cyc_Absyn_Evar_struct{
int tag;struct Cyc_Core_Opt*f1;struct Cyc_Core_Opt*f2;int f3;struct Cyc_Core_Opt*f4;}
;struct Cyc_Absyn_VarType_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_TunionType_struct{
int tag;struct Cyc_Absyn_TunionInfo f1;};struct Cyc_Absyn_TunionFieldType_struct{int
tag;struct Cyc_Absyn_TunionFieldInfo f1;};struct Cyc_Absyn_PointerType_struct{int
tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_IntType_struct{int tag;void*f1;
void*f2;};struct Cyc_Absyn_DoubleType_struct{int tag;int f1;};struct Cyc_Absyn_ArrayType_struct{
int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_struct{int tag;struct
Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_struct{int tag;struct Cyc_List_List*
f1;};struct Cyc_Absyn_AggrType_struct{int tag;struct Cyc_Absyn_AggrInfo f1;};struct
Cyc_Absyn_AnonAggrType_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_EnumType_struct{
int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumType_struct{
int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_SizeofType_struct{int tag;void*f1;
};struct Cyc_Absyn_RgnHandleType_struct{int tag;void*f1;};struct Cyc_Absyn_TypedefType_struct{
int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;
void**f4;};struct Cyc_Absyn_TagType_struct{int tag;void*f1;};struct Cyc_Absyn_TypeInt_struct{
int tag;int f1;};struct Cyc_Absyn_AccessEff_struct{int tag;void*f1;};struct Cyc_Absyn_JoinEff_struct{
int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_RgnsEff_struct{int tag;void*f1;};
struct Cyc_Absyn_NoTypes_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Position_Segment*
f2;};struct Cyc_Absyn_WithTypes_struct{int tag;struct Cyc_List_List*f1;int f2;struct
Cyc_Absyn_VarargInfo*f3;struct Cyc_Core_Opt*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Regparm_att_struct{
int tag;int f1;};struct Cyc_Absyn_Aligned_att_struct{int tag;int f1;};struct Cyc_Absyn_Section_att_struct{
int tag;struct _tagged_arr f1;};struct Cyc_Absyn_Format_att_struct{int tag;void*f1;
int f2;int f3;};struct Cyc_Absyn_Initializes_att_struct{int tag;int f1;};struct Cyc_Absyn_Mode_att_struct{
int tag;struct _tagged_arr f1;};struct Cyc_Absyn_Carray_mod_struct{int tag;struct Cyc_Absyn_Conref*
f1;};struct Cyc_Absyn_ConstArray_mod_struct{int tag;struct Cyc_Absyn_Exp*f1;struct
Cyc_Absyn_Conref*f2;};struct Cyc_Absyn_Pointer_mod_struct{int tag;struct Cyc_Absyn_PtrAtts
f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_struct{int tag;void*f1;
};struct Cyc_Absyn_TypeParams_mod_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Position_Segment*
f2;int f3;};struct Cyc_Absyn_Attributes_mod_struct{int tag;struct Cyc_Position_Segment*
f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Char_c_struct{int tag;void*f1;char f2;
};struct Cyc_Absyn_Short_c_struct{int tag;void*f1;short f2;};struct Cyc_Absyn_Int_c_struct{
int tag;void*f1;int f2;};struct Cyc_Absyn_LongLong_c_struct{int tag;void*f1;
long long f2;};struct Cyc_Absyn_Float_c_struct{int tag;struct _tagged_arr f1;};struct
Cyc_Absyn_String_c_struct{int tag;struct _tagged_arr f1;};struct Cyc_Absyn_VarargCallInfo{
int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};
struct Cyc_Absyn_StructField_struct{int tag;struct _tagged_arr*f1;};struct Cyc_Absyn_TupleIndex_struct{
int tag;unsigned int f1;};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*
rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;};struct Cyc_Absyn_Const_e_struct{
int tag;void*f1;};struct Cyc_Absyn_Var_e_struct{int tag;struct _tuple1*f1;void*f2;};
struct Cyc_Absyn_UnknownId_e_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_Primop_e_struct{
int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_struct{int tag;
struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;void*f2;};struct Cyc_Absyn_Conditional_e_struct{int
tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};
struct Cyc_Absyn_And_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*
f2;};struct Cyc_Absyn_Or_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*
f2;};struct Cyc_Absyn_SeqExp_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*
f2;};struct Cyc_Absyn_UnknownCall_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct
Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_struct{int tag;struct Cyc_Absyn_Exp*f1;
struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;};struct Cyc_Absyn_Throw_e_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoInstantiate_e_struct{int tag;
struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_struct{int tag;void*f1;struct
Cyc_Absyn_Exp*f2;int f3;void*f4;};struct Cyc_Absyn_Address_e_struct{int tag;struct
Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_struct{int tag;struct Cyc_Absyn_Exp*f1;
struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftyp_e_struct{int tag;void*f1;};
struct Cyc_Absyn_Sizeofexp_e_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_struct{
int tag;void*f1;void*f2;};struct Cyc_Absyn_Gentyp_e_struct{int tag;struct Cyc_List_List*
f1;void*f2;};struct Cyc_Absyn_Deref_e_struct{int tag;struct Cyc_Absyn_Exp*f1;};
struct Cyc_Absyn_AggrMember_e_struct{int tag;struct Cyc_Absyn_Exp*f1;struct
_tagged_arr*f2;};struct Cyc_Absyn_AggrArrow_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct _tagged_arr*f2;};struct Cyc_Absyn_Subscript_e_struct{int tag;struct Cyc_Absyn_Exp*
f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_struct{int tag;struct Cyc_List_List*
f1;};struct _tuple2{struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Tqual f2;void*f3;};
struct Cyc_Absyn_CompoundLit_e_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_Array_e_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;
int f4;};struct Cyc_Absyn_Struct_e_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*
f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_struct{
int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Tunion_e_struct{int tag;
struct Cyc_List_List*f1;struct Cyc_Absyn_Tuniondecl*f2;struct Cyc_Absyn_Tunionfield*
f3;};struct Cyc_Absyn_Enum_e_struct{int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*
f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_AnonEnum_e_struct{int tag;
struct _tuple1*f1;void*f2;struct Cyc_Absyn_Enumfield*f3;};struct Cyc_Absyn_Malloc_e_struct{
int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_UnresolvedMem_e_struct{int
tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Exp{struct Cyc_Core_Opt*topt;
void*r;struct Cyc_Position_Segment*loc;void*annot;};struct Cyc_Absyn_Exp_s_struct{
int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_struct{int tag;struct Cyc_Absyn_Stmt*
f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_struct{int tag;struct Cyc_Absyn_Exp*
f1;};struct Cyc_Absyn_IfThenElse_s_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*
f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple3{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_While_s_struct{int tag;struct _tuple3 f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_Break_s_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Continue_s_struct{
int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Goto_s_struct{int tag;struct
_tagged_arr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_For_s_struct{int tag;
struct Cyc_Absyn_Exp*f1;struct _tuple3 f2;struct _tuple3 f3;struct Cyc_Absyn_Stmt*f4;}
;struct Cyc_Absyn_Switch_s_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_Fallthru_s_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**
f2;};struct Cyc_Absyn_Decl_s_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_Label_s_struct{int tag;struct _tagged_arr*f1;struct Cyc_Absyn_Stmt*
f2;};struct Cyc_Absyn_Do_s_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple3 f2;
};struct Cyc_Absyn_TryCatch_s_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*
f2;};struct Cyc_Absyn_Region_s_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*
f2;int f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_ResetRegion_s_struct{int tag;
struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Stmt{void*r;struct Cyc_Position_Segment*
loc;struct Cyc_List_List*non_local_preds;int try_depth;void*annot;};struct Cyc_Absyn_Var_p_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Reference_p_struct{int tag;
struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_TagInt_p_struct{int tag;struct Cyc_Absyn_Tvar*
f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_struct{int tag;struct Cyc_List_List*
f1;};struct Cyc_Absyn_Pointer_p_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_struct{
int tag;struct Cyc_Absyn_AggrInfo f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;
};struct Cyc_Absyn_Tunion_p_struct{int tag;struct Cyc_Absyn_Tuniondecl*f1;struct Cyc_Absyn_Tunionfield*
f2;struct Cyc_List_List*f3;};struct Cyc_Absyn_Int_p_struct{int tag;void*f1;int f2;};
struct Cyc_Absyn_Char_p_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_struct{int
tag;struct _tagged_arr f1;};struct Cyc_Absyn_Enum_p_struct{int tag;struct Cyc_Absyn_Enumdecl*
f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_struct{int tag;void*
f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_struct{int tag;
struct _tuple1*f1;};struct Cyc_Absyn_UnknownCall_p_struct{int tag;struct _tuple1*f1;
struct Cyc_List_List*f2;};struct Cyc_Absyn_Exp_p_struct{int tag;struct Cyc_Absyn_Exp*
f1;};struct Cyc_Absyn_Pat{void*r;struct Cyc_Core_Opt*topt;struct Cyc_Position_Segment*
loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*
pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;struct Cyc_Position_Segment*
loc;};struct Cyc_Absyn_Global_b_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct
Cyc_Absyn_Funname_b_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_struct{int tag;struct
Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_struct{int tag;struct Cyc_Absyn_Vardecl*
f1;};struct Cyc_Absyn_Vardecl{void*sc;struct _tuple1*name;struct Cyc_Absyn_Tqual tq;
void*type;struct Cyc_Absyn_Exp*initializer;struct Cyc_Core_Opt*rgn;struct Cyc_List_List*
attributes;int escapes;};struct Cyc_Absyn_Fndecl{void*sc;int is_inline;struct
_tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*effect;void*ret_type;
struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;
struct Cyc_List_List*rgn_po;struct Cyc_Absyn_Stmt*body;struct Cyc_Core_Opt*
cached_typ;struct Cyc_Core_Opt*param_vardecls;struct Cyc_List_List*attributes;};
struct Cyc_Absyn_Aggrfield{struct _tagged_arr*name;struct Cyc_Absyn_Tqual tq;void*
type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;};struct Cyc_Absyn_AggrdeclImpl{
struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*
fields;};struct Cyc_Absyn_Aggrdecl{void*kind;void*sc;struct _tuple1*name;struct Cyc_List_List*
tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;};struct Cyc_Absyn_Tunionfield{
struct _tuple1*name;struct Cyc_List_List*typs;struct Cyc_Position_Segment*loc;void*
sc;};struct Cyc_Absyn_Tuniondecl{void*sc;struct _tuple1*name;struct Cyc_List_List*
tvs;struct Cyc_Core_Opt*fields;int is_xtunion;};struct Cyc_Absyn_Enumfield{struct
_tuple1*name;struct Cyc_Absyn_Exp*tag;struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_Enumdecl{
void*sc;struct _tuple1*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{
struct _tuple1*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*
kind;struct Cyc_Core_Opt*defn;struct Cyc_List_List*atts;};struct Cyc_Absyn_Var_d_struct{
int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_struct{int tag;struct Cyc_Absyn_Fndecl*
f1;};struct Cyc_Absyn_Let_d_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*
f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Letv_d_struct{int tag;struct Cyc_List_List*
f1;};struct Cyc_Absyn_Aggr_d_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct
Cyc_Absyn_Tunion_d_struct{int tag;struct Cyc_Absyn_Tuniondecl*f1;};struct Cyc_Absyn_Enum_d_struct{
int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_struct{int tag;
struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_struct{int tag;
struct _tagged_arr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_struct{int
tag;struct _tuple1*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_struct{
int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_ExternCinclude_d_struct{int tag;
struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Decl{void*r;
struct Cyc_Position_Segment*loc;};struct Cyc_Absyn_ArrayElement_struct{int tag;
struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_struct{int tag;struct
_tagged_arr*f1;};extern char Cyc_Absyn_EmptyAnnot[15];extern void*Cyc_Absyn_rel_ns_null;
int Cyc_Absyn_is_qvar_qualified(struct _tuple1*);struct Cyc_Absyn_Tqual Cyc_Absyn_combine_tqual(
struct Cyc_Absyn_Tqual x,struct Cyc_Absyn_Tqual y);struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual();
struct Cyc_Absyn_Conref*Cyc_Absyn_new_conref(void*x);struct Cyc_Absyn_Conref*Cyc_Absyn_empty_conref();
extern struct Cyc_Absyn_Conref*Cyc_Absyn_true_conref;extern struct Cyc_Absyn_Conref*
Cyc_Absyn_false_conref;extern struct Cyc_Absyn_Conref*Cyc_Absyn_bounds_one_conref;
extern struct Cyc_Absyn_Conref*Cyc_Absyn_bounds_unknown_conref;void*Cyc_Absyn_compress_kb(
void*);void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
void*Cyc_Absyn_wildtyp(struct Cyc_Core_Opt*);extern void*Cyc_Absyn_char_typ;extern
void*Cyc_Absyn_uint_typ;extern void*Cyc_Absyn_sint_typ;extern void*Cyc_Absyn_float_typ;
void*Cyc_Absyn_double_typ(int);void*Cyc_Absyn_starb_typ(void*t,void*rgn,struct
Cyc_Absyn_Tqual tq,void*b,struct Cyc_Absyn_Conref*zero_term);void*Cyc_Absyn_array_typ(
void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,struct Cyc_Absyn_Conref*
zero_term);struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_New_exp(struct Cyc_Absyn_Exp*rgn_handle,struct Cyc_Absyn_Exp*,
struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_null_exp(struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_true_exp(struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*
Cyc_Absyn_false_exp(struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_int_exp(
void*,int,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(
unsigned int,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_char_exp(
char c,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_float_exp(
struct _tagged_arr f,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_string_exp(
struct _tagged_arr s,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_prim1_exp(
void*,struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_prim2_exp(
void*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_eq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,
struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_neq_exp(struct Cyc_Absyn_Exp*,
struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_gt_exp(
struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*
Cyc_Absyn_lt_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_gte_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,
struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_lte_exp(struct Cyc_Absyn_Exp*,
struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_assignop_exp(
struct Cyc_Absyn_Exp*,struct Cyc_Core_Opt*,struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_post_inc_exp(struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_post_dec_exp(struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_pre_inc_exp(struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_pre_dec_exp(struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_conditional_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,
struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_and_exp(
struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*
Cyc_Absyn_or_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_seq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,
struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_unknowncall_exp(
struct Cyc_Absyn_Exp*,struct Cyc_List_List*,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*
Cyc_Absyn_throw_exp(struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*
Cyc_Absyn_noinstantiate_exp(struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_instantiate_exp(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,
struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_cast_exp(void*,struct
Cyc_Absyn_Exp*,int user_cast,void*,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*
Cyc_Absyn_address_exp(struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);struct
Cyc_Absyn_Exp*Cyc_Absyn_sizeoftyp_exp(void*t,struct Cyc_Position_Segment*);struct
Cyc_Absyn_Exp*Cyc_Absyn_sizeofexp_exp(struct Cyc_Absyn_Exp*e,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_offsetof_exp(void*,void*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_gentyp_exp(struct Cyc_List_List*,void*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_deref_exp(struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_aggrmember_exp(struct Cyc_Absyn_Exp*,struct
_tagged_arr*,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*Cyc_Absyn_aggrarrow_exp(
struct Cyc_Absyn_Exp*,struct _tagged_arr*,struct Cyc_Position_Segment*);struct Cyc_Absyn_Exp*
Cyc_Absyn_subscript_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_tuple_exp(struct Cyc_List_List*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Exp*Cyc_Absyn_stmt_exp(struct Cyc_Absyn_Stmt*,struct Cyc_Position_Segment*);
struct Cyc_Absyn_Stmt*Cyc_Absyn_new_stmt(void*s,struct Cyc_Position_Segment*loc);
struct Cyc_Absyn_Stmt*Cyc_Absyn_skip_stmt(struct Cyc_Position_Segment*loc);struct
Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*e,struct Cyc_Position_Segment*
loc);struct Cyc_Absyn_Stmt*Cyc_Absyn_seq_stmt(struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*
s2,struct Cyc_Position_Segment*loc);struct Cyc_Absyn_Stmt*Cyc_Absyn_return_stmt(
struct Cyc_Absyn_Exp*e,struct Cyc_Position_Segment*loc);struct Cyc_Absyn_Stmt*Cyc_Absyn_ifthenelse_stmt(
struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*s2,struct Cyc_Position_Segment*
loc);struct Cyc_Absyn_Stmt*Cyc_Absyn_while_stmt(struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Stmt*
s,struct Cyc_Position_Segment*loc);struct Cyc_Absyn_Stmt*Cyc_Absyn_break_stmt(
struct Cyc_Position_Segment*loc);struct Cyc_Absyn_Stmt*Cyc_Absyn_continue_stmt(
struct Cyc_Position_Segment*loc);struct Cyc_Absyn_Stmt*Cyc_Absyn_for_stmt(struct
Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,struct Cyc_Absyn_Exp*e3,struct Cyc_Absyn_Stmt*
s,struct Cyc_Position_Segment*loc);struct Cyc_Absyn_Stmt*Cyc_Absyn_switch_stmt(
struct Cyc_Absyn_Exp*e,struct Cyc_List_List*,struct Cyc_Position_Segment*loc);
struct Cyc_Absyn_Stmt*Cyc_Absyn_fallthru_stmt(struct Cyc_List_List*el,struct Cyc_Position_Segment*
loc);struct Cyc_Absyn_Stmt*Cyc_Absyn_do_stmt(struct Cyc_Absyn_Stmt*s,struct Cyc_Absyn_Exp*
e,struct Cyc_Position_Segment*loc);struct Cyc_Absyn_Stmt*Cyc_Absyn_goto_stmt(
struct _tagged_arr*lab,struct Cyc_Position_Segment*loc);struct Cyc_Absyn_Stmt*Cyc_Absyn_trycatch_stmt(
struct Cyc_Absyn_Stmt*,struct Cyc_List_List*,struct Cyc_Position_Segment*);struct
Cyc_Absyn_Pat*Cyc_Absyn_new_pat(void*p,struct Cyc_Position_Segment*s);struct Cyc_Absyn_Pat*
Cyc_Absyn_exp_pat(struct Cyc_Absyn_Exp*);struct Cyc_Absyn_Decl*Cyc_Absyn_new_decl(
void*r,struct Cyc_Position_Segment*loc);struct Cyc_Absyn_Decl*Cyc_Absyn_let_decl(
struct Cyc_Absyn_Pat*p,struct Cyc_Absyn_Exp*e,struct Cyc_Position_Segment*loc);
struct Cyc_Absyn_Decl*Cyc_Absyn_letv_decl(struct Cyc_List_List*,struct Cyc_Position_Segment*
loc);struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(struct _tuple1*x,void*t,struct
Cyc_Absyn_Exp*init);struct Cyc_Absyn_AggrdeclImpl*Cyc_Absyn_aggrdecl_impl(struct
Cyc_List_List*exists,struct Cyc_List_List*po,struct Cyc_List_List*fs);struct Cyc_Absyn_Decl*
Cyc_Absyn_aggr_decl(void*k,void*s,struct _tuple1*n,struct Cyc_List_List*ts,struct
Cyc_Absyn_AggrdeclImpl*i,struct Cyc_List_List*atts,struct Cyc_Position_Segment*loc);
struct Cyc_Absyn_Decl*Cyc_Absyn_tunion_decl(void*s,struct _tuple1*n,struct Cyc_List_List*
ts,struct Cyc_Core_Opt*fs,int is_xtunion,struct Cyc_Position_Segment*loc);void*Cyc_Absyn_function_typ(
struct Cyc_List_List*tvs,struct Cyc_Core_Opt*eff_typ,void*ret_typ,struct Cyc_List_List*
args,int c_varargs,struct Cyc_Absyn_VarargInfo*cyc_varargs,struct Cyc_List_List*
rgn_po,struct Cyc_List_List*atts);int Cyc_Absyn_fntype_att(void*a);struct Cyc_Iter_Iter{
void*env;int(*next)(void*env,void*dest);};int Cyc_Iter_next(struct Cyc_Iter_Iter,
void*);struct Cyc_Set_Set;extern char Cyc_Set_Absent[11];struct Cyc_Dict_Dict;extern
char Cyc_Dict_Present[12];extern char Cyc_Dict_Absent[11];struct _tuple4{void*f1;
void*f2;};struct _tuple4*Cyc_Dict_rchoose(struct _RegionHandle*r,struct Cyc_Dict_Dict*
d);struct _tuple4*Cyc_Dict_rchoose(struct _RegionHandle*,struct Cyc_Dict_Dict*d);
struct Cyc_RgnOrder_RgnPO;struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_initial_fn_po(
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
ae;struct Cyc_Core_Opt*le;};void Cyc_Tcutil_warn(struct Cyc_Position_Segment*,
struct _tagged_arr fmt,struct _tagged_arr ap);void*Cyc_Tcutil_copy_type(void*t);void*
Cyc_Tcutil_compress(void*t);struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*k);
void Cyc_Lex_register_typedef(struct _tuple1*s);void Cyc_Lex_enter_namespace(struct
_tagged_arr*);void Cyc_Lex_leave_namespace();void Cyc_Lex_enter_using(struct
_tuple1*);void Cyc_Lex_leave_using();int Cyc_Parse_no_register=0;struct Cyc_Parse_Signed_spec_struct{
int tag;struct Cyc_Position_Segment*f1;};struct Cyc_Parse_Unsigned_spec_struct{int
tag;struct Cyc_Position_Segment*f1;};struct Cyc_Parse_Short_spec_struct{int tag;
struct Cyc_Position_Segment*f1;};struct Cyc_Parse_Long_spec_struct{int tag;struct
Cyc_Position_Segment*f1;};struct Cyc_Parse_Type_spec_struct{int tag;void*f1;struct
Cyc_Position_Segment*f2;};struct Cyc_Parse_Decl_spec_struct{int tag;struct Cyc_Absyn_Decl*
f1;};struct Cyc_Parse_Declaration_spec{struct Cyc_Core_Opt*sc;struct Cyc_Absyn_Tqual
tq;struct Cyc_List_List*type_specs;int is_inline;struct Cyc_List_List*attributes;};
struct Cyc_Parse_Declarator{struct _tuple1*id;struct Cyc_List_List*tms;};struct Cyc_Parse_Abstractdeclarator{
struct Cyc_List_List*tms;};struct _tuple5{void*f1;struct Cyc_Core_Opt*f2;};static
struct _tuple5 Cyc_Parse_collapse_type_specifiers(struct Cyc_List_List*ts,struct Cyc_Position_Segment*
loc);struct _tuple6{struct Cyc_Absyn_Tqual f1;void*f2;struct Cyc_List_List*f3;struct
Cyc_List_List*f4;};static struct _tuple6 Cyc_Parse_apply_tms(struct Cyc_Absyn_Tqual,
void*,struct Cyc_List_List*,struct Cyc_List_List*);struct Cyc_Core_Opt*Cyc_Parse_lbuf=
0;static struct Cyc_List_List*Cyc_Parse_parse_result=0;static void Cyc_Parse_err(
struct _tagged_arr msg,struct Cyc_Position_Segment*sg){Cyc_Position_post_error(Cyc_Position_mk_err_parse(
sg,msg));}static void*Cyc_Parse_abort(struct Cyc_Position_Segment*sg,struct
_tagged_arr fmt,struct _tagged_arr ap){Cyc_Parse_err((struct _tagged_arr)Cyc_vrprintf(
Cyc_Core_heap_region,fmt,ap),sg);(int)_throw((void*)Cyc_Position_Exit);}static
void Cyc_Parse_unimp(struct _tagged_arr msg,struct Cyc_Position_Segment*sg){({struct
Cyc_String_pa_struct _tmp3;_tmp3.tag=0;_tmp3.f1=(struct _tagged_arr)((struct
_tagged_arr)msg);{struct Cyc_String_pa_struct _tmp2;_tmp2.tag=0;_tmp2.f1=(struct
_tagged_arr)((struct _tagged_arr)Cyc_Position_string_of_segment(sg));{void*_tmp0[
2]={& _tmp2,& _tmp3};Cyc_fprintf(Cyc_stderr,({const char*_tmp1="%s: Warning: Cyclone does not yet support %s\n";
_tag_arr(_tmp1,sizeof(char),_get_zero_arr_size(_tmp1,46));}),_tag_arr(_tmp0,
sizeof(void*),2));}}});return;}static int Cyc_Parse_enum_counter=0;struct _tuple1*
Cyc_Parse_gensym_enum(){return({struct _tuple1*_tmp4=_cycalloc(sizeof(*_tmp4));
_tmp4->f1=(void*)({struct Cyc_Absyn_Rel_n_struct*_tmp9=_cycalloc(sizeof(*_tmp9));
_tmp9[0]=({struct Cyc_Absyn_Rel_n_struct _tmpA;_tmpA.tag=0;_tmpA.f1=0;_tmpA;});
_tmp9;});_tmp4->f2=({struct _tagged_arr*_tmp5=_cycalloc(sizeof(*_tmp5));_tmp5[0]=(
struct _tagged_arr)({struct Cyc_Int_pa_struct _tmp8;_tmp8.tag=1;_tmp8.f1=(
unsigned int)Cyc_Parse_enum_counter ++;{void*_tmp6[1]={& _tmp8};Cyc_aprintf(({
const char*_tmp7="__anonymous_enum_%d__";_tag_arr(_tmp7,sizeof(char),
_get_zero_arr_size(_tmp7,22));}),_tag_arr(_tmp6,sizeof(void*),1));}});_tmp5;});
_tmp4;});}struct _tuple7{struct _tuple1*f1;struct Cyc_Absyn_Tqual f2;void*f3;struct
Cyc_List_List*f4;struct Cyc_List_List*f5;};struct _tuple8{struct _tuple7*f1;struct
Cyc_Absyn_Exp*f2;};static struct Cyc_Absyn_Aggrfield*Cyc_Parse_make_aggr_field(
struct Cyc_Position_Segment*loc,struct _tuple8*field_info){struct _tuple8 _tmpC;
struct _tuple7*_tmpD;struct _tuple7 _tmpE;struct _tuple1*_tmpF;struct Cyc_Absyn_Tqual
_tmp10;void*_tmp11;struct Cyc_List_List*_tmp12;struct Cyc_List_List*_tmp13;struct
Cyc_Absyn_Exp*_tmp14;struct _tuple8*_tmpB=field_info;_tmpC=*_tmpB;_tmpD=_tmpC.f1;
_tmpE=*_tmpD;_tmpF=_tmpE.f1;_tmp10=_tmpE.f2;_tmp11=_tmpE.f3;_tmp12=_tmpE.f4;
_tmp13=_tmpE.f5;_tmp14=_tmpC.f2;if(_tmp12 != 0)Cyc_Parse_err(({const char*_tmp15="bad type params in struct field";
_tag_arr(_tmp15,sizeof(char),_get_zero_arr_size(_tmp15,32));}),loc);if(Cyc_Absyn_is_qvar_qualified(
_tmpF))Cyc_Parse_err(({const char*_tmp16="struct or union field cannot be qualified with a namespace";
_tag_arr(_tmp16,sizeof(char),_get_zero_arr_size(_tmp16,59));}),loc);return({
struct Cyc_Absyn_Aggrfield*_tmp17=_cycalloc(sizeof(*_tmp17));_tmp17->name=(*_tmpF).f2;
_tmp17->tq=_tmp10;_tmp17->type=(void*)_tmp11;_tmp17->width=_tmp14;_tmp17->attributes=
_tmp13;_tmp17;});}static void*Cyc_Parse_type_spec(void*t,struct Cyc_Position_Segment*
loc){return(void*)({struct Cyc_Parse_Type_spec_struct*_tmp18=_cycalloc(sizeof(*
_tmp18));_tmp18[0]=({struct Cyc_Parse_Type_spec_struct _tmp19;_tmp19.tag=4;_tmp19.f1=(
void*)t;_tmp19.f2=loc;_tmp19;});_tmp18;});}static void*Cyc_Parse_array2ptr(void*t,
int argposn){void*_tmp1A=t;struct Cyc_Absyn_ArrayInfo _tmp1B;void*_tmp1C;struct Cyc_Absyn_Tqual
_tmp1D;struct Cyc_Absyn_Exp*_tmp1E;struct Cyc_Absyn_Conref*_tmp1F;_LL1: if(_tmp1A <= (
void*)3  || *((int*)_tmp1A)!= 7)goto _LL3;_tmp1B=((struct Cyc_Absyn_ArrayType_struct*)
_tmp1A)->f1;_tmp1C=(void*)_tmp1B.elt_type;_tmp1D=_tmp1B.tq;_tmp1E=_tmp1B.num_elts;
_tmp1F=_tmp1B.zero_term;_LL2: return Cyc_Absyn_starb_typ(_tmp1C,argposn?Cyc_Absyn_new_evar(({
struct Cyc_Core_Opt*_tmp20=_cycalloc(sizeof(*_tmp20));_tmp20->v=(void*)((void*)3);
_tmp20;}),0):(void*)2,_tmp1D,_tmp1E == 0?(void*)0:(void*)({struct Cyc_Absyn_Upper_b_struct*
_tmp21=_cycalloc(sizeof(*_tmp21));_tmp21[0]=({struct Cyc_Absyn_Upper_b_struct
_tmp22;_tmp22.tag=0;_tmp22.f1=(struct Cyc_Absyn_Exp*)_tmp1E;_tmp22;});_tmp21;}),
_tmp1F);_LL3:;_LL4: return t;_LL0:;}static void Cyc_Parse_arg_array2ptr(struct
_tuple2*x){(*x).f3=Cyc_Parse_array2ptr((*x).f3,1);}struct _tuple9{struct Cyc_Absyn_Tqual
f1;void*f2;};static struct _tuple9*Cyc_Parse_get_tqual_typ(struct Cyc_Position_Segment*
loc,struct _tuple2*t){return({struct _tuple9*_tmp23=_cycalloc(sizeof(*_tmp23));
_tmp23->f1=(*t).f2;_tmp23->f2=(*t).f3;_tmp23;});}static struct Cyc_Absyn_Vardecl*
Cyc_Parse_decl2vardecl(struct Cyc_Absyn_Decl*d){void*_tmp24=(void*)d->r;struct Cyc_Absyn_Vardecl*
_tmp25;_LL6: if(*((int*)_tmp24)!= 0)goto _LL8;_tmp25=((struct Cyc_Absyn_Var_d_struct*)
_tmp24)->f1;_LL7: return _tmp25;_LL8:;_LL9:({void*_tmp26[0]={};((int(*)(struct Cyc_Position_Segment*
sg,struct _tagged_arr fmt,struct _tagged_arr ap))Cyc_Parse_abort)(d->loc,({const char*
_tmp27="bad declaration in `forarray' statement";_tag_arr(_tmp27,sizeof(char),
_get_zero_arr_size(_tmp27,40));}),_tag_arr(_tmp26,sizeof(void*),0));});_LL5:;}
static int Cyc_Parse_is_typeparam(void*tm){void*_tmp28=tm;_LLB: if(*((int*)_tmp28)
!= 4)goto _LLD;_LLC: return 1;_LLD:;_LLE: return 0;_LLA:;}static void*Cyc_Parse_id2type(
struct _tagged_arr s,void*k){if(Cyc_zstrcmp((struct _tagged_arr)s,({const char*
_tmp29="`H";_tag_arr(_tmp29,sizeof(char),_get_zero_arr_size(_tmp29,3));}))== 0)
return(void*)2;else{return(void*)({struct Cyc_Absyn_VarType_struct*_tmp2A=
_cycalloc(sizeof(*_tmp2A));_tmp2A[0]=({struct Cyc_Absyn_VarType_struct _tmp2B;
_tmp2B.tag=1;_tmp2B.f1=({struct Cyc_Absyn_Tvar*_tmp2C=_cycalloc(sizeof(*_tmp2C));
_tmp2C->name=({struct _tagged_arr*_tmp2D=_cycalloc(sizeof(*_tmp2D));_tmp2D[0]=s;
_tmp2D;});_tmp2C->identity=0;_tmp2C->kind=(void*)k;_tmp2C;});_tmp2B;});_tmp2A;});}}
static struct Cyc_Absyn_Tvar*Cyc_Parse_copy_tvar(struct Cyc_Absyn_Tvar*t){void*k;{
void*_tmp2E=Cyc_Absyn_compress_kb((void*)t->kind);void*_tmp2F;void*_tmp30;_LL10:
if(*((int*)_tmp2E)!= 0)goto _LL12;_tmp2F=(void*)((struct Cyc_Absyn_Eq_kb_struct*)
_tmp2E)->f1;_LL11: k=(void*)({struct Cyc_Absyn_Eq_kb_struct*_tmp31=_cycalloc(
sizeof(*_tmp31));_tmp31[0]=({struct Cyc_Absyn_Eq_kb_struct _tmp32;_tmp32.tag=0;
_tmp32.f1=(void*)_tmp2F;_tmp32;});_tmp31;});goto _LLF;_LL12: if(*((int*)_tmp2E)!= 
1)goto _LL14;_LL13: k=(void*)({struct Cyc_Absyn_Unknown_kb_struct*_tmp33=_cycalloc(
sizeof(*_tmp33));_tmp33[0]=({struct Cyc_Absyn_Unknown_kb_struct _tmp34;_tmp34.tag=
1;_tmp34.f1=0;_tmp34;});_tmp33;});goto _LLF;_LL14: if(*((int*)_tmp2E)!= 2)goto _LLF;
_tmp30=(void*)((struct Cyc_Absyn_Less_kb_struct*)_tmp2E)->f2;_LL15: k=(void*)({
struct Cyc_Absyn_Less_kb_struct*_tmp35=_cycalloc(sizeof(*_tmp35));_tmp35[0]=({
struct Cyc_Absyn_Less_kb_struct _tmp36;_tmp36.tag=2;_tmp36.f1=0;_tmp36.f2=(void*)
_tmp30;_tmp36;});_tmp35;});goto _LLF;_LLF:;}return({struct Cyc_Absyn_Tvar*_tmp37=
_cycalloc(sizeof(*_tmp37));_tmp37->name=t->name;_tmp37->identity=0;_tmp37->kind=(
void*)k;_tmp37;});}static struct Cyc_Absyn_Tvar*Cyc_Parse_typ2tvar(struct Cyc_Position_Segment*
loc,void*t){void*_tmp38=t;struct Cyc_Absyn_Tvar*_tmp39;_LL17: if(_tmp38 <= (void*)3
 || *((int*)_tmp38)!= 1)goto _LL19;_tmp39=((struct Cyc_Absyn_VarType_struct*)
_tmp38)->f1;_LL18: return _tmp39;_LL19:;_LL1A:({void*_tmp3A[0]={};((int(*)(struct
Cyc_Position_Segment*sg,struct _tagged_arr fmt,struct _tagged_arr ap))Cyc_Parse_abort)(
loc,({const char*_tmp3B="expecting a list of type variables, not types";_tag_arr(
_tmp3B,sizeof(char),_get_zero_arr_size(_tmp3B,46));}),_tag_arr(_tmp3A,sizeof(
void*),0));});_LL16:;}static void*Cyc_Parse_tvar2typ(struct Cyc_Absyn_Tvar*pr){
return(void*)({struct Cyc_Absyn_VarType_struct*_tmp3C=_cycalloc(sizeof(*_tmp3C));
_tmp3C[0]=({struct Cyc_Absyn_VarType_struct _tmp3D;_tmp3D.tag=1;_tmp3D.f1=pr;
_tmp3D;});_tmp3C;});}static void Cyc_Parse_set_vartyp_kind(void*t,void*k){void*
_tmp3E=Cyc_Tcutil_compress(t);struct Cyc_Absyn_Tvar*_tmp3F;struct Cyc_Absyn_Tvar
_tmp40;void*_tmp41;void**_tmp42;_LL1C: if(_tmp3E <= (void*)3  || *((int*)_tmp3E)!= 
1)goto _LL1E;_tmp3F=((struct Cyc_Absyn_VarType_struct*)_tmp3E)->f1;_tmp40=*_tmp3F;
_tmp41=(void*)_tmp40.kind;_tmp42=(void**)&(*((struct Cyc_Absyn_VarType_struct*)
_tmp3E)->f1).kind;_LL1D: {void*_tmp43=Cyc_Absyn_compress_kb(*_tmp42);_LL21: if(*((
int*)_tmp43)!= 1)goto _LL23;_LL22:*_tmp42=(void*)({struct Cyc_Absyn_Eq_kb_struct*
_tmp44=_cycalloc(sizeof(*_tmp44));_tmp44[0]=({struct Cyc_Absyn_Eq_kb_struct _tmp45;
_tmp45.tag=0;_tmp45.f1=(void*)k;_tmp45;});_tmp44;});return;_LL23:;_LL24: return;
_LL20:;}_LL1E:;_LL1F: return;_LL1B:;}static struct Cyc_List_List*Cyc_Parse_oldstyle2newstyle(
struct Cyc_List_List*tms,struct Cyc_List_List*tds,struct Cyc_Position_Segment*loc){
if(tds == 0)return tms;if(tms == 0)return 0;{void*_tmp46=(void*)tms->hd;void*_tmp47;
_LL26: if(*((int*)_tmp46)!= 3)goto _LL28;_tmp47=(void*)((struct Cyc_Absyn_Function_mod_struct*)
_tmp46)->f1;_LL27: if(tms->tl == 0  || Cyc_Parse_is_typeparam((void*)((struct Cyc_List_List*)
_check_null(tms->tl))->hd) && ((struct Cyc_List_List*)_check_null(tms->tl))->tl == 
0){void*_tmp48=_tmp47;struct Cyc_List_List*_tmp49;_LL2B: if(*((int*)_tmp48)!= 1)
goto _LL2D;_LL2C:({void*_tmp4A[0]={};Cyc_Tcutil_warn(loc,({const char*_tmp4B="function declaration with both new- and old-style parameter declarations; ignoring old-style";
_tag_arr(_tmp4B,sizeof(char),_get_zero_arr_size(_tmp4B,93));}),_tag_arr(_tmp4A,
sizeof(void*),0));});return tms;_LL2D: if(*((int*)_tmp48)!= 0)goto _LL2A;_tmp49=((
struct Cyc_Absyn_NoTypes_struct*)_tmp48)->f1;_LL2E: if(((int(*)(struct Cyc_List_List*
x))Cyc_List_length)(_tmp49)!= ((int(*)(struct Cyc_List_List*x))Cyc_List_length)(
tds))({void*_tmp4C[0]={};((int(*)(struct Cyc_Position_Segment*sg,struct
_tagged_arr fmt,struct _tagged_arr ap))Cyc_Parse_abort)(loc,({const char*_tmp4D="wrong number of parameter declarations in old-style function declaration";
_tag_arr(_tmp4D,sizeof(char),_get_zero_arr_size(_tmp4D,73));}),_tag_arr(_tmp4C,
sizeof(void*),0));});{struct Cyc_List_List*_tmp4E=0;for(0;_tmp49 != 0;_tmp49=
_tmp49->tl){struct Cyc_List_List*_tmp4F=tds;for(0;_tmp4F != 0;_tmp4F=_tmp4F->tl){
struct Cyc_Absyn_Decl*_tmp50=(struct Cyc_Absyn_Decl*)_tmp4F->hd;void*_tmp51=(void*)
_tmp50->r;struct Cyc_Absyn_Vardecl*_tmp52;_LL30: if(*((int*)_tmp51)!= 0)goto _LL32;
_tmp52=((struct Cyc_Absyn_Var_d_struct*)_tmp51)->f1;_LL31: if(Cyc_zstrptrcmp((*
_tmp52->name).f2,(struct _tagged_arr*)_tmp49->hd)!= 0)continue;if(_tmp52->initializer
!= 0)({void*_tmp53[0]={};((int(*)(struct Cyc_Position_Segment*sg,struct
_tagged_arr fmt,struct _tagged_arr ap))Cyc_Parse_abort)(_tmp50->loc,({const char*
_tmp54="initializer found in parameter declaration";_tag_arr(_tmp54,sizeof(char),
_get_zero_arr_size(_tmp54,43));}),_tag_arr(_tmp53,sizeof(void*),0));});if(Cyc_Absyn_is_qvar_qualified(
_tmp52->name))({void*_tmp55[0]={};((int(*)(struct Cyc_Position_Segment*sg,struct
_tagged_arr fmt,struct _tagged_arr ap))Cyc_Parse_abort)(_tmp50->loc,({const char*
_tmp56="namespaces forbidden in parameter declarations";_tag_arr(_tmp56,sizeof(
char),_get_zero_arr_size(_tmp56,47));}),_tag_arr(_tmp55,sizeof(void*),0));});
_tmp4E=({struct Cyc_List_List*_tmp57=_cycalloc(sizeof(*_tmp57));_tmp57->hd=({
struct _tuple2*_tmp58=_cycalloc(sizeof(*_tmp58));_tmp58->f1=({struct Cyc_Core_Opt*
_tmp59=_cycalloc(sizeof(*_tmp59));_tmp59->v=(*_tmp52->name).f2;_tmp59;});_tmp58->f2=
_tmp52->tq;_tmp58->f3=(void*)_tmp52->type;_tmp58;});_tmp57->tl=_tmp4E;_tmp57;});
goto L;_LL32:;_LL33:({void*_tmp5A[0]={};((int(*)(struct Cyc_Position_Segment*sg,
struct _tagged_arr fmt,struct _tagged_arr ap))Cyc_Parse_abort)(_tmp50->loc,({const
char*_tmp5B="nonvariable declaration in parameter type";_tag_arr(_tmp5B,sizeof(
char),_get_zero_arr_size(_tmp5B,42));}),_tag_arr(_tmp5A,sizeof(void*),0));});
_LL2F:;}L: if(_tmp4F == 0)({struct Cyc_String_pa_struct _tmp5E;_tmp5E.tag=0;_tmp5E.f1=(
struct _tagged_arr)((struct _tagged_arr)*((struct _tagged_arr*)_tmp49->hd));{void*
_tmp5C[1]={& _tmp5E};((int(*)(struct Cyc_Position_Segment*sg,struct _tagged_arr fmt,
struct _tagged_arr ap))Cyc_Parse_abort)(loc,({const char*_tmp5D="%s is not given a type";
_tag_arr(_tmp5D,sizeof(char),_get_zero_arr_size(_tmp5D,23));}),_tag_arr(_tmp5C,
sizeof(void*),1));}});}return({struct Cyc_List_List*_tmp5F=_cycalloc(sizeof(*
_tmp5F));_tmp5F->hd=(void*)((void*)({struct Cyc_Absyn_Function_mod_struct*_tmp60=
_cycalloc(sizeof(*_tmp60));_tmp60[0]=({struct Cyc_Absyn_Function_mod_struct _tmp61;
_tmp61.tag=3;_tmp61.f1=(void*)((void*)({struct Cyc_Absyn_WithTypes_struct*_tmp62=
_cycalloc(sizeof(*_tmp62));_tmp62[0]=({struct Cyc_Absyn_WithTypes_struct _tmp63;
_tmp63.tag=1;_tmp63.f1=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(
_tmp4E);_tmp63.f2=0;_tmp63.f3=0;_tmp63.f4=0;_tmp63.f5=0;_tmp63;});_tmp62;}));
_tmp61;});_tmp60;}));_tmp5F->tl=0;_tmp5F;});}_LL2A:;}goto _LL29;_LL28:;_LL29:
return({struct Cyc_List_List*_tmp64=_cycalloc(sizeof(*_tmp64));_tmp64->hd=(void*)((
void*)tms->hd);_tmp64->tl=Cyc_Parse_oldstyle2newstyle(tms->tl,tds,loc);_tmp64;});
_LL25:;}}struct _tuple10{struct _tagged_arr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};
static struct Cyc_Absyn_Fndecl*Cyc_Parse_make_function(struct Cyc_Core_Opt*dso,
struct Cyc_Parse_Declarator*d,struct Cyc_List_List*tds,struct Cyc_Absyn_Stmt*body,
struct Cyc_Position_Segment*loc){if(tds != 0)d=({struct Cyc_Parse_Declarator*_tmp65=
_cycalloc(sizeof(*_tmp65));_tmp65->id=d->id;_tmp65->tms=Cyc_Parse_oldstyle2newstyle(
d->tms,tds,loc);_tmp65;});{void*sc=(void*)2;struct Cyc_List_List*tss=0;struct Cyc_Absyn_Tqual
tq=Cyc_Absyn_empty_tqual();int is_inline=0;struct Cyc_List_List*atts=0;if(dso != 0){
tss=((struct Cyc_Parse_Declaration_spec*)dso->v)->type_specs;tq=((struct Cyc_Parse_Declaration_spec*)
dso->v)->tq;is_inline=((struct Cyc_Parse_Declaration_spec*)dso->v)->is_inline;
atts=((struct Cyc_Parse_Declaration_spec*)dso->v)->attributes;if(((struct Cyc_Parse_Declaration_spec*)
dso->v)->sc != 0){void*_tmp66=(void*)((struct Cyc_Core_Opt*)_check_null(((struct
Cyc_Parse_Declaration_spec*)dso->v)->sc))->v;_LL35: if((int)_tmp66 != 1)goto _LL37;
_LL36: sc=(void*)3;goto _LL34;_LL37: if((int)_tmp66 != 3)goto _LL39;_LL38: sc=(void*)0;
goto _LL34;_LL39:;_LL3A: Cyc_Parse_err(({const char*_tmp67="bad storage class on function";
_tag_arr(_tmp67,sizeof(char),_get_zero_arr_size(_tmp67,30));}),loc);goto _LL34;
_LL34:;}}{void*_tmp69;struct Cyc_Core_Opt*_tmp6A;struct _tuple5 _tmp68=Cyc_Parse_collapse_type_specifiers(
tss,loc);_tmp69=_tmp68.f1;_tmp6A=_tmp68.f2;{struct Cyc_Absyn_Tqual _tmp6C;void*
_tmp6D;struct Cyc_List_List*_tmp6E;struct Cyc_List_List*_tmp6F;struct _tuple6 _tmp6B=
Cyc_Parse_apply_tms(tq,_tmp69,atts,d->tms);_tmp6C=_tmp6B.f1;_tmp6D=_tmp6B.f2;
_tmp6E=_tmp6B.f3;_tmp6F=_tmp6B.f4;if(_tmp6A != 0)({void*_tmp70[0]={};Cyc_Tcutil_warn(
loc,({const char*_tmp71="nested type declaration within function prototype";
_tag_arr(_tmp71,sizeof(char),_get_zero_arr_size(_tmp71,50));}),_tag_arr(_tmp70,
sizeof(void*),0));});if(_tmp6E != 0)({void*_tmp72[0]={};Cyc_Tcutil_warn(loc,({
const char*_tmp73="bad type params, ignoring";_tag_arr(_tmp73,sizeof(char),
_get_zero_arr_size(_tmp73,26));}),_tag_arr(_tmp72,sizeof(void*),0));});{void*
_tmp74=_tmp6D;struct Cyc_Absyn_FnInfo _tmp75;struct Cyc_List_List*_tmp76;struct Cyc_Core_Opt*
_tmp77;void*_tmp78;struct Cyc_List_List*_tmp79;int _tmp7A;struct Cyc_Absyn_VarargInfo*
_tmp7B;struct Cyc_List_List*_tmp7C;struct Cyc_List_List*_tmp7D;_LL3C: if(_tmp74 <= (
void*)3  || *((int*)_tmp74)!= 8)goto _LL3E;_tmp75=((struct Cyc_Absyn_FnType_struct*)
_tmp74)->f1;_tmp76=_tmp75.tvars;_tmp77=_tmp75.effect;_tmp78=(void*)_tmp75.ret_typ;
_tmp79=_tmp75.args;_tmp7A=_tmp75.c_varargs;_tmp7B=_tmp75.cyc_varargs;_tmp7C=
_tmp75.rgn_po;_tmp7D=_tmp75.attributes;_LL3D: {struct Cyc_List_List*_tmp7E=0;{
struct Cyc_List_List*_tmp7F=_tmp79;for(0;_tmp7F != 0;_tmp7F=_tmp7F->tl){struct
_tuple2 _tmp81;struct Cyc_Core_Opt*_tmp82;struct Cyc_Absyn_Tqual _tmp83;void*_tmp84;
struct _tuple2*_tmp80=(struct _tuple2*)_tmp7F->hd;_tmp81=*_tmp80;_tmp82=_tmp81.f1;
_tmp83=_tmp81.f2;_tmp84=_tmp81.f3;if(_tmp82 == 0){Cyc_Parse_err(({const char*
_tmp85="missing argument variable in function prototype";_tag_arr(_tmp85,sizeof(
char),_get_zero_arr_size(_tmp85,48));}),loc);_tmp7E=({struct Cyc_List_List*_tmp86=
_cycalloc(sizeof(*_tmp86));_tmp86->hd=({struct _tuple10*_tmp87=_cycalloc(sizeof(*
_tmp87));_tmp87->f1=_init_tag_arr(_cycalloc(sizeof(struct _tagged_arr)),"?",
sizeof(char),2);_tmp87->f2=_tmp83;_tmp87->f3=_tmp84;_tmp87;});_tmp86->tl=_tmp7E;
_tmp86;});}else{_tmp7E=({struct Cyc_List_List*_tmp89=_cycalloc(sizeof(*_tmp89));
_tmp89->hd=({struct _tuple10*_tmp8A=_cycalloc(sizeof(*_tmp8A));_tmp8A->f1=(struct
_tagged_arr*)_tmp82->v;_tmp8A->f2=_tmp83;_tmp8A->f3=_tmp84;_tmp8A;});_tmp89->tl=
_tmp7E;_tmp89;});}}}return({struct Cyc_Absyn_Fndecl*_tmp8B=_cycalloc(sizeof(*
_tmp8B));_tmp8B->sc=(void*)sc;_tmp8B->name=d->id;_tmp8B->tvs=_tmp76;_tmp8B->is_inline=
is_inline;_tmp8B->effect=_tmp77;_tmp8B->ret_type=(void*)_tmp78;_tmp8B->args=((
struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(_tmp7E);_tmp8B->c_varargs=
_tmp7A;_tmp8B->cyc_varargs=_tmp7B;_tmp8B->rgn_po=_tmp7C;_tmp8B->body=body;_tmp8B->cached_typ=
0;_tmp8B->param_vardecls=0;_tmp8B->attributes=Cyc_List_append(_tmp7D,_tmp6F);
_tmp8B;});}_LL3E:;_LL3F:({void*_tmp8C[0]={};((int(*)(struct Cyc_Position_Segment*
sg,struct _tagged_arr fmt,struct _tagged_arr ap))Cyc_Parse_abort)(loc,({const char*
_tmp8D="declarator is not a function prototype";_tag_arr(_tmp8D,sizeof(char),
_get_zero_arr_size(_tmp8D,39));}),_tag_arr(_tmp8C,sizeof(void*),0));});_LL3B:;}}}}}
static char _tmp8E[52]="at most one type may appear within a type specifier";
static struct _tagged_arr Cyc_Parse_msg1={_tmp8E,_tmp8E,_tmp8E + 52};static char
_tmp8F[63]="const or volatile may appear only once within a type specifier";
static struct _tagged_arr Cyc_Parse_msg2={_tmp8F,_tmp8F,_tmp8F + 63};static char
_tmp90[50]="type specifier includes more than one declaration";static struct
_tagged_arr Cyc_Parse_msg3={_tmp90,_tmp90,_tmp90 + 50};static char _tmp91[60]="sign specifier may appear only once within a type specifier";
static struct _tagged_arr Cyc_Parse_msg4={_tmp91,_tmp91,_tmp91 + 60};static struct
_tuple5 Cyc_Parse_collapse_type_specifiers(struct Cyc_List_List*ts,struct Cyc_Position_Segment*
loc){struct Cyc_Core_Opt*declopt=0;int seen_type=0;int seen_sign=0;int seen_size=0;
void*t=(void*)0;void*sz=(void*)2;void*sgn=(void*)0;struct Cyc_Position_Segment*
last_loc=loc;for(0;ts != 0;ts=ts->tl){void*_tmp92=(void*)ts->hd;void*_tmp93;
struct Cyc_Position_Segment*_tmp94;struct Cyc_Position_Segment*_tmp95;struct Cyc_Position_Segment*
_tmp96;struct Cyc_Position_Segment*_tmp97;struct Cyc_Position_Segment*_tmp98;
struct Cyc_Absyn_Decl*_tmp99;_LL41: if(*((int*)_tmp92)!= 4)goto _LL43;_tmp93=(void*)((
struct Cyc_Parse_Type_spec_struct*)_tmp92)->f1;_tmp94=((struct Cyc_Parse_Type_spec_struct*)
_tmp92)->f2;_LL42: if(seen_type)Cyc_Parse_err(Cyc_Parse_msg1,_tmp94);last_loc=
_tmp94;seen_type=1;t=_tmp93;goto _LL40;_LL43: if(*((int*)_tmp92)!= 0)goto _LL45;
_tmp95=((struct Cyc_Parse_Signed_spec_struct*)_tmp92)->f1;_LL44: if(seen_sign)Cyc_Parse_err(
Cyc_Parse_msg4,_tmp95);if(seen_type)Cyc_Parse_err(({const char*_tmp9A="signed qualifier must come before type";
_tag_arr(_tmp9A,sizeof(char),_get_zero_arr_size(_tmp9A,39));}),_tmp95);last_loc=
_tmp95;seen_sign=1;sgn=(void*)0;goto _LL40;_LL45: if(*((int*)_tmp92)!= 1)goto _LL47;
_tmp96=((struct Cyc_Parse_Unsigned_spec_struct*)_tmp92)->f1;_LL46: if(seen_sign)
Cyc_Parse_err(Cyc_Parse_msg4,_tmp96);if(seen_type)Cyc_Parse_err(({const char*
_tmp9B="signed qualifier must come before type";_tag_arr(_tmp9B,sizeof(char),
_get_zero_arr_size(_tmp9B,39));}),_tmp96);last_loc=_tmp96;seen_sign=1;sgn=(void*)
1;goto _LL40;_LL47: if(*((int*)_tmp92)!= 2)goto _LL49;_tmp97=((struct Cyc_Parse_Short_spec_struct*)
_tmp92)->f1;_LL48: if(seen_size)Cyc_Parse_err(({const char*_tmp9C="integral size may appear only once within a type specifier";
_tag_arr(_tmp9C,sizeof(char),_get_zero_arr_size(_tmp9C,59));}),_tmp97);if(
seen_type)Cyc_Parse_err(({const char*_tmp9D="short modifier must come before base type";
_tag_arr(_tmp9D,sizeof(char),_get_zero_arr_size(_tmp9D,42));}),_tmp97);last_loc=
_tmp97;seen_size=1;sz=(void*)1;goto _LL40;_LL49: if(*((int*)_tmp92)!= 3)goto _LL4B;
_tmp98=((struct Cyc_Parse_Long_spec_struct*)_tmp92)->f1;_LL4A: if(seen_type)Cyc_Parse_err(({
const char*_tmp9E="long modifier must come before base type";_tag_arr(_tmp9E,
sizeof(char),_get_zero_arr_size(_tmp9E,41));}),_tmp98);if(seen_size){void*_tmp9F=
sz;_LL4E: if((int)_tmp9F != 2)goto _LL50;_LL4F: sz=(void*)3;goto _LL4D;_LL50:;_LL51:
Cyc_Parse_err(({const char*_tmpA0="extra long in type specifier";_tag_arr(_tmpA0,
sizeof(char),_get_zero_arr_size(_tmpA0,29));}),_tmp98);goto _LL4D;_LL4D:;}
last_loc=_tmp98;seen_size=1;goto _LL40;_LL4B: if(*((int*)_tmp92)!= 5)goto _LL40;
_tmp99=((struct Cyc_Parse_Decl_spec_struct*)_tmp92)->f1;_LL4C: last_loc=_tmp99->loc;
if(declopt == 0  && !seen_type){seen_type=1;{void*_tmpA1=(void*)_tmp99->r;struct
Cyc_Absyn_Aggrdecl*_tmpA2;struct Cyc_Absyn_Tuniondecl*_tmpA3;struct Cyc_Absyn_Enumdecl*
_tmpA4;_LL53: if(*((int*)_tmpA1)!= 4)goto _LL55;_tmpA2=((struct Cyc_Absyn_Aggr_d_struct*)
_tmpA1)->f1;_LL54: {struct Cyc_List_List*_tmpA5=((struct Cyc_List_List*(*)(void*(*
f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Parse_tvar2typ,((
struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Absyn_Tvar*),struct
Cyc_List_List*x))Cyc_List_map)(Cyc_Parse_copy_tvar,_tmpA2->tvs));t=(void*)({
struct Cyc_Absyn_AggrType_struct*_tmpA6=_cycalloc(sizeof(*_tmpA6));_tmpA6[0]=({
struct Cyc_Absyn_AggrType_struct _tmpA7;_tmpA7.tag=10;_tmpA7.f1=({struct Cyc_Absyn_AggrInfo
_tmpA8;_tmpA8.aggr_info=(void*)((void*)({struct Cyc_Absyn_UnknownAggr_struct*
_tmpA9=_cycalloc(sizeof(*_tmpA9));_tmpA9[0]=({struct Cyc_Absyn_UnknownAggr_struct
_tmpAA;_tmpAA.tag=0;_tmpAA.f1=(void*)((void*)_tmpA2->kind);_tmpAA.f2=_tmpA2->name;
_tmpAA;});_tmpA9;}));_tmpA8.targs=_tmpA5;_tmpA8;});_tmpA7;});_tmpA6;});if(_tmpA2->impl
!= 0)declopt=({struct Cyc_Core_Opt*_tmpAB=_cycalloc(sizeof(*_tmpAB));_tmpAB->v=
_tmp99;_tmpAB;});goto _LL52;}_LL55: if(*((int*)_tmpA1)!= 5)goto _LL57;_tmpA3=((
struct Cyc_Absyn_Tunion_d_struct*)_tmpA1)->f1;_LL56: {struct Cyc_List_List*_tmpAC=((
struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))
Cyc_List_map)(Cyc_Parse_tvar2typ,((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*
f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Parse_copy_tvar,
_tmpA3->tvs));t=(void*)({struct Cyc_Absyn_TunionType_struct*_tmpAD=_cycalloc(
sizeof(*_tmpAD));_tmpAD[0]=({struct Cyc_Absyn_TunionType_struct _tmpAE;_tmpAE.tag=
2;_tmpAE.f1=({struct Cyc_Absyn_TunionInfo _tmpAF;_tmpAF.tunion_info=(void*)((void*)({
struct Cyc_Absyn_KnownTunion_struct*_tmpB0=_cycalloc(sizeof(*_tmpB0));_tmpB0[0]=({
struct Cyc_Absyn_KnownTunion_struct _tmpB1;_tmpB1.tag=1;_tmpB1.f1=({struct Cyc_Absyn_Tuniondecl**
_tmpB2=_cycalloc(sizeof(*_tmpB2));_tmpB2[0]=_tmpA3;_tmpB2;});_tmpB1;});_tmpB0;}));
_tmpAF.targs=_tmpAC;_tmpAF.rgn=(void*)((void*)2);_tmpAF;});_tmpAE;});_tmpAD;});
if(_tmpA3->fields != 0)declopt=({struct Cyc_Core_Opt*_tmpB3=_cycalloc(sizeof(*
_tmpB3));_tmpB3->v=_tmp99;_tmpB3;});goto _LL52;}_LL57: if(*((int*)_tmpA1)!= 6)goto
_LL59;_tmpA4=((struct Cyc_Absyn_Enum_d_struct*)_tmpA1)->f1;_LL58: t=(void*)({
struct Cyc_Absyn_EnumType_struct*_tmpB4=_cycalloc(sizeof(*_tmpB4));_tmpB4[0]=({
struct Cyc_Absyn_EnumType_struct _tmpB5;_tmpB5.tag=12;_tmpB5.f1=_tmpA4->name;
_tmpB5.f2=0;_tmpB5;});_tmpB4;});declopt=({struct Cyc_Core_Opt*_tmpB6=_cycalloc(
sizeof(*_tmpB6));_tmpB6->v=_tmp99;_tmpB6;});goto _LL52;_LL59:;_LL5A:({void*_tmpB7[
0]={};((int(*)(struct Cyc_Position_Segment*sg,struct _tagged_arr fmt,struct
_tagged_arr ap))Cyc_Parse_abort)(_tmp99->loc,({const char*_tmpB8="bad declaration within type specifier";
_tag_arr(_tmpB8,sizeof(char),_get_zero_arr_size(_tmpB8,38));}),_tag_arr(_tmpB7,
sizeof(void*),0));});_LL52:;}}else{Cyc_Parse_err(Cyc_Parse_msg3,_tmp99->loc);}
goto _LL40;_LL40:;}if(!seen_type){if(!seen_sign  && !seen_size)({void*_tmpB9[0]={};
Cyc_Tcutil_warn(last_loc,({const char*_tmpBA="missing type within specifier";
_tag_arr(_tmpBA,sizeof(char),_get_zero_arr_size(_tmpBA,30));}),_tag_arr(_tmpB9,
sizeof(void*),0));});t=(void*)({struct Cyc_Absyn_IntType_struct*_tmpBB=_cycalloc(
sizeof(*_tmpBB));_tmpBB[0]=({struct Cyc_Absyn_IntType_struct _tmpBC;_tmpBC.tag=5;
_tmpBC.f1=(void*)sgn;_tmpBC.f2=(void*)sz;_tmpBC;});_tmpBB;});}else{if(seen_sign){
void*_tmpBD=t;void*_tmpBE;_LL5C: if(_tmpBD <= (void*)3  || *((int*)_tmpBD)!= 5)goto
_LL5E;_tmpBE=(void*)((struct Cyc_Absyn_IntType_struct*)_tmpBD)->f2;_LL5D: t=(void*)({
struct Cyc_Absyn_IntType_struct*_tmpBF=_cycalloc(sizeof(*_tmpBF));_tmpBF[0]=({
struct Cyc_Absyn_IntType_struct _tmpC0;_tmpC0.tag=5;_tmpC0.f1=(void*)sgn;_tmpC0.f2=(
void*)_tmpBE;_tmpC0;});_tmpBF;});goto _LL5B;_LL5E:;_LL5F: Cyc_Parse_err(({const
char*_tmpC1="sign specification on non-integral type";_tag_arr(_tmpC1,sizeof(
char),_get_zero_arr_size(_tmpC1,40));}),last_loc);goto _LL5B;_LL5B:;}if(seen_size){
void*_tmpC2=t;void*_tmpC3;_LL61: if(_tmpC2 <= (void*)3  || *((int*)_tmpC2)!= 5)goto
_LL63;_tmpC3=(void*)((struct Cyc_Absyn_IntType_struct*)_tmpC2)->f1;_LL62: t=(void*)({
struct Cyc_Absyn_IntType_struct*_tmpC4=_cycalloc(sizeof(*_tmpC4));_tmpC4[0]=({
struct Cyc_Absyn_IntType_struct _tmpC5;_tmpC5.tag=5;_tmpC5.f1=(void*)_tmpC3;_tmpC5.f2=(
void*)sz;_tmpC5;});_tmpC4;});goto _LL60;_LL63: if(_tmpC2 <= (void*)3  || *((int*)
_tmpC2)!= 6)goto _LL65;_LL64: t=(void*)({struct Cyc_Absyn_DoubleType_struct*_tmpC6=
_cycalloc_atomic(sizeof(*_tmpC6));_tmpC6[0]=({struct Cyc_Absyn_DoubleType_struct
_tmpC7;_tmpC7.tag=6;_tmpC7.f1=1;_tmpC7;});_tmpC6;});goto _LL60;_LL65:;_LL66: Cyc_Parse_err(({
const char*_tmpC8="size qualifier on non-integral type";_tag_arr(_tmpC8,sizeof(
char),_get_zero_arr_size(_tmpC8,36));}),last_loc);goto _LL60;_LL60:;}}return({
struct _tuple5 _tmpC9;_tmpC9.f1=t;_tmpC9.f2=declopt;_tmpC9;});}static struct Cyc_List_List*
Cyc_Parse_apply_tmss(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_List_List*ds,
struct Cyc_List_List*shared_atts){if(ds == 0)return 0;{struct Cyc_Parse_Declarator*
_tmpCA=(struct Cyc_Parse_Declarator*)ds->hd;struct _tuple1*_tmpCB=_tmpCA->id;
struct Cyc_Absyn_Tqual _tmpCD;void*_tmpCE;struct Cyc_List_List*_tmpCF;struct Cyc_List_List*
_tmpD0;struct _tuple6 _tmpCC=Cyc_Parse_apply_tms(tq,t,shared_atts,_tmpCA->tms);
_tmpCD=_tmpCC.f1;_tmpCE=_tmpCC.f2;_tmpCF=_tmpCC.f3;_tmpD0=_tmpCC.f4;return({
struct Cyc_List_List*_tmpD1=_cycalloc(sizeof(*_tmpD1));_tmpD1->hd=({struct _tuple7*
_tmpD2=_cycalloc(sizeof(*_tmpD2));_tmpD2->f1=_tmpCB;_tmpD2->f2=_tmpCD;_tmpD2->f3=
_tmpCE;_tmpD2->f4=_tmpCF;_tmpD2->f5=_tmpD0;_tmpD2;});_tmpD1->tl=Cyc_Parse_apply_tmss(
_tmpCD,Cyc_Tcutil_copy_type(t),ds->tl,shared_atts);_tmpD1;});}}static struct
_tuple6 Cyc_Parse_apply_tms(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_List_List*
atts,struct Cyc_List_List*tms){if(tms == 0)return({struct _tuple6 _tmpD3;_tmpD3.f1=
tq;_tmpD3.f2=t;_tmpD3.f3=0;_tmpD3.f4=atts;_tmpD3;});{void*_tmpD4=(void*)tms->hd;
struct Cyc_Absyn_Conref*_tmpD5;struct Cyc_Absyn_Exp*_tmpD6;struct Cyc_Absyn_Conref*
_tmpD7;void*_tmpD8;struct Cyc_List_List*_tmpD9;struct Cyc_Position_Segment*_tmpDA;
struct Cyc_Absyn_PtrAtts _tmpDB;struct Cyc_Absyn_Tqual _tmpDC;struct Cyc_Position_Segment*
_tmpDD;struct Cyc_List_List*_tmpDE;_LL68: if(*((int*)_tmpD4)!= 0)goto _LL6A;_tmpD5=((
struct Cyc_Absyn_Carray_mod_struct*)_tmpD4)->f1;_LL69: return Cyc_Parse_apply_tms(
Cyc_Absyn_empty_tqual(),Cyc_Absyn_array_typ(t,tq,0,_tmpD5),atts,tms->tl);_LL6A:
if(*((int*)_tmpD4)!= 1)goto _LL6C;_tmpD6=((struct Cyc_Absyn_ConstArray_mod_struct*)
_tmpD4)->f1;_tmpD7=((struct Cyc_Absyn_ConstArray_mod_struct*)_tmpD4)->f2;_LL6B:
return Cyc_Parse_apply_tms(Cyc_Absyn_empty_tqual(),Cyc_Absyn_array_typ(t,tq,(
struct Cyc_Absyn_Exp*)_tmpD6,_tmpD7),atts,tms->tl);_LL6C: if(*((int*)_tmpD4)!= 3)
goto _LL6E;_tmpD8=(void*)((struct Cyc_Absyn_Function_mod_struct*)_tmpD4)->f1;_LL6D: {
void*_tmpDF=_tmpD8;struct Cyc_List_List*_tmpE0;int _tmpE1;struct Cyc_Absyn_VarargInfo*
_tmpE2;struct Cyc_Core_Opt*_tmpE3;struct Cyc_List_List*_tmpE4;struct Cyc_Position_Segment*
_tmpE5;_LL75: if(*((int*)_tmpDF)!= 1)goto _LL77;_tmpE0=((struct Cyc_Absyn_WithTypes_struct*)
_tmpDF)->f1;_tmpE1=((struct Cyc_Absyn_WithTypes_struct*)_tmpDF)->f2;_tmpE2=((
struct Cyc_Absyn_WithTypes_struct*)_tmpDF)->f3;_tmpE3=((struct Cyc_Absyn_WithTypes_struct*)
_tmpDF)->f4;_tmpE4=((struct Cyc_Absyn_WithTypes_struct*)_tmpDF)->f5;_LL76: {
struct Cyc_List_List*typvars=0;struct Cyc_List_List*fn_atts=0;struct Cyc_List_List*
new_atts=0;{struct Cyc_List_List*as=atts;for(0;as != 0;as=as->tl){if(Cyc_Absyn_fntype_att((
void*)as->hd))fn_atts=({struct Cyc_List_List*_tmpE6=_cycalloc(sizeof(*_tmpE6));
_tmpE6->hd=(void*)((void*)as->hd);_tmpE6->tl=fn_atts;_tmpE6;});else{new_atts=({
struct Cyc_List_List*_tmpE7=_cycalloc(sizeof(*_tmpE7));_tmpE7->hd=(void*)((void*)
as->hd);_tmpE7->tl=new_atts;_tmpE7;});}}}if(tms->tl != 0){void*_tmpE8=(void*)((
struct Cyc_List_List*)_check_null(tms->tl))->hd;struct Cyc_List_List*_tmpE9;_LL7A:
if(*((int*)_tmpE8)!= 4)goto _LL7C;_tmpE9=((struct Cyc_Absyn_TypeParams_mod_struct*)
_tmpE8)->f1;_LL7B: typvars=_tmpE9;tms=tms->tl;goto _LL79;_LL7C:;_LL7D: goto _LL79;
_LL79:;}if(((((!_tmpE1  && _tmpE2 == 0) && _tmpE0 != 0) && _tmpE0->tl == 0) && (*((
struct _tuple2*)_tmpE0->hd)).f1 == 0) && (*((struct _tuple2*)_tmpE0->hd)).f3 == (
void*)0)_tmpE0=0;t=Cyc_Parse_array2ptr(t,0);((void(*)(void(*f)(struct _tuple2*),
struct Cyc_List_List*x))Cyc_List_iter)(Cyc_Parse_arg_array2ptr,_tmpE0);return Cyc_Parse_apply_tms(
Cyc_Absyn_empty_tqual(),Cyc_Absyn_function_typ(typvars,_tmpE3,t,_tmpE0,_tmpE1,
_tmpE2,_tmpE4,fn_atts),new_atts,((struct Cyc_List_List*)_check_null(tms))->tl);}
_LL77: if(*((int*)_tmpDF)!= 0)goto _LL74;_tmpE5=((struct Cyc_Absyn_NoTypes_struct*)
_tmpDF)->f2;_LL78:({void*_tmpEA[0]={};((int(*)(struct Cyc_Position_Segment*sg,
struct _tagged_arr fmt,struct _tagged_arr ap))Cyc_Parse_abort)(_tmpE5,({const char*
_tmpEB="function declaration without parameter types";_tag_arr(_tmpEB,sizeof(
char),_get_zero_arr_size(_tmpEB,45));}),_tag_arr(_tmpEA,sizeof(void*),0));});
_LL74:;}_LL6E: if(*((int*)_tmpD4)!= 4)goto _LL70;_tmpD9=((struct Cyc_Absyn_TypeParams_mod_struct*)
_tmpD4)->f1;_tmpDA=((struct Cyc_Absyn_TypeParams_mod_struct*)_tmpD4)->f2;_LL6F:
if(tms->tl == 0)return({struct _tuple6 _tmpEC;_tmpEC.f1=tq;_tmpEC.f2=t;_tmpEC.f3=
_tmpD9;_tmpEC.f4=atts;_tmpEC;});({void*_tmpED[0]={};((int(*)(struct Cyc_Position_Segment*
sg,struct _tagged_arr fmt,struct _tagged_arr ap))Cyc_Parse_abort)(_tmpDA,({const char*
_tmpEE="type parameters must appear before function arguments in declarator";
_tag_arr(_tmpEE,sizeof(char),_get_zero_arr_size(_tmpEE,68));}),_tag_arr(_tmpED,
sizeof(void*),0));});_LL70: if(*((int*)_tmpD4)!= 2)goto _LL72;_tmpDB=((struct Cyc_Absyn_Pointer_mod_struct*)
_tmpD4)->f1;_tmpDC=((struct Cyc_Absyn_Pointer_mod_struct*)_tmpD4)->f2;_LL71:
return Cyc_Parse_apply_tms(_tmpDC,(void*)({struct Cyc_Absyn_PointerType_struct*
_tmpEF=_cycalloc(sizeof(*_tmpEF));_tmpEF[0]=({struct Cyc_Absyn_PointerType_struct
_tmpF0;_tmpF0.tag=4;_tmpF0.f1=({struct Cyc_Absyn_PtrInfo _tmpF1;_tmpF1.elt_typ=(
void*)t;_tmpF1.elt_tq=tq;_tmpF1.ptr_atts=_tmpDB;_tmpF1;});_tmpF0;});_tmpEF;}),
atts,tms->tl);_LL72: if(*((int*)_tmpD4)!= 5)goto _LL67;_tmpDD=((struct Cyc_Absyn_Attributes_mod_struct*)
_tmpD4)->f1;_tmpDE=((struct Cyc_Absyn_Attributes_mod_struct*)_tmpD4)->f2;_LL73:
return Cyc_Parse_apply_tms(tq,t,Cyc_List_append(atts,_tmpDE),tms->tl);_LL67:;}}
void*Cyc_Parse_speclist2typ(struct Cyc_List_List*tss,struct Cyc_Position_Segment*
loc){void*_tmpF3;struct Cyc_Core_Opt*_tmpF4;struct _tuple5 _tmpF2=Cyc_Parse_collapse_type_specifiers(
tss,loc);_tmpF3=_tmpF2.f1;_tmpF4=_tmpF2.f2;if(_tmpF4 != 0)({void*_tmpF5[0]={};Cyc_Tcutil_warn(
loc,({const char*_tmpF6="ignoring nested type declaration(s) in specifier list";
_tag_arr(_tmpF6,sizeof(char),_get_zero_arr_size(_tmpF6,54));}),_tag_arr(_tmpF5,
sizeof(void*),0));});return _tmpF3;}static struct Cyc_Absyn_Decl*Cyc_Parse_v_typ_to_typedef(
struct Cyc_Position_Segment*loc,struct _tuple7*t){struct _tuple7 _tmpF8;struct
_tuple1*_tmpF9;struct Cyc_Absyn_Tqual _tmpFA;void*_tmpFB;struct Cyc_List_List*
_tmpFC;struct Cyc_List_List*_tmpFD;struct _tuple7*_tmpF7=t;_tmpF8=*_tmpF7;_tmpF9=
_tmpF8.f1;_tmpFA=_tmpF8.f2;_tmpFB=_tmpF8.f3;_tmpFC=_tmpF8.f4;_tmpFD=_tmpF8.f5;
Cyc_Lex_register_typedef(_tmpF9);{struct Cyc_Core_Opt*kind;struct Cyc_Core_Opt*
type;{void*_tmpFE=_tmpFB;struct Cyc_Core_Opt*_tmpFF;_LL7F: if(_tmpFE <= (void*)3
 || *((int*)_tmpFE)!= 0)goto _LL81;_tmpFF=((struct Cyc_Absyn_Evar_struct*)_tmpFE)->f1;
_LL80: type=0;if(_tmpFF == 0)kind=({struct Cyc_Core_Opt*_tmp100=_cycalloc(sizeof(*
_tmp100));_tmp100->v=(void*)((void*)2);_tmp100;});else{kind=_tmpFF;}goto _LL7E;
_LL81:;_LL82: kind=0;type=({struct Cyc_Core_Opt*_tmp101=_cycalloc(sizeof(*_tmp101));
_tmp101->v=(void*)_tmpFB;_tmp101;});goto _LL7E;_LL7E:;}return Cyc_Absyn_new_decl((
void*)({struct Cyc_Absyn_Typedef_d_struct*_tmp102=_cycalloc(sizeof(*_tmp102));
_tmp102[0]=({struct Cyc_Absyn_Typedef_d_struct _tmp103;_tmp103.tag=7;_tmp103.f1=({
struct Cyc_Absyn_Typedefdecl*_tmp104=_cycalloc(sizeof(*_tmp104));_tmp104->name=
_tmpF9;_tmp104->tvs=_tmpFC;_tmp104->kind=kind;_tmp104->defn=type;_tmp104->atts=
_tmpFD;_tmp104->tq=_tmpFA;_tmp104;});_tmp103;});_tmp102;}),loc);}}static struct
Cyc_Absyn_Stmt*Cyc_Parse_flatten_decl(struct Cyc_Absyn_Decl*d,struct Cyc_Absyn_Stmt*
s){return Cyc_Absyn_new_stmt((void*)({struct Cyc_Absyn_Decl_s_struct*_tmp105=
_cycalloc(sizeof(*_tmp105));_tmp105[0]=({struct Cyc_Absyn_Decl_s_struct _tmp106;
_tmp106.tag=11;_tmp106.f1=d;_tmp106.f2=s;_tmp106;});_tmp105;}),Cyc_Position_segment_join(
d->loc,s->loc));}static struct Cyc_Absyn_Stmt*Cyc_Parse_flatten_declarations(
struct Cyc_List_List*ds,struct Cyc_Absyn_Stmt*s){return((struct Cyc_Absyn_Stmt*(*)(
struct Cyc_Absyn_Stmt*(*f)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*
x,struct Cyc_Absyn_Stmt*accum))Cyc_List_fold_right)(Cyc_Parse_flatten_decl,ds,s);}
static struct Cyc_List_List*Cyc_Parse_make_declarations(struct Cyc_Parse_Declaration_spec*
ds,struct Cyc_List_List*ids,struct Cyc_Position_Segment*loc){struct Cyc_Parse_Declaration_spec
_tmp108;struct Cyc_Absyn_Tqual _tmp109;struct Cyc_List_List*_tmp10A;struct Cyc_List_List*
_tmp10B;struct Cyc_Parse_Declaration_spec*_tmp107=ds;_tmp108=*_tmp107;_tmp109=
_tmp108.tq;_tmp10A=_tmp108.type_specs;_tmp10B=_tmp108.attributes;if(ds->is_inline)
Cyc_Parse_err(({const char*_tmp10C="inline is allowed only on function definitions";
_tag_arr(_tmp10C,sizeof(char),_get_zero_arr_size(_tmp10C,47));}),loc);if(_tmp10A
== 0){Cyc_Parse_err(({const char*_tmp10D="missing type specifiers in declaration";
_tag_arr(_tmp10D,sizeof(char),_get_zero_arr_size(_tmp10D,39));}),loc);return 0;}{
void*s=(void*)2;int istypedef=0;if(ds->sc != 0){void*_tmp10E=(void*)((struct Cyc_Core_Opt*)
_check_null(ds->sc))->v;_LL84: if((int)_tmp10E != 0)goto _LL86;_LL85: istypedef=1;
goto _LL83;_LL86: if((int)_tmp10E != 1)goto _LL88;_LL87: s=(void*)3;goto _LL83;_LL88:
if((int)_tmp10E != 2)goto _LL8A;_LL89: s=(void*)4;goto _LL83;_LL8A: if((int)_tmp10E != 
3)goto _LL8C;_LL8B: s=(void*)0;goto _LL83;_LL8C: if((int)_tmp10E != 4)goto _LL8E;_LL8D:
s=(void*)2;goto _LL83;_LL8E: if((int)_tmp10E != 5)goto _LL90;_LL8F: if(Cyc_Parse_no_register)
s=(void*)2;else{s=(void*)5;}goto _LL83;_LL90: if((int)_tmp10E != 6)goto _LL83;_LL91:
s=(void*)1;goto _LL83;_LL83:;}{struct Cyc_List_List*_tmp110;struct Cyc_List_List*
_tmp111;struct _tuple0 _tmp10F=((struct _tuple0(*)(struct Cyc_List_List*x))Cyc_List_split)(
ids);_tmp110=_tmp10F.f1;_tmp111=_tmp10F.f2;{int exps_empty=1;{struct Cyc_List_List*
es=_tmp111;for(0;es != 0;es=es->tl){if((struct Cyc_Absyn_Exp*)es->hd != 0){
exps_empty=0;break;}}}{struct _tuple5 _tmp112=Cyc_Parse_collapse_type_specifiers(
_tmp10A,loc);if(_tmp110 == 0){void*_tmp114;struct Cyc_Core_Opt*_tmp115;struct
_tuple5 _tmp113=_tmp112;_tmp114=_tmp113.f1;_tmp115=_tmp113.f2;if(_tmp115 != 0){
struct Cyc_Absyn_Decl*d=(struct Cyc_Absyn_Decl*)_tmp115->v;{void*_tmp116=(void*)d->r;
struct Cyc_Absyn_Enumdecl*_tmp117;struct Cyc_Absyn_Aggrdecl*_tmp118;struct Cyc_Absyn_Tuniondecl*
_tmp119;_LL93: if(*((int*)_tmp116)!= 6)goto _LL95;_tmp117=((struct Cyc_Absyn_Enum_d_struct*)
_tmp116)->f1;_LL94:(void*)(_tmp117->sc=(void*)s);if(_tmp10B != 0)Cyc_Parse_err(({
const char*_tmp11A="bad attributes on enum";_tag_arr(_tmp11A,sizeof(char),
_get_zero_arr_size(_tmp11A,23));}),loc);goto _LL92;_LL95: if(*((int*)_tmp116)!= 4)
goto _LL97;_tmp118=((struct Cyc_Absyn_Aggr_d_struct*)_tmp116)->f1;_LL96:(void*)(
_tmp118->sc=(void*)s);_tmp118->attributes=_tmp10B;goto _LL92;_LL97: if(*((int*)
_tmp116)!= 5)goto _LL99;_tmp119=((struct Cyc_Absyn_Tunion_d_struct*)_tmp116)->f1;
_LL98:(void*)(_tmp119->sc=(void*)s);if(_tmp10B != 0)Cyc_Parse_err(({const char*
_tmp11B="bad attributes on tunion";_tag_arr(_tmp11B,sizeof(char),
_get_zero_arr_size(_tmp11B,25));}),loc);goto _LL92;_LL99:;_LL9A: Cyc_Parse_err(({
const char*_tmp11C="bad declaration";_tag_arr(_tmp11C,sizeof(char),
_get_zero_arr_size(_tmp11C,16));}),loc);return 0;_LL92:;}return({struct Cyc_List_List*
_tmp11D=_cycalloc(sizeof(*_tmp11D));_tmp11D->hd=d;_tmp11D->tl=0;_tmp11D;});}
else{void*_tmp11E=_tmp114;struct Cyc_Absyn_AggrInfo _tmp11F;void*_tmp120;void*
_tmp121;struct _tuple1*_tmp122;struct Cyc_List_List*_tmp123;struct Cyc_Absyn_TunionInfo
_tmp124;void*_tmp125;struct Cyc_Absyn_Tuniondecl**_tmp126;struct Cyc_Absyn_TunionInfo
_tmp127;void*_tmp128;struct Cyc_Absyn_UnknownTunionInfo _tmp129;struct _tuple1*
_tmp12A;int _tmp12B;struct Cyc_List_List*_tmp12C;struct _tuple1*_tmp12D;struct Cyc_List_List*
_tmp12E;_LL9C: if(_tmp11E <= (void*)3  || *((int*)_tmp11E)!= 10)goto _LL9E;_tmp11F=((
struct Cyc_Absyn_AggrType_struct*)_tmp11E)->f1;_tmp120=(void*)_tmp11F.aggr_info;
if(*((int*)_tmp120)!= 0)goto _LL9E;_tmp121=(void*)((struct Cyc_Absyn_UnknownAggr_struct*)
_tmp120)->f1;_tmp122=((struct Cyc_Absyn_UnknownAggr_struct*)_tmp120)->f2;_tmp123=
_tmp11F.targs;_LL9D: {struct Cyc_List_List*_tmp12F=((struct Cyc_List_List*(*)(
struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Position_Segment*,void*),struct Cyc_Position_Segment*
env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,loc,_tmp123);
struct Cyc_Absyn_Aggrdecl*_tmp130=({struct Cyc_Absyn_Aggrdecl*_tmp135=_cycalloc(
sizeof(*_tmp135));_tmp135->kind=(void*)_tmp121;_tmp135->sc=(void*)s;_tmp135->name=
_tmp122;_tmp135->tvs=_tmp12F;_tmp135->impl=0;_tmp135->attributes=0;_tmp135;});
if(_tmp10B != 0)Cyc_Parse_err(({const char*_tmp131="bad attributes on type declaration";
_tag_arr(_tmp131,sizeof(char),_get_zero_arr_size(_tmp131,35));}),loc);return({
struct Cyc_List_List*_tmp132=_cycalloc(sizeof(*_tmp132));_tmp132->hd=Cyc_Absyn_new_decl((
void*)({struct Cyc_Absyn_Aggr_d_struct*_tmp133=_cycalloc(sizeof(*_tmp133));
_tmp133[0]=({struct Cyc_Absyn_Aggr_d_struct _tmp134;_tmp134.tag=4;_tmp134.f1=
_tmp130;_tmp134;});_tmp133;}),loc);_tmp132->tl=0;_tmp132;});}_LL9E: if(_tmp11E <= (
void*)3  || *((int*)_tmp11E)!= 2)goto _LLA0;_tmp124=((struct Cyc_Absyn_TunionType_struct*)
_tmp11E)->f1;_tmp125=(void*)_tmp124.tunion_info;if(*((int*)_tmp125)!= 1)goto
_LLA0;_tmp126=((struct Cyc_Absyn_KnownTunion_struct*)_tmp125)->f1;_LL9F: if(
_tmp10B != 0)Cyc_Parse_err(({const char*_tmp136="bad attributes on tunion";
_tag_arr(_tmp136,sizeof(char),_get_zero_arr_size(_tmp136,25));}),loc);return({
struct Cyc_List_List*_tmp137=_cycalloc(sizeof(*_tmp137));_tmp137->hd=Cyc_Absyn_new_decl((
void*)({struct Cyc_Absyn_Tunion_d_struct*_tmp138=_cycalloc(sizeof(*_tmp138));
_tmp138[0]=({struct Cyc_Absyn_Tunion_d_struct _tmp139;_tmp139.tag=5;_tmp139.f1=*
_tmp126;_tmp139;});_tmp138;}),loc);_tmp137->tl=0;_tmp137;});_LLA0: if(_tmp11E <= (
void*)3  || *((int*)_tmp11E)!= 2)goto _LLA2;_tmp127=((struct Cyc_Absyn_TunionType_struct*)
_tmp11E)->f1;_tmp128=(void*)_tmp127.tunion_info;if(*((int*)_tmp128)!= 0)goto
_LLA2;_tmp129=((struct Cyc_Absyn_UnknownTunion_struct*)_tmp128)->f1;_tmp12A=
_tmp129.name;_tmp12B=_tmp129.is_xtunion;_tmp12C=_tmp127.targs;_LLA1: {struct Cyc_List_List*
_tmp13A=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Position_Segment*,
void*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,
loc,_tmp12C);struct Cyc_Absyn_Decl*_tmp13B=Cyc_Absyn_tunion_decl(s,_tmp12A,
_tmp13A,0,_tmp12B,loc);if(_tmp10B != 0)Cyc_Parse_err(({const char*_tmp13C="bad attributes on tunion";
_tag_arr(_tmp13C,sizeof(char),_get_zero_arr_size(_tmp13C,25));}),loc);return({
struct Cyc_List_List*_tmp13D=_cycalloc(sizeof(*_tmp13D));_tmp13D->hd=_tmp13B;
_tmp13D->tl=0;_tmp13D;});}_LLA2: if(_tmp11E <= (void*)3  || *((int*)_tmp11E)!= 12)
goto _LLA4;_tmp12D=((struct Cyc_Absyn_EnumType_struct*)_tmp11E)->f1;_LLA3: {struct
Cyc_Absyn_Enumdecl*_tmp13E=({struct Cyc_Absyn_Enumdecl*_tmp144=_cycalloc(sizeof(*
_tmp144));_tmp144->sc=(void*)s;_tmp144->name=_tmp12D;_tmp144->fields=0;_tmp144;});
if(_tmp10B != 0)Cyc_Parse_err(({const char*_tmp13F="bad attributes on enum";
_tag_arr(_tmp13F,sizeof(char),_get_zero_arr_size(_tmp13F,23));}),loc);return({
struct Cyc_List_List*_tmp140=_cycalloc(sizeof(*_tmp140));_tmp140->hd=({struct Cyc_Absyn_Decl*
_tmp141=_cycalloc(sizeof(*_tmp141));_tmp141->r=(void*)((void*)({struct Cyc_Absyn_Enum_d_struct*
_tmp142=_cycalloc(sizeof(*_tmp142));_tmp142[0]=({struct Cyc_Absyn_Enum_d_struct
_tmp143;_tmp143.tag=6;_tmp143.f1=_tmp13E;_tmp143;});_tmp142;}));_tmp141->loc=loc;
_tmp141;});_tmp140->tl=0;_tmp140;});}_LLA4: if(_tmp11E <= (void*)3  || *((int*)
_tmp11E)!= 13)goto _LLA6;_tmp12E=((struct Cyc_Absyn_AnonEnumType_struct*)_tmp11E)->f1;
_LLA5: {struct Cyc_Absyn_Enumdecl*_tmp145=({struct Cyc_Absyn_Enumdecl*_tmp14B=
_cycalloc(sizeof(*_tmp14B));_tmp14B->sc=(void*)s;_tmp14B->name=Cyc_Parse_gensym_enum();
_tmp14B->fields=({struct Cyc_Core_Opt*_tmp14C=_cycalloc(sizeof(*_tmp14C));_tmp14C->v=
_tmp12E;_tmp14C;});_tmp14B;});if(_tmp10B != 0)Cyc_Parse_err(({const char*_tmp146="bad attributes on enum";
_tag_arr(_tmp146,sizeof(char),_get_zero_arr_size(_tmp146,23));}),loc);return({
struct Cyc_List_List*_tmp147=_cycalloc(sizeof(*_tmp147));_tmp147->hd=({struct Cyc_Absyn_Decl*
_tmp148=_cycalloc(sizeof(*_tmp148));_tmp148->r=(void*)((void*)({struct Cyc_Absyn_Enum_d_struct*
_tmp149=_cycalloc(sizeof(*_tmp149));_tmp149[0]=({struct Cyc_Absyn_Enum_d_struct
_tmp14A;_tmp14A.tag=6;_tmp14A.f1=_tmp145;_tmp14A;});_tmp149;}));_tmp148->loc=loc;
_tmp148;});_tmp147->tl=0;_tmp147;});}_LLA6:;_LLA7: Cyc_Parse_err(({const char*
_tmp14D="missing declarator";_tag_arr(_tmp14D,sizeof(char),_get_zero_arr_size(
_tmp14D,19));}),loc);return 0;_LL9B:;}}else{void*t=_tmp112.f1;struct Cyc_List_List*
_tmp14E=Cyc_Parse_apply_tmss(_tmp109,t,_tmp110,_tmp10B);if(istypedef){if(!
exps_empty)Cyc_Parse_err(({const char*_tmp14F="initializer in typedef declaration";
_tag_arr(_tmp14F,sizeof(char),_get_zero_arr_size(_tmp14F,35));}),loc);{struct Cyc_List_List*
decls=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Decl*(*f)(struct Cyc_Position_Segment*,
struct _tuple7*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(
Cyc_Parse_v_typ_to_typedef,loc,_tmp14E);if(_tmp112.f2 != 0){struct Cyc_Absyn_Decl*
d=(struct Cyc_Absyn_Decl*)(_tmp112.f2)->v;{void*_tmp150=(void*)d->r;struct Cyc_Absyn_Aggrdecl*
_tmp151;struct Cyc_Absyn_Tuniondecl*_tmp152;struct Cyc_Absyn_Enumdecl*_tmp153;
_LLA9: if(*((int*)_tmp150)!= 4)goto _LLAB;_tmp151=((struct Cyc_Absyn_Aggr_d_struct*)
_tmp150)->f1;_LLAA:(void*)(_tmp151->sc=(void*)s);_tmp151->attributes=_tmp10B;
_tmp10B=0;goto _LLA8;_LLAB: if(*((int*)_tmp150)!= 5)goto _LLAD;_tmp152=((struct Cyc_Absyn_Tunion_d_struct*)
_tmp150)->f1;_LLAC:(void*)(_tmp152->sc=(void*)s);goto _LLA8;_LLAD: if(*((int*)
_tmp150)!= 6)goto _LLAF;_tmp153=((struct Cyc_Absyn_Enum_d_struct*)_tmp150)->f1;
_LLAE:(void*)(_tmp153->sc=(void*)s);goto _LLA8;_LLAF:;_LLB0: Cyc_Parse_err(({const
char*_tmp154="declaration within typedef is not a struct, union, tunion, or xtunion";
_tag_arr(_tmp154,sizeof(char),_get_zero_arr_size(_tmp154,70));}),loc);goto _LLA8;
_LLA8:;}decls=({struct Cyc_List_List*_tmp155=_cycalloc(sizeof(*_tmp155));_tmp155->hd=
d;_tmp155->tl=decls;_tmp155;});}return decls;}}else{if(_tmp112.f2 != 0)Cyc_Parse_unimp(({
const char*_tmp156="nested type declaration within declarator";_tag_arr(_tmp156,
sizeof(char),_get_zero_arr_size(_tmp156,42));}),loc);{struct Cyc_List_List*decls=
0;{struct Cyc_List_List*_tmp157=_tmp14E;for(0;_tmp157 != 0;(_tmp157=_tmp157->tl,
_tmp111=_tmp111->tl)){struct _tuple7 _tmp159;struct _tuple1*_tmp15A;struct Cyc_Absyn_Tqual
_tmp15B;void*_tmp15C;struct Cyc_List_List*_tmp15D;struct Cyc_List_List*_tmp15E;
struct _tuple7*_tmp158=(struct _tuple7*)_tmp157->hd;_tmp159=*_tmp158;_tmp15A=
_tmp159.f1;_tmp15B=_tmp159.f2;_tmp15C=_tmp159.f3;_tmp15D=_tmp159.f4;_tmp15E=
_tmp159.f5;if(_tmp15D != 0)({void*_tmp15F[0]={};Cyc_Tcutil_warn(loc,({const char*
_tmp160="bad type params, ignoring";_tag_arr(_tmp160,sizeof(char),
_get_zero_arr_size(_tmp160,26));}),_tag_arr(_tmp15F,sizeof(void*),0));});if(
_tmp111 == 0)({void*_tmp161[0]={};((int(*)(struct Cyc_Position_Segment*sg,struct
_tagged_arr fmt,struct _tagged_arr ap))Cyc_Parse_abort)(loc,({const char*_tmp162="unexpected NULL in parse!";
_tag_arr(_tmp162,sizeof(char),_get_zero_arr_size(_tmp162,26));}),_tag_arr(
_tmp161,sizeof(void*),0));});{struct Cyc_Absyn_Exp*_tmp163=(struct Cyc_Absyn_Exp*)
_tmp111->hd;struct Cyc_Absyn_Vardecl*_tmp164=Cyc_Absyn_new_vardecl(_tmp15A,
_tmp15C,_tmp163);_tmp164->tq=_tmp15B;(void*)(_tmp164->sc=(void*)s);_tmp164->attributes=
_tmp15E;{struct Cyc_Absyn_Decl*_tmp165=({struct Cyc_Absyn_Decl*_tmp167=_cycalloc(
sizeof(*_tmp167));_tmp167->r=(void*)((void*)({struct Cyc_Absyn_Var_d_struct*
_tmp168=_cycalloc(sizeof(*_tmp168));_tmp168[0]=({struct Cyc_Absyn_Var_d_struct
_tmp169;_tmp169.tag=0;_tmp169.f1=_tmp164;_tmp169;});_tmp168;}));_tmp167->loc=loc;
_tmp167;});decls=({struct Cyc_List_List*_tmp166=_cycalloc(sizeof(*_tmp166));
_tmp166->hd=_tmp165;_tmp166->tl=decls;_tmp166;});}}}}return((struct Cyc_List_List*(*)(
struct Cyc_List_List*x))Cyc_List_imp_rev)(decls);}}}}}}}}static void*Cyc_Parse_id_to_kind(
struct _tagged_arr s,struct Cyc_Position_Segment*loc){if(Cyc_strlen((struct
_tagged_arr)s)== 1)switch(*((const char*)_check_unknown_subscript(s,sizeof(char),
0))){case 'A': _LLB1: return(void*)0;case 'M': _LLB2: return(void*)1;case 'B': _LLB3:
return(void*)2;case 'R': _LLB4: return(void*)3;case 'E': _LLB5: return(void*)4;case 'I':
_LLB6: return(void*)5;default: _LLB7: break;}Cyc_Parse_err((struct _tagged_arr)({
struct Cyc_String_pa_struct _tmp16C;_tmp16C.tag=0;_tmp16C.f1=(struct _tagged_arr)((
struct _tagged_arr)s);{void*_tmp16A[1]={& _tmp16C};Cyc_aprintf(({const char*_tmp16B="bad kind: %s";
_tag_arr(_tmp16B,sizeof(char),_get_zero_arr_size(_tmp16B,13));}),_tag_arr(
_tmp16A,sizeof(void*),1));}}),loc);return(void*)2;}static struct Cyc_Absyn_Exp*Cyc_Parse_pat2exp(
struct Cyc_Absyn_Pat*p){void*_tmp16D=(void*)p->r;struct _tuple1*_tmp16E;struct Cyc_Absyn_Vardecl*
_tmp16F;struct Cyc_Absyn_Pat*_tmp170;void*_tmp171;int _tmp172;char _tmp173;struct
_tagged_arr _tmp174;struct _tuple1*_tmp175;struct Cyc_List_List*_tmp176;struct Cyc_Absyn_Exp*
_tmp177;_LLBA: if(_tmp16D <= (void*)2  || *((int*)_tmp16D)!= 12)goto _LLBC;_tmp16E=((
struct Cyc_Absyn_UnknownId_p_struct*)_tmp16D)->f1;_LLBB: return Cyc_Absyn_new_exp((
void*)({struct Cyc_Absyn_UnknownId_e_struct*_tmp178=_cycalloc(sizeof(*_tmp178));
_tmp178[0]=({struct Cyc_Absyn_UnknownId_e_struct _tmp179;_tmp179.tag=2;_tmp179.f1=
_tmp16E;_tmp179;});_tmp178;}),p->loc);_LLBC: if(_tmp16D <= (void*)2  || *((int*)
_tmp16D)!= 1)goto _LLBE;_tmp16F=((struct Cyc_Absyn_Reference_p_struct*)_tmp16D)->f1;
_LLBD: return Cyc_Absyn_deref_exp(Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_UnknownId_e_struct*
_tmp17A=_cycalloc(sizeof(*_tmp17A));_tmp17A[0]=({struct Cyc_Absyn_UnknownId_e_struct
_tmp17B;_tmp17B.tag=2;_tmp17B.f1=_tmp16F->name;_tmp17B;});_tmp17A;}),p->loc),p->loc);
_LLBE: if(_tmp16D <= (void*)2  || *((int*)_tmp16D)!= 4)goto _LLC0;_tmp170=((struct
Cyc_Absyn_Pointer_p_struct*)_tmp16D)->f1;_LLBF: return Cyc_Absyn_address_exp(Cyc_Parse_pat2exp(
_tmp170),p->loc);_LLC0: if((int)_tmp16D != 1)goto _LLC2;_LLC1: return Cyc_Absyn_null_exp(
p->loc);_LLC2: if(_tmp16D <= (void*)2  || *((int*)_tmp16D)!= 7)goto _LLC4;_tmp171=(
void*)((struct Cyc_Absyn_Int_p_struct*)_tmp16D)->f1;_tmp172=((struct Cyc_Absyn_Int_p_struct*)
_tmp16D)->f2;_LLC3: return Cyc_Absyn_int_exp(_tmp171,_tmp172,p->loc);_LLC4: if(
_tmp16D <= (void*)2  || *((int*)_tmp16D)!= 8)goto _LLC6;_tmp173=((struct Cyc_Absyn_Char_p_struct*)
_tmp16D)->f1;_LLC5: return Cyc_Absyn_char_exp(_tmp173,p->loc);_LLC6: if(_tmp16D <= (
void*)2  || *((int*)_tmp16D)!= 9)goto _LLC8;_tmp174=((struct Cyc_Absyn_Float_p_struct*)
_tmp16D)->f1;_LLC7: return Cyc_Absyn_float_exp(_tmp174,p->loc);_LLC8: if(_tmp16D <= (
void*)2  || *((int*)_tmp16D)!= 13)goto _LLCA;_tmp175=((struct Cyc_Absyn_UnknownCall_p_struct*)
_tmp16D)->f1;_tmp176=((struct Cyc_Absyn_UnknownCall_p_struct*)_tmp16D)->f2;_LLC9: {
struct Cyc_Absyn_Exp*e1=Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_UnknownId_e_struct*
_tmp17C=_cycalloc(sizeof(*_tmp17C));_tmp17C[0]=({struct Cyc_Absyn_UnknownId_e_struct
_tmp17D;_tmp17D.tag=2;_tmp17D.f1=_tmp175;_tmp17D;});_tmp17C;}),p->loc);struct Cyc_List_List*
es=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Pat*),
struct Cyc_List_List*x))Cyc_List_map)(Cyc_Parse_pat2exp,_tmp176);return Cyc_Absyn_unknowncall_exp(
e1,es,p->loc);}_LLCA: if(_tmp16D <= (void*)2  || *((int*)_tmp16D)!= 14)goto _LLCC;
_tmp177=((struct Cyc_Absyn_Exp_p_struct*)_tmp16D)->f1;_LLCB: return _tmp177;_LLCC:;
_LLCD: Cyc_Parse_err(({const char*_tmp17E="cannot mix patterns and expressions in case";
_tag_arr(_tmp17E,sizeof(char),_get_zero_arr_size(_tmp17E,44));}),p->loc);return
Cyc_Absyn_null_exp(p->loc);_LLB9:;}char Cyc_AbstractDeclarator_tok[27]="\000\000\000\000AbstractDeclarator_tok\000";
struct Cyc_AbstractDeclarator_tok_struct{char*tag;struct Cyc_Parse_Abstractdeclarator*
f1;};char Cyc_AggrFieldDeclListList_tok[30]="\000\000\000\000AggrFieldDeclListList_tok\000";
struct Cyc_AggrFieldDeclListList_tok_struct{char*tag;struct Cyc_List_List*f1;};
char Cyc_AggrFieldDeclList_tok[26]="\000\000\000\000AggrFieldDeclList_tok\000";
struct Cyc_AggrFieldDeclList_tok_struct{char*tag;struct Cyc_List_List*f1;};char Cyc_AttributeList_tok[
22]="\000\000\000\000AttributeList_tok\000";struct Cyc_AttributeList_tok_struct{
char*tag;struct Cyc_List_List*f1;};char Cyc_Attribute_tok[18]="\000\000\000\000Attribute_tok\000";
struct Cyc_Attribute_tok_struct{char*tag;void*f1;};char Cyc_Bool_tok[13]="\000\000\000\000Bool_tok\000";
struct Cyc_Bool_tok_struct{char*tag;int f1;};char Cyc_Char_tok[13]="\000\000\000\000Char_tok\000";
struct Cyc_Char_tok_struct{char*tag;char f1;};char Cyc_DeclList_tok[17]="\000\000\000\000DeclList_tok\000";
struct Cyc_DeclList_tok_struct{char*tag;struct Cyc_List_List*f1;};char Cyc_DeclSpec_tok[
17]="\000\000\000\000DeclSpec_tok\000";struct Cyc_DeclSpec_tok_struct{char*tag;
struct Cyc_Parse_Declaration_spec*f1;};char Cyc_Declarator_tok[19]="\000\000\000\000Declarator_tok\000";
struct Cyc_Declarator_tok_struct{char*tag;struct Cyc_Parse_Declarator*f1;};char Cyc_DesignatorList_tok[
23]="\000\000\000\000DesignatorList_tok\000";struct Cyc_DesignatorList_tok_struct{
char*tag;struct Cyc_List_List*f1;};char Cyc_Designator_tok[19]="\000\000\000\000Designator_tok\000";
struct Cyc_Designator_tok_struct{char*tag;void*f1;};char Cyc_EnumfieldList_tok[22]="\000\000\000\000EnumfieldList_tok\000";
struct Cyc_EnumfieldList_tok_struct{char*tag;struct Cyc_List_List*f1;};char Cyc_Enumfield_tok[
18]="\000\000\000\000Enumfield_tok\000";struct Cyc_Enumfield_tok_struct{char*tag;
struct Cyc_Absyn_Enumfield*f1;};char Cyc_ExpList_tok[16]="\000\000\000\000ExpList_tok\000";
struct Cyc_ExpList_tok_struct{char*tag;struct Cyc_List_List*f1;};char Cyc_Exp_tok[
12]="\000\000\000\000Exp_tok\000";struct Cyc_Exp_tok_struct{char*tag;struct Cyc_Absyn_Exp*
f1;};char Cyc_FieldPatternList_tok[25]="\000\000\000\000FieldPatternList_tok\000";
struct Cyc_FieldPatternList_tok_struct{char*tag;struct Cyc_List_List*f1;};char Cyc_FieldPattern_tok[
21]="\000\000\000\000FieldPattern_tok\000";struct _tuple11{struct Cyc_List_List*f1;
struct Cyc_Absyn_Pat*f2;};struct Cyc_FieldPattern_tok_struct{char*tag;struct
_tuple11*f1;};char Cyc_FnDecl_tok[15]="\000\000\000\000FnDecl_tok\000";struct Cyc_FnDecl_tok_struct{
char*tag;struct Cyc_Absyn_Fndecl*f1;};char Cyc_IdList_tok[15]="\000\000\000\000IdList_tok\000";
struct Cyc_IdList_tok_struct{char*tag;struct Cyc_List_List*f1;};char Cyc_InitDeclList_tok[
21]="\000\000\000\000InitDeclList_tok\000";struct Cyc_InitDeclList_tok_struct{
char*tag;struct Cyc_List_List*f1;};char Cyc_InitDecl_tok[17]="\000\000\000\000InitDecl_tok\000";
struct _tuple12{struct Cyc_Parse_Declarator*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_InitDecl_tok_struct{
char*tag;struct _tuple12*f1;};char Cyc_InitializerList_tok[24]="\000\000\000\000InitializerList_tok\000";
struct Cyc_InitializerList_tok_struct{char*tag;struct Cyc_List_List*f1;};char Cyc_Int_tok[
12]="\000\000\000\000Int_tok\000";struct _tuple13{void*f1;int f2;};struct Cyc_Int_tok_struct{
char*tag;struct _tuple13*f1;};char Cyc_Kind_tok[13]="\000\000\000\000Kind_tok\000";
struct Cyc_Kind_tok_struct{char*tag;void*f1;};char Cyc_ParamDeclListBool_tok[26]="\000\000\000\000ParamDeclListBool_tok\000";
struct _tuple14{struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;struct
Cyc_Core_Opt*f4;struct Cyc_List_List*f5;};struct Cyc_ParamDeclListBool_tok_struct{
char*tag;struct _tuple14*f1;};char Cyc_ParamDeclList_tok[22]="\000\000\000\000ParamDeclList_tok\000";
struct Cyc_ParamDeclList_tok_struct{char*tag;struct Cyc_List_List*f1;};char Cyc_ParamDecl_tok[
18]="\000\000\000\000ParamDecl_tok\000";struct Cyc_ParamDecl_tok_struct{char*tag;
struct _tuple2*f1;};char Cyc_PatternList_tok[20]="\000\000\000\000PatternList_tok\000";
struct Cyc_PatternList_tok_struct{char*tag;struct Cyc_List_List*f1;};char Cyc_Pattern_tok[
16]="\000\000\000\000Pattern_tok\000";struct Cyc_Pattern_tok_struct{char*tag;
struct Cyc_Absyn_Pat*f1;};char Cyc_Pointer_Sort_tok[21]="\000\000\000\000Pointer_Sort_tok\000";
struct Cyc_Pointer_Sort_tok_struct{char*tag;void*f1;};char Cyc_Primop_tok[15]="\000\000\000\000Primop_tok\000";
struct Cyc_Primop_tok_struct{char*tag;void*f1;};char Cyc_Primopopt_tok[18]="\000\000\000\000Primopopt_tok\000";
struct Cyc_Primopopt_tok_struct{char*tag;struct Cyc_Core_Opt*f1;};char Cyc_QualId_tok[
15]="\000\000\000\000QualId_tok\000";struct Cyc_QualId_tok_struct{char*tag;struct
_tuple1*f1;};char Cyc_QualSpecList_tok[21]="\000\000\000\000QualSpecList_tok\000";
struct _tuple15{struct Cyc_Absyn_Tqual f1;struct Cyc_List_List*f2;struct Cyc_List_List*
f3;};struct Cyc_QualSpecList_tok_struct{char*tag;struct _tuple15*f1;};char Cyc_Rgnorder_tok[
17]="\000\000\000\000Rgnorder_tok\000";struct Cyc_Rgnorder_tok_struct{char*tag;
struct Cyc_List_List*f1;};char Cyc_Scope_tok[14]="\000\000\000\000Scope_tok\000";
struct Cyc_Scope_tok_struct{char*tag;void*f1;};char Cyc_Short_tok[14]="\000\000\000\000Short_tok\000";
struct Cyc_Short_tok_struct{char*tag;short f1;};char Cyc_Stmt_tok[13]="\000\000\000\000Stmt_tok\000";
struct Cyc_Stmt_tok_struct{char*tag;struct Cyc_Absyn_Stmt*f1;};char Cyc_StorageClass_tok[
21]="\000\000\000\000StorageClass_tok\000";struct Cyc_StorageClass_tok_struct{
char*tag;void*f1;};char Cyc_String_tok[15]="\000\000\000\000String_tok\000";
struct Cyc_String_tok_struct{char*tag;struct _tagged_arr f1;};char Cyc_Stringopt_tok[
18]="\000\000\000\000Stringopt_tok\000";struct Cyc_Stringopt_tok_struct{char*tag;
struct Cyc_Core_Opt*f1;};char Cyc_StructOrUnion_tok[22]="\000\000\000\000StructOrUnion_tok\000";
struct Cyc_StructOrUnion_tok_struct{char*tag;void*f1;};char Cyc_SwitchClauseList_tok[
25]="\000\000\000\000SwitchClauseList_tok\000";struct Cyc_SwitchClauseList_tok_struct{
char*tag;struct Cyc_List_List*f1;};char Cyc_TunionFieldList_tok[24]="\000\000\000\000TunionFieldList_tok\000";
struct Cyc_TunionFieldList_tok_struct{char*tag;struct Cyc_List_List*f1;};char Cyc_TunionField_tok[
20]="\000\000\000\000TunionField_tok\000";struct Cyc_TunionField_tok_struct{char*
tag;struct Cyc_Absyn_Tunionfield*f1;};char Cyc_TypeList_tok[17]="\000\000\000\000TypeList_tok\000";
struct Cyc_TypeList_tok_struct{char*tag;struct Cyc_List_List*f1;};char Cyc_TypeModifierList_tok[
25]="\000\000\000\000TypeModifierList_tok\000";struct Cyc_TypeModifierList_tok_struct{
char*tag;struct Cyc_List_List*f1;};char Cyc_TypeOpt_tok[16]="\000\000\000\000TypeOpt_tok\000";
struct Cyc_TypeOpt_tok_struct{char*tag;struct Cyc_Core_Opt*f1;};char Cyc_TypeQual_tok[
17]="\000\000\000\000TypeQual_tok\000";struct Cyc_TypeQual_tok_struct{char*tag;
struct Cyc_Absyn_Tqual f1;};char Cyc_TypeSpecifier_tok[22]="\000\000\000\000TypeSpecifier_tok\000";
struct Cyc_TypeSpecifier_tok_struct{char*tag;void*f1;};char Cyc_Type_tok[13]="\000\000\000\000Type_tok\000";
struct Cyc_Type_tok_struct{char*tag;void*f1;};char Cyc_YY1[8]="\000\000\000\000YY1\000";
struct _tuple16{struct Cyc_Absyn_Conref*f1;struct Cyc_Absyn_Conref*f2;};struct Cyc_YY1_struct{
char*tag;struct _tuple16*f1;};char Cyc_YY2[8]="\000\000\000\000YY2\000";struct Cyc_YY2_struct{
char*tag;struct Cyc_Absyn_Conref*f1;};char Cyc_YY3[8]="\000\000\000\000YY3\000";
struct Cyc_YY3_struct{char*tag;struct _tuple14*f1;};char Cyc_YY4[8]="\000\000\000\000YY4\000";
struct Cyc_YY4_struct{char*tag;struct Cyc_Absyn_Conref*f1;};char Cyc_YY5[8]="\000\000\000\000YY5\000";
struct Cyc_YY5_struct{char*tag;struct Cyc_List_List*f1;};char Cyc_YYINITIALSVAL[18]="\000\000\000\000YYINITIALSVAL\000";
char Cyc_Okay_tok[13]="\000\000\000\000Okay_tok\000";static char _tmp180[15]="$(sign_t,int)@";
static struct Cyc_Core_Failure_struct Cyc_yyfail_Int_tok={Cyc_Core_Failure,{_tmp180,
_tmp180,_tmp180 + 15}};struct _tuple13*Cyc_yyget_Int_tok(void*yy1){struct _tuple13*
yyzzz;{void*_tmp181=yy1;struct _tuple13*_tmp182;_LLCF: if(*((void**)_tmp181)!= Cyc_Int_tok)
goto _LLD1;_tmp182=((struct Cyc_Int_tok_struct*)_tmp181)->f1;_LLD0: yyzzz=_tmp182;
goto _LLCE;_LLD1:;_LLD2:(int)_throw((void*)& Cyc_yyfail_Int_tok);_LLCE:;}return
yyzzz;}static char _tmp184[5]="char";static struct Cyc_Core_Failure_struct Cyc_yyfail_Char_tok={
Cyc_Core_Failure,{_tmp184,_tmp184,_tmp184 + 5}};char Cyc_yyget_Char_tok(void*yy1){
char yyzzz;{void*_tmp185=yy1;char _tmp186;_LLD4: if(*((void**)_tmp185)!= Cyc_Char_tok)
goto _LLD6;_tmp186=((struct Cyc_Char_tok_struct*)_tmp185)->f1;_LLD5: yyzzz=_tmp186;
goto _LLD3;_LLD6:;_LLD7:(int)_throw((void*)& Cyc_yyfail_Char_tok);_LLD3:;}return
yyzzz;}static char _tmp188[9]="string_t";static struct Cyc_Core_Failure_struct Cyc_yyfail_String_tok={
Cyc_Core_Failure,{_tmp188,_tmp188,_tmp188 + 9}};struct _tagged_arr Cyc_yyget_String_tok(
void*yy1){struct _tagged_arr yyzzz;{void*_tmp189=yy1;struct _tagged_arr _tmp18A;
_LLD9: if(*((void**)_tmp189)!= Cyc_String_tok)goto _LLDB;_tmp18A=((struct Cyc_String_tok_struct*)
_tmp189)->f1;_LLDA: yyzzz=_tmp18A;goto _LLD8;_LLDB:;_LLDC:(int)_throw((void*)& Cyc_yyfail_String_tok);
_LLD8:;}return yyzzz;}static char _tmp18C[38]="$(conref_t<bool>,conref_t<bounds_t>)@";
static struct Cyc_Core_Failure_struct Cyc_yyfail_YY1={Cyc_Core_Failure,{_tmp18C,
_tmp18C,_tmp18C + 38}};struct _tuple16*Cyc_yyget_YY1(void*yy1){struct _tuple16*
yyzzz;{void*_tmp18D=yy1;struct _tuple16*_tmp18E;_LLDE: if(*((void**)_tmp18D)!= Cyc_YY1)
goto _LLE0;_tmp18E=((struct Cyc_YY1_struct*)_tmp18D)->f1;_LLDF: yyzzz=_tmp18E;goto
_LLDD;_LLE0:;_LLE1:(int)_throw((void*)& Cyc_yyfail_YY1);_LLDD:;}return yyzzz;}
static char _tmp190[19]="conref_t<bounds_t>";static struct Cyc_Core_Failure_struct
Cyc_yyfail_YY2={Cyc_Core_Failure,{_tmp190,_tmp190,_tmp190 + 19}};struct Cyc_Absyn_Conref*
Cyc_yyget_YY2(void*yy1){struct Cyc_Absyn_Conref*yyzzz;{void*_tmp191=yy1;struct Cyc_Absyn_Conref*
_tmp192;_LLE3: if(*((void**)_tmp191)!= Cyc_YY2)goto _LLE5;_tmp192=((struct Cyc_YY2_struct*)
_tmp191)->f1;_LLE4: yyzzz=_tmp192;goto _LLE2;_LLE5:;_LLE6:(int)_throw((void*)& Cyc_yyfail_YY2);
_LLE2:;}return yyzzz;}static char _tmp194[6]="exp_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_Exp_tok={Cyc_Core_Failure,{_tmp194,_tmp194,_tmp194 + 6}};struct Cyc_Absyn_Exp*
Cyc_yyget_Exp_tok(void*yy1){struct Cyc_Absyn_Exp*yyzzz;{void*_tmp195=yy1;struct
Cyc_Absyn_Exp*_tmp196;_LLE8: if(*((void**)_tmp195)!= Cyc_Exp_tok)goto _LLEA;
_tmp196=((struct Cyc_Exp_tok_struct*)_tmp195)->f1;_LLE9: yyzzz=_tmp196;goto _LLE7;
_LLEA:;_LLEB:(int)_throw((void*)& Cyc_yyfail_Exp_tok);_LLE7:;}return yyzzz;}static
char _tmp198[14]="list_t<exp_t>";static struct Cyc_Core_Failure_struct Cyc_yyfail_ExpList_tok={
Cyc_Core_Failure,{_tmp198,_tmp198,_tmp198 + 14}};struct Cyc_List_List*Cyc_yyget_ExpList_tok(
void*yy1){struct Cyc_List_List*yyzzz;{void*_tmp199=yy1;struct Cyc_List_List*
_tmp19A;_LLED: if(*((void**)_tmp199)!= Cyc_ExpList_tok)goto _LLEF;_tmp19A=((struct
Cyc_ExpList_tok_struct*)_tmp199)->f1;_LLEE: yyzzz=_tmp19A;goto _LLEC;_LLEF:;_LLF0:(
int)_throw((void*)& Cyc_yyfail_ExpList_tok);_LLEC:;}return yyzzz;}static char
_tmp19C[39]="list_t<$(list_t<designator_t>,exp_t)@>";static struct Cyc_Core_Failure_struct
Cyc_yyfail_InitializerList_tok={Cyc_Core_Failure,{_tmp19C,_tmp19C,_tmp19C + 39}};
struct Cyc_List_List*Cyc_yyget_InitializerList_tok(void*yy1){struct Cyc_List_List*
yyzzz;{void*_tmp19D=yy1;struct Cyc_List_List*_tmp19E;_LLF2: if(*((void**)_tmp19D)
!= Cyc_InitializerList_tok)goto _LLF4;_tmp19E=((struct Cyc_InitializerList_tok_struct*)
_tmp19D)->f1;_LLF3: yyzzz=_tmp19E;goto _LLF1;_LLF4:;_LLF5:(int)_throw((void*)& Cyc_yyfail_InitializerList_tok);
_LLF1:;}return yyzzz;}static char _tmp1A0[9]="primop_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_Primop_tok={Cyc_Core_Failure,{_tmp1A0,_tmp1A0,_tmp1A0 + 9}};void*Cyc_yyget_Primop_tok(
void*yy1){void*yyzzz;{void*_tmp1A1=yy1;void*_tmp1A2;_LLF7: if(*((void**)_tmp1A1)
!= Cyc_Primop_tok)goto _LLF9;_tmp1A2=(void*)((struct Cyc_Primop_tok_struct*)
_tmp1A1)->f1;_LLF8: yyzzz=_tmp1A2;goto _LLF6;_LLF9:;_LLFA:(int)_throw((void*)& Cyc_yyfail_Primop_tok);
_LLF6:;}return yyzzz;}static char _tmp1A4[16]="opt_t<primop_t>";static struct Cyc_Core_Failure_struct
Cyc_yyfail_Primopopt_tok={Cyc_Core_Failure,{_tmp1A4,_tmp1A4,_tmp1A4 + 16}};struct
Cyc_Core_Opt*Cyc_yyget_Primopopt_tok(void*yy1){struct Cyc_Core_Opt*yyzzz;{void*
_tmp1A5=yy1;struct Cyc_Core_Opt*_tmp1A6;_LLFC: if(*((void**)_tmp1A5)!= Cyc_Primopopt_tok)
goto _LLFE;_tmp1A6=((struct Cyc_Primopopt_tok_struct*)_tmp1A5)->f1;_LLFD: yyzzz=
_tmp1A6;goto _LLFB;_LLFE:;_LLFF:(int)_throw((void*)& Cyc_yyfail_Primopopt_tok);
_LLFB:;}return yyzzz;}static char _tmp1A8[7]="qvar_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_QualId_tok={Cyc_Core_Failure,{_tmp1A8,_tmp1A8,_tmp1A8 + 7}};struct
_tuple1*Cyc_yyget_QualId_tok(void*yy1){struct _tuple1*yyzzz;{void*_tmp1A9=yy1;
struct _tuple1*_tmp1AA;_LL101: if(*((void**)_tmp1A9)!= Cyc_QualId_tok)goto _LL103;
_tmp1AA=((struct Cyc_QualId_tok_struct*)_tmp1A9)->f1;_LL102: yyzzz=_tmp1AA;goto
_LL100;_LL103:;_LL104:(int)_throw((void*)& Cyc_yyfail_QualId_tok);_LL100:;}return
yyzzz;}static char _tmp1AC[7]="stmt_t";static struct Cyc_Core_Failure_struct Cyc_yyfail_Stmt_tok={
Cyc_Core_Failure,{_tmp1AC,_tmp1AC,_tmp1AC + 7}};struct Cyc_Absyn_Stmt*Cyc_yyget_Stmt_tok(
void*yy1){struct Cyc_Absyn_Stmt*yyzzz;{void*_tmp1AD=yy1;struct Cyc_Absyn_Stmt*
_tmp1AE;_LL106: if(*((void**)_tmp1AD)!= Cyc_Stmt_tok)goto _LL108;_tmp1AE=((struct
Cyc_Stmt_tok_struct*)_tmp1AD)->f1;_LL107: yyzzz=_tmp1AE;goto _LL105;_LL108:;_LL109:(
int)_throw((void*)& Cyc_yyfail_Stmt_tok);_LL105:;}return yyzzz;}static char _tmp1B0[
24]="list_t<switch_clause_t>";static struct Cyc_Core_Failure_struct Cyc_yyfail_SwitchClauseList_tok={
Cyc_Core_Failure,{_tmp1B0,_tmp1B0,_tmp1B0 + 24}};struct Cyc_List_List*Cyc_yyget_SwitchClauseList_tok(
void*yy1){struct Cyc_List_List*yyzzz;{void*_tmp1B1=yy1;struct Cyc_List_List*
_tmp1B2;_LL10B: if(*((void**)_tmp1B1)!= Cyc_SwitchClauseList_tok)goto _LL10D;
_tmp1B2=((struct Cyc_SwitchClauseList_tok_struct*)_tmp1B1)->f1;_LL10C: yyzzz=
_tmp1B2;goto _LL10A;_LL10D:;_LL10E:(int)_throw((void*)& Cyc_yyfail_SwitchClauseList_tok);
_LL10A:;}return yyzzz;}static char _tmp1B4[6]="pat_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_Pattern_tok={Cyc_Core_Failure,{_tmp1B4,_tmp1B4,_tmp1B4 + 6}};struct Cyc_Absyn_Pat*
Cyc_yyget_Pattern_tok(void*yy1){struct Cyc_Absyn_Pat*yyzzz;{void*_tmp1B5=yy1;
struct Cyc_Absyn_Pat*_tmp1B6;_LL110: if(*((void**)_tmp1B5)!= Cyc_Pattern_tok)goto
_LL112;_tmp1B6=((struct Cyc_Pattern_tok_struct*)_tmp1B5)->f1;_LL111: yyzzz=_tmp1B6;
goto _LL10F;_LL112:;_LL113:(int)_throw((void*)& Cyc_yyfail_Pattern_tok);_LL10F:;}
return yyzzz;}static char _tmp1B8[14]="list_t<pat_t>";static struct Cyc_Core_Failure_struct
Cyc_yyfail_PatternList_tok={Cyc_Core_Failure,{_tmp1B8,_tmp1B8,_tmp1B8 + 14}};
struct Cyc_List_List*Cyc_yyget_PatternList_tok(void*yy1){struct Cyc_List_List*
yyzzz;{void*_tmp1B9=yy1;struct Cyc_List_List*_tmp1BA;_LL115: if(*((void**)_tmp1B9)
!= Cyc_PatternList_tok)goto _LL117;_tmp1BA=((struct Cyc_PatternList_tok_struct*)
_tmp1B9)->f1;_LL116: yyzzz=_tmp1BA;goto _LL114;_LL117:;_LL118:(int)_throw((void*)&
Cyc_yyfail_PatternList_tok);_LL114:;}return yyzzz;}static char _tmp1BC[31]="$(list_t<designator_t>,pat_t)@";
static struct Cyc_Core_Failure_struct Cyc_yyfail_FieldPattern_tok={Cyc_Core_Failure,{
_tmp1BC,_tmp1BC,_tmp1BC + 31}};struct _tuple11*Cyc_yyget_FieldPattern_tok(void*yy1){
struct _tuple11*yyzzz;{void*_tmp1BD=yy1;struct _tuple11*_tmp1BE;_LL11A: if(*((void**)
_tmp1BD)!= Cyc_FieldPattern_tok)goto _LL11C;_tmp1BE=((struct Cyc_FieldPattern_tok_struct*)
_tmp1BD)->f1;_LL11B: yyzzz=_tmp1BE;goto _LL119;_LL11C:;_LL11D:(int)_throw((void*)&
Cyc_yyfail_FieldPattern_tok);_LL119:;}return yyzzz;}static char _tmp1C0[39]="list_t<$(list_t<designator_t>,pat_t)@>";
static struct Cyc_Core_Failure_struct Cyc_yyfail_FieldPatternList_tok={Cyc_Core_Failure,{
_tmp1C0,_tmp1C0,_tmp1C0 + 39}};struct Cyc_List_List*Cyc_yyget_FieldPatternList_tok(
void*yy1){struct Cyc_List_List*yyzzz;{void*_tmp1C1=yy1;struct Cyc_List_List*
_tmp1C2;_LL11F: if(*((void**)_tmp1C1)!= Cyc_FieldPatternList_tok)goto _LL121;
_tmp1C2=((struct Cyc_FieldPatternList_tok_struct*)_tmp1C1)->f1;_LL120: yyzzz=
_tmp1C2;goto _LL11E;_LL121:;_LL122:(int)_throw((void*)& Cyc_yyfail_FieldPatternList_tok);
_LL11E:;}return yyzzz;}static char _tmp1C4[9]="fndecl_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_FnDecl_tok={Cyc_Core_Failure,{_tmp1C4,_tmp1C4,_tmp1C4 + 9}};struct Cyc_Absyn_Fndecl*
Cyc_yyget_FnDecl_tok(void*yy1){struct Cyc_Absyn_Fndecl*yyzzz;{void*_tmp1C5=yy1;
struct Cyc_Absyn_Fndecl*_tmp1C6;_LL124: if(*((void**)_tmp1C5)!= Cyc_FnDecl_tok)
goto _LL126;_tmp1C6=((struct Cyc_FnDecl_tok_struct*)_tmp1C5)->f1;_LL125: yyzzz=
_tmp1C6;goto _LL123;_LL126:;_LL127:(int)_throw((void*)& Cyc_yyfail_FnDecl_tok);
_LL123:;}return yyzzz;}static char _tmp1C8[15]="list_t<decl_t>";static struct Cyc_Core_Failure_struct
Cyc_yyfail_DeclList_tok={Cyc_Core_Failure,{_tmp1C8,_tmp1C8,_tmp1C8 + 15}};struct
Cyc_List_List*Cyc_yyget_DeclList_tok(void*yy1){struct Cyc_List_List*yyzzz;{void*
_tmp1C9=yy1;struct Cyc_List_List*_tmp1CA;_LL129: if(*((void**)_tmp1C9)!= Cyc_DeclList_tok)
goto _LL12B;_tmp1CA=((struct Cyc_DeclList_tok_struct*)_tmp1C9)->f1;_LL12A: yyzzz=
_tmp1CA;goto _LL128;_LL12B:;_LL12C:(int)_throw((void*)& Cyc_yyfail_DeclList_tok);
_LL128:;}return yyzzz;}static char _tmp1CC[12]="decl_spec_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_DeclSpec_tok={Cyc_Core_Failure,{_tmp1CC,_tmp1CC,_tmp1CC + 12}};struct
Cyc_Parse_Declaration_spec*Cyc_yyget_DeclSpec_tok(void*yy1){struct Cyc_Parse_Declaration_spec*
yyzzz;{void*_tmp1CD=yy1;struct Cyc_Parse_Declaration_spec*_tmp1CE;_LL12E: if(*((
void**)_tmp1CD)!= Cyc_DeclSpec_tok)goto _LL130;_tmp1CE=((struct Cyc_DeclSpec_tok_struct*)
_tmp1CD)->f1;_LL12F: yyzzz=_tmp1CE;goto _LL12D;_LL130:;_LL131:(int)_throw((void*)&
Cyc_yyfail_DeclSpec_tok);_LL12D:;}return yyzzz;}static char _tmp1D0[27]="$(declarator_t,exp_opt_t)@";
static struct Cyc_Core_Failure_struct Cyc_yyfail_InitDecl_tok={Cyc_Core_Failure,{
_tmp1D0,_tmp1D0,_tmp1D0 + 27}};struct _tuple12*Cyc_yyget_InitDecl_tok(void*yy1){
struct _tuple12*yyzzz;{void*_tmp1D1=yy1;struct _tuple12*_tmp1D2;_LL133: if(*((void**)
_tmp1D1)!= Cyc_InitDecl_tok)goto _LL135;_tmp1D2=((struct Cyc_InitDecl_tok_struct*)
_tmp1D1)->f1;_LL134: yyzzz=_tmp1D2;goto _LL132;_LL135:;_LL136:(int)_throw((void*)&
Cyc_yyfail_InitDecl_tok);_LL132:;}return yyzzz;}static char _tmp1D4[35]="list_t<$(declarator_t,exp_opt_t)@>";
static struct Cyc_Core_Failure_struct Cyc_yyfail_InitDeclList_tok={Cyc_Core_Failure,{
_tmp1D4,_tmp1D4,_tmp1D4 + 35}};struct Cyc_List_List*Cyc_yyget_InitDeclList_tok(
void*yy1){struct Cyc_List_List*yyzzz;{void*_tmp1D5=yy1;struct Cyc_List_List*
_tmp1D6;_LL138: if(*((void**)_tmp1D5)!= Cyc_InitDeclList_tok)goto _LL13A;_tmp1D6=((
struct Cyc_InitDeclList_tok_struct*)_tmp1D5)->f1;_LL139: yyzzz=_tmp1D6;goto _LL137;
_LL13A:;_LL13B:(int)_throw((void*)& Cyc_yyfail_InitDeclList_tok);_LL137:;}return
yyzzz;}static char _tmp1D8[16]="storage_class_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_StorageClass_tok={Cyc_Core_Failure,{_tmp1D8,_tmp1D8,_tmp1D8 + 16}};
void*Cyc_yyget_StorageClass_tok(void*yy1){void*yyzzz;{void*_tmp1D9=yy1;void*
_tmp1DA;_LL13D: if(*((void**)_tmp1D9)!= Cyc_StorageClass_tok)goto _LL13F;_tmp1DA=(
void*)((struct Cyc_StorageClass_tok_struct*)_tmp1D9)->f1;_LL13E: yyzzz=_tmp1DA;
goto _LL13C;_LL13F:;_LL140:(int)_throw((void*)& Cyc_yyfail_StorageClass_tok);
_LL13C:;}return yyzzz;}static char _tmp1DC[17]="type_specifier_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_TypeSpecifier_tok={Cyc_Core_Failure,{_tmp1DC,_tmp1DC,_tmp1DC + 17}};
void*Cyc_yyget_TypeSpecifier_tok(void*yy1){void*yyzzz;{void*_tmp1DD=yy1;void*
_tmp1DE;_LL142: if(*((void**)_tmp1DD)!= Cyc_TypeSpecifier_tok)goto _LL144;_tmp1DE=(
void*)((struct Cyc_TypeSpecifier_tok_struct*)_tmp1DD)->f1;_LL143: yyzzz=_tmp1DE;
goto _LL141;_LL144:;_LL145:(int)_throw((void*)& Cyc_yyfail_TypeSpecifier_tok);
_LL141:;}return yyzzz;}static char _tmp1E0[12]="aggr_kind_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_StructOrUnion_tok={Cyc_Core_Failure,{_tmp1E0,_tmp1E0,_tmp1E0 + 12}};
void*Cyc_yyget_StructOrUnion_tok(void*yy1){void*yyzzz;{void*_tmp1E1=yy1;void*
_tmp1E2;_LL147: if(*((void**)_tmp1E1)!= Cyc_StructOrUnion_tok)goto _LL149;_tmp1E2=(
void*)((struct Cyc_StructOrUnion_tok_struct*)_tmp1E1)->f1;_LL148: yyzzz=_tmp1E2;
goto _LL146;_LL149:;_LL14A:(int)_throw((void*)& Cyc_yyfail_StructOrUnion_tok);
_LL146:;}return yyzzz;}static char _tmp1E4[8]="tqual_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_TypeQual_tok={Cyc_Core_Failure,{_tmp1E4,_tmp1E4,_tmp1E4 + 8}};struct
Cyc_Absyn_Tqual Cyc_yyget_TypeQual_tok(void*yy1){struct Cyc_Absyn_Tqual yyzzz;{void*
_tmp1E5=yy1;struct Cyc_Absyn_Tqual _tmp1E6;_LL14C: if(*((void**)_tmp1E5)!= Cyc_TypeQual_tok)
goto _LL14E;_tmp1E6=((struct Cyc_TypeQual_tok_struct*)_tmp1E5)->f1;_LL14D: yyzzz=
_tmp1E6;goto _LL14B;_LL14E:;_LL14F:(int)_throw((void*)& Cyc_yyfail_TypeQual_tok);
_LL14B:;}return yyzzz;}static char _tmp1E8[20]="list_t<aggrfield_t>";static struct
Cyc_Core_Failure_struct Cyc_yyfail_AggrFieldDeclList_tok={Cyc_Core_Failure,{
_tmp1E8,_tmp1E8,_tmp1E8 + 20}};struct Cyc_List_List*Cyc_yyget_AggrFieldDeclList_tok(
void*yy1){struct Cyc_List_List*yyzzz;{void*_tmp1E9=yy1;struct Cyc_List_List*
_tmp1EA;_LL151: if(*((void**)_tmp1E9)!= Cyc_AggrFieldDeclList_tok)goto _LL153;
_tmp1EA=((struct Cyc_AggrFieldDeclList_tok_struct*)_tmp1E9)->f1;_LL152: yyzzz=
_tmp1EA;goto _LL150;_LL153:;_LL154:(int)_throw((void*)& Cyc_yyfail_AggrFieldDeclList_tok);
_LL150:;}return yyzzz;}static char _tmp1EC[28]="list_t<list_t<aggrfield_t>>";static
struct Cyc_Core_Failure_struct Cyc_yyfail_AggrFieldDeclListList_tok={Cyc_Core_Failure,{
_tmp1EC,_tmp1EC,_tmp1EC + 28}};struct Cyc_List_List*Cyc_yyget_AggrFieldDeclListList_tok(
void*yy1){struct Cyc_List_List*yyzzz;{void*_tmp1ED=yy1;struct Cyc_List_List*
_tmp1EE;_LL156: if(*((void**)_tmp1ED)!= Cyc_AggrFieldDeclListList_tok)goto _LL158;
_tmp1EE=((struct Cyc_AggrFieldDeclListList_tok_struct*)_tmp1ED)->f1;_LL157: yyzzz=
_tmp1EE;goto _LL155;_LL158:;_LL159:(int)_throw((void*)& Cyc_yyfail_AggrFieldDeclListList_tok);
_LL155:;}return yyzzz;}static char _tmp1F0[24]="list_t<type_modifier_t>";static
struct Cyc_Core_Failure_struct Cyc_yyfail_TypeModifierList_tok={Cyc_Core_Failure,{
_tmp1F0,_tmp1F0,_tmp1F0 + 24}};struct Cyc_List_List*Cyc_yyget_TypeModifierList_tok(
void*yy1){struct Cyc_List_List*yyzzz;{void*_tmp1F1=yy1;struct Cyc_List_List*
_tmp1F2;_LL15B: if(*((void**)_tmp1F1)!= Cyc_TypeModifierList_tok)goto _LL15D;
_tmp1F2=((struct Cyc_TypeModifierList_tok_struct*)_tmp1F1)->f1;_LL15C: yyzzz=
_tmp1F2;goto _LL15A;_LL15D:;_LL15E:(int)_throw((void*)& Cyc_yyfail_TypeModifierList_tok);
_LL15A:;}return yyzzz;}static char _tmp1F4[13]="declarator_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_Declarator_tok={Cyc_Core_Failure,{_tmp1F4,_tmp1F4,_tmp1F4 + 13}};
struct Cyc_Parse_Declarator*Cyc_yyget_Declarator_tok(void*yy1){struct Cyc_Parse_Declarator*
yyzzz;{void*_tmp1F5=yy1;struct Cyc_Parse_Declarator*_tmp1F6;_LL160: if(*((void**)
_tmp1F5)!= Cyc_Declarator_tok)goto _LL162;_tmp1F6=((struct Cyc_Declarator_tok_struct*)
_tmp1F5)->f1;_LL161: yyzzz=_tmp1F6;goto _LL15F;_LL162:;_LL163:(int)_throw((void*)&
Cyc_yyfail_Declarator_tok);_LL15F:;}return yyzzz;}static char _tmp1F8[21]="abstractdeclarator_t";
static struct Cyc_Core_Failure_struct Cyc_yyfail_AbstractDeclarator_tok={Cyc_Core_Failure,{
_tmp1F8,_tmp1F8,_tmp1F8 + 21}};struct Cyc_Parse_Abstractdeclarator*Cyc_yyget_AbstractDeclarator_tok(
void*yy1){struct Cyc_Parse_Abstractdeclarator*yyzzz;{void*_tmp1F9=yy1;struct Cyc_Parse_Abstractdeclarator*
_tmp1FA;_LL165: if(*((void**)_tmp1F9)!= Cyc_AbstractDeclarator_tok)goto _LL167;
_tmp1FA=((struct Cyc_AbstractDeclarator_tok_struct*)_tmp1F9)->f1;_LL166: yyzzz=
_tmp1FA;goto _LL164;_LL167:;_LL168:(int)_throw((void*)& Cyc_yyfail_AbstractDeclarator_tok);
_LL164:;}return yyzzz;}static char _tmp1FC[5]="bool";static struct Cyc_Core_Failure_struct
Cyc_yyfail_Bool_tok={Cyc_Core_Failure,{_tmp1FC,_tmp1FC,_tmp1FC + 5}};int Cyc_yyget_Bool_tok(
void*yy1){int yyzzz;{void*_tmp1FD=yy1;int _tmp1FE;_LL16A: if(*((void**)_tmp1FD)!= 
Cyc_Bool_tok)goto _LL16C;_tmp1FE=((struct Cyc_Bool_tok_struct*)_tmp1FD)->f1;_LL16B:
yyzzz=_tmp1FE;goto _LL169;_LL16C:;_LL16D:(int)_throw((void*)& Cyc_yyfail_Bool_tok);
_LL169:;}return yyzzz;}static char _tmp200[8]="scope_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_Scope_tok={Cyc_Core_Failure,{_tmp200,_tmp200,_tmp200 + 8}};void*Cyc_yyget_Scope_tok(
void*yy1){void*yyzzz;{void*_tmp201=yy1;void*_tmp202;_LL16F: if(*((void**)_tmp201)
!= Cyc_Scope_tok)goto _LL171;_tmp202=(void*)((struct Cyc_Scope_tok_struct*)_tmp201)->f1;
_LL170: yyzzz=_tmp202;goto _LL16E;_LL171:;_LL172:(int)_throw((void*)& Cyc_yyfail_Scope_tok);
_LL16E:;}return yyzzz;}static char _tmp204[14]="tunionfield_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_TunionField_tok={Cyc_Core_Failure,{_tmp204,_tmp204,_tmp204 + 14}};
struct Cyc_Absyn_Tunionfield*Cyc_yyget_TunionField_tok(void*yy1){struct Cyc_Absyn_Tunionfield*
yyzzz;{void*_tmp205=yy1;struct Cyc_Absyn_Tunionfield*_tmp206;_LL174: if(*((void**)
_tmp205)!= Cyc_TunionField_tok)goto _LL176;_tmp206=((struct Cyc_TunionField_tok_struct*)
_tmp205)->f1;_LL175: yyzzz=_tmp206;goto _LL173;_LL176:;_LL177:(int)_throw((void*)&
Cyc_yyfail_TunionField_tok);_LL173:;}return yyzzz;}static char _tmp208[22]="list_t<tunionfield_t>";
static struct Cyc_Core_Failure_struct Cyc_yyfail_TunionFieldList_tok={Cyc_Core_Failure,{
_tmp208,_tmp208,_tmp208 + 22}};struct Cyc_List_List*Cyc_yyget_TunionFieldList_tok(
void*yy1){struct Cyc_List_List*yyzzz;{void*_tmp209=yy1;struct Cyc_List_List*
_tmp20A;_LL179: if(*((void**)_tmp209)!= Cyc_TunionFieldList_tok)goto _LL17B;
_tmp20A=((struct Cyc_TunionFieldList_tok_struct*)_tmp209)->f1;_LL17A: yyzzz=
_tmp20A;goto _LL178;_LL17B:;_LL17C:(int)_throw((void*)& Cyc_yyfail_TunionFieldList_tok);
_LL178:;}return yyzzz;}static char _tmp20C[50]="$(tqual_t,list_t<type_specifier_t>,attributes_t)@";
static struct Cyc_Core_Failure_struct Cyc_yyfail_QualSpecList_tok={Cyc_Core_Failure,{
_tmp20C,_tmp20C,_tmp20C + 50}};struct _tuple15*Cyc_yyget_QualSpecList_tok(void*yy1){
struct _tuple15*yyzzz;{void*_tmp20D=yy1;struct _tuple15*_tmp20E;_LL17E: if(*((void**)
_tmp20D)!= Cyc_QualSpecList_tok)goto _LL180;_tmp20E=((struct Cyc_QualSpecList_tok_struct*)
_tmp20D)->f1;_LL17F: yyzzz=_tmp20E;goto _LL17D;_LL180:;_LL181:(int)_throw((void*)&
Cyc_yyfail_QualSpecList_tok);_LL17D:;}return yyzzz;}static char _tmp210[14]="list_t<var_t>";
static struct Cyc_Core_Failure_struct Cyc_yyfail_IdList_tok={Cyc_Core_Failure,{
_tmp210,_tmp210,_tmp210 + 14}};struct Cyc_List_List*Cyc_yyget_IdList_tok(void*yy1){
struct Cyc_List_List*yyzzz;{void*_tmp211=yy1;struct Cyc_List_List*_tmp212;_LL183:
if(*((void**)_tmp211)!= Cyc_IdList_tok)goto _LL185;_tmp212=((struct Cyc_IdList_tok_struct*)
_tmp211)->f1;_LL184: yyzzz=_tmp212;goto _LL182;_LL185:;_LL186:(int)_throw((void*)&
Cyc_yyfail_IdList_tok);_LL182:;}return yyzzz;}static char _tmp214[32]="$(opt_t<var_t>,tqual_t,type_t)@";
static struct Cyc_Core_Failure_struct Cyc_yyfail_ParamDecl_tok={Cyc_Core_Failure,{
_tmp214,_tmp214,_tmp214 + 32}};struct _tuple2*Cyc_yyget_ParamDecl_tok(void*yy1){
struct _tuple2*yyzzz;{void*_tmp215=yy1;struct _tuple2*_tmp216;_LL188: if(*((void**)
_tmp215)!= Cyc_ParamDecl_tok)goto _LL18A;_tmp216=((struct Cyc_ParamDecl_tok_struct*)
_tmp215)->f1;_LL189: yyzzz=_tmp216;goto _LL187;_LL18A:;_LL18B:(int)_throw((void*)&
Cyc_yyfail_ParamDecl_tok);_LL187:;}return yyzzz;}static char _tmp218[40]="list_t<$(opt_t<var_t>,tqual_t,type_t)@>";
static struct Cyc_Core_Failure_struct Cyc_yyfail_ParamDeclList_tok={Cyc_Core_Failure,{
_tmp218,_tmp218,_tmp218 + 40}};struct Cyc_List_List*Cyc_yyget_ParamDeclList_tok(
void*yy1){struct Cyc_List_List*yyzzz;{void*_tmp219=yy1;struct Cyc_List_List*
_tmp21A;_LL18D: if(*((void**)_tmp219)!= Cyc_ParamDeclList_tok)goto _LL18F;_tmp21A=((
struct Cyc_ParamDeclList_tok_struct*)_tmp219)->f1;_LL18E: yyzzz=_tmp21A;goto _LL18C;
_LL18F:;_LL190:(int)_throw((void*)& Cyc_yyfail_ParamDeclList_tok);_LL18C:;}return
yyzzz;}static char _tmp21C[107]="$(list_t<$(opt_t<var_t>,tqual_t,type_t)@>, bool,vararg_info_t *,opt_t<type_t>, list_t<$(type_t,type_t)@>)@";
static struct Cyc_Core_Failure_struct Cyc_yyfail_YY3={Cyc_Core_Failure,{_tmp21C,
_tmp21C,_tmp21C + 107}};struct _tuple14*Cyc_yyget_YY3(void*yy1){struct _tuple14*
yyzzz;{void*_tmp21D=yy1;struct _tuple14*_tmp21E;_LL192: if(*((void**)_tmp21D)!= 
Cyc_YY3)goto _LL194;_tmp21E=((struct Cyc_YY3_struct*)_tmp21D)->f1;_LL193: yyzzz=
_tmp21E;goto _LL191;_LL194:;_LL195:(int)_throw((void*)& Cyc_yyfail_YY3);_LL191:;}
return yyzzz;}static char _tmp220[15]="list_t<type_t>";static struct Cyc_Core_Failure_struct
Cyc_yyfail_TypeList_tok={Cyc_Core_Failure,{_tmp220,_tmp220,_tmp220 + 15}};struct
Cyc_List_List*Cyc_yyget_TypeList_tok(void*yy1){struct Cyc_List_List*yyzzz;{void*
_tmp221=yy1;struct Cyc_List_List*_tmp222;_LL197: if(*((void**)_tmp221)!= Cyc_TypeList_tok)
goto _LL199;_tmp222=((struct Cyc_TypeList_tok_struct*)_tmp221)->f1;_LL198: yyzzz=
_tmp222;goto _LL196;_LL199:;_LL19A:(int)_throw((void*)& Cyc_yyfail_TypeList_tok);
_LL196:;}return yyzzz;}static char _tmp224[21]="list_t<designator_t>";static struct
Cyc_Core_Failure_struct Cyc_yyfail_DesignatorList_tok={Cyc_Core_Failure,{_tmp224,
_tmp224,_tmp224 + 21}};struct Cyc_List_List*Cyc_yyget_DesignatorList_tok(void*yy1){
struct Cyc_List_List*yyzzz;{void*_tmp225=yy1;struct Cyc_List_List*_tmp226;_LL19C:
if(*((void**)_tmp225)!= Cyc_DesignatorList_tok)goto _LL19E;_tmp226=((struct Cyc_DesignatorList_tok_struct*)
_tmp225)->f1;_LL19D: yyzzz=_tmp226;goto _LL19B;_LL19E:;_LL19F:(int)_throw((void*)&
Cyc_yyfail_DesignatorList_tok);_LL19B:;}return yyzzz;}static char _tmp228[13]="designator_t";
static struct Cyc_Core_Failure_struct Cyc_yyfail_Designator_tok={Cyc_Core_Failure,{
_tmp228,_tmp228,_tmp228 + 13}};void*Cyc_yyget_Designator_tok(void*yy1){void*yyzzz;{
void*_tmp229=yy1;void*_tmp22A;_LL1A1: if(*((void**)_tmp229)!= Cyc_Designator_tok)
goto _LL1A3;_tmp22A=(void*)((struct Cyc_Designator_tok_struct*)_tmp229)->f1;_LL1A2:
yyzzz=_tmp22A;goto _LL1A0;_LL1A3:;_LL1A4:(int)_throw((void*)& Cyc_yyfail_Designator_tok);
_LL1A0:;}return yyzzz;}static char _tmp22C[7]="kind_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_Kind_tok={Cyc_Core_Failure,{_tmp22C,_tmp22C,_tmp22C + 7}};void*Cyc_yyget_Kind_tok(
void*yy1){void*yyzzz;{void*_tmp22D=yy1;void*_tmp22E;_LL1A6: if(*((void**)_tmp22D)
!= Cyc_Kind_tok)goto _LL1A8;_tmp22E=(void*)((struct Cyc_Kind_tok_struct*)_tmp22D)->f1;
_LL1A7: yyzzz=_tmp22E;goto _LL1A5;_LL1A8:;_LL1A9:(int)_throw((void*)& Cyc_yyfail_Kind_tok);
_LL1A5:;}return yyzzz;}static char _tmp230[7]="type_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_Type_tok={Cyc_Core_Failure,{_tmp230,_tmp230,_tmp230 + 7}};void*Cyc_yyget_Type_tok(
void*yy1){void*yyzzz;{void*_tmp231=yy1;void*_tmp232;_LL1AB: if(*((void**)_tmp231)
!= Cyc_Type_tok)goto _LL1AD;_tmp232=(void*)((struct Cyc_Type_tok_struct*)_tmp231)->f1;
_LL1AC: yyzzz=_tmp232;goto _LL1AA;_LL1AD:;_LL1AE:(int)_throw((void*)& Cyc_yyfail_Type_tok);
_LL1AA:;}return yyzzz;}static char _tmp234[20]="list_t<attribute_t>";static struct
Cyc_Core_Failure_struct Cyc_yyfail_AttributeList_tok={Cyc_Core_Failure,{_tmp234,
_tmp234,_tmp234 + 20}};struct Cyc_List_List*Cyc_yyget_AttributeList_tok(void*yy1){
struct Cyc_List_List*yyzzz;{void*_tmp235=yy1;struct Cyc_List_List*_tmp236;_LL1B0:
if(*((void**)_tmp235)!= Cyc_AttributeList_tok)goto _LL1B2;_tmp236=((struct Cyc_AttributeList_tok_struct*)
_tmp235)->f1;_LL1B1: yyzzz=_tmp236;goto _LL1AF;_LL1B2:;_LL1B3:(int)_throw((void*)&
Cyc_yyfail_AttributeList_tok);_LL1AF:;}return yyzzz;}static char _tmp238[12]="attribute_t";
static struct Cyc_Core_Failure_struct Cyc_yyfail_Attribute_tok={Cyc_Core_Failure,{
_tmp238,_tmp238,_tmp238 + 12}};void*Cyc_yyget_Attribute_tok(void*yy1){void*yyzzz;{
void*_tmp239=yy1;void*_tmp23A;_LL1B5: if(*((void**)_tmp239)!= Cyc_Attribute_tok)
goto _LL1B7;_tmp23A=(void*)((struct Cyc_Attribute_tok_struct*)_tmp239)->f1;_LL1B6:
yyzzz=_tmp23A;goto _LL1B4;_LL1B7:;_LL1B8:(int)_throw((void*)& Cyc_yyfail_Attribute_tok);
_LL1B4:;}return yyzzz;}static char _tmp23C[12]="enumfield_t";static struct Cyc_Core_Failure_struct
Cyc_yyfail_Enumfield_tok={Cyc_Core_Failure,{_tmp23C,_tmp23C,_tmp23C + 12}};struct
Cyc_Absyn_Enumfield*Cyc_yyget_Enumfield_tok(void*yy1){struct Cyc_Absyn_Enumfield*
yyzzz;{void*_tmp23D=yy1;struct Cyc_Absyn_Enumfield*_tmp23E;_LL1BA: if(*((void**)
_tmp23D)!= Cyc_Enumfield_tok)goto _LL1BC;_tmp23E=((struct Cyc_Enumfield_tok_struct*)
_tmp23D)->f1;_LL1BB: yyzzz=_tmp23E;goto _LL1B9;_LL1BC:;_LL1BD:(int)_throw((void*)&
Cyc_yyfail_Enumfield_tok);_LL1B9:;}return yyzzz;}static char _tmp240[20]="list_t<enumfield_t>";
static struct Cyc_Core_Failure_struct Cyc_yyfail_EnumfieldList_tok={Cyc_Core_Failure,{
_tmp240,_tmp240,_tmp240 + 20}};struct Cyc_List_List*Cyc_yyget_EnumfieldList_tok(
void*yy1){struct Cyc_List_List*yyzzz;{void*_tmp241=yy1;struct Cyc_List_List*
_tmp242;_LL1BF: if(*((void**)_tmp241)!= Cyc_EnumfieldList_tok)goto _LL1C1;_tmp242=((
struct Cyc_EnumfieldList_tok_struct*)_tmp241)->f1;_LL1C0: yyzzz=_tmp242;goto _LL1BE;
_LL1C1:;_LL1C2:(int)_throw((void*)& Cyc_yyfail_EnumfieldList_tok);_LL1BE:;}return
yyzzz;}static char _tmp244[14]="opt_t<type_t>";static struct Cyc_Core_Failure_struct
Cyc_yyfail_TypeOpt_tok={Cyc_Core_Failure,{_tmp244,_tmp244,_tmp244 + 14}};struct
Cyc_Core_Opt*Cyc_yyget_TypeOpt_tok(void*yy1){struct Cyc_Core_Opt*yyzzz;{void*
_tmp245=yy1;struct Cyc_Core_Opt*_tmp246;_LL1C4: if(*((void**)_tmp245)!= Cyc_TypeOpt_tok)
goto _LL1C6;_tmp246=((struct Cyc_TypeOpt_tok_struct*)_tmp245)->f1;_LL1C5: yyzzz=
_tmp246;goto _LL1C3;_LL1C6:;_LL1C7:(int)_throw((void*)& Cyc_yyfail_TypeOpt_tok);
_LL1C3:;}return yyzzz;}static char _tmp248[26]="list_t<$(type_t,type_t)@>";static
struct Cyc_Core_Failure_struct Cyc_yyfail_Rgnorder_tok={Cyc_Core_Failure,{_tmp248,
_tmp248,_tmp248 + 26}};struct Cyc_List_List*Cyc_yyget_Rgnorder_tok(void*yy1){
struct Cyc_List_List*yyzzz;{void*_tmp249=yy1;struct Cyc_List_List*_tmp24A;_LL1C9:
if(*((void**)_tmp249)!= Cyc_Rgnorder_tok)goto _LL1CB;_tmp24A=((struct Cyc_Rgnorder_tok_struct*)
_tmp249)->f1;_LL1CA: yyzzz=_tmp24A;goto _LL1C8;_LL1CB:;_LL1CC:(int)_throw((void*)&
Cyc_yyfail_Rgnorder_tok);_LL1C8:;}return yyzzz;}static char _tmp24C[15]="conref_t<bool>";
static struct Cyc_Core_Failure_struct Cyc_yyfail_YY4={Cyc_Core_Failure,{_tmp24C,
_tmp24C,_tmp24C + 15}};struct Cyc_Absyn_Conref*Cyc_yyget_YY4(void*yy1){struct Cyc_Absyn_Conref*
yyzzz;{void*_tmp24D=yy1;struct Cyc_Absyn_Conref*_tmp24E;_LL1CE: if(*((void**)
_tmp24D)!= Cyc_YY4)goto _LL1D0;_tmp24E=((struct Cyc_YY4_struct*)_tmp24D)->f1;
_LL1CF: yyzzz=_tmp24E;goto _LL1CD;_LL1D0:;_LL1D1:(int)_throw((void*)& Cyc_yyfail_YY4);
_LL1CD:;}return yyzzz;}static char _tmp250[40]="list_t<$(Position::seg_t,qvar_t,bool)@>";
static struct Cyc_Core_Failure_struct Cyc_yyfail_YY5={Cyc_Core_Failure,{_tmp250,
_tmp250,_tmp250 + 40}};struct Cyc_List_List*Cyc_yyget_YY5(void*yy1){struct Cyc_List_List*
yyzzz;{void*_tmp251=yy1;struct Cyc_List_List*_tmp252;_LL1D3: if(*((void**)_tmp251)
!= Cyc_YY5)goto _LL1D5;_tmp252=((struct Cyc_YY5_struct*)_tmp251)->f1;_LL1D4: yyzzz=
_tmp252;goto _LL1D2;_LL1D5:;_LL1D6:(int)_throw((void*)& Cyc_yyfail_YY5);_LL1D2:;}
return yyzzz;}struct Cyc_Yyltype{int timestamp;int first_line;int first_column;int
last_line;int last_column;};struct Cyc_Yyltype Cyc_yynewloc(){return({struct Cyc_Yyltype
_tmp253;_tmp253.timestamp=0;_tmp253.first_line=0;_tmp253.first_column=0;_tmp253.last_line=
0;_tmp253.last_column=0;_tmp253;});}struct Cyc_Yyltype Cyc_yylloc={0,0,0,0,0};
static short Cyc_yytranslate[355]={0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,126,2,2,108,124,121,2,105,106,114,118,103,122,111,123,2,2,2,2,
2,2,2,2,2,2,110,100,109,104,117,116,115,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,112,2,113,120,107,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,101,119,102,125,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,
18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,
45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,
72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,
99};static char _tmp254[2]="$";static char _tmp255[6]="error";static char _tmp256[12]="$undefined.";
static char _tmp257[5]="AUTO";static char _tmp258[9]="REGISTER";static char _tmp259[7]="STATIC";
static char _tmp25A[7]="EXTERN";static char _tmp25B[8]="TYPEDEF";static char _tmp25C[5]="VOID";
static char _tmp25D[5]="CHAR";static char _tmp25E[6]="SHORT";static char _tmp25F[4]="INT";
static char _tmp260[5]="LONG";static char _tmp261[6]="FLOAT";static char _tmp262[7]="DOUBLE";
static char _tmp263[7]="SIGNED";static char _tmp264[9]="UNSIGNED";static char _tmp265[
6]="CONST";static char _tmp266[9]="VOLATILE";static char _tmp267[9]="RESTRICT";
static char _tmp268[7]="STRUCT";static char _tmp269[6]="UNION";static char _tmp26A[5]="CASE";
static char _tmp26B[8]="DEFAULT";static char _tmp26C[7]="INLINE";static char _tmp26D[7]="SIZEOF";
static char _tmp26E[9]="OFFSETOF";static char _tmp26F[3]="IF";static char _tmp270[5]="ELSE";
static char _tmp271[7]="SWITCH";static char _tmp272[6]="WHILE";static char _tmp273[3]="DO";
static char _tmp274[4]="FOR";static char _tmp275[5]="GOTO";static char _tmp276[9]="CONTINUE";
static char _tmp277[6]="BREAK";static char _tmp278[7]="RETURN";static char _tmp279[5]="ENUM";
static char _tmp27A[8]="NULL_kw";static char _tmp27B[4]="LET";static char _tmp27C[6]="THROW";
static char _tmp27D[4]="TRY";static char _tmp27E[6]="CATCH";static char _tmp27F[7]="EXPORT";
static char _tmp280[4]="NEW";static char _tmp281[9]="ABSTRACT";static char _tmp282[9]="FALLTHRU";
static char _tmp283[6]="USING";static char _tmp284[10]="NAMESPACE";static char _tmp285[
7]="TUNION";static char _tmp286[8]="XTUNION";static char _tmp287[7]="MALLOC";static
char _tmp288[8]="RMALLOC";static char _tmp289[7]="CALLOC";static char _tmp28A[8]="RCALLOC";
static char _tmp28B[9]="REGION_T";static char _tmp28C[9]="SIZEOF_T";static char
_tmp28D[6]="TAG_T";static char _tmp28E[7]="REGION";static char _tmp28F[5]="RNEW";
static char _tmp290[8]="REGIONS";static char _tmp291[13]="RESET_REGION";static char
_tmp292[4]="GEN";static char _tmp293[14]="NOZEROTERM_kw";static char _tmp294[12]="ZEROTERM_kw";
static char _tmp295[7]="PTR_OP";static char _tmp296[7]="INC_OP";static char _tmp297[7]="DEC_OP";
static char _tmp298[8]="LEFT_OP";static char _tmp299[9]="RIGHT_OP";static char _tmp29A[
6]="LE_OP";static char _tmp29B[6]="GE_OP";static char _tmp29C[6]="EQ_OP";static char
_tmp29D[6]="NE_OP";static char _tmp29E[7]="AND_OP";static char _tmp29F[6]="OR_OP";
static char _tmp2A0[11]="MUL_ASSIGN";static char _tmp2A1[11]="DIV_ASSIGN";static char
_tmp2A2[11]="MOD_ASSIGN";static char _tmp2A3[11]="ADD_ASSIGN";static char _tmp2A4[11]="SUB_ASSIGN";
static char _tmp2A5[12]="LEFT_ASSIGN";static char _tmp2A6[13]="RIGHT_ASSIGN";static
char _tmp2A7[11]="AND_ASSIGN";static char _tmp2A8[11]="XOR_ASSIGN";static char
_tmp2A9[10]="OR_ASSIGN";static char _tmp2AA[9]="ELLIPSIS";static char _tmp2AB[11]="LEFT_RIGHT";
static char _tmp2AC[12]="COLON_COLON";static char _tmp2AD[11]="IDENTIFIER";static
char _tmp2AE[17]="INTEGER_CONSTANT";static char _tmp2AF[7]="STRING";static char
_tmp2B0[19]="CHARACTER_CONSTANT";static char _tmp2B1[18]="FLOATING_CONSTANT";
static char _tmp2B2[9]="TYPE_VAR";static char _tmp2B3[13]="TYPEDEF_NAME";static char
_tmp2B4[16]="QUAL_IDENTIFIER";static char _tmp2B5[18]="QUAL_TYPEDEF_NAME";static
char _tmp2B6[13]="TYPE_INTEGER";static char _tmp2B7[10]="ATTRIBUTE";static char
_tmp2B8[4]="';'";static char _tmp2B9[4]="'{'";static char _tmp2BA[4]="'}'";static
char _tmp2BB[4]="','";static char _tmp2BC[4]="'='";static char _tmp2BD[4]="'('";
static char _tmp2BE[4]="')'";static char _tmp2BF[4]="'_'";static char _tmp2C0[4]="'$'";
static char _tmp2C1[4]="'<'";static char _tmp2C2[4]="':'";static char _tmp2C3[4]="'.'";
static char _tmp2C4[4]="'['";static char _tmp2C5[4]="']'";static char _tmp2C6[4]="'*'";
static char _tmp2C7[4]="'@'";static char _tmp2C8[4]="'?'";static char _tmp2C9[4]="'>'";
static char _tmp2CA[4]="'+'";static char _tmp2CB[4]="'|'";static char _tmp2CC[4]="'^'";
static char _tmp2CD[4]="'&'";static char _tmp2CE[4]="'-'";static char _tmp2CF[4]="'/'";
static char _tmp2D0[4]="'%'";static char _tmp2D1[4]="'~'";static char _tmp2D2[4]="'!'";
static char _tmp2D3[5]="prog";static char _tmp2D4[17]="translation_unit";static char
_tmp2D5[12]="export_list";static char _tmp2D6[19]="export_list_values";static char
_tmp2D7[21]="external_declaration";static char _tmp2D8[20]="function_definition";
static char _tmp2D9[21]="function_definition2";static char _tmp2DA[13]="using_action";
static char _tmp2DB[15]="unusing_action";static char _tmp2DC[17]="namespace_action";
static char _tmp2DD[19]="unnamespace_action";static char _tmp2DE[12]="declaration";
static char _tmp2DF[17]="declaration_list";static char _tmp2E0[23]="declaration_specifiers";
static char _tmp2E1[24]="storage_class_specifier";static char _tmp2E2[15]="attributes_opt";
static char _tmp2E3[11]="attributes";static char _tmp2E4[15]="attribute_list";static
char _tmp2E5[10]="attribute";static char _tmp2E6[15]="type_specifier";static char
_tmp2E7[25]="type_specifier_notypedef";static char _tmp2E8[5]="kind";static char
_tmp2E9[15]="type_qualifier";static char _tmp2EA[15]="enum_specifier";static char
_tmp2EB[11]="enum_field";static char _tmp2EC[22]="enum_declaration_list";static
char _tmp2ED[26]="struct_or_union_specifier";static char _tmp2EE[16]="type_params_opt";
static char _tmp2EF[16]="struct_or_union";static char _tmp2F0[24]="struct_declaration_list";
static char _tmp2F1[25]="struct_declaration_list0";static char _tmp2F2[21]="init_declarator_list";
static char _tmp2F3[22]="init_declarator_list0";static char _tmp2F4[16]="init_declarator";
static char _tmp2F5[19]="struct_declaration";static char _tmp2F6[25]="specifier_qualifier_list";
static char _tmp2F7[35]="notypedef_specifier_qualifier_list";static char _tmp2F8[23]="struct_declarator_list";
static char _tmp2F9[24]="struct_declarator_list0";static char _tmp2FA[18]="struct_declarator";
static char _tmp2FB[17]="tunion_specifier";static char _tmp2FC[18]="tunion_or_xtunion";
static char _tmp2FD[17]="tunionfield_list";static char _tmp2FE[18]="tunionfield_scope";
static char _tmp2FF[12]="tunionfield";static char _tmp300[11]="declarator";static
char _tmp301[23]="declarator_withtypedef";static char _tmp302[18]="direct_declarator";
static char _tmp303[30]="direct_declarator_withtypedef";static char _tmp304[8]="pointer";
static char _tmp305[12]="one_pointer";static char _tmp306[23]="pointer_null_and_bound";
static char _tmp307[14]="pointer_bound";static char _tmp308[18]="zeroterm_qual_opt";
static char _tmp309[8]="rgn_opt";static char _tmp30A[11]="tqual_list";static char
_tmp30B[20]="parameter_type_list";static char _tmp30C[9]="type_var";static char
_tmp30D[16]="optional_effect";static char _tmp30E[19]="optional_rgn_order";static
char _tmp30F[10]="rgn_order";static char _tmp310[16]="optional_inject";static char
_tmp311[11]="effect_set";static char _tmp312[14]="atomic_effect";static char _tmp313[
11]="region_set";static char _tmp314[15]="parameter_list";static char _tmp315[22]="parameter_declaration";
static char _tmp316[16]="identifier_list";static char _tmp317[17]="identifier_list0";
static char _tmp318[12]="initializer";static char _tmp319[18]="array_initializer";
static char _tmp31A[17]="initializer_list";static char _tmp31B[12]="designation";
static char _tmp31C[16]="designator_list";static char _tmp31D[11]="designator";
static char _tmp31E[10]="type_name";static char _tmp31F[14]="any_type_name";static
char _tmp320[15]="type_name_list";static char _tmp321[20]="abstract_declarator";
static char _tmp322[27]="direct_abstract_declarator";static char _tmp323[10]="statement";
static char _tmp324[18]="labeled_statement";static char _tmp325[21]="expression_statement";
static char _tmp326[19]="compound_statement";static char _tmp327[16]="block_item_list";
static char _tmp328[20]="selection_statement";static char _tmp329[15]="switch_clauses";
static char _tmp32A[20]="iteration_statement";static char _tmp32B[15]="jump_statement";
static char _tmp32C[12]="exp_pattern";static char _tmp32D[20]="conditional_pattern";
static char _tmp32E[19]="logical_or_pattern";static char _tmp32F[20]="logical_and_pattern";
static char _tmp330[21]="inclusive_or_pattern";static char _tmp331[21]="exclusive_or_pattern";
static char _tmp332[12]="and_pattern";static char _tmp333[17]="equality_pattern";
static char _tmp334[19]="relational_pattern";static char _tmp335[14]="shift_pattern";
static char _tmp336[17]="additive_pattern";static char _tmp337[23]="multiplicative_pattern";
static char _tmp338[13]="cast_pattern";static char _tmp339[14]="unary_pattern";
static char _tmp33A[16]="postfix_pattern";static char _tmp33B[16]="primary_pattern";
static char _tmp33C[8]="pattern";static char _tmp33D[19]="tuple_pattern_list";static
char _tmp33E[14]="field_pattern";static char _tmp33F[19]="field_pattern_list";
static char _tmp340[11]="expression";static char _tmp341[22]="assignment_expression";
static char _tmp342[20]="assignment_operator";static char _tmp343[23]="conditional_expression";
static char _tmp344[20]="constant_expression";static char _tmp345[22]="logical_or_expression";
static char _tmp346[23]="logical_and_expression";static char _tmp347[24]="inclusive_or_expression";
static char _tmp348[24]="exclusive_or_expression";static char _tmp349[15]="and_expression";
static char _tmp34A[20]="equality_expression";static char _tmp34B[22]="relational_expression";
static char _tmp34C[17]="shift_expression";static char _tmp34D[20]="additive_expression";
static char _tmp34E[26]="multiplicative_expression";static char _tmp34F[16]="cast_expression";
static char _tmp350[17]="unary_expression";static char _tmp351[15]="unary_operator";
static char _tmp352[19]="postfix_expression";static char _tmp353[19]="primary_expression";
static char _tmp354[25]="argument_expression_list";static char _tmp355[26]="argument_expression_list0";
static char _tmp356[9]="constant";static char _tmp357[20]="qual_opt_identifier";
static char _tmp358[17]="qual_opt_typedef";static char _tmp359[18]="struct_union_name";
static char _tmp35A[11]="field_name";static char _tmp35B[12]="right_angle";static
struct _tagged_arr Cyc_yytname[264]={{_tmp254,_tmp254,_tmp254 + 2},{_tmp255,_tmp255,
_tmp255 + 6},{_tmp256,_tmp256,_tmp256 + 12},{_tmp257,_tmp257,_tmp257 + 5},{_tmp258,
_tmp258,_tmp258 + 9},{_tmp259,_tmp259,_tmp259 + 7},{_tmp25A,_tmp25A,_tmp25A + 7},{
_tmp25B,_tmp25B,_tmp25B + 8},{_tmp25C,_tmp25C,_tmp25C + 5},{_tmp25D,_tmp25D,
_tmp25D + 5},{_tmp25E,_tmp25E,_tmp25E + 6},{_tmp25F,_tmp25F,_tmp25F + 4},{_tmp260,
_tmp260,_tmp260 + 5},{_tmp261,_tmp261,_tmp261 + 6},{_tmp262,_tmp262,_tmp262 + 7},{
_tmp263,_tmp263,_tmp263 + 7},{_tmp264,_tmp264,_tmp264 + 9},{_tmp265,_tmp265,
_tmp265 + 6},{_tmp266,_tmp266,_tmp266 + 9},{_tmp267,_tmp267,_tmp267 + 9},{_tmp268,
_tmp268,_tmp268 + 7},{_tmp269,_tmp269,_tmp269 + 6},{_tmp26A,_tmp26A,_tmp26A + 5},{
_tmp26B,_tmp26B,_tmp26B + 8},{_tmp26C,_tmp26C,_tmp26C + 7},{_tmp26D,_tmp26D,
_tmp26D + 7},{_tmp26E,_tmp26E,_tmp26E + 9},{_tmp26F,_tmp26F,_tmp26F + 3},{_tmp270,
_tmp270,_tmp270 + 5},{_tmp271,_tmp271,_tmp271 + 7},{_tmp272,_tmp272,_tmp272 + 6},{
_tmp273,_tmp273,_tmp273 + 3},{_tmp274,_tmp274,_tmp274 + 4},{_tmp275,_tmp275,
_tmp275 + 5},{_tmp276,_tmp276,_tmp276 + 9},{_tmp277,_tmp277,_tmp277 + 6},{_tmp278,
_tmp278,_tmp278 + 7},{_tmp279,_tmp279,_tmp279 + 5},{_tmp27A,_tmp27A,_tmp27A + 8},{
_tmp27B,_tmp27B,_tmp27B + 4},{_tmp27C,_tmp27C,_tmp27C + 6},{_tmp27D,_tmp27D,
_tmp27D + 4},{_tmp27E,_tmp27E,_tmp27E + 6},{_tmp27F,_tmp27F,_tmp27F + 7},{_tmp280,
_tmp280,_tmp280 + 4},{_tmp281,_tmp281,_tmp281 + 9},{_tmp282,_tmp282,_tmp282 + 9},{
_tmp283,_tmp283,_tmp283 + 6},{_tmp284,_tmp284,_tmp284 + 10},{_tmp285,_tmp285,
_tmp285 + 7},{_tmp286,_tmp286,_tmp286 + 8},{_tmp287,_tmp287,_tmp287 + 7},{_tmp288,
_tmp288,_tmp288 + 8},{_tmp289,_tmp289,_tmp289 + 7},{_tmp28A,_tmp28A,_tmp28A + 8},{
_tmp28B,_tmp28B,_tmp28B + 9},{_tmp28C,_tmp28C,_tmp28C + 9},{_tmp28D,_tmp28D,
_tmp28D + 6},{_tmp28E,_tmp28E,_tmp28E + 7},{_tmp28F,_tmp28F,_tmp28F + 5},{_tmp290,
_tmp290,_tmp290 + 8},{_tmp291,_tmp291,_tmp291 + 13},{_tmp292,_tmp292,_tmp292 + 4},{
_tmp293,_tmp293,_tmp293 + 14},{_tmp294,_tmp294,_tmp294 + 12},{_tmp295,_tmp295,
_tmp295 + 7},{_tmp296,_tmp296,_tmp296 + 7},{_tmp297,_tmp297,_tmp297 + 7},{_tmp298,
_tmp298,_tmp298 + 8},{_tmp299,_tmp299,_tmp299 + 9},{_tmp29A,_tmp29A,_tmp29A + 6},{
_tmp29B,_tmp29B,_tmp29B + 6},{_tmp29C,_tmp29C,_tmp29C + 6},{_tmp29D,_tmp29D,
_tmp29D + 6},{_tmp29E,_tmp29E,_tmp29E + 7},{_tmp29F,_tmp29F,_tmp29F + 6},{_tmp2A0,
_tmp2A0,_tmp2A0 + 11},{_tmp2A1,_tmp2A1,_tmp2A1 + 11},{_tmp2A2,_tmp2A2,_tmp2A2 + 11},{
_tmp2A3,_tmp2A3,_tmp2A3 + 11},{_tmp2A4,_tmp2A4,_tmp2A4 + 11},{_tmp2A5,_tmp2A5,
_tmp2A5 + 12},{_tmp2A6,_tmp2A6,_tmp2A6 + 13},{_tmp2A7,_tmp2A7,_tmp2A7 + 11},{
_tmp2A8,_tmp2A8,_tmp2A8 + 11},{_tmp2A9,_tmp2A9,_tmp2A9 + 10},{_tmp2AA,_tmp2AA,
_tmp2AA + 9},{_tmp2AB,_tmp2AB,_tmp2AB + 11},{_tmp2AC,_tmp2AC,_tmp2AC + 12},{_tmp2AD,
_tmp2AD,_tmp2AD + 11},{_tmp2AE,_tmp2AE,_tmp2AE + 17},{_tmp2AF,_tmp2AF,_tmp2AF + 7},{
_tmp2B0,_tmp2B0,_tmp2B0 + 19},{_tmp2B1,_tmp2B1,_tmp2B1 + 18},{_tmp2B2,_tmp2B2,
_tmp2B2 + 9},{_tmp2B3,_tmp2B3,_tmp2B3 + 13},{_tmp2B4,_tmp2B4,_tmp2B4 + 16},{_tmp2B5,
_tmp2B5,_tmp2B5 + 18},{_tmp2B6,_tmp2B6,_tmp2B6 + 13},{_tmp2B7,_tmp2B7,_tmp2B7 + 10},{
_tmp2B8,_tmp2B8,_tmp2B8 + 4},{_tmp2B9,_tmp2B9,_tmp2B9 + 4},{_tmp2BA,_tmp2BA,
_tmp2BA + 4},{_tmp2BB,_tmp2BB,_tmp2BB + 4},{_tmp2BC,_tmp2BC,_tmp2BC + 4},{_tmp2BD,
_tmp2BD,_tmp2BD + 4},{_tmp2BE,_tmp2BE,_tmp2BE + 4},{_tmp2BF,_tmp2BF,_tmp2BF + 4},{
_tmp2C0,_tmp2C0,_tmp2C0 + 4},{_tmp2C1,_tmp2C1,_tmp2C1 + 4},{_tmp2C2,_tmp2C2,
_tmp2C2 + 4},{_tmp2C3,_tmp2C3,_tmp2C3 + 4},{_tmp2C4,_tmp2C4,_tmp2C4 + 4},{_tmp2C5,
_tmp2C5,_tmp2C5 + 4},{_tmp2C6,_tmp2C6,_tmp2C6 + 4},{_tmp2C7,_tmp2C7,_tmp2C7 + 4},{
_tmp2C8,_tmp2C8,_tmp2C8 + 4},{_tmp2C9,_tmp2C9,_tmp2C9 + 4},{_tmp2CA,_tmp2CA,
_tmp2CA + 4},{_tmp2CB,_tmp2CB,_tmp2CB + 4},{_tmp2CC,_tmp2CC,_tmp2CC + 4},{_tmp2CD,
_tmp2CD,_tmp2CD + 4},{_tmp2CE,_tmp2CE,_tmp2CE + 4},{_tmp2CF,_tmp2CF,_tmp2CF + 4},{
_tmp2D0,_tmp2D0,_tmp2D0 + 4},{_tmp2D1,_tmp2D1,_tmp2D1 + 4},{_tmp2D2,_tmp2D2,
_tmp2D2 + 4},{_tmp2D3,_tmp2D3,_tmp2D3 + 5},{_tmp2D4,_tmp2D4,_tmp2D4 + 17},{_tmp2D5,
_tmp2D5,_tmp2D5 + 12},{_tmp2D6,_tmp2D6,_tmp2D6 + 19},{_tmp2D7,_tmp2D7,_tmp2D7 + 21},{
_tmp2D8,_tmp2D8,_tmp2D8 + 20},{_tmp2D9,_tmp2D9,_tmp2D9 + 21},{_tmp2DA,_tmp2DA,
_tmp2DA + 13},{_tmp2DB,_tmp2DB,_tmp2DB + 15},{_tmp2DC,_tmp2DC,_tmp2DC + 17},{
_tmp2DD,_tmp2DD,_tmp2DD + 19},{_tmp2DE,_tmp2DE,_tmp2DE + 12},{_tmp2DF,_tmp2DF,
_tmp2DF + 17},{_tmp2E0,_tmp2E0,_tmp2E0 + 23},{_tmp2E1,_tmp2E1,_tmp2E1 + 24},{
_tmp2E2,_tmp2E2,_tmp2E2 + 15},{_tmp2E3,_tmp2E3,_tmp2E3 + 11},{_tmp2E4,_tmp2E4,
_tmp2E4 + 15},{_tmp2E5,_tmp2E5,_tmp2E5 + 10},{_tmp2E6,_tmp2E6,_tmp2E6 + 15},{
_tmp2E7,_tmp2E7,_tmp2E7 + 25},{_tmp2E8,_tmp2E8,_tmp2E8 + 5},{_tmp2E9,_tmp2E9,
_tmp2E9 + 15},{_tmp2EA,_tmp2EA,_tmp2EA + 15},{_tmp2EB,_tmp2EB,_tmp2EB + 11},{
_tmp2EC,_tmp2EC,_tmp2EC + 22},{_tmp2ED,_tmp2ED,_tmp2ED + 26},{_tmp2EE,_tmp2EE,
_tmp2EE + 16},{_tmp2EF,_tmp2EF,_tmp2EF + 16},{_tmp2F0,_tmp2F0,_tmp2F0 + 24},{
_tmp2F1,_tmp2F1,_tmp2F1 + 25},{_tmp2F2,_tmp2F2,_tmp2F2 + 21},{_tmp2F3,_tmp2F3,
_tmp2F3 + 22},{_tmp2F4,_tmp2F4,_tmp2F4 + 16},{_tmp2F5,_tmp2F5,_tmp2F5 + 19},{
_tmp2F6,_tmp2F6,_tmp2F6 + 25},{_tmp2F7,_tmp2F7,_tmp2F7 + 35},{_tmp2F8,_tmp2F8,
_tmp2F8 + 23},{_tmp2F9,_tmp2F9,_tmp2F9 + 24},{_tmp2FA,_tmp2FA,_tmp2FA + 18},{
_tmp2FB,_tmp2FB,_tmp2FB + 17},{_tmp2FC,_tmp2FC,_tmp2FC + 18},{_tmp2FD,_tmp2FD,
_tmp2FD + 17},{_tmp2FE,_tmp2FE,_tmp2FE + 18},{_tmp2FF,_tmp2FF,_tmp2FF + 12},{
_tmp300,_tmp300,_tmp300 + 11},{_tmp301,_tmp301,_tmp301 + 23},{_tmp302,_tmp302,
_tmp302 + 18},{_tmp303,_tmp303,_tmp303 + 30},{_tmp304,_tmp304,_tmp304 + 8},{_tmp305,
_tmp305,_tmp305 + 12},{_tmp306,_tmp306,_tmp306 + 23},{_tmp307,_tmp307,_tmp307 + 14},{
_tmp308,_tmp308,_tmp308 + 18},{_tmp309,_tmp309,_tmp309 + 8},{_tmp30A,_tmp30A,
_tmp30A + 11},{_tmp30B,_tmp30B,_tmp30B + 20},{_tmp30C,_tmp30C,_tmp30C + 9},{_tmp30D,
_tmp30D,_tmp30D + 16},{_tmp30E,_tmp30E,_tmp30E + 19},{_tmp30F,_tmp30F,_tmp30F + 10},{
_tmp310,_tmp310,_tmp310 + 16},{_tmp311,_tmp311,_tmp311 + 11},{_tmp312,_tmp312,
_tmp312 + 14},{_tmp313,_tmp313,_tmp313 + 11},{_tmp314,_tmp314,_tmp314 + 15},{
_tmp315,_tmp315,_tmp315 + 22},{_tmp316,_tmp316,_tmp316 + 16},{_tmp317,_tmp317,
_tmp317 + 17},{_tmp318,_tmp318,_tmp318 + 12},{_tmp319,_tmp319,_tmp319 + 18},{
_tmp31A,_tmp31A,_tmp31A + 17},{_tmp31B,_tmp31B,_tmp31B + 12},{_tmp31C,_tmp31C,
_tmp31C + 16},{_tmp31D,_tmp31D,_tmp31D + 11},{_tmp31E,_tmp31E,_tmp31E + 10},{
_tmp31F,_tmp31F,_tmp31F + 14},{_tmp320,_tmp320,_tmp320 + 15},{_tmp321,_tmp321,
_tmp321 + 20},{_tmp322,_tmp322,_tmp322 + 27},{_tmp323,_tmp323,_tmp323 + 10},{
_tmp324,_tmp324,_tmp324 + 18},{_tmp325,_tmp325,_tmp325 + 21},{_tmp326,_tmp326,
_tmp326 + 19},{_tmp327,_tmp327,_tmp327 + 16},{_tmp328,_tmp328,_tmp328 + 20},{
_tmp329,_tmp329,_tmp329 + 15},{_tmp32A,_tmp32A,_tmp32A + 20},{_tmp32B,_tmp32B,
_tmp32B + 15},{_tmp32C,_tmp32C,_tmp32C + 12},{_tmp32D,_tmp32D,_tmp32D + 20},{
_tmp32E,_tmp32E,_tmp32E + 19},{_tmp32F,_tmp32F,_tmp32F + 20},{_tmp330,_tmp330,
_tmp330 + 21},{_tmp331,_tmp331,_tmp331 + 21},{_tmp332,_tmp332,_tmp332 + 12},{
_tmp333,_tmp333,_tmp333 + 17},{_tmp334,_tmp334,_tmp334 + 19},{_tmp335,_tmp335,
_tmp335 + 14},{_tmp336,_tmp336,_tmp336 + 17},{_tmp337,_tmp337,_tmp337 + 23},{
_tmp338,_tmp338,_tmp338 + 13},{_tmp339,_tmp339,_tmp339 + 14},{_tmp33A,_tmp33A,
_tmp33A + 16},{_tmp33B,_tmp33B,_tmp33B + 16},{_tmp33C,_tmp33C,_tmp33C + 8},{_tmp33D,
_tmp33D,_tmp33D + 19},{_tmp33E,_tmp33E,_tmp33E + 14},{_tmp33F,_tmp33F,_tmp33F + 19},{
_tmp340,_tmp340,_tmp340 + 11},{_tmp341,_tmp341,_tmp341 + 22},{_tmp342,_tmp342,
_tmp342 + 20},{_tmp343,_tmp343,_tmp343 + 23},{_tmp344,_tmp344,_tmp344 + 20},{
_tmp345,_tmp345,_tmp345 + 22},{_tmp346,_tmp346,_tmp346 + 23},{_tmp347,_tmp347,
_tmp347 + 24},{_tmp348,_tmp348,_tmp348 + 24},{_tmp349,_tmp349,_tmp349 + 15},{
_tmp34A,_tmp34A,_tmp34A + 20},{_tmp34B,_tmp34B,_tmp34B + 22},{_tmp34C,_tmp34C,
_tmp34C + 17},{_tmp34D,_tmp34D,_tmp34D + 20},{_tmp34E,_tmp34E,_tmp34E + 26},{
_tmp34F,_tmp34F,_tmp34F + 16},{_tmp350,_tmp350,_tmp350 + 17},{_tmp351,_tmp351,
_tmp351 + 15},{_tmp352,_tmp352,_tmp352 + 19},{_tmp353,_tmp353,_tmp353 + 19},{
_tmp354,_tmp354,_tmp354 + 25},{_tmp355,_tmp355,_tmp355 + 26},{_tmp356,_tmp356,
_tmp356 + 9},{_tmp357,_tmp357,_tmp357 + 20},{_tmp358,_tmp358,_tmp358 + 17},{_tmp359,
_tmp359,_tmp359 + 18},{_tmp35A,_tmp35A,_tmp35A + 11},{_tmp35B,_tmp35B,_tmp35B + 12}};
static short Cyc_yyr1[477]={0,127,128,128,128,128,128,128,128,128,129,129,130,130,
131,131,131,132,132,132,132,133,133,134,135,136,137,138,138,138,138,139,139,140,
140,140,140,140,140,140,140,140,140,141,141,141,141,141,141,141,142,142,143,144,
144,145,145,145,145,145,145,146,146,147,147,147,147,147,147,147,147,147,147,147,
147,147,147,147,147,147,147,147,147,147,148,149,149,149,150,150,150,151,151,152,
152,152,153,153,153,154,154,155,155,156,156,157,157,158,159,159,160,160,161,162,
162,162,162,162,162,163,163,163,163,163,163,164,165,165,166,166,166,167,167,167,
168,168,169,169,169,169,170,170,170,171,171,172,172,173,173,174,174,174,174,174,
174,174,174,174,174,175,175,175,175,175,175,175,175,175,175,175,176,176,177,178,
178,178,179,179,179,180,180,180,181,181,181,182,182,183,183,183,183,184,184,185,
185,186,186,187,187,188,188,189,189,190,190,190,190,191,191,192,192,193,193,193,
194,195,195,196,196,197,197,197,197,198,198,198,198,199,200,200,201,201,202,202,
203,203,203,203,203,204,204,205,205,205,206,206,206,206,206,206,206,206,206,206,
206,207,207,207,207,207,207,207,207,207,207,207,208,209,209,210,210,211,211,211,
211,211,211,212,212,212,212,213,213,213,213,213,213,214,214,214,214,214,214,214,
214,214,214,214,214,214,214,215,215,215,215,215,215,215,215,216,217,217,218,218,
219,219,220,220,221,221,222,222,223,223,223,224,224,224,224,224,225,225,225,226,
226,226,227,227,227,227,228,228,229,229,229,229,229,229,229,230,231,232,232,232,
232,232,232,232,232,232,232,232,232,233,233,234,234,235,235,236,236,237,237,238,
238,238,238,238,238,238,238,238,238,238,239,239,239,239,239,239,239,240,241,241,
242,242,243,243,244,244,245,245,246,246,246,247,247,247,247,247,248,248,248,249,
249,249,250,250,250,250,251,251,252,252,252,252,252,252,252,252,252,252,252,252,
252,252,252,252,253,253,253,254,254,254,254,254,254,254,254,254,254,255,255,255,
255,255,255,255,255,255,256,257,257,258,258,258,258,259,259,260,260,261,261,262,
262,263,263};static short Cyc_yyr2[477]={0,1,2,3,5,3,5,6,7,0,4,3,2,3,1,1,1,2,3,3,4,
3,4,2,1,2,1,2,3,5,3,1,2,1,2,1,2,1,2,1,2,1,2,1,1,1,1,2,1,1,0,1,6,1,3,1,1,4,4,4,8,
1,2,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,4,4,4,4,1,1,1,1,1,1,5,2,4,1,3,1,2,3,4,8,3,0,3,
1,1,0,1,1,2,1,1,3,1,3,3,1,2,1,2,1,2,1,2,1,2,1,2,1,1,3,1,2,3,7,4,6,1,1,1,2,3,3,0,
1,1,2,5,1,2,1,2,1,3,4,4,5,4,5,4,4,2,1,1,3,4,4,5,4,5,4,4,2,1,2,5,2,2,1,0,3,3,0,1,
1,0,1,1,0,2,3,5,5,7,1,3,0,2,0,2,3,5,0,1,1,3,2,3,4,1,1,3,1,3,2,1,2,1,1,3,1,1,2,3,
4,8,1,2,3,4,2,1,2,3,2,1,2,1,2,3,4,3,1,3,1,1,2,3,3,4,4,5,4,3,5,4,4,2,1,1,1,1,1,1,
6,3,9,6,5,3,1,2,2,3,1,2,1,2,1,2,5,7,7,6,0,3,4,5,6,7,5,7,6,7,7,8,7,8,8,9,6,7,7,8,
3,2,2,2,3,2,4,5,1,1,5,1,3,1,3,1,3,1,3,1,3,1,3,3,1,3,3,3,3,1,3,3,1,3,3,1,3,3,3,1,
4,1,2,2,4,2,6,6,1,1,1,3,1,1,4,4,5,2,2,2,4,4,1,3,1,2,1,3,1,3,1,3,1,1,1,1,1,1,1,1,
1,1,1,1,5,2,2,2,5,5,1,1,3,1,3,1,3,1,3,1,3,1,3,3,1,3,3,3,3,1,3,3,1,3,3,1,3,3,3,1,
4,1,2,2,2,2,2,2,4,2,6,6,5,4,6,9,11,1,1,1,1,4,3,4,3,3,2,2,6,7,1,1,1,3,2,5,4,5,5,1,
1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1};static short Cyc_yydefact[947]={0,16,43,44,45,46,
48,63,66,67,68,69,70,71,72,73,85,86,87,101,102,39,0,0,49,0,0,134,135,0,0,82,467,
191,469,468,470,83,0,0,64,0,176,176,175,1,0,14,0,0,15,0,33,41,35,61,37,74,75,0,
76,182,0,145,0,170,179,77,149,99,47,46,40,0,89,466,0,467,463,464,465,0,351,0,0,0,
0,214,0,353,354,23,25,0,0,0,0,0,0,0,0,0,0,173,174,2,0,0,0,0,27,0,107,108,110,34,
42,36,38,103,471,472,99,184,0,183,0,31,0,0,17,193,0,0,158,146,171,181,180,182,0,
62,0,47,93,0,91,0,467,359,0,0,0,0,0,0,0,0,0,0,99,0,0,453,0,0,0,0,0,440,438,439,0,
369,371,384,392,394,396,398,400,402,405,410,413,416,420,0,422,441,452,451,0,360,
358,30,0,0,99,0,0,0,117,113,115,232,234,0,0,0,473,474,192,84,0,0,150,65,212,0,
209,0,0,0,3,0,5,0,28,0,0,0,18,0,104,105,0,98,99,0,0,0,0,0,0,0,0,0,0,0,0,0,467,
267,269,275,271,0,273,255,256,257,0,258,259,260,0,32,19,110,199,215,0,0,195,193,
0,239,0,179,0,50,0,0,94,90,0,0,0,0,0,430,0,386,420,0,387,388,0,0,0,0,0,0,0,423,
424,0,0,0,0,426,427,425,0,352,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,374,375,376,
377,378,379,380,381,382,383,373,0,428,0,447,448,0,0,0,455,0,99,363,0,216,0,0,0,0,
235,207,0,118,123,119,121,114,116,193,0,241,233,242,476,475,0,79,80,81,56,55,0,
53,151,193,211,147,241,213,159,160,0,78,0,178,177,24,0,26,0,109,111,218,217,20,
96,106,0,0,0,125,126,128,0,99,0,132,0,0,0,0,0,0,302,303,304,0,0,306,0,0,0,0,0,0,
276,272,110,274,270,268,200,0,0,0,206,194,201,154,0,0,0,195,156,0,157,152,179,
185,51,100,0,95,391,92,88,361,362,0,0,0,219,0,0,223,0,0,0,228,0,0,0,0,0,0,0,0,0,
454,461,0,460,370,393,0,395,397,399,401,403,404,408,409,406,407,411,412,414,415,
417,418,419,372,446,443,0,445,0,0,0,0,355,29,0,365,367,0,356,0,0,236,124,120,122,
0,195,0,179,0,243,193,0,0,254,238,0,0,0,117,0,99,193,0,0,169,148,210,4,6,0,129,
112,0,0,195,99,140,0,0,0,0,0,0,0,301,305,0,0,0,262,0,0,0,266,0,21,193,0,203,0,0,
196,0,155,199,187,240,153,185,172,0,7,0,429,0,0,231,0,220,0,224,227,229,434,0,0,
0,0,0,0,0,0,421,457,0,0,444,442,0,0,364,366,357,0,237,208,250,0,244,245,179,0,
195,0,179,0,0,0,0,52,54,0,161,0,195,0,0,179,0,127,130,103,133,142,141,0,0,136,0,
0,0,0,0,0,0,0,0,281,307,0,0,0,0,22,195,0,204,202,0,195,0,186,0,8,0,0,0,230,221,
225,0,0,0,0,389,390,433,459,0,462,385,456,458,0,368,249,247,252,0,253,246,179,0,
59,57,58,162,165,0,167,168,163,179,0,131,143,140,140,277,281,287,0,0,0,0,0,0,0,0,
0,0,0,0,308,0,0,0,265,189,205,197,188,193,11,0,0,432,431,0,226,435,0,0,449,0,251,
248,0,166,164,97,0,139,138,0,0,0,289,0,0,0,297,0,0,0,0,0,0,0,0,0,0,0,309,310,312,
314,316,318,320,322,325,330,333,336,340,342,349,350,0,0,280,261,264,0,0,195,10,
12,0,0,0,0,450,0,0,278,279,288,290,291,0,299,298,0,293,0,0,0,0,346,0,0,343,281,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,344,282,0,198,190,13,0,0,0,0,144,292,300,
294,295,0,0,0,0,281,283,313,0,315,317,319,321,323,324,328,329,326,327,331,332,
334,335,337,338,339,0,0,222,436,0,60,296,345,0,341,284,0,281,263,0,0,0,311,281,
285,437,348,347,286,0,0,0};static short Cyc_yydefgoto[137]={944,45,607,769,46,47,
255,48,402,49,404,50,128,51,52,463,202,386,387,203,55,212,204,57,144,145,58,141,
59,233,234,111,112,113,235,205,371,414,415,416,60,61,670,671,672,62,417,63,391,
64,65,66,103,139,124,604,539,67,540,455,596,447,451,452,366,275,220,86,87,479,
407,480,481,482,483,206,277,278,541,377,258,259,260,261,262,263,757,264,265,807,
808,809,810,811,812,813,814,815,816,817,818,819,820,821,822,357,358,530,531,266,
173,346,174,469,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,495,
496,190,191,69,770,213,381};static short Cyc_yypact[947]={2278,- -32768,- -32768,-
-32768,- -32768,- 6,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,-
-32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,2846,- 41,2920,-
-32768,215,5,- -32768,- -32768,28,34,53,- -32768,95,- -32768,- -32768,- -32768,- -32768,
125,108,122,204,171,171,- -32768,- -32768,2161,- -32768,256,327,- -32768,599,2846,
2846,2846,- -32768,2846,- -32768,- -32768,190,- -32768,133,2791,155,177,657,396,-
-32768,- -32768,276,242,280,- -32768,215,278,- -32768,4950,392,- -32768,- -32768,-
-32768,4911,- -32768,285,317,4950,308,318,339,- -32768,250,- -32768,- -32768,5395,
5395,5395,230,349,418,375,230,5612,1125,- -32768,- -32768,- -32768,2161,2394,2161,
2394,- -32768,373,388,- -32768,2725,- -32768,- -32768,- -32768,- -32768,5612,- -32768,-
-32768,276,- -32768,215,- -32768,1431,- -32768,2791,599,- -32768,5380,5395,3381,-
-32768,155,- -32768,- -32768,- -32768,133,5395,- -32768,2394,- -32768,403,457,425,215,
440,- -32768,140,5114,459,4911,4960,491,501,509,544,549,276,5163,5163,- -32768,
1798,556,5212,5212,5212,- -32768,- -32768,- -32768,332,- -32768,- -32768,- 33,506,570,
543,572,469,179,559,287,270,- -32768,1025,5212,172,- 29,- -32768,602,4950,- -32768,-
-32768,- -32768,623,4911,276,4950,616,134,5612,5627,5612,423,- -32768,48,48,48,-
-32768,- -32768,- -32768,- -32768,44,634,- -32768,- -32768,672,346,- -32768,637,78,644,
- -32768,646,- -32768,664,- -32768,418,3471,2791,- -32768,680,5612,- -32768,2001,658,
596,648,684,690,2930,695,696,711,716,3561,2930,234,105,700,713,- -32768,- -32768,
1555,1555,599,1555,- -32768,- -32768,- -32768,725,- -32768,- -32768,- -32768,367,-
-32768,- -32768,727,743,- -32768,101,728,729,421,731,722,55,396,734,745,55,740,215,
- -32768,4911,747,733,735,1798,- -32768,5612,- -32768,- -32768,3021,- -32768,778,4911,
4911,4911,4911,4911,750,1798,- -32768,- -32768,1555,755,390,4911,- -32768,- -32768,-
-32768,4911,- -32768,5212,4911,5212,5212,5212,5212,5212,5212,5212,5212,5212,5212,
5212,5212,5212,5212,5212,5212,5212,- -32768,- -32768,- -32768,- -32768,- -32768,-
-32768,- -32768,- -32768,- -32768,- -32768,- -32768,4911,- -32768,230,- -32768,- -32768,
3651,230,4911,- -32768,753,276,- -32768,419,- -32768,427,973,447,5395,- -32768,760,
754,- -32768,5627,5627,5627,- -32768,- -32768,2557,3741,240,- -32768,311,- -32768,-
-32768,101,- -32768,- -32768,- -32768,- -32768,759,761,762,- -32768,2448,- -32768,366,
235,- -32768,- -32768,- -32768,5612,- -32768,1917,- -32768,- -32768,- -32768,2161,-
-32768,2161,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,809,4911,766,
767,- -32768,763,221,276,215,768,4911,4911,4911,841,1679,774,- -32768,- -32768,-
-32768,475,833,- -32768,3831,2930,782,788,4911,2930,- -32768,- -32768,2725,- -32768,
- -32768,- -32768,- -32768,5612,775,195,- -32768,- -32768,765,- -32768,101,772,5511,
729,- -32768,5395,- -32768,- -32768,396,757,- -32768,- -32768,2044,- -32768,- -32768,-
-32768,- -32768,- -32768,- -32768,779,783,798,- -32768,230,4911,- -32768,532,3471,210,
- -32768,476,785,786,789,481,5612,793,792,5037,- -32768,- -32768,795,804,- -32768,
506,257,570,543,572,469,179,179,559,559,559,559,287,287,270,270,- -32768,- -32768,
- -32768,- -32768,- -32768,- -32768,805,- -32768,167,5395,3291,4950,- -32768,- -32768,
4950,- -32768,- -32768,567,- -32768,- 14,816,- -32768,- -32768,- -32768,- -32768,810,729,
811,396,808,311,5496,5395,3921,- -32768,- -32768,708,820,44,2611,821,276,5380,5395,
4011,- -32768,366,- -32768,- -32768,- -32768,2013,- -32768,- -32768,2001,4911,729,276,
773,494,495,502,817,4101,4191,510,- -32768,- -32768,828,830,827,- -32768,818,824,
515,- -32768,2791,- -32768,834,5395,- -32768,836,101,- -32768,822,- -32768,119,-
-32768,722,- -32768,757,- -32768,839,- -32768,2161,840,116,835,- -32768,842,- -32768,
3111,- -32768,- -32768,- -32768,- -32768,4911,918,4911,4960,843,840,848,3291,- -32768,
- -32768,4911,4911,- -32768,- -32768,55,660,- -32768,- -32768,- -32768,973,- -32768,-
-32768,- -32768,850,- -32768,- -32768,396,854,729,55,396,849,516,858,859,- -32768,-
-32768,862,- -32768,863,729,865,55,396,860,- -32768,- -32768,5612,- -32768,- -32768,-
-32768,846,215,526,2930,875,2930,4911,4281,539,4371,541,4461,770,- -32768,877,890,
- 19,880,- -32768,729,187,- -32768,- -32768,887,729,5612,- -32768,361,- -32768,876,878,
4911,- -32768,- -32768,- -32768,3471,542,881,882,- -32768,778,- -32768,- -32768,705,-
-32768,- -32768,- -32768,- -32768,3291,- -32768,- -32768,- -32768,- -32768,883,- -32768,
- -32768,396,893,- -32768,- -32768,- -32768,- -32768,- -32768,884,- -32768,- -32768,-
-32768,396,886,- -32768,888,97,773,963,770,- -32768,552,2930,553,4551,2930,565,
4641,4731,581,932,885,892,- -32768,2930,2930,898,- -32768,- -32768,- -32768,895,-
-32768,834,- -32768,894,583,- -32768,- -32768,262,- -32768,- -32768,5612,975,- -32768,
3201,- -32768,- -32768,900,- -32768,- -32768,- -32768,5612,- -32768,- -32768,2930,899,
909,- -32768,2930,2930,594,- -32768,2930,2930,595,2930,638,4821,5261,905,1917,5212,
902,- -32768,- 18,- -32768,939,901,689,741,191,751,393,310,- -32768,- -32768,- -32768,
- -32768,940,5212,1555,- -32768,- -32768,- -32768,906,101,729,- -32768,- -32768,528,
4911,911,922,- -32768,941,651,- -32768,- -32768,- -32768,- -32768,- -32768,2930,-
-32768,- -32768,2930,- -32768,2930,2930,652,1798,- -32768,5612,923,- -32768,1307,
5212,4911,5212,5212,5212,5212,5212,5212,5212,5212,5212,5212,5212,5212,5212,5212,
5212,5212,5212,4911,- -32768,- -32768,943,- -32768,- -32768,- -32768,719,924,5612,927,
- -32768,- -32768,- -32768,- -32768,- -32768,2930,929,933,5212,770,- -32768,506,277,
570,543,543,469,179,179,559,559,559,559,287,287,270,270,- -32768,- -32768,- -32768,
297,2930,- -32768,- -32768,935,- -32768,- -32768,840,408,- -32768,- -32768,4911,1307,-
-32768,936,937,938,- -32768,770,- -32768,- -32768,- -32768,- -32768,- -32768,1038,1051,
- -32768};static short Cyc_yypgoto[137]={- -32768,70,- -32768,218,- -32768,- -32768,-
-32768,- -32768,- -32768,- -32768,- -32768,- 13,- 100,20,- -32768,- -32768,0,503,- -32768,
43,- 181,956,13,- -32768,- -32768,- 103,- -32768,3,- -32768,394,- -32768,- -32768,-
-32768,832,825,462,434,- -32768,- -32768,500,- -32768,- -32768,88,- -32768,- -32768,
117,- 191,1006,- 225,547,- -32768,- -32768,1029,- 59,934,474,- 120,51,- 105,147,252,484,
493,- 343,- 416,- 95,- 303,- 108,- -32768,- 228,- 138,- 488,- 277,- -32768,604,- 145,- 15,- 93,
30,- 183,153,- -32768,- -32768,26,- 226,- -32768,- 673,- -32768,- -32768,- -32768,- -32768,
- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,
- -32768,- -32768,- -32768,- -32768,15,891,451,- -32768,214,450,- -32768,- 150,- 368,-
134,- 312,- 308,- 318,771,- 309,- 305,- 195,- 304,- 280,- 159,404,337,- -32768,- -32768,-
311,- -32768,- 8,290,- 54,1036,- 317,24};static short Cyc_yytable[5736]={53,501,406,
293,498,121,219,311,312,313,500,273,503,56,231,89,296,504,505,308,297,53,369,276,
510,511,274,390,347,440,441,518,443,594,56,521,634,549,88,98,520,72,316,54,287,
565,53,282,32,127,512,513,53,53,53,35,53,860,354,56,73,384,53,134,54,56,56,56,89,
56,760,790,115,116,117,56,118,89,207,208,209,491,129,317,528,70,355,222,130,54,
761,149,639,561,92,54,54,54,861,54,194,127,668,669,380,54,53,53,53,53,612,597,
125,256,53,267,105,378,640,56,56,56,56,583,378,237,53,56,53,506,507,508,509,385,
129,134,468,93,713,56,232,56,53,94,591,473,257,474,129,54,54,54,54,561,268,56,99,
54,459,490,611,448,95,303,395,379,380,560,114,54,457,54,379,514,515,516,224,225,
226,227,399,467,395,96,89,54,900,369,369,369,125,89,544,560,435,33,380,32,554,-
137,665,361,449,368,35,210,699,38,446,544,100,211,283,39,436,215,370,437,267,272,
461,554,42,43,44,- 193,930,33,33,- 193,97,53,382,383,288,376,364,348,349,350,123,
421,256,256,56,256,269,289,393,324,325,129,365,615,38,53,53,409,53,939,131,868,
869,468,132,943,32,133,56,56,314,56,102,35,54,257,257,351,257,32,632,464,39,352,
353,34,35,36,326,33,68,119,700,764,256,172,327,593,54,54,870,54,460,219,32,380,
465,53,871,101,32,35,74,90,616,91,34,35,36,210,56,477,478,450,32,211,412,257,468,
68,34,35,36,627,433,555,68,705,395,434,389,68,589,142,623,373,269,374,533,120,54,
198,374,89,68,199,106,107,395,524,314,528,656,146,395,314,90,630,368,368,368,143,
835,656,442,90,529,548,309,147,314,370,370,370,332,140,704,931,68,553,192,559,
767,333,334,425,68,68,68,68,314,432,53,602,53,330,193,932,195,331,38,360,564,577,
238,56,545,56,468,68,546,196,569,547,876,646,53,108,109,127,633,450,68,877,878,
314,658,146,315,56,647,705,53,197,601,54,129,54,660,396,32,659,397,648,214,56,34,
35,36,137,138,431,129,768,661,38,53,445,590,54,314,556,562,228,563,557,603,774,
558,56,715,216,90,644,709,54,689,597,710,90,704,229,- 215,314,694,- 215,493,210,
935,555,365,150,705,211,309,450,284,32,394,54,395,874,484,395,35,875,488,89,309,
68,89,272,525,39,456,526,394,527,373,286,314,499,42,43,44,374,606,42,43,44,635,
322,323,636,548,904,905,68,901,150,525,704,223,532,903,291,906,294,294,285,559,
907,908,218,292,305,306,522,913,914,294,294,294,667,146,580,267,690,314,314,318,
236,618,280,314,365,721,622,584,53,725,294,588,218,915,916,298,314,314,881,673,
674,56,736,600,314,299,53,675,129,681,936,136,314,300,688,603,32,314,727,56,687,
728,34,35,36,741,328,329,742,89,836,54,899,613,614,572,573,574,749,578,752,314,
121,314,314,450,858,775,301,54,90,587,529,302,314,314,716,791,793,857,310,831,
320,367,880,372,781,314,637,638,797,724,909,910,911,912,698,784,394,408,802,394,
833,314,735,834,642,32,319,294,840,68,321,68,35,236,314,314,110,846,849,394,356,
39,140,938,420,394,896,570,897,359,42,43,44,666,917,918,919,294,363,294,294,294,
294,294,294,294,294,294,294,294,294,294,294,294,294,294,929,388,314,398,924,851,
408,400,89,401,485,486,487,375,422,396,314,68,890,895,419,494,32,717,718,497,392,
403,34,35,36,823,42,43,44,16,17,18,389,668,669,121,937,410,418,374,427,42,43,44,
423,678,680,755,756,723,424,517,651,652,653,426,494,536,537,538,438,733,778,779,
864,865,428,256,866,867,90,429,294,90,872,873,922,314,439,543,53,743,444,745,787,
788,230,446,706,453,218,763,458,56,454,380,766,466,394,38,257,256,462,494,470,
471,218,472,316,394,489,535,394,218,53,218,492,523,534,550,552,566,551,54,571,
567,575,56,568,579,581,585,586,598,257,592,450,294,595,494,608,609,610,619,620,
746,748,621,751,625,754,294,68,32,624,792,628,54,796,34,35,36,629,38,218,33,631,
827,828,412,773,641,643,218,256,375,645,676,42,43,44,654,657,90,682,683,408,53,
684,272,685,392,686,691,693,697,626,841,707,701,56,844,845,739,711,847,848,257,
850,712,702,720,803,804,418,722,740,726,795,729,730,799,801,731,732,75,734,294,
737,408,54,744,758,884,759,762,765,771,782,772,777,776,120,785,780,783,789,829,
786,826,825,832,650,830,891,837,842,892,839,893,894,76,218,663,843,856,75,859,
862,879,367,853,887,218,172,863,148,78,882,79,80,294,888,35,898,923,889,921,925,
294,927,928,805,945,82,83,934,940,941,942,90,84,76,926,886,806,946,885,85,169,
655,217,170,171,411,738,405,148,78,408,79,80,664,309,35,135,708,104,281,933,902,
408,696,81,714,82,83,883,695,477,478,617,84,692,719,362,502,824,920,85,122,0,0,0,
0,418,335,336,337,338,339,340,341,342,343,344,418,0,0,418,0,0,0,0,0,0,0,0,0,120,
0,0,0,236,345,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,151,152,0,0,0,
408,0,218,0,0,0,0,22,75,0,153,0,0,408,154,0,0,0,0,27,28,155,156,157,158,29,30,31,
0,159,200,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,855,0,0,294,0,0,0,32,
78,163,79,80,33,34,35,36,37,38,0,201,0,294,408,164,0,40,221,0,0,0,0,0,166,0,0,0,
167,0,0,168,169,218,0,170,171,0,0,0,0,0,0,0,0,0,0,0,0,294,0,294,294,294,294,294,
294,294,294,294,294,294,294,294,294,294,294,294,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,294,0,0,0,0,0,0,0,2,3,4,71,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,755,
756,21,151,152,239,294,240,241,242,243,244,245,246,247,22,75,23,153,248,0,0,154,
24,249,0,0,27,28,155,156,157,158,29,30,31,250,159,0,251,160,0,0,0,161,162,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,252,78,163,79,80,33,34,35,36,37,38,253,126,0,
0,0,164,0,40,221,0,0,0,0,0,166,0,0,0,167,0,0,168,169,0,0,170,171,2,3,4,71,6,7,8,
9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,151,152,239,0,240,241,242,243,244,245,
246,247,22,75,23,153,248,0,0,154,24,249,0,0,27,28,155,156,157,158,29,30,31,250,
159,0,251,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,252,78,163,
79,80,33,34,35,36,37,38,253,126,254,0,0,164,0,40,221,0,0,0,0,0,166,0,0,0,167,0,0,
168,169,0,0,170,171,2,3,4,71,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,151,
152,239,0,240,241,242,243,244,245,246,247,22,75,23,153,248,0,0,154,24,249,0,0,27,
28,155,156,157,158,29,30,31,250,159,0,251,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,252,78,163,79,80,33,34,35,36,37,38,253,126,0,0,0,164,0,40,
221,0,0,0,0,0,166,0,0,0,167,0,0,168,169,0,0,170,171,2,3,4,71,6,7,8,9,10,11,12,13,
14,15,16,17,18,19,20,0,0,21,151,152,0,0,0,0,0,0,0,0,0,0,22,75,23,153,0,0,0,154,
24,0,0,0,27,28,155,156,157,158,29,30,31,0,159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,33,34,35,36,37,38,576,0,0,0,0,164,
0,40,221,0,0,0,0,0,166,0,0,0,167,0,0,168,169,0,0,170,171,7,8,9,10,11,12,13,14,15,
16,17,18,19,20,0,0,0,151,152,0,0,0,0,0,0,0,0,0,0,22,75,0,153,0,0,0,154,0,0,0,0,
27,28,155,156,157,158,29,30,31,0,159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,33,34,35,36,37,38,0,307,0,0,0,164,0,40,221,0,
0,0,0,0,166,0,0,0,167,0,0,168,169,0,0,170,171,7,8,9,10,11,12,13,14,15,16,17,18,
19,20,0,0,0,151,152,0,0,0,0,0,0,0,0,0,0,22,75,0,153,0,0,0,154,0,0,0,0,27,28,155,
156,157,158,29,30,31,0,159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,32,78,163,79,80,33,34,35,36,37,38,0,0,0,0,0,164,0,40,221,0,0,0,0,0,166,
0,0,0,167,0,0,168,169,0,0,170,171,- 9,1,0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,
18,19,20,0,0,21,0,0,0,0,0,0,0,0,0,0,0,0,22,0,23,0,0,0,605,0,24,32,25,26,27,28,0,
34,35,36,29,30,31,32,0,0,0,412,0,34,35,36,413,0,0,0,42,43,44,412,0,0,0,0,0,0,0,0,
42,43,44,0,0,0,32,0,0,0,0,33,34,35,36,37,38,0,0,- 9,0,0,39,0,40,41,0,0,0,0,0,42,
43,44,- 9,1,0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,0,0,0,0,0,0,
0,0,0,0,0,0,22,0,23,0,0,0,0,0,24,0,25,26,27,28,0,0,0,0,29,30,31,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,0,33,34,35,36,37,38,0,0,-
9,0,0,39,0,40,41,0,0,0,0,0,42,43,44,- 9,1,0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
17,18,19,20,0,0,21,0,0,0,0,0,0,0,0,0,0,0,0,22,0,23,0,0,0,0,0,24,0,25,26,27,28,0,
0,0,0,29,30,31,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,
0,0,0,0,33,34,35,36,37,38,0,0,0,0,0,39,0,40,41,0,0,0,0,0,42,43,44,1,0,2,3,4,5,6,
7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,0,0,0,0,0,0,0,0,0,0,0,0,22,0,23,0,
0,0,0,0,24,0,25,26,27,28,0,0,0,0,29,30,31,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,
18,19,20,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,22,0,0,33,34,35,36,37,38,0,0,- 9,27,28,39,
0,40,41,29,30,31,0,0,42,43,44,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,270,
0,0,32,0,0,0,0,33,34,35,36,37,38,272,0,0,0,0,389,0,40,41,0,0,0,374,0,42,43,44,7,
8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,22,0,0,0,0,0,
0,0,0,0,0,0,27,28,0,0,0,0,29,30,31,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,
20,0,0,0,0,0,0,0,0,0,0,270,0,0,0,0,22,0,0,33,34,0,36,37,38,272,0,0,27,28,373,0,
40,41,29,30,31,374,0,42,43,44,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,32,0,0,0,0,33,34,35,36,37,38,0,0,0,0,0,412,0,40,41,0,0,0,0,0,42,43,44,2,3,4,71,
6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,0,0,0,0,0,0,0,0,0,0,0,0,22,0,23,
0,0,0,0,0,24,0,0,0,27,28,0,0,0,0,29,30,31,0,0,0,0,0,0,0,0,0,0,0,2,3,4,71,6,7,8,9,
10,11,12,13,14,15,16,17,18,19,20,0,0,21,0,0,0,33,34,0,36,37,38,0,126,0,22,230,23,
0,40,41,0,0,24,0,0,0,27,28,0,0,0,0,29,30,31,2,3,4,71,6,7,8,9,10,11,12,13,14,15,
16,17,18,19,20,0,0,21,0,0,0,0,0,0,0,0,0,0,0,0,22,0,33,34,0,36,37,38,24,126,0,0,
27,28,0,40,41,0,29,30,31,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,33,34,0,36,37,38,0,0,0,0,0,0,0,40,41,151,152,239,75,240,241,242,
243,244,245,246,247,0,75,0,153,248,0,0,154,0,249,0,0,0,0,155,156,157,158,0,0,0,
250,159,0,251,160,0,76,0,161,162,0,0,0,0,0,0,0,0,0,0,0,77,78,0,79,80,0,0,35,0,0,
252,78,163,79,80,0,81,35,82,83,0,253,126,0,0,84,164,0,0,165,0,0,85,0,0,166,0,151,
152,167,0,0,168,169,475,0,170,171,0,0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,
158,0,0,0,0,159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
32,78,163,79,80,0,0,35,0,0,0,0,295,476,0,0,164,0,0,165,0,0,477,478,0,166,151,152,
0,167,0,0,168,169,0,0,170,171,0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,
0,0,0,159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,
163,79,80,0,0,35,0,0,0,0,295,703,0,0,164,0,0,165,0,0,477,478,0,166,151,152,0,167,
0,0,168,169,0,0,170,171,0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,0,0,0,
159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,
80,0,0,35,0,0,0,0,295,838,0,0,164,0,0,165,0,0,477,478,0,166,151,152,0,167,0,0,
168,169,0,0,170,171,0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,0,0,0,159,
0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,
0,0,35,0,0,0,0,295,0,0,0,164,0,0,165,0,0,477,478,0,166,151,152,0,167,0,0,168,169,
0,0,170,171,0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,0,0,0,159,0,0,160,
0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,0,0,35,0,
0,0,0,0,0,0,0,164,0,0,165,0,0,0,0,279,166,151,152,0,167,0,0,168,169,0,0,170,171,
0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,0,0,0,159,0,0,160,0,0,0,161,
162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,0,0,35,0,0,0,0,295,
0,0,0,164,0,0,165,0,0,0,0,0,166,151,152,0,167,0,0,168,169,0,0,170,171,0,75,0,153,
0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,0,0,0,159,0,0,160,0,0,0,161,162,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,0,0,35,0,0,0,430,0,0,0,0,164,0,
0,165,0,0,0,0,0,166,151,152,0,167,0,0,168,169,0,0,170,171,0,75,0,153,0,0,0,154,0,
0,0,0,0,0,155,156,157,158,0,0,0,0,159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,0,0,35,0,0,0,0,0,0,0,0,164,519,0,165,0,0,0,
0,0,166,151,152,0,167,0,0,168,169,0,0,170,171,0,75,0,153,0,0,0,154,0,0,0,0,0,0,
155,156,157,158,0,0,0,0,159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,32,78,163,79,80,0,0,35,0,0,0,0,0,0,0,0,164,0,0,165,0,0,0,0,542,166,
151,152,0,167,0,0,168,169,0,0,170,171,0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,
157,158,0,0,0,0,159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,32,78,163,79,80,0,0,35,0,0,0,0,0,0,0,0,164,582,0,165,0,0,0,0,0,166,151,152,0,
167,0,0,168,169,0,0,170,171,0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,0,
0,0,159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,
163,79,80,0,0,35,0,0,0,0,0,0,0,0,164,0,0,165,0,0,0,0,649,166,151,152,0,167,0,0,
168,169,0,0,170,171,0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,0,0,0,159,
0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,
0,0,35,0,0,0,0,0,0,0,0,164,0,0,165,0,0,0,0,662,166,151,152,0,167,0,0,168,169,0,0,
170,171,0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,0,0,0,159,0,0,160,0,0,
0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,0,0,35,0,0,0,
677,0,0,0,0,164,0,0,165,0,0,0,0,0,166,151,152,0,167,0,0,168,169,0,0,170,171,0,75,
0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,0,0,0,159,0,0,160,0,0,0,161,162,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,0,0,35,0,0,0,679,0,0,0,0,
164,0,0,165,0,0,0,0,0,166,151,152,0,167,0,0,168,169,0,0,170,171,0,75,0,153,0,0,0,
154,0,0,0,0,0,0,155,156,157,158,0,0,0,0,159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,0,0,35,0,0,0,0,0,0,0,0,164,747,0,165,
0,0,0,0,0,166,151,152,0,167,0,0,168,169,0,0,170,171,0,75,0,153,0,0,0,154,0,0,0,0,
0,0,155,156,157,158,0,0,0,0,159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,32,78,163,79,80,0,0,35,0,0,0,0,0,0,0,0,164,750,0,165,0,0,0,0,0,
166,151,152,0,167,0,0,168,169,0,0,170,171,0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,
156,157,158,0,0,0,0,159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,32,78,163,79,80,0,0,35,0,0,0,753,0,0,0,0,164,0,0,165,0,0,0,0,0,166,151,
152,0,167,0,0,168,169,0,0,170,171,0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,
158,0,0,0,0,159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
32,78,163,79,80,0,0,35,0,0,0,0,0,0,0,0,164,794,0,165,0,0,0,0,0,166,151,152,0,167,
0,0,168,169,0,0,170,171,0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,0,0,0,
159,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,
80,0,0,35,0,0,0,0,0,0,0,0,164,798,0,165,0,0,0,0,0,166,151,152,0,167,0,0,168,169,
0,0,170,171,0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,0,0,0,159,0,0,160,
0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,0,0,35,0,
0,0,0,0,0,0,0,164,800,0,165,0,0,0,0,0,166,151,152,0,167,0,0,168,169,0,0,170,171,
0,75,0,153,0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,0,0,0,159,0,0,160,0,0,0,161,
162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,0,0,35,0,0,0,0,0,0,
0,0,164,852,0,165,0,0,0,0,0,166,151,152,0,167,0,0,168,169,0,0,170,171,0,75,0,153,
0,0,0,154,0,0,0,0,0,0,155,156,157,158,0,0,0,0,159,0,0,160,0,0,0,161,162,0,0,0,0,
0,0,151,152,0,75,0,0,0,0,0,0,0,0,0,75,0,32,78,163,79,80,0,0,35,0,0,0,155,156,157,
158,0,164,0,0,165,0,0,160,0,76,166,161,162,0,167,0,0,168,169,0,0,170,171,0,148,
78,0,79,80,0,0,35,0,0,32,78,163,79,80,0,81,35,82,83,0,0,295,151,152,84,164,0,0,
165,0,0,85,0,0,166,75,0,0,167,0,0,168,169,0,0,170,171,0,155,156,157,158,0,0,0,0,
0,0,0,160,0,0,0,161,162,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,
80,0,0,35,0,0,0,0,626,151,152,0,164,0,0,165,0,0,0,0,0,166,75,0,0,167,0,0,168,169,
0,0,170,171,0,155,156,157,158,0,0,0,0,0,0,0,160,0,0,0,161,162,0,0,0,0,0,0,151,
152,0,0,0,0,0,0,0,0,0,0,0,75,0,32,78,163,79,80,0,0,35,0,0,0,155,156,157,158,0,
290,0,0,165,0,0,160,0,0,166,161,162,0,167,0,0,168,169,151,152,170,171,0,0,0,0,0,
0,0,0,0,75,0,32,78,163,79,80,0,0,35,0,0,0,155,156,157,158,0,304,0,0,165,0,0,160,
0,0,166,161,162,0,167,0,0,168,169,151,152,170,171,0,0,0,0,0,0,0,0,0,75,0,32,78,
163,79,80,0,0,35,0,0,0,155,156,157,158,0,164,0,0,165,0,0,160,0,0,166,161,162,0,
167,0,0,168,169,0,0,170,171,0,0,0,0,0,0,0,0,0,0,0,32,78,163,79,80,0,0,35,0,0,0,0,
0,0,0,0,854,0,0,165,0,0,0,0,0,166,0,0,0,167,0,0,168,169,0,0,170,171,7,8,9,10,11,
12,13,14,15,16,17,18,19,20,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,22,0,0,0,0,0,
0,0,0,0,0,0,27,28,0,22,0,0,29,30,31,0,0,0,0,0,0,27,28,0,0,0,0,29,30,31,0,0,200,0,
0,0,0,0,0,0,0,0,0,270,0,0,271,0,0,0,0,33,34,0,36,37,38,272,0,0,0,0,0,0,40,41,33,
34,0,36,37,38,0,201,0,0,0,0,0,40,41,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,7,8,
9,10,11,12,13,14,15,16,17,18,19,20,22,0,0,0,0,0,0,0,0,0,0,0,27,28,0,22,0,0,29,30,
31,0,0,0,0,0,0,27,28,0,0,0,0,29,30,31,0,0,0,0,0,0,0,0,0,0,0,0,0,270,0,0,0,0,0,0,
0,33,34,0,36,37,38,272,599,0,0,0,0,0,40,41,33,34,0,36,37,38,0,0,0,0,0,0,0,40,41,
7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,
22,0,0,0,0,0,0,0,0,0,0,0,27,28,0,22,0,0,29,30,31,0,0,0,0,0,0,27,28,0,0,0,0,29,30,
31,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,33,34,0,36,37,38,0,0,0,0,0,0,0,40,
41,33,0,0,0,37,38,0,0,0,0,0,0,0,40,41};static short Cyc_yycheck[5736]={0,319,230,
153,316,59,101,166,167,168,318,131,321,0,114,23,154,322,323,164,154,21,203,131,
328,329,131,218,187,255,256,348,258,449,21,352,524,380,23,39,351,21,75,0,147,413,
46,140,89,62,330,331,52,53,54,96,56,75,87,46,101,17,62,63,21,52,53,54,76,56,89,
744,52,53,54,62,56,85,93,94,95,307,62,116,361,91,115,102,62,46,109,76,106,396,89,
52,53,54,116,56,85,114,5,6,118,62,106,107,108,109,478,454,61,126,114,128,46,69,
534,106,107,108,109,434,69,122,126,114,128,324,325,326,327,89,114,135,286,109,
626,126,114,128,142,109,447,290,126,292,128,106,107,108,109,456,128,142,39,114,
103,304,477,60,109,160,218,117,118,392,51,126,275,128,117,332,333,334,106,107,
108,109,102,284,236,88,192,142,859,368,369,370,139,199,375,418,89,94,118,89,389,
102,568,198,101,203,96,89,90,99,89,392,88,95,142,105,109,98,203,112,231,100,279,
412,114,115,116,106,899,94,94,110,105,231,208,209,94,205,102,65,66,67,107,238,
255,256,231,258,129,107,218,70,71,231,201,481,99,255,256,231,258,932,105,70,71,
413,109,938,89,112,255,256,103,258,101,96,231,255,256,105,258,89,113,281,105,111,
112,95,96,97,109,94,0,101,609,106,307,81,117,102,255,256,109,258,278,398,89,118,
282,307,117,105,89,96,22,23,104,25,95,96,97,89,307,111,112,272,89,95,105,307,478,
39,95,96,97,492,100,389,46,614,392,105,105,51,442,101,489,105,229,112,363,59,307,
101,112,361,64,105,100,101,412,356,103,638,553,73,418,103,76,110,368,369,370,91,
110,564,257,85,361,377,164,101,103,368,369,370,114,109,614,110,98,389,105,391,
695,123,124,242,106,107,108,109,103,248,402,462,404,118,89,110,100,122,99,197,
412,426,124,402,105,404,568,129,109,103,419,112,114,545,426,100,101,442,523,380,
142,123,124,103,556,147,106,426,545,718,442,104,459,402,426,404,556,103,89,556,
106,546,105,442,95,96,97,63,64,247,442,102,557,99,466,100,442,426,103,105,402,
100,404,109,463,705,112,466,630,106,192,542,622,442,591,830,622,199,718,103,100,
103,599,103,106,89,90,553,449,109,779,95,290,454,103,89,218,466,564,118,298,567,
96,122,302,525,304,229,528,100,103,105,103,106,236,100,105,104,103,317,114,115,
116,112,466,114,115,116,525,72,73,528,544,863,864,257,860,109,103,779,102,106,
862,151,865,153,154,102,560,866,867,101,105,161,162,353,872,873,166,167,168,570,
284,100,589,592,103,103,74,119,106,133,103,534,645,106,435,589,649,187,439,131,
874,875,105,103,103,825,106,106,589,662,457,103,105,607,106,589,100,928,65,103,
105,589,603,89,103,103,607,106,106,95,96,97,100,68,69,103,638,776,589,859,102,
103,422,423,424,100,426,100,103,697,103,103,595,806,106,105,607,361,438,638,105,
103,103,633,106,106,805,105,767,120,202,824,204,726,103,102,103,106,648,868,869,
870,871,607,737,389,230,100,392,100,103,661,103,540,89,119,286,786,402,121,404,
96,234,103,103,100,106,106,412,101,105,109,932,111,418,854,420,856,89,114,115,
116,569,876,877,878,316,105,318,319,320,321,322,323,324,325,326,327,328,329,330,
331,332,333,334,898,106,103,105,888,106,295,102,755,102,299,300,301,205,105,103,
103,466,106,106,101,310,89,102,103,314,218,102,95,96,97,755,114,115,116,17,18,19,
105,5,6,834,931,102,236,112,89,114,115,116,105,576,577,22,23,647,105,346,89,90,
91,105,351,368,369,370,105,659,102,103,120,121,100,825,72,73,525,100,413,528,68,
69,102,103,110,374,825,673,102,675,741,742,104,89,619,106,373,689,106,825,110,
118,694,102,553,99,825,859,113,398,102,117,389,117,75,564,105,102,567,396,859,
398,106,109,103,105,103,100,106,825,101,103,30,859,110,100,42,94,89,106,859,105,
830,478,118,434,106,103,89,103,103,676,677,103,679,102,681,492,607,89,106,747,
106,859,750,95,96,97,103,99,447,94,106,759,760,105,701,106,106,456,932,373,113,
105,114,115,116,106,106,638,101,100,481,932,106,100,117,389,113,102,117,101,101,
789,25,109,932,793,794,102,106,797,798,932,800,106,113,106,25,26,412,106,671,113,
749,106,106,752,753,106,106,38,106,568,113,524,932,101,100,831,89,100,94,106,90,
106,103,105,697,102,106,106,28,94,105,102,110,102,547,103,846,25,102,849,103,851,
852,74,545,558,100,105,38,110,74,74,553,802,106,556,805,119,89,90,117,92,93,622,
105,96,106,106,90,89,106,630,106,103,105,0,107,108,106,106,106,106,755,114,74,
895,835,118,0,834,121,122,552,100,125,126,234,666,229,89,90,614,92,93,567,854,96,
64,621,43,139,921,861,626,603,105,629,107,108,830,599,111,112,482,114,595,638,
199,320,755,879,121,59,- 1,- 1,- 1,- 1,553,76,77,78,79,80,81,82,83,84,85,564,- 1,- 1,
567,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,834,- 1,- 1,- 1,666,104,- 1,- 1,- 1,8,9,10,11,12,13,14,
15,16,17,18,19,20,21,- 1,- 1,- 1,25,26,- 1,- 1,- 1,705,- 1,695,- 1,- 1,- 1,- 1,37,38,- 1,40,
- 1,- 1,718,44,- 1,- 1,- 1,- 1,49,50,51,52,53,54,55,56,57,- 1,59,60,- 1,62,- 1,- 1,- 1,66,
67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,803,- 1,- 1,806,- 1,- 1,- 1,89,90,91,92,
93,94,95,96,97,98,99,- 1,101,- 1,824,779,105,- 1,107,108,- 1,- 1,- 1,- 1,- 1,114,- 1,- 1,-
1,118,- 1,- 1,121,122,786,- 1,125,126,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,860,- 1,
862,863,864,865,866,867,868,869,870,871,872,873,874,875,876,877,878,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,898,- 1,- 1,- 1,- 1,- 1,- 1,- 1,3,4,5,6,7,
8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,931,29,30,31,32,33,34,
35,36,37,38,39,40,41,- 1,- 1,44,45,46,- 1,- 1,49,50,51,52,53,54,55,56,57,58,59,- 1,61,
62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
89,90,91,92,93,94,95,96,97,98,99,100,101,- 1,- 1,- 1,105,- 1,107,108,- 1,- 1,- 1,- 1,- 1,
114,- 1,- 1,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
17,18,19,20,21,- 1,- 1,24,25,26,27,- 1,29,30,31,32,33,34,35,36,37,38,39,40,41,- 1,- 1,
44,45,46,- 1,- 1,49,50,51,52,53,54,55,56,57,58,59,- 1,61,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,94,95,96,97,
98,99,100,101,102,- 1,- 1,105,- 1,107,108,- 1,- 1,- 1,- 1,- 1,114,- 1,- 1,- 1,118,- 1,- 1,121,
122,- 1,- 1,125,126,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,25,
26,27,- 1,29,30,31,32,33,34,35,36,37,38,39,40,41,- 1,- 1,44,45,46,- 1,- 1,49,50,51,52,
53,54,55,56,57,58,59,- 1,61,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,94,95,96,97,98,99,100,101,- 1,- 1,- 1,105,
- 1,107,108,- 1,- 1,- 1,- 1,- 1,114,- 1,- 1,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,3,4,5,6,7,
8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,-
1,- 1,37,38,39,40,- 1,- 1,- 1,44,45,- 1,- 1,- 1,49,50,51,52,53,54,55,56,57,- 1,59,- 1,- 1,
62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
89,90,91,92,93,94,95,96,97,98,99,100,- 1,- 1,- 1,- 1,105,- 1,107,108,- 1,- 1,- 1,- 1,- 1,
114,- 1,- 1,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,8,9,10,11,12,13,14,15,16,17,18,19,
20,21,- 1,- 1,- 1,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,
- 1,- 1,49,50,51,52,53,54,55,56,57,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,94,95,96,97,98,99,- 1,
101,- 1,- 1,- 1,105,- 1,107,108,- 1,- 1,- 1,- 1,- 1,114,- 1,- 1,- 1,118,- 1,- 1,121,122,- 1,- 1,
125,126,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,25,26,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,37,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,49,50,51,52,53,54,55,56,57,- 1,59,
- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,89,90,91,92,93,94,95,96,97,98,99,- 1,- 1,- 1,- 1,- 1,105,- 1,107,108,- 1,- 1,- 1,- 1,
- 1,114,- 1,- 1,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,0,1,- 1,3,4,5,6,7,8,9,10,11,12,13,
14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,- 1,39,- 1,
- 1,- 1,43,- 1,45,89,47,48,49,50,- 1,95,96,97,55,56,57,89,- 1,- 1,- 1,105,- 1,95,96,97,
110,- 1,- 1,- 1,114,115,116,105,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,114,115,116,- 1,- 1,- 1,89,- 1,
- 1,- 1,- 1,94,95,96,97,98,99,- 1,- 1,102,- 1,- 1,105,- 1,107,108,- 1,- 1,- 1,- 1,- 1,114,115,
116,0,1,- 1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,-
1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,- 1,39,- 1,- 1,- 1,- 1,- 1,45,- 1,47,48,49,50,- 1,- 1,- 1,- 1,
55,56,57,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,- 1,- 1,- 1,- 1,94,95,96,97,98,99,- 1,- 1,102,- 1,- 1,105,- 1,107,
108,- 1,- 1,- 1,- 1,- 1,114,115,116,0,1,- 1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,
19,20,21,- 1,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,- 1,39,- 1,- 1,- 1,- 1,- 1,45,
- 1,47,48,49,50,- 1,- 1,- 1,- 1,55,56,57,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,- 1,- 1,- 1,- 1,94,95,96,97,98,99,
- 1,- 1,- 1,- 1,- 1,105,- 1,107,108,- 1,- 1,- 1,- 1,- 1,114,115,116,1,- 1,3,4,5,6,7,8,9,10,
11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,
- 1,39,- 1,- 1,- 1,- 1,- 1,45,- 1,47,48,49,50,- 1,- 1,- 1,- 1,55,56,57,- 1,- 1,- 1,- 1,8,9,10,
11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,- 1,37,
- 1,- 1,94,95,96,97,98,99,- 1,- 1,102,49,50,105,- 1,107,108,55,56,57,- 1,- 1,114,115,
116,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,-
1,89,- 1,- 1,- 1,- 1,94,95,96,97,98,99,100,- 1,- 1,- 1,- 1,105,- 1,107,108,- 1,- 1,- 1,112,-
1,114,115,116,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,37,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,49,50,- 1,- 1,- 1,- 1,55,56,57,
- 1,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,-
1,86,- 1,- 1,- 1,- 1,37,- 1,- 1,94,95,- 1,97,98,99,100,- 1,- 1,49,50,105,- 1,107,108,55,56,
57,112,- 1,114,115,116,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,-
1,- 1,- 1,- 1,- 1,- 1,- 1,89,- 1,- 1,- 1,- 1,94,95,96,97,98,99,- 1,- 1,- 1,- 1,- 1,105,- 1,107,
108,- 1,- 1,- 1,- 1,- 1,114,115,116,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,
- 1,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,- 1,39,- 1,- 1,- 1,- 1,- 1,45,- 1,- 1,- 1,
49,50,- 1,- 1,- 1,- 1,55,56,57,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,3,4,5,6,7,8,9,10,11,
12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,94,95,- 1,97,98,99,- 1,101,- 1,37,
104,39,- 1,107,108,- 1,- 1,45,- 1,- 1,- 1,49,50,- 1,- 1,- 1,- 1,55,56,57,3,4,5,6,7,8,9,10,
11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,
- 1,94,95,- 1,97,98,99,45,101,- 1,- 1,49,50,- 1,107,108,- 1,55,56,57,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,94,95,- 1,97,98,99,- 1,- 1,- 1,- 1,- 1,- 1,- 1,107,108,25,26,27,38,29,30,31,32,
33,34,35,36,- 1,38,- 1,40,41,- 1,- 1,44,- 1,46,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,58,59,
- 1,61,62,- 1,74,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,- 1,92,93,- 1,- 1,96,
- 1,- 1,89,90,91,92,93,- 1,105,96,107,108,- 1,100,101,- 1,- 1,114,105,- 1,- 1,108,- 1,- 1,
121,- 1,- 1,114,- 1,25,26,118,- 1,- 1,121,122,32,- 1,125,126,- 1,- 1,38,- 1,40,- 1,- 1,- 1,
44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,
- 1,- 1,- 1,101,102,- 1,- 1,105,- 1,- 1,108,- 1,- 1,111,112,- 1,114,25,26,- 1,118,- 1,- 1,121,
122,- 1,- 1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,
- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,- 1,101,102,- 1,- 1,105,- 1,- 1,108,- 1,-
1,111,112,- 1,114,25,26,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,
44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,
- 1,- 1,- 1,101,102,- 1,- 1,105,- 1,- 1,108,- 1,- 1,111,112,- 1,114,25,26,- 1,118,- 1,- 1,121,
122,- 1,- 1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,
- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,- 1,101,- 1,- 1,- 1,105,- 1,- 1,108,- 1,- 1,
111,112,- 1,114,25,26,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,44,
- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,105,- 1,- 1,108,- 1,- 1,- 1,- 1,113,114,25,26,- 1,118,- 1,- 1,121,122,-
1,- 1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,
59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,- 1,101,- 1,- 1,- 1,105,- 1,- 1,108,- 1,- 1,- 1,
- 1,- 1,114,25,26,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,
- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,
100,- 1,- 1,- 1,- 1,105,- 1,- 1,108,- 1,- 1,- 1,- 1,- 1,114,25,26,- 1,118,- 1,- 1,121,122,- 1,-
1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,-
1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,105,106,- 1,108,- 1,- 1,- 1,- 1,
- 1,114,25,26,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,
- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,105,- 1,- 1,108,- 1,- 1,- 1,- 1,113,114,25,26,- 1,118,- 1,- 1,121,122,- 1,- 1,125,
126,- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,
62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,105,106,- 1,108,- 1,- 1,- 1,- 1,- 1,
114,25,26,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,
- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,105,- 1,- 1,108,- 1,- 1,- 1,- 1,113,114,25,26,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,
- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,
- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,
91,92,93,- 1,- 1,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,105,- 1,- 1,108,- 1,- 1,- 1,- 1,113,114,25,
26,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,
51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,100,- 1,- 1,- 1,-
1,105,- 1,- 1,108,- 1,- 1,- 1,- 1,- 1,114,25,26,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,- 1,
38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,
- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,
92,93,- 1,- 1,96,- 1,- 1,- 1,100,- 1,- 1,- 1,- 1,105,- 1,- 1,108,- 1,- 1,- 1,- 1,- 1,114,25,26,-
1,118,- 1,- 1,121,122,- 1,- 1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,
52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
105,106,- 1,108,- 1,- 1,- 1,- 1,- 1,114,25,26,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,- 1,38,
- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,
66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,
93,- 1,- 1,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,105,106,- 1,108,- 1,- 1,- 1,- 1,- 1,114,25,26,- 1,
118,- 1,- 1,121,122,- 1,- 1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,
53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,100,- 1,- 1,- 1,- 1,105,
- 1,- 1,108,- 1,- 1,- 1,- 1,- 1,114,25,26,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,- 1,38,- 1,
40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,
67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,
- 1,- 1,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,105,106,- 1,108,- 1,- 1,- 1,- 1,- 1,114,25,26,- 1,118,
- 1,- 1,121,122,- 1,- 1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,
54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,105,106,
- 1,108,- 1,- 1,- 1,- 1,- 1,114,25,26,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,- 1,38,- 1,40,-
1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,
- 1,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,105,106,- 1,108,- 1,- 1,- 1,- 1,- 1,114,25,26,- 1,118,- 1,
- 1,121,122,- 1,- 1,125,126,- 1,38,- 1,40,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,-
1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,105,106,- 1,
108,- 1,- 1,- 1,- 1,- 1,114,25,26,- 1,118,- 1,- 1,121,122,- 1,- 1,125,126,- 1,38,- 1,40,- 1,-
1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,51,52,53,54,- 1,- 1,- 1,- 1,59,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,
- 1,- 1,- 1,- 1,- 1,25,26,- 1,38,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,38,- 1,89,90,91,92,93,- 1,- 1,
96,- 1,- 1,- 1,51,52,53,54,- 1,105,- 1,- 1,108,- 1,- 1,62,- 1,74,114,66,67,- 1,118,- 1,- 1,
121,122,- 1,- 1,125,126,- 1,89,90,- 1,92,93,- 1,- 1,96,- 1,- 1,89,90,91,92,93,- 1,105,96,
107,108,- 1,- 1,101,25,26,114,105,- 1,- 1,108,- 1,- 1,121,- 1,- 1,114,38,- 1,- 1,118,- 1,- 1,
121,122,- 1,- 1,125,126,- 1,51,52,53,54,- 1,- 1,- 1,- 1,- 1,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,-
1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,
96,- 1,- 1,- 1,- 1,101,25,26,- 1,105,- 1,- 1,108,- 1,- 1,- 1,- 1,- 1,114,38,- 1,- 1,118,- 1,- 1,
121,122,- 1,- 1,125,126,- 1,51,52,53,54,- 1,- 1,- 1,- 1,- 1,- 1,- 1,62,- 1,- 1,- 1,66,67,- 1,-
1,- 1,- 1,- 1,- 1,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,38,- 1,89,90,91,92,93,- 1,- 1,
96,- 1,- 1,- 1,51,52,53,54,- 1,105,- 1,- 1,108,- 1,- 1,62,- 1,- 1,114,66,67,- 1,118,- 1,- 1,
121,122,25,26,125,126,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,38,- 1,89,90,91,92,93,- 1,- 1,96,-
1,- 1,- 1,51,52,53,54,- 1,105,- 1,- 1,108,- 1,- 1,62,- 1,- 1,114,66,67,- 1,118,- 1,- 1,121,
122,25,26,125,126,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,38,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,
- 1,51,52,53,54,- 1,105,- 1,- 1,108,- 1,- 1,62,- 1,- 1,114,66,67,- 1,118,- 1,- 1,121,122,- 1,
- 1,125,126,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,90,91,92,93,- 1,- 1,96,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,105,- 1,- 1,108,- 1,- 1,- 1,- 1,- 1,114,- 1,- 1,- 1,118,- 1,- 1,121,122,- 1,- 1,
125,126,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,8,9,10,11,12,13,14,15,16,17,
18,19,20,21,37,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,49,50,- 1,37,- 1,- 1,55,56,57,- 1,- 1,
- 1,- 1,- 1,- 1,49,50,- 1,- 1,- 1,- 1,55,56,57,- 1,- 1,60,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,
- 1,- 1,89,- 1,- 1,- 1,- 1,94,95,- 1,97,98,99,100,- 1,- 1,- 1,- 1,- 1,- 1,107,108,94,95,- 1,97,
98,99,- 1,101,- 1,- 1,- 1,- 1,- 1,107,108,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,8,
9,10,11,12,13,14,15,16,17,18,19,20,21,37,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,49,50,
- 1,37,- 1,- 1,55,56,57,- 1,- 1,- 1,- 1,- 1,- 1,49,50,- 1,- 1,- 1,- 1,55,56,57,- 1,- 1,- 1,- 1,- 1,
- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,- 1,- 1,- 1,- 1,- 1,94,95,- 1,97,98,99,100,86,- 1,- 1,-
1,- 1,- 1,107,108,94,95,- 1,97,98,99,- 1,- 1,- 1,- 1,- 1,- 1,- 1,107,108,8,9,10,11,12,13,
14,15,16,17,18,19,20,21,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,37,- 1,- 1,- 1,-
1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,49,50,- 1,37,- 1,- 1,55,56,57,- 1,- 1,- 1,- 1,- 1,- 1,49,50,- 1,- 1,
- 1,- 1,55,56,57,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,94,
95,- 1,97,98,99,- 1,- 1,- 1,- 1,- 1,- 1,- 1,107,108,94,- 1,- 1,- 1,98,99,- 1,- 1,- 1,- 1,- 1,- 1,
- 1,107,108};char Cyc_Yyimpossible[17]="\000\000\000\000Yyimpossible\000";char Cyc_Yystack_overflow[
21]="\000\000\000\000Yystack_overflow\000";void Cyc_yyerror(struct _tagged_arr);
int Cyc_yylex();static int Cyc_yychar=(int)'\000';void*Cyc_yylval=(void*)Cyc_YYINITIALSVAL;
static int Cyc_yynerrs=0;struct _tuple17{struct Cyc_Position_Segment*f1;struct
_tuple1*f2;int f3;};struct _tuple18{struct _tagged_arr f1;void*f2;};static char
_tmp423[8]="stdcall";static char _tmp424[6]="cdecl";static char _tmp425[9]="fastcall";
static char _tmp426[9]="noreturn";static char _tmp427[6]="const";static char _tmp428[8]="aligned";
static char _tmp429[7]="packed";static char _tmp42A[7]="shared";static char _tmp42B[7]="unused";
static char _tmp42C[5]="weak";static char _tmp42D[10]="dllimport";static char _tmp42E[
10]="dllexport";static char _tmp42F[23]="no_instrument_function";static char _tmp430[
12]="constructor";static char _tmp431[11]="destructor";static char _tmp432[22]="no_check_memory_usage";
static char _tmp433[5]="pure";struct _tuple19{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*
f2;};int Cyc_yyparse(){int yystate;int yyn=0;int yyerrstatus;int yychar1=0;int
yyssp_offset;short*yyss=(short*)({unsigned int _tmp9E7=10000;short*_tmp9E8=(short*)
_cycalloc_atomic(_check_times(sizeof(short),_tmp9E7));{unsigned int _tmp9E9=
_tmp9E7;unsigned int i;for(i=0;i < _tmp9E9;i ++){_tmp9E8[i]=(short)0;}}_tmp9E8;});
int yyvsp_offset;void**yyvs=(void**)({unsigned int _tmp9E4=10000;void**_tmp9E5=(
void**)_cycalloc(_check_times(sizeof(void*),_tmp9E4));{unsigned int _tmp9E6=
_tmp9E4;unsigned int i;for(i=0;i < _tmp9E6;i ++){_tmp9E5[i]=Cyc_yylval;}}_tmp9E5;});
int yylsp_offset;struct Cyc_Yyltype*yyls=(struct Cyc_Yyltype*)({unsigned int _tmp9E1=
10000;struct Cyc_Yyltype*_tmp9E2=(struct Cyc_Yyltype*)_cycalloc_atomic(
_check_times(sizeof(struct Cyc_Yyltype),_tmp9E1));{unsigned int _tmp9E3=_tmp9E1;
unsigned int i;for(i=0;i < _tmp9E3;i ++){_tmp9E2[i]=Cyc_yynewloc();}}_tmp9E2;});int
yystacksize=10000;void*yyval=Cyc_yylval;int yylen;yystate=0;yyerrstatus=0;Cyc_yynerrs=
0;Cyc_yychar=- 2;yyssp_offset=- 1;yyvsp_offset=0;yylsp_offset=0;yynewstate: yyss[
_check_known_subscript_notnull(10000,++ yyssp_offset)]=(short)yystate;if(
yyssp_offset >= yystacksize - 1){if(yystacksize >= 10000){Cyc_yyerror(({const char*
_tmp35C="parser stack overflow";_tag_arr(_tmp35C,sizeof(char),_get_zero_arr_size(
_tmp35C,22));}));(int)_throw((void*)Cyc_Yystack_overflow);}}goto yybackup;
yybackup: yyn=(int)Cyc_yypact[_check_known_subscript_notnull(947,yystate)];if(yyn
== - 32768)goto yydefault;if(Cyc_yychar == - 2)Cyc_yychar=Cyc_yylex();if(Cyc_yychar
<= 0){yychar1=0;Cyc_yychar=0;}else{yychar1=Cyc_yychar > 0  && Cyc_yychar <= 354?(
int)Cyc_yytranslate[_check_known_subscript_notnull(355,Cyc_yychar)]: 264;}yyn +=
yychar1;if((yyn < 0  || yyn > 5735) || Cyc_yycheck[_check_known_subscript_notnull(
5736,yyn)]!= yychar1)goto yydefault;yyn=(int)Cyc_yytable[
_check_known_subscript_notnull(5736,yyn)];if(yyn < 0){if(yyn == - 32768)goto
yyerrlab;yyn=- yyn;goto yyreduce;}else{if(yyn == 0)goto yyerrlab;}if(yyn == 946)
return 0;if(Cyc_yychar != 0)Cyc_yychar=- 2;yyvs[_check_known_subscript_notnull(
10000,++ yyvsp_offset)]=Cyc_yylval;yyls[_check_known_subscript_notnull(10000,++
yylsp_offset)]=Cyc_yylloc;if(yyerrstatus != 0)yyerrstatus --;yystate=yyn;goto
yynewstate;yydefault: yyn=(int)Cyc_yydefact[_check_known_subscript_notnull(947,
yystate)];if(yyn == 0)goto yyerrlab;yyreduce: yylen=(int)Cyc_yyr2[
_check_known_subscript_notnull(477,yyn)];if(yylen > 0)yyval=yyvs[
_check_known_subscript_notnull(10000,(yyvsp_offset + 1)- yylen)];switch(yyn){case
1: _LL1D7: yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];Cyc_Parse_parse_result=
Cyc_yyget_DeclList_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);
break;case 2: _LL1D8: {struct Cyc_List_List*x;struct Cyc_List_List*y;{struct
_handler_cons _tmp35D;_push_handler(& _tmp35D);{int _tmp35F=0;if(setjmp(_tmp35D.handler))
_tmp35F=1;if(!_tmp35F){x=Cyc_yyget_DeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);;_pop_handler();}else{
void*_tmp35E=(void*)_exn_thrown;void*_tmp361=_tmp35E;_LL1DB: if(*((void**)_tmp361)
!= Cyc_Core_Failure)goto _LL1DD;_LL1DC: x=0;goto _LL1DA;_LL1DD:;_LL1DE:(void)_throw(
_tmp361);_LL1DA:;}}}{struct _handler_cons _tmp362;_push_handler(& _tmp362);{int
_tmp364=0;if(setjmp(_tmp362.handler))_tmp364=1;if(!_tmp364){y=Cyc_yyget_DeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);;_pop_handler();}else{
void*_tmp363=(void*)_exn_thrown;void*_tmp366=_tmp363;_LL1E0: if(*((void**)_tmp366)
!= Cyc_Core_Failure)goto _LL1E2;_LL1E1: y=0;goto _LL1DF;_LL1E2:;_LL1E3:(void)_throw(
_tmp366);_LL1DF:;}}}yyval=(void*)({struct Cyc_DeclList_tok_struct*_tmp367=
_cycalloc(sizeof(*_tmp367));_tmp367[0]=({struct Cyc_DeclList_tok_struct _tmp368;
_tmp368.tag=Cyc_DeclList_tok;_tmp368.f1=((struct Cyc_List_List*(*)(struct Cyc_List_List*
x,struct Cyc_List_List*y))Cyc_List_imp_append)(x,y);_tmp368;});_tmp367;});break;}
case 3: _LL1D9: yyval=(void*)({struct Cyc_DeclList_tok_struct*_tmp369=_cycalloc(
sizeof(*_tmp369));_tmp369[0]=({struct Cyc_DeclList_tok_struct _tmp36A;_tmp36A.tag=
Cyc_DeclList_tok;_tmp36A.f1=({struct Cyc_List_List*_tmp36B=_cycalloc(sizeof(*
_tmp36B));_tmp36B->hd=({struct Cyc_Absyn_Decl*_tmp36C=_cycalloc(sizeof(*_tmp36C));
_tmp36C->r=(void*)((void*)({struct Cyc_Absyn_Using_d_struct*_tmp36D=_cycalloc(
sizeof(*_tmp36D));_tmp36D[0]=({struct Cyc_Absyn_Using_d_struct _tmp36E;_tmp36E.tag=
9;_tmp36E.f1=Cyc_yyget_QualId_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 2)]);_tmp36E.f2=Cyc_yyget_DeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp36E;});_tmp36D;}));
_tmp36C->loc=Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(
10000,yylsp_offset - 2)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line);_tmp36C;});_tmp36B->tl=0;_tmp36B;});_tmp36A;});
_tmp369;});Cyc_Lex_leave_using();break;case 4: _LL1E4: yyval=(void*)({struct Cyc_DeclList_tok_struct*
_tmp36F=_cycalloc(sizeof(*_tmp36F));_tmp36F[0]=({struct Cyc_DeclList_tok_struct
_tmp370;_tmp370.tag=Cyc_DeclList_tok;_tmp370.f1=({struct Cyc_List_List*_tmp371=
_cycalloc(sizeof(*_tmp371));_tmp371->hd=({struct Cyc_Absyn_Decl*_tmp372=_cycalloc(
sizeof(*_tmp372));_tmp372->r=(void*)((void*)({struct Cyc_Absyn_Using_d_struct*
_tmp373=_cycalloc(sizeof(*_tmp373));_tmp373[0]=({struct Cyc_Absyn_Using_d_struct
_tmp374;_tmp374.tag=9;_tmp374.f1=Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]);_tmp374.f2=Cyc_yyget_DeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp374;});_tmp373;}));
_tmp372->loc=Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(
10000,yylsp_offset - 4)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).last_line);_tmp372;});_tmp371->tl=Cyc_yyget_DeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp371;});_tmp370;});
_tmp36F;});break;case 5: _LL1E5: yyval=(void*)({struct Cyc_DeclList_tok_struct*
_tmp375=_cycalloc(sizeof(*_tmp375));_tmp375[0]=({struct Cyc_DeclList_tok_struct
_tmp376;_tmp376.tag=Cyc_DeclList_tok;_tmp376.f1=({struct Cyc_List_List*_tmp377=
_cycalloc(sizeof(*_tmp377));_tmp377->hd=({struct Cyc_Absyn_Decl*_tmp378=_cycalloc(
sizeof(*_tmp378));_tmp378->r=(void*)((void*)({struct Cyc_Absyn_Namespace_d_struct*
_tmp379=_cycalloc(sizeof(*_tmp379));_tmp379[0]=({struct Cyc_Absyn_Namespace_d_struct
_tmp37A;_tmp37A.tag=8;_tmp37A.f1=({struct _tagged_arr*_tmp37B=_cycalloc(sizeof(*
_tmp37B));_tmp37B[0]=Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 2)]);_tmp37B;});_tmp37A.f2=Cyc_yyget_DeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp37A;});_tmp379;}));
_tmp378->loc=Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(
10000,yylsp_offset - 2)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line);_tmp378;});_tmp377->tl=0;_tmp377;});_tmp376;});
_tmp375;});Cyc_Lex_leave_namespace();break;case 6: _LL1E6: {struct _tagged_arr
nspace;struct Cyc_List_List*x;struct Cyc_List_List*y;{struct _handler_cons _tmp37C;
_push_handler(& _tmp37C);{int _tmp37E=0;if(setjmp(_tmp37C.handler))_tmp37E=1;if(!
_tmp37E){nspace=Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 4)]);;_pop_handler();}else{void*_tmp37D=(void*)_exn_thrown;void*
_tmp380=_tmp37D;_LL1E9: if(*((void**)_tmp380)!= Cyc_Core_Failure)goto _LL1EB;
_LL1EA: nspace=({const char*_tmp381="";_tag_arr(_tmp381,sizeof(char),
_get_zero_arr_size(_tmp381,1));});goto _LL1E8;_LL1EB:;_LL1EC:(void)_throw(_tmp380);
_LL1E8:;}}}{struct _handler_cons _tmp382;_push_handler(& _tmp382);{int _tmp384=0;if(
setjmp(_tmp382.handler))_tmp384=1;if(!_tmp384){x=Cyc_yyget_DeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);;_pop_handler();}else{
void*_tmp383=(void*)_exn_thrown;void*_tmp386=_tmp383;_LL1EE: if(*((void**)_tmp386)
!= Cyc_Core_Failure)goto _LL1F0;_LL1EF: x=0;goto _LL1ED;_LL1F0:;_LL1F1:(void)_throw(
_tmp386);_LL1ED:;}}}{struct _handler_cons _tmp387;_push_handler(& _tmp387);{int
_tmp389=0;if(setjmp(_tmp387.handler))_tmp389=1;if(!_tmp389){y=Cyc_yyget_DeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);;_pop_handler();}else{
void*_tmp388=(void*)_exn_thrown;void*_tmp38B=_tmp388;_LL1F3: if(*((void**)_tmp38B)
!= Cyc_Core_Failure)goto _LL1F5;_LL1F4: y=0;goto _LL1F2;_LL1F5:;_LL1F6:(void)_throw(
_tmp38B);_LL1F2:;}}}yyval=(void*)({struct Cyc_DeclList_tok_struct*_tmp38C=
_cycalloc(sizeof(*_tmp38C));_tmp38C[0]=({struct Cyc_DeclList_tok_struct _tmp38D;
_tmp38D.tag=Cyc_DeclList_tok;_tmp38D.f1=({struct Cyc_List_List*_tmp38E=_cycalloc(
sizeof(*_tmp38E));_tmp38E->hd=({struct Cyc_Absyn_Decl*_tmp38F=_cycalloc(sizeof(*
_tmp38F));_tmp38F->r=(void*)((void*)({struct Cyc_Absyn_Namespace_d_struct*_tmp390=
_cycalloc(sizeof(*_tmp390));_tmp390[0]=({struct Cyc_Absyn_Namespace_d_struct
_tmp391;_tmp391.tag=8;_tmp391.f1=({struct _tagged_arr*_tmp392=_cycalloc(sizeof(*
_tmp392));_tmp392[0]=nspace;_tmp392;});_tmp391.f2=x;_tmp391;});_tmp390;}));
_tmp38F->loc=Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(
10000,yylsp_offset - 4)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).last_line);_tmp38F;});_tmp38E->tl=y;_tmp38E;});_tmp38D;});
_tmp38C;});break;}case 7: _LL1E7: if(Cyc_strcmp((struct _tagged_arr)Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 4)]),({const char*_tmp393="C";
_tag_arr(_tmp393,sizeof(char),_get_zero_arr_size(_tmp393,2));}))== 0)yyval=(void*)({
struct Cyc_DeclList_tok_struct*_tmp394=_cycalloc(sizeof(*_tmp394));_tmp394[0]=({
struct Cyc_DeclList_tok_struct _tmp395;_tmp395.tag=Cyc_DeclList_tok;_tmp395.f1=({
struct Cyc_List_List*_tmp396=_cycalloc(sizeof(*_tmp396));_tmp396->hd=({struct Cyc_Absyn_Decl*
_tmp397=_cycalloc(sizeof(*_tmp397));_tmp397->r=(void*)((void*)({struct Cyc_Absyn_ExternC_d_struct*
_tmp398=_cycalloc(sizeof(*_tmp398));_tmp398[0]=({struct Cyc_Absyn_ExternC_d_struct
_tmp399;_tmp399.tag=10;_tmp399.f1=Cyc_yyget_DeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp399;});_tmp398;}));
_tmp397->loc=Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(
10000,yylsp_offset - 5)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 2)]).last_line);_tmp397;});_tmp396->tl=Cyc_yyget_DeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp396;});_tmp395;});
_tmp394;});else{if(Cyc_strcmp((struct _tagged_arr)Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]),({const char*_tmp39A="C include";
_tag_arr(_tmp39A,sizeof(char),_get_zero_arr_size(_tmp39A,10));}))!= 0)Cyc_Parse_err(({
const char*_tmp39B="expecting \"C\" or \"C include\"";_tag_arr(_tmp39B,sizeof(
char),_get_zero_arr_size(_tmp39B,29));}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 5)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 4)]).last_line));yyval=(void*)({
struct Cyc_DeclList_tok_struct*_tmp39C=_cycalloc(sizeof(*_tmp39C));_tmp39C[0]=({
struct Cyc_DeclList_tok_struct _tmp39D;_tmp39D.tag=Cyc_DeclList_tok;_tmp39D.f1=({
struct Cyc_List_List*_tmp39E=_cycalloc(sizeof(*_tmp39E));_tmp39E->hd=({struct Cyc_Absyn_Decl*
_tmp39F=_cycalloc(sizeof(*_tmp39F));_tmp39F->r=(void*)((void*)({struct Cyc_Absyn_ExternCinclude_d_struct*
_tmp3A0=_cycalloc(sizeof(*_tmp3A0));_tmp3A0[0]=({struct Cyc_Absyn_ExternCinclude_d_struct
_tmp3A1;_tmp3A1.tag=11;_tmp3A1.f1=Cyc_yyget_DeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp3A1.f2=0;_tmp3A1;});
_tmp3A0;}));_tmp39F->loc=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 5)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line);_tmp39F;});
_tmp39E->tl=Cyc_yyget_DeclList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]);_tmp39E;});_tmp39D;});_tmp39C;});}break;case 8: _LL1F7: if(Cyc_strcmp((
struct _tagged_arr)Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 5)]),({const char*_tmp3A2="C include";_tag_arr(_tmp3A2,sizeof(char),
_get_zero_arr_size(_tmp3A2,10));}))!= 0)Cyc_Parse_err(({const char*_tmp3A3="expecting \"C include\"";
_tag_arr(_tmp3A3,sizeof(char),_get_zero_arr_size(_tmp3A3,22));}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 6)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 5)]).last_line));{struct Cyc_List_List*
exs=Cyc_yyget_YY5(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);
yyval=(void*)({struct Cyc_DeclList_tok_struct*_tmp3A4=_cycalloc(sizeof(*_tmp3A4));
_tmp3A4[0]=({struct Cyc_DeclList_tok_struct _tmp3A5;_tmp3A5.tag=Cyc_DeclList_tok;
_tmp3A5.f1=({struct Cyc_List_List*_tmp3A6=_cycalloc(sizeof(*_tmp3A6));_tmp3A6->hd=({
struct Cyc_Absyn_Decl*_tmp3A7=_cycalloc(sizeof(*_tmp3A7));_tmp3A7->r=(void*)((
void*)({struct Cyc_Absyn_ExternCinclude_d_struct*_tmp3A8=_cycalloc(sizeof(*
_tmp3A8));_tmp3A8[0]=({struct Cyc_Absyn_ExternCinclude_d_struct _tmp3A9;_tmp3A9.tag=
11;_tmp3A9.f1=Cyc_yyget_DeclList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 3)]);_tmp3A9.f2=exs;_tmp3A9;});_tmp3A8;}));_tmp3A7->loc=Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 6)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line);_tmp3A7;});
_tmp3A6->tl=Cyc_yyget_DeclList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]);_tmp3A6;});_tmp3A5;});_tmp3A4;});break;}case 9: _LL1F8: yyval=(void*)({
struct Cyc_DeclList_tok_struct*_tmp3AA=_cycalloc(sizeof(*_tmp3AA));_tmp3AA[0]=({
struct Cyc_DeclList_tok_struct _tmp3AB;_tmp3AB.tag=Cyc_DeclList_tok;_tmp3AB.f1=0;
_tmp3AB;});_tmp3AA;});break;case 10: _LL1F9: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)];break;case 11: _LL1FA: yyval=(
void*)({struct Cyc_YY5_struct*_tmp3AC=_cycalloc(sizeof(*_tmp3AC));_tmp3AC[0]=({
struct Cyc_YY5_struct _tmp3AD;_tmp3AD.tag=Cyc_YY5;_tmp3AD.f1=0;_tmp3AD;});_tmp3AC;});
break;case 12: _LL1FB: yyval=(void*)({struct Cyc_YY5_struct*_tmp3AE=_cycalloc(
sizeof(*_tmp3AE));_tmp3AE[0]=({struct Cyc_YY5_struct _tmp3AF;_tmp3AF.tag=Cyc_YY5;
_tmp3AF.f1=({struct Cyc_List_List*_tmp3B0=_cycalloc(sizeof(*_tmp3B0));_tmp3B0->hd=({
struct _tuple17*_tmp3B1=_cycalloc(sizeof(*_tmp3B1));_tmp3B1->f1=Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line);_tmp3B1->f2=
Cyc_yyget_QualId_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);
_tmp3B1->f3=0;_tmp3B1;});_tmp3B0->tl=0;_tmp3B0;});_tmp3AF;});_tmp3AE;});break;
case 13: _LL1FC: yyval=(void*)({struct Cyc_YY5_struct*_tmp3B2=_cycalloc(sizeof(*
_tmp3B2));_tmp3B2[0]=({struct Cyc_YY5_struct _tmp3B3;_tmp3B3.tag=Cyc_YY5;_tmp3B3.f1=({
struct Cyc_List_List*_tmp3B4=_cycalloc(sizeof(*_tmp3B4));_tmp3B4->hd=({struct
_tuple17*_tmp3B5=_cycalloc(sizeof(*_tmp3B5));_tmp3B5->f1=Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).last_line);_tmp3B5->f2=
Cyc_yyget_QualId_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);
_tmp3B5->f3=0;_tmp3B5;});_tmp3B4->tl=Cyc_yyget_YY5(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp3B4;});_tmp3B3;});
_tmp3B2;});break;case 14: _LL1FD: yyval=(void*)({struct Cyc_DeclList_tok_struct*
_tmp3B6=_cycalloc(sizeof(*_tmp3B6));_tmp3B6[0]=({struct Cyc_DeclList_tok_struct
_tmp3B7;_tmp3B7.tag=Cyc_DeclList_tok;_tmp3B7.f1=({struct Cyc_List_List*_tmp3B8=
_cycalloc(sizeof(*_tmp3B8));_tmp3B8->hd=Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Fn_d_struct*
_tmp3B9=_cycalloc(sizeof(*_tmp3B9));_tmp3B9[0]=({struct Cyc_Absyn_Fn_d_struct
_tmp3BA;_tmp3BA.tag=1;_tmp3BA.f1=Cyc_yyget_FnDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp3BA;});_tmp3B9;}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp3B8->tl=0;
_tmp3B8;});_tmp3B7;});_tmp3B6;});break;case 15: _LL1FE: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 16: _LL1FF: yyval=(
void*)({struct Cyc_DeclList_tok_struct*_tmp3BB=_cycalloc(sizeof(*_tmp3BB));
_tmp3BB[0]=({struct Cyc_DeclList_tok_struct _tmp3BC;_tmp3BC.tag=Cyc_DeclList_tok;
_tmp3BC.f1=0;_tmp3BC;});_tmp3BB;});break;case 17: _LL200: yyval=(void*)({struct Cyc_FnDecl_tok_struct*
_tmp3BD=_cycalloc(sizeof(*_tmp3BD));_tmp3BD[0]=({struct Cyc_FnDecl_tok_struct
_tmp3BE;_tmp3BE.tag=Cyc_FnDecl_tok;_tmp3BE.f1=Cyc_Parse_make_function(0,Cyc_yyget_Declarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),0,Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp3BE;});
_tmp3BD;});break;case 18: _LL201: yyval=(void*)({struct Cyc_FnDecl_tok_struct*
_tmp3BF=_cycalloc(sizeof(*_tmp3BF));_tmp3BF[0]=({struct Cyc_FnDecl_tok_struct
_tmp3C0;_tmp3C0.tag=Cyc_FnDecl_tok;_tmp3C0.f1=Cyc_Parse_make_function(({struct
Cyc_Core_Opt*_tmp3C1=_cycalloc(sizeof(*_tmp3C1));_tmp3C1->v=Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp3C1;}),Cyc_yyget_Declarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),0,Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp3C0;});
_tmp3BF;});break;case 19: _LL202: yyval=(void*)({struct Cyc_FnDecl_tok_struct*
_tmp3C2=_cycalloc(sizeof(*_tmp3C2));_tmp3C2[0]=({struct Cyc_FnDecl_tok_struct
_tmp3C3;_tmp3C3.tag=Cyc_FnDecl_tok;_tmp3C3.f1=Cyc_Parse_make_function(0,Cyc_yyget_Declarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_DeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp3C3;});
_tmp3C2;});break;case 20: _LL203: yyval=(void*)({struct Cyc_FnDecl_tok_struct*
_tmp3C4=_cycalloc(sizeof(*_tmp3C4));_tmp3C4[0]=({struct Cyc_FnDecl_tok_struct
_tmp3C5;_tmp3C5.tag=Cyc_FnDecl_tok;_tmp3C5.f1=Cyc_Parse_make_function(({struct
Cyc_Core_Opt*_tmp3C6=_cycalloc(sizeof(*_tmp3C6));_tmp3C6->v=Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp3C6;}),Cyc_yyget_Declarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_DeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp3C5;});
_tmp3C4;});break;case 21: _LL204: yyval=(void*)({struct Cyc_FnDecl_tok_struct*
_tmp3C7=_cycalloc(sizeof(*_tmp3C7));_tmp3C7[0]=({struct Cyc_FnDecl_tok_struct
_tmp3C8;_tmp3C8.tag=Cyc_FnDecl_tok;_tmp3C8.f1=Cyc_Parse_make_function(({struct
Cyc_Core_Opt*_tmp3C9=_cycalloc(sizeof(*_tmp3C9));_tmp3C9->v=Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp3C9;}),Cyc_yyget_Declarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),0,Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp3C8;});
_tmp3C7;});break;case 22: _LL205: yyval=(void*)({struct Cyc_FnDecl_tok_struct*
_tmp3CA=_cycalloc(sizeof(*_tmp3CA));_tmp3CA[0]=({struct Cyc_FnDecl_tok_struct
_tmp3CB;_tmp3CB.tag=Cyc_FnDecl_tok;_tmp3CB.f1=Cyc_Parse_make_function(({struct
Cyc_Core_Opt*_tmp3CC=_cycalloc(sizeof(*_tmp3CC));_tmp3CC->v=Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp3CC;}),Cyc_yyget_Declarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_DeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp3CB;});
_tmp3CA;});break;case 23: _LL206: Cyc_Lex_enter_using(Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]));yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 24: _LL207: Cyc_Lex_leave_using();
break;case 25: _LL208: Cyc_Lex_enter_namespace(({struct _tagged_arr*_tmp3CD=
_cycalloc(sizeof(*_tmp3CD));_tmp3CD[0]=Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp3CD;}));yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 26: _LL209: Cyc_Lex_leave_namespace();
break;case 27: _LL20A: yyval=(void*)({struct Cyc_DeclList_tok_struct*_tmp3CE=
_cycalloc(sizeof(*_tmp3CE));_tmp3CE[0]=({struct Cyc_DeclList_tok_struct _tmp3CF;
_tmp3CF.tag=Cyc_DeclList_tok;_tmp3CF.f1=Cyc_Parse_make_declarations(Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),0,Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line));_tmp3CF;});
_tmp3CE;});break;case 28: _LL20B: yyval=(void*)({struct Cyc_DeclList_tok_struct*
_tmp3D0=_cycalloc(sizeof(*_tmp3D0));_tmp3D0[0]=({struct Cyc_DeclList_tok_struct
_tmp3D1;_tmp3D1.tag=Cyc_DeclList_tok;_tmp3D1.f1=Cyc_Parse_make_declarations(Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_InitDeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp3D1;});
_tmp3D0;});break;case 29: _LL20C: yyval=(void*)({struct Cyc_DeclList_tok_struct*
_tmp3D2=_cycalloc(sizeof(*_tmp3D2));_tmp3D2[0]=({struct Cyc_DeclList_tok_struct
_tmp3D3;_tmp3D3.tag=Cyc_DeclList_tok;_tmp3D3.f1=({struct Cyc_List_List*_tmp3D4=
_cycalloc(sizeof(*_tmp3D4));_tmp3D4->hd=Cyc_Absyn_let_decl(Cyc_yyget_Pattern_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp3D4->tl=0;
_tmp3D4;});_tmp3D3;});_tmp3D2;});break;case 30: _LL20D: {struct Cyc_List_List*
_tmp3D5=0;{struct Cyc_List_List*_tmp3D6=Cyc_yyget_IdList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);for(0;_tmp3D6 != 0;
_tmp3D6=_tmp3D6->tl){struct _tagged_arr*_tmp3D7=(struct _tagged_arr*)_tmp3D6->hd;
struct _tuple1*_tmp3D8=({struct _tuple1*_tmp3DB=_cycalloc(sizeof(*_tmp3DB));
_tmp3DB->f1=Cyc_Absyn_rel_ns_null;_tmp3DB->f2=_tmp3D7;_tmp3DB;});struct Cyc_Absyn_Vardecl*
_tmp3D9=Cyc_Absyn_new_vardecl(_tmp3D8,Cyc_Absyn_wildtyp(0),0);_tmp3D5=({struct
Cyc_List_List*_tmp3DA=_cycalloc(sizeof(*_tmp3DA));_tmp3DA->hd=_tmp3D9;_tmp3DA->tl=
_tmp3D5;_tmp3DA;});}}_tmp3D5=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))
Cyc_List_imp_rev)(_tmp3D5);yyval=(void*)({struct Cyc_DeclList_tok_struct*_tmp3DC=
_cycalloc(sizeof(*_tmp3DC));_tmp3DC[0]=({struct Cyc_DeclList_tok_struct _tmp3DD;
_tmp3DD.tag=Cyc_DeclList_tok;_tmp3DD.f1=({struct Cyc_List_List*_tmp3DE=_cycalloc(
sizeof(*_tmp3DE));_tmp3DE->hd=Cyc_Absyn_letv_decl(_tmp3D5,Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp3DE->tl=0;
_tmp3DE;});_tmp3DD;});_tmp3DC;});break;}case 31: _LL20E: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 32: _LL20F: yyval=(
void*)({struct Cyc_DeclList_tok_struct*_tmp3DF=_cycalloc(sizeof(*_tmp3DF));
_tmp3DF[0]=({struct Cyc_DeclList_tok_struct _tmp3E0;_tmp3E0.tag=Cyc_DeclList_tok;
_tmp3E0.f1=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))
Cyc_List_imp_append)(Cyc_yyget_DeclList_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 1)]),Cyc_yyget_DeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]));_tmp3E0;});_tmp3DF;});
break;case 33: _LL210: yyval=(void*)({struct Cyc_DeclSpec_tok_struct*_tmp3E1=
_cycalloc(sizeof(*_tmp3E1));_tmp3E1[0]=({struct Cyc_DeclSpec_tok_struct _tmp3E2;
_tmp3E2.tag=Cyc_DeclSpec_tok;_tmp3E2.f1=({struct Cyc_Parse_Declaration_spec*
_tmp3E3=_cycalloc(sizeof(*_tmp3E3));_tmp3E3->sc=({struct Cyc_Core_Opt*_tmp3E4=
_cycalloc(sizeof(*_tmp3E4));_tmp3E4->v=(void*)Cyc_yyget_StorageClass_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp3E4;});_tmp3E3->tq=Cyc_Absyn_empty_tqual();
_tmp3E3->type_specs=0;_tmp3E3->is_inline=0;_tmp3E3->attributes=0;_tmp3E3;});
_tmp3E2;});_tmp3E1;});break;case 34: _LL211: if((Cyc_yyget_DeclSpec_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]))->sc != 0)({void*_tmp3E5[0]={};
Cyc_Tcutil_warn(Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(
10000,yylsp_offset - 1)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line),({const char*_tmp3E6="Only one storage class is allowed in a declaration";
_tag_arr(_tmp3E6,sizeof(char),_get_zero_arr_size(_tmp3E6,51));}),_tag_arr(
_tmp3E5,sizeof(void*),0));});yyval=(void*)({struct Cyc_DeclSpec_tok_struct*
_tmp3E7=_cycalloc(sizeof(*_tmp3E7));_tmp3E7[0]=({struct Cyc_DeclSpec_tok_struct
_tmp3E8;_tmp3E8.tag=Cyc_DeclSpec_tok;_tmp3E8.f1=({struct Cyc_Parse_Declaration_spec*
_tmp3E9=_cycalloc(sizeof(*_tmp3E9));_tmp3E9->sc=({struct Cyc_Core_Opt*_tmp3EA=
_cycalloc(sizeof(*_tmp3EA));_tmp3EA->v=(void*)Cyc_yyget_StorageClass_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp3EA;});_tmp3E9->tq=(
Cyc_yyget_DeclSpec_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->tq;
_tmp3E9->type_specs=(Cyc_yyget_DeclSpec_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]))->type_specs;_tmp3E9->is_inline=(Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->is_inline;_tmp3E9->attributes=(
Cyc_yyget_DeclSpec_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->attributes;
_tmp3E9;});_tmp3E8;});_tmp3E7;});break;case 35: _LL212: yyval=(void*)({struct Cyc_DeclSpec_tok_struct*
_tmp3EB=_cycalloc(sizeof(*_tmp3EB));_tmp3EB[0]=({struct Cyc_DeclSpec_tok_struct
_tmp3EC;_tmp3EC.tag=Cyc_DeclSpec_tok;_tmp3EC.f1=({struct Cyc_Parse_Declaration_spec*
_tmp3ED=_cycalloc(sizeof(*_tmp3ED));_tmp3ED->sc=0;_tmp3ED->tq=Cyc_Absyn_empty_tqual();
_tmp3ED->type_specs=({struct Cyc_List_List*_tmp3EE=_cycalloc(sizeof(*_tmp3EE));
_tmp3EE->hd=(void*)Cyc_yyget_TypeSpecifier_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp3EE->tl=0;_tmp3EE;});
_tmp3ED->is_inline=0;_tmp3ED->attributes=0;_tmp3ED;});_tmp3EC;});_tmp3EB;});
break;case 36: _LL213: yyval=(void*)({struct Cyc_DeclSpec_tok_struct*_tmp3EF=
_cycalloc(sizeof(*_tmp3EF));_tmp3EF[0]=({struct Cyc_DeclSpec_tok_struct _tmp3F0;
_tmp3F0.tag=Cyc_DeclSpec_tok;_tmp3F0.f1=({struct Cyc_Parse_Declaration_spec*
_tmp3F1=_cycalloc(sizeof(*_tmp3F1));_tmp3F1->sc=(Cyc_yyget_DeclSpec_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]))->sc;_tmp3F1->tq=(Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->tq;_tmp3F1->type_specs=({
struct Cyc_List_List*_tmp3F2=_cycalloc(sizeof(*_tmp3F2));_tmp3F2->hd=(void*)Cyc_yyget_TypeSpecifier_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp3F2->tl=(Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->type_specs;_tmp3F2;});
_tmp3F1->is_inline=(Cyc_yyget_DeclSpec_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]))->is_inline;_tmp3F1->attributes=(Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->attributes;_tmp3F1;});
_tmp3F0;});_tmp3EF;});break;case 37: _LL214: yyval=(void*)({struct Cyc_DeclSpec_tok_struct*
_tmp3F3=_cycalloc(sizeof(*_tmp3F3));_tmp3F3[0]=({struct Cyc_DeclSpec_tok_struct
_tmp3F4;_tmp3F4.tag=Cyc_DeclSpec_tok;_tmp3F4.f1=({struct Cyc_Parse_Declaration_spec*
_tmp3F5=_cycalloc(sizeof(*_tmp3F5));_tmp3F5->sc=0;_tmp3F5->tq=Cyc_yyget_TypeQual_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp3F5->type_specs=0;
_tmp3F5->is_inline=0;_tmp3F5->attributes=0;_tmp3F5;});_tmp3F4;});_tmp3F3;});
break;case 38: _LL215: yyval=(void*)({struct Cyc_DeclSpec_tok_struct*_tmp3F6=
_cycalloc(sizeof(*_tmp3F6));_tmp3F6[0]=({struct Cyc_DeclSpec_tok_struct _tmp3F7;
_tmp3F7.tag=Cyc_DeclSpec_tok;_tmp3F7.f1=({struct Cyc_Parse_Declaration_spec*
_tmp3F8=_cycalloc(sizeof(*_tmp3F8));_tmp3F8->sc=(Cyc_yyget_DeclSpec_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]))->sc;_tmp3F8->tq=Cyc_Absyn_combine_tqual(
Cyc_yyget_TypeQual_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),(
Cyc_yyget_DeclSpec_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->tq);
_tmp3F8->type_specs=(Cyc_yyget_DeclSpec_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]))->type_specs;_tmp3F8->is_inline=(Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->is_inline;_tmp3F8->attributes=(
Cyc_yyget_DeclSpec_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->attributes;
_tmp3F8;});_tmp3F7;});_tmp3F6;});break;case 39: _LL216: yyval=(void*)({struct Cyc_DeclSpec_tok_struct*
_tmp3F9=_cycalloc(sizeof(*_tmp3F9));_tmp3F9[0]=({struct Cyc_DeclSpec_tok_struct
_tmp3FA;_tmp3FA.tag=Cyc_DeclSpec_tok;_tmp3FA.f1=({struct Cyc_Parse_Declaration_spec*
_tmp3FB=_cycalloc(sizeof(*_tmp3FB));_tmp3FB->sc=0;_tmp3FB->tq=Cyc_Absyn_empty_tqual();
_tmp3FB->type_specs=0;_tmp3FB->is_inline=1;_tmp3FB->attributes=0;_tmp3FB;});
_tmp3FA;});_tmp3F9;});break;case 40: _LL217: yyval=(void*)({struct Cyc_DeclSpec_tok_struct*
_tmp3FC=_cycalloc(sizeof(*_tmp3FC));_tmp3FC[0]=({struct Cyc_DeclSpec_tok_struct
_tmp3FD;_tmp3FD.tag=Cyc_DeclSpec_tok;_tmp3FD.f1=({struct Cyc_Parse_Declaration_spec*
_tmp3FE=_cycalloc(sizeof(*_tmp3FE));_tmp3FE->sc=(Cyc_yyget_DeclSpec_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]))->sc;_tmp3FE->tq=(Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->tq;_tmp3FE->type_specs=(
Cyc_yyget_DeclSpec_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->type_specs;
_tmp3FE->is_inline=1;_tmp3FE->attributes=(Cyc_yyget_DeclSpec_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]))->attributes;_tmp3FE;});
_tmp3FD;});_tmp3FC;});break;case 41: _LL218: yyval=(void*)({struct Cyc_DeclSpec_tok_struct*
_tmp3FF=_cycalloc(sizeof(*_tmp3FF));_tmp3FF[0]=({struct Cyc_DeclSpec_tok_struct
_tmp400;_tmp400.tag=Cyc_DeclSpec_tok;_tmp400.f1=({struct Cyc_Parse_Declaration_spec*
_tmp401=_cycalloc(sizeof(*_tmp401));_tmp401->sc=0;_tmp401->tq=Cyc_Absyn_empty_tqual();
_tmp401->type_specs=0;_tmp401->is_inline=0;_tmp401->attributes=Cyc_yyget_AttributeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp401;});_tmp400;});
_tmp3FF;});break;case 42: _LL219: yyval=(void*)({struct Cyc_DeclSpec_tok_struct*
_tmp402=_cycalloc(sizeof(*_tmp402));_tmp402[0]=({struct Cyc_DeclSpec_tok_struct
_tmp403;_tmp403.tag=Cyc_DeclSpec_tok;_tmp403.f1=({struct Cyc_Parse_Declaration_spec*
_tmp404=_cycalloc(sizeof(*_tmp404));_tmp404->sc=(Cyc_yyget_DeclSpec_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]))->sc;_tmp404->tq=(Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->tq;_tmp404->type_specs=(
Cyc_yyget_DeclSpec_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->type_specs;
_tmp404->is_inline=(Cyc_yyget_DeclSpec_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]))->is_inline;_tmp404->attributes=Cyc_List_imp_append(Cyc_yyget_AttributeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),(Cyc_yyget_DeclSpec_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->attributes);_tmp404;});
_tmp403;});_tmp402;});break;case 43: _LL21A: yyval=(void*)({struct Cyc_StorageClass_tok_struct*
_tmp405=_cycalloc(sizeof(*_tmp405));_tmp405[0]=({struct Cyc_StorageClass_tok_struct
_tmp406;_tmp406.tag=Cyc_StorageClass_tok;_tmp406.f1=(void*)((void*)4);_tmp406;});
_tmp405;});break;case 44: _LL21B: yyval=(void*)({struct Cyc_StorageClass_tok_struct*
_tmp407=_cycalloc(sizeof(*_tmp407));_tmp407[0]=({struct Cyc_StorageClass_tok_struct
_tmp408;_tmp408.tag=Cyc_StorageClass_tok;_tmp408.f1=(void*)((void*)5);_tmp408;});
_tmp407;});break;case 45: _LL21C: yyval=(void*)({struct Cyc_StorageClass_tok_struct*
_tmp409=_cycalloc(sizeof(*_tmp409));_tmp409[0]=({struct Cyc_StorageClass_tok_struct
_tmp40A;_tmp40A.tag=Cyc_StorageClass_tok;_tmp40A.f1=(void*)((void*)3);_tmp40A;});
_tmp409;});break;case 46: _LL21D: yyval=(void*)({struct Cyc_StorageClass_tok_struct*
_tmp40B=_cycalloc(sizeof(*_tmp40B));_tmp40B[0]=({struct Cyc_StorageClass_tok_struct
_tmp40C;_tmp40C.tag=Cyc_StorageClass_tok;_tmp40C.f1=(void*)((void*)1);_tmp40C;});
_tmp40B;});break;case 47: _LL21E: if(Cyc_strcmp((struct _tagged_arr)Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),({const char*_tmp40D="C";
_tag_arr(_tmp40D,sizeof(char),_get_zero_arr_size(_tmp40D,2));}))!= 0)Cyc_Parse_err(({
const char*_tmp40E="only extern or extern \"C\" is allowed";_tag_arr(_tmp40E,
sizeof(char),_get_zero_arr_size(_tmp40E,37));}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));yyval=(void*)({
struct Cyc_StorageClass_tok_struct*_tmp40F=_cycalloc(sizeof(*_tmp40F));_tmp40F[0]=({
struct Cyc_StorageClass_tok_struct _tmp410;_tmp410.tag=Cyc_StorageClass_tok;
_tmp410.f1=(void*)((void*)2);_tmp410;});_tmp40F;});break;case 48: _LL21F: yyval=(
void*)({struct Cyc_StorageClass_tok_struct*_tmp411=_cycalloc(sizeof(*_tmp411));
_tmp411[0]=({struct Cyc_StorageClass_tok_struct _tmp412;_tmp412.tag=Cyc_StorageClass_tok;
_tmp412.f1=(void*)((void*)0);_tmp412;});_tmp411;});break;case 49: _LL220: yyval=(
void*)({struct Cyc_StorageClass_tok_struct*_tmp413=_cycalloc(sizeof(*_tmp413));
_tmp413[0]=({struct Cyc_StorageClass_tok_struct _tmp414;_tmp414.tag=Cyc_StorageClass_tok;
_tmp414.f1=(void*)((void*)6);_tmp414;});_tmp413;});break;case 50: _LL221: yyval=(
void*)({struct Cyc_AttributeList_tok_struct*_tmp415=_cycalloc(sizeof(*_tmp415));
_tmp415[0]=({struct Cyc_AttributeList_tok_struct _tmp416;_tmp416.tag=Cyc_AttributeList_tok;
_tmp416.f1=0;_tmp416;});_tmp415;});break;case 51: _LL222: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 52: _LL223: yyval=
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)];break;case 53: _LL224:
yyval=(void*)({struct Cyc_AttributeList_tok_struct*_tmp417=_cycalloc(sizeof(*
_tmp417));_tmp417[0]=({struct Cyc_AttributeList_tok_struct _tmp418;_tmp418.tag=Cyc_AttributeList_tok;
_tmp418.f1=({struct Cyc_List_List*_tmp419=_cycalloc(sizeof(*_tmp419));_tmp419->hd=(
void*)Cyc_yyget_Attribute_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]);_tmp419->tl=0;_tmp419;});_tmp418;});_tmp417;});break;case 54:
_LL225: yyval=(void*)({struct Cyc_AttributeList_tok_struct*_tmp41A=_cycalloc(
sizeof(*_tmp41A));_tmp41A[0]=({struct Cyc_AttributeList_tok_struct _tmp41B;_tmp41B.tag=
Cyc_AttributeList_tok;_tmp41B.f1=({struct Cyc_List_List*_tmp41C=_cycalloc(sizeof(*
_tmp41C));_tmp41C->hd=(void*)Cyc_yyget_Attribute_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp41C->tl=Cyc_yyget_AttributeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp41C;});_tmp41B;});
_tmp41A;});break;case 55: _LL226: {static struct Cyc_Absyn_Aligned_att_struct
att_aligned={1,- 1};static struct _tuple18 att_map[17]={{{_tmp423,_tmp423,_tmp423 + 8},(
void*)0},{{_tmp424,_tmp424,_tmp424 + 6},(void*)1},{{_tmp425,_tmp425,_tmp425 + 9},(
void*)2},{{_tmp426,_tmp426,_tmp426 + 9},(void*)3},{{_tmp427,_tmp427,_tmp427 + 6},(
void*)4},{{_tmp428,_tmp428,_tmp428 + 8},(void*)& att_aligned},{{_tmp429,_tmp429,
_tmp429 + 7},(void*)5},{{_tmp42A,_tmp42A,_tmp42A + 7},(void*)7},{{_tmp42B,_tmp42B,
_tmp42B + 7},(void*)8},{{_tmp42C,_tmp42C,_tmp42C + 5},(void*)9},{{_tmp42D,_tmp42D,
_tmp42D + 10},(void*)10},{{_tmp42E,_tmp42E,_tmp42E + 10},(void*)11},{{_tmp42F,
_tmp42F,_tmp42F + 23},(void*)12},{{_tmp430,_tmp430,_tmp430 + 12},(void*)13},{{
_tmp431,_tmp431,_tmp431 + 11},(void*)14},{{_tmp432,_tmp432,_tmp432 + 22},(void*)15},{{
_tmp433,_tmp433,_tmp433 + 5},(void*)16}};struct _tagged_arr _tmp41D=Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);if((((_get_arr_size(
_tmp41D,sizeof(char))> 4  && *((const char*)_check_unknown_subscript(_tmp41D,
sizeof(char),0))== '_') && *((const char*)_check_unknown_subscript(_tmp41D,
sizeof(char),1))== '_') && *((const char*)_check_unknown_subscript(_tmp41D,
sizeof(char),(int)(_get_arr_size(_tmp41D,sizeof(char))- 2)))== '_') && *((const
char*)_check_unknown_subscript(_tmp41D,sizeof(char),(int)(_get_arr_size(_tmp41D,
sizeof(char))- 3)))== '_')_tmp41D=(struct _tagged_arr)Cyc_substring((struct
_tagged_arr)_tmp41D,2,_get_arr_size(_tmp41D,sizeof(char))- 5);{int i=0;for(0;i < 
17;++ i){if(Cyc_strcmp((struct _tagged_arr)_tmp41D,(struct _tagged_arr)(att_map[i]).f1)
== 0){yyval=(void*)({struct Cyc_Attribute_tok_struct*_tmp41E=_cycalloc(sizeof(*
_tmp41E));_tmp41E[0]=({struct Cyc_Attribute_tok_struct _tmp41F;_tmp41F.tag=Cyc_Attribute_tok;
_tmp41F.f1=(void*)(att_map[i]).f2;_tmp41F;});_tmp41E;});break;}}if(i == 17){Cyc_Parse_err(({
const char*_tmp420="unrecognized attribute";_tag_arr(_tmp420,sizeof(char),
_get_zero_arr_size(_tmp420,23));}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));yyval=(void*)({
struct Cyc_Attribute_tok_struct*_tmp421=_cycalloc(sizeof(*_tmp421));_tmp421[0]=({
struct Cyc_Attribute_tok_struct _tmp422;_tmp422.tag=Cyc_Attribute_tok;_tmp422.f1=(
void*)((void*)1);_tmp422;});_tmp421;});}break;}}case 56: _LL227: yyval=(void*)({
struct Cyc_Attribute_tok_struct*_tmp435=_cycalloc(sizeof(*_tmp435));_tmp435[0]=({
struct Cyc_Attribute_tok_struct _tmp436;_tmp436.tag=Cyc_Attribute_tok;_tmp436.f1=(
void*)((void*)4);_tmp436;});_tmp435;});break;case 57: _LL228: {struct _tagged_arr
_tmp437=Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 3)]);struct _tuple13 _tmp439;int _tmp43A;struct _tuple13*_tmp438=Cyc_yyget_Int_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp439=*_tmp438;
_tmp43A=_tmp439.f2;{void*a;if(Cyc_zstrcmp((struct _tagged_arr)_tmp437,({const char*
_tmp43B="regparm";_tag_arr(_tmp43B,sizeof(char),_get_zero_arr_size(_tmp43B,8));}))
== 0  || Cyc_zstrcmp((struct _tagged_arr)_tmp437,({const char*_tmp43C="__regparm__";
_tag_arr(_tmp43C,sizeof(char),_get_zero_arr_size(_tmp43C,12));}))== 0){if(
_tmp43A < 0  || _tmp43A > 3)Cyc_Parse_err(({const char*_tmp43D="regparm requires value between 0 and 3";
_tag_arr(_tmp43D,sizeof(char),_get_zero_arr_size(_tmp43D,39));}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line));a=(void*)({
struct Cyc_Absyn_Regparm_att_struct*_tmp43E=_cycalloc_atomic(sizeof(*_tmp43E));
_tmp43E[0]=({struct Cyc_Absyn_Regparm_att_struct _tmp43F;_tmp43F.tag=0;_tmp43F.f1=
_tmp43A;_tmp43F;});_tmp43E;});}else{if(Cyc_zstrcmp((struct _tagged_arr)_tmp437,({
const char*_tmp440="aligned";_tag_arr(_tmp440,sizeof(char),_get_zero_arr_size(
_tmp440,8));}))== 0  || Cyc_zstrcmp((struct _tagged_arr)_tmp437,({const char*
_tmp441="__aligned__";_tag_arr(_tmp441,sizeof(char),_get_zero_arr_size(_tmp441,
12));}))== 0){if(_tmp43A < 0)Cyc_Parse_err(({const char*_tmp442="aligned requires positive power of two";
_tag_arr(_tmp442,sizeof(char),_get_zero_arr_size(_tmp442,39));}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line));{
unsigned int j=(unsigned int)_tmp43A;for(0;(j & 1)== 0;j=j >> 1){;}j=j >> 1;if(j != 0)
Cyc_Parse_err(({const char*_tmp443="aligned requires positive power of two";
_tag_arr(_tmp443,sizeof(char),_get_zero_arr_size(_tmp443,39));}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line));a=(void*)({
struct Cyc_Absyn_Aligned_att_struct*_tmp444=_cycalloc_atomic(sizeof(*_tmp444));
_tmp444[0]=({struct Cyc_Absyn_Aligned_att_struct _tmp445;_tmp445.tag=1;_tmp445.f1=
_tmp43A;_tmp445;});_tmp444;});}}else{if(Cyc_zstrcmp((struct _tagged_arr)_tmp437,({
const char*_tmp446="initializes";_tag_arr(_tmp446,sizeof(char),_get_zero_arr_size(
_tmp446,12));}))== 0  || Cyc_zstrcmp((struct _tagged_arr)_tmp437,({const char*
_tmp447="__initializes__";_tag_arr(_tmp447,sizeof(char),_get_zero_arr_size(
_tmp447,16));}))== 0)a=(void*)({struct Cyc_Absyn_Initializes_att_struct*_tmp448=
_cycalloc_atomic(sizeof(*_tmp448));_tmp448[0]=({struct Cyc_Absyn_Initializes_att_struct
_tmp449;_tmp449.tag=4;_tmp449.f1=_tmp43A;_tmp449;});_tmp448;});else{Cyc_Parse_err(({
const char*_tmp44A="unrecognized attribute";_tag_arr(_tmp44A,sizeof(char),
_get_zero_arr_size(_tmp44A,23));}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).last_line));a=(void*)1;}}}
yyval=(void*)({struct Cyc_Attribute_tok_struct*_tmp44B=_cycalloc(sizeof(*_tmp44B));
_tmp44B[0]=({struct Cyc_Attribute_tok_struct _tmp44C;_tmp44C.tag=Cyc_Attribute_tok;
_tmp44C.f1=(void*)a;_tmp44C;});_tmp44B;});break;}}case 58: _LL229: {struct
_tagged_arr _tmp44D=Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 3)]);struct _tagged_arr _tmp44E=Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);void*a;if(Cyc_zstrcmp((
struct _tagged_arr)_tmp44D,({const char*_tmp44F="section";_tag_arr(_tmp44F,sizeof(
char),_get_zero_arr_size(_tmp44F,8));}))== 0  || Cyc_zstrcmp((struct _tagged_arr)
_tmp44D,({const char*_tmp450="__section__";_tag_arr(_tmp450,sizeof(char),
_get_zero_arr_size(_tmp450,12));}))== 0)a=(void*)({struct Cyc_Absyn_Section_att_struct*
_tmp451=_cycalloc(sizeof(*_tmp451));_tmp451[0]=({struct Cyc_Absyn_Section_att_struct
_tmp452;_tmp452.tag=2;_tmp452.f1=_tmp44E;_tmp452;});_tmp451;});else{Cyc_Parse_err(({
const char*_tmp453="unrecognized attribute";_tag_arr(_tmp453,sizeof(char),
_get_zero_arr_size(_tmp453,23));}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).last_line));a=(void*)1;}
yyval=(void*)({struct Cyc_Attribute_tok_struct*_tmp454=_cycalloc(sizeof(*_tmp454));
_tmp454[0]=({struct Cyc_Attribute_tok_struct _tmp455;_tmp455.tag=Cyc_Attribute_tok;
_tmp455.f1=(void*)a;_tmp455;});_tmp454;});break;}case 59: _LL22A: {struct
_tagged_arr _tmp456=Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 3)]);struct _tagged_arr _tmp457=Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);void*a;if(Cyc_zstrcmp((
struct _tagged_arr)_tmp456,({const char*_tmp458="__mode__";_tag_arr(_tmp458,
sizeof(char),_get_zero_arr_size(_tmp458,9));}))== 0)a=(void*)({struct Cyc_Absyn_Mode_att_struct*
_tmp459=_cycalloc(sizeof(*_tmp459));_tmp459[0]=({struct Cyc_Absyn_Mode_att_struct
_tmp45A;_tmp45A.tag=5;_tmp45A.f1=_tmp457;_tmp45A;});_tmp459;});else{Cyc_Parse_err(({
const char*_tmp45B="unrecognized attribute";_tag_arr(_tmp45B,sizeof(char),
_get_zero_arr_size(_tmp45B,23));}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).last_line));a=(void*)1;}
yyval=(void*)({struct Cyc_Attribute_tok_struct*_tmp45C=_cycalloc(sizeof(*_tmp45C));
_tmp45C[0]=({struct Cyc_Attribute_tok_struct _tmp45D;_tmp45D.tag=Cyc_Attribute_tok;
_tmp45D.f1=(void*)a;_tmp45D;});_tmp45C;});break;}case 60: _LL22B: {struct
_tagged_arr _tmp45E=Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 7)]);struct _tagged_arr _tmp45F=Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 5)]);int _tmp461;struct
_tuple13 _tmp460=*Cyc_yyget_Int_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 3)]);_tmp461=_tmp460.f2;{int _tmp463;struct _tuple13 _tmp462=*Cyc_yyget_Int_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp463=_tmp462.f2;{
void*a=(void*)1;if(Cyc_zstrcmp((struct _tagged_arr)_tmp45E,({const char*_tmp464="format";
_tag_arr(_tmp464,sizeof(char),_get_zero_arr_size(_tmp464,7));}))== 0  || Cyc_zstrcmp((
struct _tagged_arr)_tmp45E,({const char*_tmp465="__format__";_tag_arr(_tmp465,
sizeof(char),_get_zero_arr_size(_tmp465,11));}))== 0){if(Cyc_zstrcmp((struct
_tagged_arr)_tmp45F,({const char*_tmp466="printf";_tag_arr(_tmp466,sizeof(char),
_get_zero_arr_size(_tmp466,7));}))== 0)a=(void*)({struct Cyc_Absyn_Format_att_struct*
_tmp467=_cycalloc(sizeof(*_tmp467));_tmp467[0]=({struct Cyc_Absyn_Format_att_struct
_tmp468;_tmp468.tag=3;_tmp468.f1=(void*)((void*)0);_tmp468.f2=_tmp461;_tmp468.f3=
_tmp463;_tmp468;});_tmp467;});else{if(Cyc_zstrcmp((struct _tagged_arr)_tmp45F,({
const char*_tmp469="scanf";_tag_arr(_tmp469,sizeof(char),_get_zero_arr_size(
_tmp469,6));}))== 0)a=(void*)({struct Cyc_Absyn_Format_att_struct*_tmp46A=
_cycalloc(sizeof(*_tmp46A));_tmp46A[0]=({struct Cyc_Absyn_Format_att_struct
_tmp46B;_tmp46B.tag=3;_tmp46B.f1=(void*)((void*)1);_tmp46B.f2=_tmp461;_tmp46B.f3=
_tmp463;_tmp46B;});_tmp46A;});else{Cyc_Parse_err(({const char*_tmp46C="unrecognized format type";
_tag_arr(_tmp46C,sizeof(char),_get_zero_arr_size(_tmp46C,25));}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 5)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 5)]).last_line));}}}else{Cyc_Parse_err(({
const char*_tmp46D="unrecognized attribute";_tag_arr(_tmp46D,sizeof(char),
_get_zero_arr_size(_tmp46D,23));}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 7)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));}yyval=(void*)({
struct Cyc_Attribute_tok_struct*_tmp46E=_cycalloc(sizeof(*_tmp46E));_tmp46E[0]=({
struct Cyc_Attribute_tok_struct _tmp46F;_tmp46F.tag=Cyc_Attribute_tok;_tmp46F.f1=(
void*)a;_tmp46F;});_tmp46E;});break;}}}case 61: _LL22C: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 62: _LL22D: yyval=(
void*)({struct Cyc_TypeSpecifier_tok_struct*_tmp470=_cycalloc(sizeof(*_tmp470));
_tmp470[0]=({struct Cyc_TypeSpecifier_tok_struct _tmp471;_tmp471.tag=Cyc_TypeSpecifier_tok;
_tmp471.f1=(void*)Cyc_Parse_type_spec((void*)({struct Cyc_Absyn_TypedefType_struct*
_tmp472=_cycalloc(sizeof(*_tmp472));_tmp472[0]=({struct Cyc_Absyn_TypedefType_struct
_tmp473;_tmp473.tag=16;_tmp473.f1=Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp473.f2=Cyc_yyget_TypeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp473.f3=0;_tmp473.f4=
0;_tmp473;});_tmp472;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp471;});
_tmp470;});break;case 63: _LL22E: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp474=_cycalloc(sizeof(*_tmp474));_tmp474[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp475;_tmp475.tag=Cyc_TypeSpecifier_tok;_tmp475.f1=(void*)Cyc_Parse_type_spec((
void*)0,Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp475;});_tmp474;});break;case 64: _LL22F: yyval=(
void*)({struct Cyc_TypeSpecifier_tok_struct*_tmp476=_cycalloc(sizeof(*_tmp476));
_tmp476[0]=({struct Cyc_TypeSpecifier_tok_struct _tmp477;_tmp477.tag=Cyc_TypeSpecifier_tok;
_tmp477.f1=(void*)Cyc_Parse_type_spec(Cyc_Absyn_new_evar(0,0),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp477;});
_tmp476;});break;case 65: _LL230: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp478=_cycalloc(sizeof(*_tmp478));_tmp478[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp479;_tmp479.tag=Cyc_TypeSpecifier_tok;_tmp479.f1=(void*)Cyc_Parse_type_spec(
Cyc_Absyn_new_evar(({struct Cyc_Core_Opt*_tmp47A=_cycalloc(sizeof(*_tmp47A));
_tmp47A->v=(void*)Cyc_yyget_Kind_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]);_tmp47A;}),0),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp479;});
_tmp478;});break;case 66: _LL231: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp47B=_cycalloc(sizeof(*_tmp47B));_tmp47B[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp47C;_tmp47C.tag=Cyc_TypeSpecifier_tok;_tmp47C.f1=(void*)Cyc_Parse_type_spec(
Cyc_Absyn_char_typ,Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp47C;});
_tmp47B;});break;case 67: _LL232: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp47D=_cycalloc(sizeof(*_tmp47D));_tmp47D[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp47E;_tmp47E.tag=Cyc_TypeSpecifier_tok;_tmp47E.f1=(void*)((void*)({struct Cyc_Parse_Short_spec_struct*
_tmp47F=_cycalloc(sizeof(*_tmp47F));_tmp47F[0]=({struct Cyc_Parse_Short_spec_struct
_tmp480;_tmp480.tag=2;_tmp480.f1=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp480;});
_tmp47F;}));_tmp47E;});_tmp47D;});break;case 68: _LL233: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp481=_cycalloc(sizeof(*_tmp481));_tmp481[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp482;_tmp482.tag=Cyc_TypeSpecifier_tok;_tmp482.f1=(void*)Cyc_Parse_type_spec(
Cyc_Absyn_sint_typ,Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp482;});
_tmp481;});break;case 69: _LL234: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp483=_cycalloc(sizeof(*_tmp483));_tmp483[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp484;_tmp484.tag=Cyc_TypeSpecifier_tok;_tmp484.f1=(void*)((void*)({struct Cyc_Parse_Long_spec_struct*
_tmp485=_cycalloc(sizeof(*_tmp485));_tmp485[0]=({struct Cyc_Parse_Long_spec_struct
_tmp486;_tmp486.tag=3;_tmp486.f1=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp486;});
_tmp485;}));_tmp484;});_tmp483;});break;case 70: _LL235: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp487=_cycalloc(sizeof(*_tmp487));_tmp487[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp488;_tmp488.tag=Cyc_TypeSpecifier_tok;_tmp488.f1=(void*)Cyc_Parse_type_spec(
Cyc_Absyn_float_typ,Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp488;});
_tmp487;});break;case 71: _LL236: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp489=_cycalloc(sizeof(*_tmp489));_tmp489[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp48A;_tmp48A.tag=Cyc_TypeSpecifier_tok;_tmp48A.f1=(void*)Cyc_Parse_type_spec(
Cyc_Absyn_double_typ(0),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp48A;});
_tmp489;});break;case 72: _LL237: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp48B=_cycalloc(sizeof(*_tmp48B));_tmp48B[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp48C;_tmp48C.tag=Cyc_TypeSpecifier_tok;_tmp48C.f1=(void*)((void*)({struct Cyc_Parse_Signed_spec_struct*
_tmp48D=_cycalloc(sizeof(*_tmp48D));_tmp48D[0]=({struct Cyc_Parse_Signed_spec_struct
_tmp48E;_tmp48E.tag=0;_tmp48E.f1=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp48E;});
_tmp48D;}));_tmp48C;});_tmp48B;});break;case 73: _LL238: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp48F=_cycalloc(sizeof(*_tmp48F));_tmp48F[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp490;_tmp490.tag=Cyc_TypeSpecifier_tok;_tmp490.f1=(void*)((void*)({struct Cyc_Parse_Unsigned_spec_struct*
_tmp491=_cycalloc(sizeof(*_tmp491));_tmp491[0]=({struct Cyc_Parse_Unsigned_spec_struct
_tmp492;_tmp492.tag=1;_tmp492.f1=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp492;});
_tmp491;}));_tmp490;});_tmp48F;});break;case 74: _LL239: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 75: _LL23A: yyval=
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 76: _LL23B:
yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 77:
_LL23C: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*_tmp493=_cycalloc(
sizeof(*_tmp493));_tmp493[0]=({struct Cyc_TypeSpecifier_tok_struct _tmp494;_tmp494.tag=
Cyc_TypeSpecifier_tok;_tmp494.f1=(void*)Cyc_Parse_type_spec(Cyc_yyget_Type_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp494;});
_tmp493;});break;case 78: _LL23D: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp495=_cycalloc(sizeof(*_tmp495));_tmp495[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp496;_tmp496.tag=Cyc_TypeSpecifier_tok;_tmp496.f1=(void*)Cyc_Parse_type_spec((
void*)({struct Cyc_Absyn_TupleType_struct*_tmp497=_cycalloc(sizeof(*_tmp497));
_tmp497[0]=({struct Cyc_Absyn_TupleType_struct _tmp498;_tmp498.tag=9;_tmp498.f1=((
struct Cyc_List_List*(*)(struct _tuple9*(*f)(struct Cyc_Position_Segment*,struct
_tuple2*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(
Cyc_Parse_get_tqual_typ,Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line),((struct Cyc_List_List*(*)(
struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_ParamDeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)])));_tmp498;});_tmp497;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp496;});_tmp495;});break;case 79: _LL23E: yyval=(
void*)({struct Cyc_TypeSpecifier_tok_struct*_tmp499=_cycalloc(sizeof(*_tmp499));
_tmp499[0]=({struct Cyc_TypeSpecifier_tok_struct _tmp49A;_tmp49A.tag=Cyc_TypeSpecifier_tok;
_tmp49A.f1=(void*)Cyc_Parse_type_spec((void*)({struct Cyc_Absyn_RgnHandleType_struct*
_tmp49B=_cycalloc(sizeof(*_tmp49B));_tmp49B[0]=({struct Cyc_Absyn_RgnHandleType_struct
_tmp49C;_tmp49C.tag=15;_tmp49C.f1=(void*)Cyc_yyget_Type_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp49C;});_tmp49B;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp49A;});_tmp499;});break;case 80: _LL23F: yyval=(
void*)({struct Cyc_TypeSpecifier_tok_struct*_tmp49D=_cycalloc(sizeof(*_tmp49D));
_tmp49D[0]=({struct Cyc_TypeSpecifier_tok_struct _tmp49E;_tmp49E.tag=Cyc_TypeSpecifier_tok;
_tmp49E.f1=(void*)Cyc_Parse_type_spec((void*)({struct Cyc_Absyn_SizeofType_struct*
_tmp49F=_cycalloc(sizeof(*_tmp49F));_tmp49F[0]=({struct Cyc_Absyn_SizeofType_struct
_tmp4A0;_tmp4A0.tag=14;_tmp4A0.f1=(void*)Cyc_yyget_Type_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp4A0;});_tmp49F;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp49E;});_tmp49D;});break;case 81: _LL240: yyval=(
void*)({struct Cyc_TypeSpecifier_tok_struct*_tmp4A1=_cycalloc(sizeof(*_tmp4A1));
_tmp4A1[0]=({struct Cyc_TypeSpecifier_tok_struct _tmp4A2;_tmp4A2.tag=Cyc_TypeSpecifier_tok;
_tmp4A2.f1=(void*)Cyc_Parse_type_spec((void*)({struct Cyc_Absyn_TagType_struct*
_tmp4A3=_cycalloc(sizeof(*_tmp4A3));_tmp4A3[0]=({struct Cyc_Absyn_TagType_struct
_tmp4A4;_tmp4A4.tag=17;_tmp4A4.f1=(void*)Cyc_yyget_Type_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp4A4;});_tmp4A3;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp4A2;});_tmp4A1;});break;case 82: _LL241: yyval=(
void*)({struct Cyc_TypeSpecifier_tok_struct*_tmp4A5=_cycalloc(sizeof(*_tmp4A5));
_tmp4A5[0]=({struct Cyc_TypeSpecifier_tok_struct _tmp4A6;_tmp4A6.tag=Cyc_TypeSpecifier_tok;
_tmp4A6.f1=(void*)Cyc_Parse_type_spec((void*)({struct Cyc_Absyn_TagType_struct*
_tmp4A7=_cycalloc(sizeof(*_tmp4A7));_tmp4A7[0]=({struct Cyc_Absyn_TagType_struct
_tmp4A8;_tmp4A8.tag=17;_tmp4A8.f1=(void*)Cyc_Absyn_new_evar(({struct Cyc_Core_Opt*
_tmp4A9=_cycalloc(sizeof(*_tmp4A9));_tmp4A9->v=(void*)((void*)5);_tmp4A9;}),0);
_tmp4A8;});_tmp4A7;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp4A6;});
_tmp4A5;});break;case 83: _LL242: {struct _tuple13 _tmp4AB;int _tmp4AC;struct _tuple13*
_tmp4AA=Cyc_yyget_Int_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);
_tmp4AB=*_tmp4AA;_tmp4AC=_tmp4AB.f2;yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp4AD=_cycalloc(sizeof(*_tmp4AD));_tmp4AD[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp4AE;_tmp4AE.tag=Cyc_TypeSpecifier_tok;_tmp4AE.f1=(void*)Cyc_Parse_type_spec((
void*)({struct Cyc_Absyn_TypeInt_struct*_tmp4AF=_cycalloc_atomic(sizeof(*_tmp4AF));
_tmp4AF[0]=({struct Cyc_Absyn_TypeInt_struct _tmp4B0;_tmp4B0.tag=18;_tmp4B0.f1=
_tmp4AC;_tmp4B0;});_tmp4AF;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp4AE;});
_tmp4AD;});break;}case 84: _LL243: yyval=(void*)({struct Cyc_Kind_tok_struct*_tmp4B1=
_cycalloc(sizeof(*_tmp4B1));_tmp4B1[0]=({struct Cyc_Kind_tok_struct _tmp4B2;
_tmp4B2.tag=Cyc_Kind_tok;_tmp4B2.f1=(void*)Cyc_Parse_id_to_kind(Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp4B2;});
_tmp4B1;});break;case 85: _LL244: yyval=(void*)({struct Cyc_TypeQual_tok_struct*
_tmp4B3=_cycalloc_atomic(sizeof(*_tmp4B3));_tmp4B3[0]=({struct Cyc_TypeQual_tok_struct
_tmp4B4;_tmp4B4.tag=Cyc_TypeQual_tok;_tmp4B4.f1=({struct Cyc_Absyn_Tqual _tmp4B5;
_tmp4B5.print_const=1;_tmp4B5.q_volatile=0;_tmp4B5.q_restrict=0;_tmp4B5.real_const=
1;_tmp4B5;});_tmp4B4;});_tmp4B3;});break;case 86: _LL245: yyval=(void*)({struct Cyc_TypeQual_tok_struct*
_tmp4B6=_cycalloc_atomic(sizeof(*_tmp4B6));_tmp4B6[0]=({struct Cyc_TypeQual_tok_struct
_tmp4B7;_tmp4B7.tag=Cyc_TypeQual_tok;_tmp4B7.f1=({struct Cyc_Absyn_Tqual _tmp4B8;
_tmp4B8.print_const=0;_tmp4B8.q_volatile=1;_tmp4B8.q_restrict=0;_tmp4B8.real_const=
0;_tmp4B8;});_tmp4B7;});_tmp4B6;});break;case 87: _LL246: yyval=(void*)({struct Cyc_TypeQual_tok_struct*
_tmp4B9=_cycalloc_atomic(sizeof(*_tmp4B9));_tmp4B9[0]=({struct Cyc_TypeQual_tok_struct
_tmp4BA;_tmp4BA.tag=Cyc_TypeQual_tok;_tmp4BA.f1=({struct Cyc_Absyn_Tqual _tmp4BB;
_tmp4BB.print_const=0;_tmp4BB.q_volatile=0;_tmp4BB.q_restrict=1;_tmp4BB.real_const=
0;_tmp4BB;});_tmp4BA;});_tmp4B9;});break;case 88: _LL247: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp4BC=_cycalloc(sizeof(*_tmp4BC));_tmp4BC[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp4BD;_tmp4BD.tag=Cyc_TypeSpecifier_tok;_tmp4BD.f1=(void*)((void*)({struct Cyc_Parse_Decl_spec_struct*
_tmp4BE=_cycalloc(sizeof(*_tmp4BE));_tmp4BE[0]=({struct Cyc_Parse_Decl_spec_struct
_tmp4BF;_tmp4BF.tag=5;_tmp4BF.f1=({struct Cyc_Absyn_Decl*_tmp4C0=_cycalloc(
sizeof(*_tmp4C0));_tmp4C0->r=(void*)((void*)({struct Cyc_Absyn_Enum_d_struct*
_tmp4C1=_cycalloc(sizeof(*_tmp4C1));_tmp4C1[0]=({struct Cyc_Absyn_Enum_d_struct
_tmp4C2;_tmp4C2.tag=6;_tmp4C2.f1=({struct Cyc_Absyn_Enumdecl*_tmp4C3=_cycalloc(
sizeof(*_tmp4C3));_tmp4C3->sc=(void*)((void*)2);_tmp4C3->name=Cyc_yyget_QualId_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp4C3->fields=({
struct Cyc_Core_Opt*_tmp4C4=_cycalloc(sizeof(*_tmp4C4));_tmp4C4->v=Cyc_yyget_EnumfieldList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp4C4;});_tmp4C3;});
_tmp4C2;});_tmp4C1;}));_tmp4C0->loc=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp4C0;});
_tmp4BF;});_tmp4BE;}));_tmp4BD;});_tmp4BC;});break;case 89: _LL248: yyval=(void*)({
struct Cyc_TypeSpecifier_tok_struct*_tmp4C5=_cycalloc(sizeof(*_tmp4C5));_tmp4C5[0]=({
struct Cyc_TypeSpecifier_tok_struct _tmp4C6;_tmp4C6.tag=Cyc_TypeSpecifier_tok;
_tmp4C6.f1=(void*)Cyc_Parse_type_spec((void*)({struct Cyc_Absyn_EnumType_struct*
_tmp4C7=_cycalloc(sizeof(*_tmp4C7));_tmp4C7[0]=({struct Cyc_Absyn_EnumType_struct
_tmp4C8;_tmp4C8.tag=12;_tmp4C8.f1=Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp4C8.f2=0;_tmp4C8;});
_tmp4C7;}),Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(
10000,yylsp_offset - 1)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp4C6;});_tmp4C5;});break;case 90: _LL249: yyval=(
void*)({struct Cyc_TypeSpecifier_tok_struct*_tmp4C9=_cycalloc(sizeof(*_tmp4C9));
_tmp4C9[0]=({struct Cyc_TypeSpecifier_tok_struct _tmp4CA;_tmp4CA.tag=Cyc_TypeSpecifier_tok;
_tmp4CA.f1=(void*)((void*)({struct Cyc_Parse_Type_spec_struct*_tmp4CB=_cycalloc(
sizeof(*_tmp4CB));_tmp4CB[0]=({struct Cyc_Parse_Type_spec_struct _tmp4CC;_tmp4CC.tag=
4;_tmp4CC.f1=(void*)((void*)({struct Cyc_Absyn_AnonEnumType_struct*_tmp4CD=
_cycalloc(sizeof(*_tmp4CD));_tmp4CD[0]=({struct Cyc_Absyn_AnonEnumType_struct
_tmp4CE;_tmp4CE.tag=13;_tmp4CE.f1=Cyc_yyget_EnumfieldList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp4CE;});_tmp4CD;}));
_tmp4CC.f2=Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(
10000,yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line);_tmp4CC;});_tmp4CB;}));_tmp4CA;});_tmp4C9;});break;
case 91: _LL24A: yyval=(void*)({struct Cyc_Enumfield_tok_struct*_tmp4CF=_cycalloc(
sizeof(*_tmp4CF));_tmp4CF[0]=({struct Cyc_Enumfield_tok_struct _tmp4D0;_tmp4D0.tag=
Cyc_Enumfield_tok;_tmp4D0.f1=({struct Cyc_Absyn_Enumfield*_tmp4D1=_cycalloc(
sizeof(*_tmp4D1));_tmp4D1->name=Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp4D1->tag=0;_tmp4D1->loc=
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line);_tmp4D1;});_tmp4D0;});_tmp4CF;});break;case 92: _LL24B:
yyval=(void*)({struct Cyc_Enumfield_tok_struct*_tmp4D2=_cycalloc(sizeof(*_tmp4D2));
_tmp4D2[0]=({struct Cyc_Enumfield_tok_struct _tmp4D3;_tmp4D3.tag=Cyc_Enumfield_tok;
_tmp4D3.f1=({struct Cyc_Absyn_Enumfield*_tmp4D4=_cycalloc(sizeof(*_tmp4D4));
_tmp4D4->name=Cyc_yyget_QualId_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 2)]);_tmp4D4->tag=(struct Cyc_Absyn_Exp*)Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp4D4->loc=Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp4D4;});
_tmp4D3;});_tmp4D2;});break;case 93: _LL24C: yyval=(void*)({struct Cyc_EnumfieldList_tok_struct*
_tmp4D5=_cycalloc(sizeof(*_tmp4D5));_tmp4D5[0]=({struct Cyc_EnumfieldList_tok_struct
_tmp4D6;_tmp4D6.tag=Cyc_EnumfieldList_tok;_tmp4D6.f1=({struct Cyc_List_List*
_tmp4D7=_cycalloc(sizeof(*_tmp4D7));_tmp4D7->hd=Cyc_yyget_Enumfield_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp4D7->tl=0;_tmp4D7;});
_tmp4D6;});_tmp4D5;});break;case 94: _LL24D: yyval=(void*)({struct Cyc_EnumfieldList_tok_struct*
_tmp4D8=_cycalloc(sizeof(*_tmp4D8));_tmp4D8[0]=({struct Cyc_EnumfieldList_tok_struct
_tmp4D9;_tmp4D9.tag=Cyc_EnumfieldList_tok;_tmp4D9.f1=({struct Cyc_List_List*
_tmp4DA=_cycalloc(sizeof(*_tmp4DA));_tmp4DA->hd=Cyc_yyget_Enumfield_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp4DA->tl=0;_tmp4DA;});
_tmp4D9;});_tmp4D8;});break;case 95: _LL24E: yyval=(void*)({struct Cyc_EnumfieldList_tok_struct*
_tmp4DB=_cycalloc(sizeof(*_tmp4DB));_tmp4DB[0]=({struct Cyc_EnumfieldList_tok_struct
_tmp4DC;_tmp4DC.tag=Cyc_EnumfieldList_tok;_tmp4DC.f1=({struct Cyc_List_List*
_tmp4DD=_cycalloc(sizeof(*_tmp4DD));_tmp4DD->hd=Cyc_yyget_Enumfield_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp4DD->tl=Cyc_yyget_EnumfieldList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp4DD;});_tmp4DC;});
_tmp4DB;});break;case 96: _LL24F: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp4DE=_cycalloc(sizeof(*_tmp4DE));_tmp4DE[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp4DF;_tmp4DF.tag=Cyc_TypeSpecifier_tok;_tmp4DF.f1=(void*)Cyc_Parse_type_spec((
void*)({struct Cyc_Absyn_AnonAggrType_struct*_tmp4E0=_cycalloc(sizeof(*_tmp4E0));
_tmp4E0[0]=({struct Cyc_Absyn_AnonAggrType_struct _tmp4E1;_tmp4E1.tag=11;_tmp4E1.f1=(
void*)Cyc_yyget_StructOrUnion_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 3)]);_tmp4E1.f2=Cyc_yyget_AggrFieldDeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp4E1;});_tmp4E0;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp4DF;});_tmp4DE;});break;case 97: _LL250: {struct
Cyc_List_List*_tmp4E2=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct
Cyc_Position_Segment*,void*),struct Cyc_Position_Segment*env,struct Cyc_List_List*
x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 5)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 5)]).last_line),Cyc_yyget_TypeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 5)]));struct Cyc_List_List*
_tmp4E3=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Position_Segment*,
void*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).last_line),Cyc_yyget_TypeList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]));yyval=(void*)({struct
Cyc_TypeSpecifier_tok_struct*_tmp4E4=_cycalloc(sizeof(*_tmp4E4));_tmp4E4[0]=({
struct Cyc_TypeSpecifier_tok_struct _tmp4E5;_tmp4E5.tag=Cyc_TypeSpecifier_tok;
_tmp4E5.f1=(void*)((void*)({struct Cyc_Parse_Decl_spec_struct*_tmp4E6=_cycalloc(
sizeof(*_tmp4E6));_tmp4E6[0]=({struct Cyc_Parse_Decl_spec_struct _tmp4E7;_tmp4E7.tag=
5;_tmp4E7.f1=Cyc_Absyn_aggr_decl(Cyc_yyget_StructOrUnion_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 7)]),(void*)2,Cyc_yyget_QualId_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 6)]),_tmp4E2,(struct Cyc_Absyn_AggrdeclImpl*)
Cyc_Absyn_aggrdecl_impl(_tmp4E3,Cyc_yyget_Rgnorder_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_AggrFieldDeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)])),0,Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 7)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp4E7;});
_tmp4E6;}));_tmp4E5;});_tmp4E4;});break;}case 98: _LL251: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp4E8=_cycalloc(sizeof(*_tmp4E8));_tmp4E8[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp4E9;_tmp4E9.tag=Cyc_TypeSpecifier_tok;_tmp4E9.f1=(void*)Cyc_Parse_type_spec((
void*)({struct Cyc_Absyn_AggrType_struct*_tmp4EA=_cycalloc(sizeof(*_tmp4EA));
_tmp4EA[0]=({struct Cyc_Absyn_AggrType_struct _tmp4EB;_tmp4EB.tag=10;_tmp4EB.f1=({
struct Cyc_Absyn_AggrInfo _tmp4EC;_tmp4EC.aggr_info=(void*)((void*)({struct Cyc_Absyn_UnknownAggr_struct*
_tmp4ED=_cycalloc(sizeof(*_tmp4ED));_tmp4ED[0]=({struct Cyc_Absyn_UnknownAggr_struct
_tmp4EE;_tmp4EE.tag=0;_tmp4EE.f1=(void*)Cyc_yyget_StructOrUnion_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp4EE.f2=Cyc_yyget_QualId_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp4EE;});_tmp4ED;}));
_tmp4EC.targs=Cyc_yyget_TypeList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]);_tmp4EC;});_tmp4EB;});_tmp4EA;}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp4E9;});
_tmp4E8;});break;case 99: _LL252: yyval=(void*)({struct Cyc_TypeList_tok_struct*
_tmp4EF=_cycalloc(sizeof(*_tmp4EF));_tmp4EF[0]=({struct Cyc_TypeList_tok_struct
_tmp4F0;_tmp4F0.tag=Cyc_TypeList_tok;_tmp4F0.f1=0;_tmp4F0;});_tmp4EF;});break;
case 100: _LL253: yyval=(void*)({struct Cyc_TypeList_tok_struct*_tmp4F1=_cycalloc(
sizeof(*_tmp4F1));_tmp4F1[0]=({struct Cyc_TypeList_tok_struct _tmp4F2;_tmp4F2.tag=
Cyc_TypeList_tok;_tmp4F2.f1=Cyc_List_imp_rev(Cyc_yyget_TypeList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]));_tmp4F2;});_tmp4F1;});
break;case 101: _LL254: yyval=(void*)({struct Cyc_StructOrUnion_tok_struct*_tmp4F3=
_cycalloc(sizeof(*_tmp4F3));_tmp4F3[0]=({struct Cyc_StructOrUnion_tok_struct
_tmp4F4;_tmp4F4.tag=Cyc_StructOrUnion_tok;_tmp4F4.f1=(void*)((void*)0);_tmp4F4;});
_tmp4F3;});break;case 102: _LL255: yyval=(void*)({struct Cyc_StructOrUnion_tok_struct*
_tmp4F5=_cycalloc(sizeof(*_tmp4F5));_tmp4F5[0]=({struct Cyc_StructOrUnion_tok_struct
_tmp4F6;_tmp4F6.tag=Cyc_StructOrUnion_tok;_tmp4F6.f1=(void*)((void*)1);_tmp4F6;});
_tmp4F5;});break;case 103: _LL256: yyval=(void*)({struct Cyc_AggrFieldDeclList_tok_struct*
_tmp4F7=_cycalloc(sizeof(*_tmp4F7));_tmp4F7[0]=({struct Cyc_AggrFieldDeclList_tok_struct
_tmp4F8;_tmp4F8.tag=Cyc_AggrFieldDeclList_tok;_tmp4F8.f1=0;_tmp4F8;});_tmp4F7;});
break;case 104: _LL257: yyval=(void*)({struct Cyc_AggrFieldDeclList_tok_struct*
_tmp4F9=_cycalloc(sizeof(*_tmp4F9));_tmp4F9[0]=({struct Cyc_AggrFieldDeclList_tok_struct
_tmp4FA;_tmp4FA.tag=Cyc_AggrFieldDeclList_tok;_tmp4FA.f1=((struct Cyc_List_List*(*)(
struct Cyc_List_List*x))Cyc_List_flatten)(((struct Cyc_List_List*(*)(struct Cyc_List_List*
x))Cyc_List_imp_rev)(Cyc_yyget_AggrFieldDeclListList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)])));_tmp4FA;});_tmp4F9;});
break;case 105: _LL258: yyval=(void*)({struct Cyc_AggrFieldDeclListList_tok_struct*
_tmp4FB=_cycalloc(sizeof(*_tmp4FB));_tmp4FB[0]=({struct Cyc_AggrFieldDeclListList_tok_struct
_tmp4FC;_tmp4FC.tag=Cyc_AggrFieldDeclListList_tok;_tmp4FC.f1=({struct Cyc_List_List*
_tmp4FD=_cycalloc(sizeof(*_tmp4FD));_tmp4FD->hd=Cyc_yyget_AggrFieldDeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp4FD->tl=0;_tmp4FD;});
_tmp4FC;});_tmp4FB;});break;case 106: _LL259: yyval=(void*)({struct Cyc_AggrFieldDeclListList_tok_struct*
_tmp4FE=_cycalloc(sizeof(*_tmp4FE));_tmp4FE[0]=({struct Cyc_AggrFieldDeclListList_tok_struct
_tmp4FF;_tmp4FF.tag=Cyc_AggrFieldDeclListList_tok;_tmp4FF.f1=({struct Cyc_List_List*
_tmp500=_cycalloc(sizeof(*_tmp500));_tmp500->hd=Cyc_yyget_AggrFieldDeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp500->tl=Cyc_yyget_AggrFieldDeclListList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp500;});_tmp4FF;});
_tmp4FE;});break;case 107: _LL25A: yyval=(void*)({struct Cyc_InitDeclList_tok_struct*
_tmp501=_cycalloc(sizeof(*_tmp501));_tmp501[0]=({struct Cyc_InitDeclList_tok_struct
_tmp502;_tmp502.tag=Cyc_InitDeclList_tok;_tmp502.f1=((struct Cyc_List_List*(*)(
struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_InitDeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]));_tmp502;});_tmp501;});
break;case 108: _LL25B: yyval=(void*)({struct Cyc_InitDeclList_tok_struct*_tmp503=
_cycalloc(sizeof(*_tmp503));_tmp503[0]=({struct Cyc_InitDeclList_tok_struct
_tmp504;_tmp504.tag=Cyc_InitDeclList_tok;_tmp504.f1=({struct Cyc_List_List*
_tmp505=_cycalloc(sizeof(*_tmp505));_tmp505->hd=Cyc_yyget_InitDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp505->tl=0;_tmp505;});
_tmp504;});_tmp503;});break;case 109: _LL25C: yyval=(void*)({struct Cyc_InitDeclList_tok_struct*
_tmp506=_cycalloc(sizeof(*_tmp506));_tmp506[0]=({struct Cyc_InitDeclList_tok_struct
_tmp507;_tmp507.tag=Cyc_InitDeclList_tok;_tmp507.f1=({struct Cyc_List_List*
_tmp508=_cycalloc(sizeof(*_tmp508));_tmp508->hd=Cyc_yyget_InitDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp508->tl=Cyc_yyget_InitDeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp508;});_tmp507;});
_tmp506;});break;case 110: _LL25D: yyval=(void*)({struct Cyc_InitDecl_tok_struct*
_tmp509=_cycalloc(sizeof(*_tmp509));_tmp509[0]=({struct Cyc_InitDecl_tok_struct
_tmp50A;_tmp50A.tag=Cyc_InitDecl_tok;_tmp50A.f1=({struct _tuple12*_tmp50B=
_cycalloc(sizeof(*_tmp50B));_tmp50B->f1=Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp50B->f2=0;_tmp50B;});
_tmp50A;});_tmp509;});break;case 111: _LL25E: yyval=(void*)({struct Cyc_InitDecl_tok_struct*
_tmp50C=_cycalloc(sizeof(*_tmp50C));_tmp50C[0]=({struct Cyc_InitDecl_tok_struct
_tmp50D;_tmp50D.tag=Cyc_InitDecl_tok;_tmp50D.f1=({struct _tuple12*_tmp50E=
_cycalloc(sizeof(*_tmp50E));_tmp50E->f1=Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp50E->f2=(struct Cyc_Absyn_Exp*)
Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);
_tmp50E;});_tmp50D;});_tmp50C;});break;case 112: _LL25F: {struct _tuple15 _tmp510;
struct Cyc_Absyn_Tqual _tmp511;struct Cyc_List_List*_tmp512;struct Cyc_List_List*
_tmp513;struct _tuple15*_tmp50F=Cyc_yyget_QualSpecList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp510=*_tmp50F;_tmp511=
_tmp510.f1;_tmp512=_tmp510.f2;_tmp513=_tmp510.f3;{struct Cyc_List_List*_tmp515;
struct Cyc_List_List*_tmp516;struct _tuple0 _tmp514=((struct _tuple0(*)(struct Cyc_List_List*
x))Cyc_List_split)(Cyc_yyget_InitDeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]));_tmp515=_tmp514.f1;
_tmp516=_tmp514.f2;{void*_tmp517=Cyc_Parse_speclist2typ(_tmp512,Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).last_line));struct Cyc_List_List*
_tmp518=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))
Cyc_List_zip)(Cyc_Parse_apply_tmss(_tmp511,_tmp517,_tmp515,_tmp513),_tmp516);
yyval=(void*)({struct Cyc_AggrFieldDeclList_tok_struct*_tmp519=_cycalloc(sizeof(*
_tmp519));_tmp519[0]=({struct Cyc_AggrFieldDeclList_tok_struct _tmp51A;_tmp51A.tag=
Cyc_AggrFieldDeclList_tok;_tmp51A.f1=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Aggrfield*(*
f)(struct Cyc_Position_Segment*,struct _tuple8*),struct Cyc_Position_Segment*env,
struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_make_aggr_field,Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line),_tmp518);
_tmp51A;});_tmp519;});break;}}}case 113: _LL260: yyval=(void*)({struct Cyc_QualSpecList_tok_struct*
_tmp51B=_cycalloc(sizeof(*_tmp51B));_tmp51B[0]=({struct Cyc_QualSpecList_tok_struct
_tmp51C;_tmp51C.tag=Cyc_QualSpecList_tok;_tmp51C.f1=({struct _tuple15*_tmp51D=
_cycalloc(sizeof(*_tmp51D));_tmp51D->f1=Cyc_Absyn_empty_tqual();_tmp51D->f2=({
struct Cyc_List_List*_tmp51E=_cycalloc(sizeof(*_tmp51E));_tmp51E->hd=(void*)Cyc_yyget_TypeSpecifier_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp51E->tl=0;_tmp51E;});
_tmp51D->f3=0;_tmp51D;});_tmp51C;});_tmp51B;});break;case 114: _LL261: yyval=(void*)({
struct Cyc_QualSpecList_tok_struct*_tmp51F=_cycalloc(sizeof(*_tmp51F));_tmp51F[0]=({
struct Cyc_QualSpecList_tok_struct _tmp520;_tmp520.tag=Cyc_QualSpecList_tok;
_tmp520.f1=({struct _tuple15*_tmp521=_cycalloc(sizeof(*_tmp521));_tmp521->f1=(*
Cyc_yyget_QualSpecList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)])).f1;_tmp521->f2=({struct Cyc_List_List*_tmp522=_cycalloc(sizeof(*
_tmp522));_tmp522->hd=(void*)Cyc_yyget_TypeSpecifier_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp522->tl=(*Cyc_yyget_QualSpecList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)])).f2;_tmp522;});_tmp521->f3=(*
Cyc_yyget_QualSpecList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)])).f3;_tmp521;});_tmp520;});_tmp51F;});break;case 115: _LL262: yyval=(
void*)({struct Cyc_QualSpecList_tok_struct*_tmp523=_cycalloc(sizeof(*_tmp523));
_tmp523[0]=({struct Cyc_QualSpecList_tok_struct _tmp524;_tmp524.tag=Cyc_QualSpecList_tok;
_tmp524.f1=({struct _tuple15*_tmp525=_cycalloc(sizeof(*_tmp525));_tmp525->f1=Cyc_yyget_TypeQual_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp525->f2=0;_tmp525->f3=
0;_tmp525;});_tmp524;});_tmp523;});break;case 116: _LL263: yyval=(void*)({struct Cyc_QualSpecList_tok_struct*
_tmp526=_cycalloc(sizeof(*_tmp526));_tmp526[0]=({struct Cyc_QualSpecList_tok_struct
_tmp527;_tmp527.tag=Cyc_QualSpecList_tok;_tmp527.f1=({struct _tuple15*_tmp528=
_cycalloc(sizeof(*_tmp528));_tmp528->f1=Cyc_Absyn_combine_tqual(Cyc_yyget_TypeQual_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),(*Cyc_yyget_QualSpecList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)])).f1);_tmp528->f2=(*Cyc_yyget_QualSpecList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)])).f2;_tmp528->f3=(*Cyc_yyget_QualSpecList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)])).f3;_tmp528;});_tmp527;});
_tmp526;});break;case 117: _LL264: yyval=(void*)({struct Cyc_QualSpecList_tok_struct*
_tmp529=_cycalloc(sizeof(*_tmp529));_tmp529[0]=({struct Cyc_QualSpecList_tok_struct
_tmp52A;_tmp52A.tag=Cyc_QualSpecList_tok;_tmp52A.f1=({struct _tuple15*_tmp52B=
_cycalloc(sizeof(*_tmp52B));_tmp52B->f1=Cyc_Absyn_empty_tqual();_tmp52B->f2=0;
_tmp52B->f3=Cyc_yyget_AttributeList_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]);_tmp52B;});_tmp52A;});_tmp529;});break;case 118: _LL265:
yyval=(void*)({struct Cyc_QualSpecList_tok_struct*_tmp52C=_cycalloc(sizeof(*
_tmp52C));_tmp52C[0]=({struct Cyc_QualSpecList_tok_struct _tmp52D;_tmp52D.tag=Cyc_QualSpecList_tok;
_tmp52D.f1=({struct _tuple15*_tmp52E=_cycalloc(sizeof(*_tmp52E));_tmp52E->f1=(*
Cyc_yyget_QualSpecList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)])).f1;_tmp52E->f2=(*Cyc_yyget_QualSpecList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)])).f2;_tmp52E->f3=Cyc_List_append(
Cyc_yyget_AttributeList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 1)]),(*Cyc_yyget_QualSpecList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)])).f3);_tmp52E;});_tmp52D;});
_tmp52C;});break;case 119: _LL266: yyval=(void*)({struct Cyc_QualSpecList_tok_struct*
_tmp52F=_cycalloc(sizeof(*_tmp52F));_tmp52F[0]=({struct Cyc_QualSpecList_tok_struct
_tmp530;_tmp530.tag=Cyc_QualSpecList_tok;_tmp530.f1=({struct _tuple15*_tmp531=
_cycalloc(sizeof(*_tmp531));_tmp531->f1=Cyc_Absyn_empty_tqual();_tmp531->f2=({
struct Cyc_List_List*_tmp532=_cycalloc(sizeof(*_tmp532));_tmp532->hd=(void*)Cyc_yyget_TypeSpecifier_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp532->tl=0;_tmp532;});
_tmp531->f3=0;_tmp531;});_tmp530;});_tmp52F;});break;case 120: _LL267: yyval=(void*)({
struct Cyc_QualSpecList_tok_struct*_tmp533=_cycalloc(sizeof(*_tmp533));_tmp533[0]=({
struct Cyc_QualSpecList_tok_struct _tmp534;_tmp534.tag=Cyc_QualSpecList_tok;
_tmp534.f1=({struct _tuple15*_tmp535=_cycalloc(sizeof(*_tmp535));_tmp535->f1=(*
Cyc_yyget_QualSpecList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)])).f1;_tmp535->f2=({struct Cyc_List_List*_tmp536=_cycalloc(sizeof(*
_tmp536));_tmp536->hd=(void*)Cyc_yyget_TypeSpecifier_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp536->tl=(*Cyc_yyget_QualSpecList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)])).f2;_tmp536;});_tmp535->f3=(*
Cyc_yyget_QualSpecList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)])).f3;_tmp535;});_tmp534;});_tmp533;});break;case 121: _LL268: yyval=(
void*)({struct Cyc_QualSpecList_tok_struct*_tmp537=_cycalloc(sizeof(*_tmp537));
_tmp537[0]=({struct Cyc_QualSpecList_tok_struct _tmp538;_tmp538.tag=Cyc_QualSpecList_tok;
_tmp538.f1=({struct _tuple15*_tmp539=_cycalloc(sizeof(*_tmp539));_tmp539->f1=Cyc_yyget_TypeQual_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp539->f2=0;_tmp539->f3=
0;_tmp539;});_tmp538;});_tmp537;});break;case 122: _LL269: yyval=(void*)({struct Cyc_QualSpecList_tok_struct*
_tmp53A=_cycalloc(sizeof(*_tmp53A));_tmp53A[0]=({struct Cyc_QualSpecList_tok_struct
_tmp53B;_tmp53B.tag=Cyc_QualSpecList_tok;_tmp53B.f1=({struct _tuple15*_tmp53C=
_cycalloc(sizeof(*_tmp53C));_tmp53C->f1=Cyc_Absyn_combine_tqual(Cyc_yyget_TypeQual_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),(*Cyc_yyget_QualSpecList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)])).f1);_tmp53C->f2=(*Cyc_yyget_QualSpecList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)])).f2;_tmp53C->f3=(*Cyc_yyget_QualSpecList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)])).f3;_tmp53C;});_tmp53B;});
_tmp53A;});break;case 123: _LL26A: yyval=(void*)({struct Cyc_QualSpecList_tok_struct*
_tmp53D=_cycalloc(sizeof(*_tmp53D));_tmp53D[0]=({struct Cyc_QualSpecList_tok_struct
_tmp53E;_tmp53E.tag=Cyc_QualSpecList_tok;_tmp53E.f1=({struct _tuple15*_tmp53F=
_cycalloc(sizeof(*_tmp53F));_tmp53F->f1=Cyc_Absyn_empty_tqual();_tmp53F->f2=0;
_tmp53F->f3=Cyc_yyget_AttributeList_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]);_tmp53F;});_tmp53E;});_tmp53D;});break;case 124: _LL26B:
yyval=(void*)({struct Cyc_QualSpecList_tok_struct*_tmp540=_cycalloc(sizeof(*
_tmp540));_tmp540[0]=({struct Cyc_QualSpecList_tok_struct _tmp541;_tmp541.tag=Cyc_QualSpecList_tok;
_tmp541.f1=({struct _tuple15*_tmp542=_cycalloc(sizeof(*_tmp542));_tmp542->f1=(*
Cyc_yyget_QualSpecList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)])).f1;_tmp542->f2=(*Cyc_yyget_QualSpecList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)])).f2;_tmp542->f3=Cyc_List_append(
Cyc_yyget_AttributeList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 1)]),(*Cyc_yyget_QualSpecList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)])).f3);_tmp542;});_tmp541;});
_tmp540;});break;case 125: _LL26C: yyval=(void*)({struct Cyc_InitDeclList_tok_struct*
_tmp543=_cycalloc(sizeof(*_tmp543));_tmp543[0]=({struct Cyc_InitDeclList_tok_struct
_tmp544;_tmp544.tag=Cyc_InitDeclList_tok;_tmp544.f1=((struct Cyc_List_List*(*)(
struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_InitDeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]));_tmp544;});_tmp543;});
break;case 126: _LL26D: yyval=(void*)({struct Cyc_InitDeclList_tok_struct*_tmp545=
_cycalloc(sizeof(*_tmp545));_tmp545[0]=({struct Cyc_InitDeclList_tok_struct
_tmp546;_tmp546.tag=Cyc_InitDeclList_tok;_tmp546.f1=({struct Cyc_List_List*
_tmp547=_cycalloc(sizeof(*_tmp547));_tmp547->hd=Cyc_yyget_InitDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp547->tl=0;_tmp547;});
_tmp546;});_tmp545;});break;case 127: _LL26E: yyval=(void*)({struct Cyc_InitDeclList_tok_struct*
_tmp548=_cycalloc(sizeof(*_tmp548));_tmp548[0]=({struct Cyc_InitDeclList_tok_struct
_tmp549;_tmp549.tag=Cyc_InitDeclList_tok;_tmp549.f1=({struct Cyc_List_List*
_tmp54A=_cycalloc(sizeof(*_tmp54A));_tmp54A->hd=Cyc_yyget_InitDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp54A->tl=Cyc_yyget_InitDeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp54A;});_tmp549;});
_tmp548;});break;case 128: _LL26F: yyval=(void*)({struct Cyc_InitDecl_tok_struct*
_tmp54B=_cycalloc(sizeof(*_tmp54B));_tmp54B[0]=({struct Cyc_InitDecl_tok_struct
_tmp54C;_tmp54C.tag=Cyc_InitDecl_tok;_tmp54C.f1=({struct _tuple12*_tmp54D=
_cycalloc(sizeof(*_tmp54D));_tmp54D->f1=Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp54D->f2=0;_tmp54D;});
_tmp54C;});_tmp54B;});break;case 129: _LL270: yyval=(void*)({struct Cyc_InitDecl_tok_struct*
_tmp54E=_cycalloc(sizeof(*_tmp54E));_tmp54E[0]=({struct Cyc_InitDecl_tok_struct
_tmp54F;_tmp54F.tag=Cyc_InitDecl_tok;_tmp54F.f1=({struct _tuple12*_tmp550=
_cycalloc(sizeof(*_tmp550));_tmp550->f1=({struct Cyc_Parse_Declarator*_tmp551=
_cycalloc(sizeof(*_tmp551));_tmp551->id=({struct _tuple1*_tmp552=_cycalloc(
sizeof(*_tmp552));_tmp552->f1=Cyc_Absyn_rel_ns_null;_tmp552->f2=_init_tag_arr(
_cycalloc(sizeof(struct _tagged_arr)),"",sizeof(char),1);_tmp552;});_tmp551->tms=
0;_tmp551;});_tmp550->f2=(struct Cyc_Absyn_Exp*)Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp550;});_tmp54F;});
_tmp54E;});break;case 130: _LL271: yyval=(void*)({struct Cyc_InitDecl_tok_struct*
_tmp554=_cycalloc(sizeof(*_tmp554));_tmp554[0]=({struct Cyc_InitDecl_tok_struct
_tmp555;_tmp555.tag=Cyc_InitDecl_tok;_tmp555.f1=({struct _tuple12*_tmp556=
_cycalloc(sizeof(*_tmp556));_tmp556->f1=Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp556->f2=(struct Cyc_Absyn_Exp*)
Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);
_tmp556;});_tmp555;});_tmp554;});break;case 131: _LL272: {struct Cyc_List_List*
_tmp557=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Position_Segment*,
void*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).last_line),Cyc_yyget_TypeList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]));yyval=(void*)({struct
Cyc_TypeSpecifier_tok_struct*_tmp558=_cycalloc(sizeof(*_tmp558));_tmp558[0]=({
struct Cyc_TypeSpecifier_tok_struct _tmp559;_tmp559.tag=Cyc_TypeSpecifier_tok;
_tmp559.f1=(void*)((void*)({struct Cyc_Parse_Decl_spec_struct*_tmp55A=_cycalloc(
sizeof(*_tmp55A));_tmp55A[0]=({struct Cyc_Parse_Decl_spec_struct _tmp55B;_tmp55B.tag=
5;_tmp55B.f1=Cyc_Absyn_tunion_decl((void*)2,Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]),_tmp557,({struct Cyc_Core_Opt*
_tmp55C=_cycalloc(sizeof(*_tmp55C));_tmp55C->v=Cyc_yyget_TunionFieldList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp55C;}),Cyc_yyget_Bool_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 6)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 6)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp55B;});
_tmp55A;}));_tmp559;});_tmp558;});break;}case 132: _LL273: yyval=(void*)({struct Cyc_TypeSpecifier_tok_struct*
_tmp55D=_cycalloc(sizeof(*_tmp55D));_tmp55D[0]=({struct Cyc_TypeSpecifier_tok_struct
_tmp55E;_tmp55E.tag=Cyc_TypeSpecifier_tok;_tmp55E.f1=(void*)Cyc_Parse_type_spec((
void*)({struct Cyc_Absyn_TunionType_struct*_tmp55F=_cycalloc(sizeof(*_tmp55F));
_tmp55F[0]=({struct Cyc_Absyn_TunionType_struct _tmp560;_tmp560.tag=2;_tmp560.f1=({
struct Cyc_Absyn_TunionInfo _tmp561;_tmp561.tunion_info=(void*)((void*)({struct Cyc_Absyn_UnknownTunion_struct*
_tmp562=_cycalloc(sizeof(*_tmp562));_tmp562[0]=({struct Cyc_Absyn_UnknownTunion_struct
_tmp563;_tmp563.tag=0;_tmp563.f1=({struct Cyc_Absyn_UnknownTunionInfo _tmp564;
_tmp564.name=Cyc_yyget_QualId_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 1)]);_tmp564.is_xtunion=Cyc_yyget_Bool_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp564;});_tmp563;});
_tmp562;}));_tmp561.targs=Cyc_yyget_TypeList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp561.rgn=(void*)Cyc_yyget_Type_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp561;});_tmp560;});
_tmp55F;}),Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(
10000,yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp55E;});_tmp55D;});break;case 133: _LL274: yyval=(
void*)({struct Cyc_TypeSpecifier_tok_struct*_tmp565=_cycalloc(sizeof(*_tmp565));
_tmp565[0]=({struct Cyc_TypeSpecifier_tok_struct _tmp566;_tmp566.tag=Cyc_TypeSpecifier_tok;
_tmp566.f1=(void*)Cyc_Parse_type_spec((void*)({struct Cyc_Absyn_TunionFieldType_struct*
_tmp567=_cycalloc(sizeof(*_tmp567));_tmp567[0]=({struct Cyc_Absyn_TunionFieldType_struct
_tmp568;_tmp568.tag=3;_tmp568.f1=({struct Cyc_Absyn_TunionFieldInfo _tmp569;
_tmp569.field_info=(void*)((void*)({struct Cyc_Absyn_UnknownTunionfield_struct*
_tmp56A=_cycalloc(sizeof(*_tmp56A));_tmp56A[0]=({struct Cyc_Absyn_UnknownTunionfield_struct
_tmp56B;_tmp56B.tag=0;_tmp56B.f1=({struct Cyc_Absyn_UnknownTunionFieldInfo _tmp56C;
_tmp56C.tunion_name=Cyc_yyget_QualId_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 3)]);_tmp56C.field_name=Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp56C.is_xtunion=Cyc_yyget_Bool_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 5)]);_tmp56C;});_tmp56B;});
_tmp56A;}));_tmp569.targs=Cyc_yyget_TypeList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp569;});_tmp568;});
_tmp567;}),Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(
10000,yylsp_offset - 5)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp566;});_tmp565;});break;case 134: _LL275: yyval=(
void*)({struct Cyc_Bool_tok_struct*_tmp56D=_cycalloc_atomic(sizeof(*_tmp56D));
_tmp56D[0]=({struct Cyc_Bool_tok_struct _tmp56E;_tmp56E.tag=Cyc_Bool_tok;_tmp56E.f1=
0;_tmp56E;});_tmp56D;});break;case 135: _LL276: yyval=(void*)({struct Cyc_Bool_tok_struct*
_tmp56F=_cycalloc_atomic(sizeof(*_tmp56F));_tmp56F[0]=({struct Cyc_Bool_tok_struct
_tmp570;_tmp570.tag=Cyc_Bool_tok;_tmp570.f1=1;_tmp570;});_tmp56F;});break;case
136: _LL277: yyval=(void*)({struct Cyc_TunionFieldList_tok_struct*_tmp571=_cycalloc(
sizeof(*_tmp571));_tmp571[0]=({struct Cyc_TunionFieldList_tok_struct _tmp572;
_tmp572.tag=Cyc_TunionFieldList_tok;_tmp572.f1=({struct Cyc_List_List*_tmp573=
_cycalloc(sizeof(*_tmp573));_tmp573->hd=Cyc_yyget_TunionField_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp573->tl=0;_tmp573;});
_tmp572;});_tmp571;});break;case 137: _LL278: yyval=(void*)({struct Cyc_TunionFieldList_tok_struct*
_tmp574=_cycalloc(sizeof(*_tmp574));_tmp574[0]=({struct Cyc_TunionFieldList_tok_struct
_tmp575;_tmp575.tag=Cyc_TunionFieldList_tok;_tmp575.f1=({struct Cyc_List_List*
_tmp576=_cycalloc(sizeof(*_tmp576));_tmp576->hd=Cyc_yyget_TunionField_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp576->tl=0;_tmp576;});
_tmp575;});_tmp574;});break;case 138: _LL279: yyval=(void*)({struct Cyc_TunionFieldList_tok_struct*
_tmp577=_cycalloc(sizeof(*_tmp577));_tmp577[0]=({struct Cyc_TunionFieldList_tok_struct
_tmp578;_tmp578.tag=Cyc_TunionFieldList_tok;_tmp578.f1=({struct Cyc_List_List*
_tmp579=_cycalloc(sizeof(*_tmp579));_tmp579->hd=Cyc_yyget_TunionField_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp579->tl=Cyc_yyget_TunionFieldList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp579;});_tmp578;});
_tmp577;});break;case 139: _LL27A: yyval=(void*)({struct Cyc_TunionFieldList_tok_struct*
_tmp57A=_cycalloc(sizeof(*_tmp57A));_tmp57A[0]=({struct Cyc_TunionFieldList_tok_struct
_tmp57B;_tmp57B.tag=Cyc_TunionFieldList_tok;_tmp57B.f1=({struct Cyc_List_List*
_tmp57C=_cycalloc(sizeof(*_tmp57C));_tmp57C->hd=Cyc_yyget_TunionField_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp57C->tl=Cyc_yyget_TunionFieldList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp57C;});_tmp57B;});
_tmp57A;});break;case 140: _LL27B: yyval=(void*)({struct Cyc_Scope_tok_struct*
_tmp57D=_cycalloc(sizeof(*_tmp57D));_tmp57D[0]=({struct Cyc_Scope_tok_struct
_tmp57E;_tmp57E.tag=Cyc_Scope_tok;_tmp57E.f1=(void*)((void*)2);_tmp57E;});
_tmp57D;});break;case 141: _LL27C: yyval=(void*)({struct Cyc_Scope_tok_struct*
_tmp57F=_cycalloc(sizeof(*_tmp57F));_tmp57F[0]=({struct Cyc_Scope_tok_struct
_tmp580;_tmp580.tag=Cyc_Scope_tok;_tmp580.f1=(void*)((void*)3);_tmp580;});
_tmp57F;});break;case 142: _LL27D: yyval=(void*)({struct Cyc_Scope_tok_struct*
_tmp581=_cycalloc(sizeof(*_tmp581));_tmp581[0]=({struct Cyc_Scope_tok_struct
_tmp582;_tmp582.tag=Cyc_Scope_tok;_tmp582.f1=(void*)((void*)0);_tmp582;});
_tmp581;});break;case 143: _LL27E: yyval=(void*)({struct Cyc_TunionField_tok_struct*
_tmp583=_cycalloc(sizeof(*_tmp583));_tmp583[0]=({struct Cyc_TunionField_tok_struct
_tmp584;_tmp584.tag=Cyc_TunionField_tok;_tmp584.f1=({struct Cyc_Absyn_Tunionfield*
_tmp585=_cycalloc(sizeof(*_tmp585));_tmp585->name=Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp585->typs=0;_tmp585->loc=
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line);_tmp585->sc=(void*)Cyc_yyget_Scope_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp585;});_tmp584;});
_tmp583;});break;case 144: _LL27F: {struct Cyc_List_List*_tmp586=((struct Cyc_List_List*(*)(
struct _tuple9*(*f)(struct Cyc_Position_Segment*,struct _tuple2*),struct Cyc_Position_Segment*
env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_get_tqual_typ,Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line),((struct Cyc_List_List*(*)(
struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_ParamDeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)])));yyval=(void*)({struct
Cyc_TunionField_tok_struct*_tmp587=_cycalloc(sizeof(*_tmp587));_tmp587[0]=({
struct Cyc_TunionField_tok_struct _tmp588;_tmp588.tag=Cyc_TunionField_tok;_tmp588.f1=({
struct Cyc_Absyn_Tunionfield*_tmp589=_cycalloc(sizeof(*_tmp589));_tmp589->name=
Cyc_yyget_QualId_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);
_tmp589->typs=_tmp586;_tmp589->loc=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp589->sc=(
void*)Cyc_yyget_Scope_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset
- 4)]);_tmp589;});_tmp588;});_tmp587;});break;}case 145: _LL280: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 146: _LL281: yyval=(
void*)({struct Cyc_Declarator_tok_struct*_tmp58A=_cycalloc(sizeof(*_tmp58A));
_tmp58A[0]=({struct Cyc_Declarator_tok_struct _tmp58B;_tmp58B.tag=Cyc_Declarator_tok;
_tmp58B.f1=({struct Cyc_Parse_Declarator*_tmp58C=_cycalloc(sizeof(*_tmp58C));
_tmp58C->id=(Cyc_yyget_Declarator_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]))->id;_tmp58C->tms=Cyc_List_imp_append(Cyc_yyget_TypeModifierList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),(Cyc_yyget_Declarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->tms);_tmp58C;});
_tmp58B;});_tmp58A;});break;case 147: _LL282: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 148: _LL283: yyval=(
void*)({struct Cyc_Declarator_tok_struct*_tmp58D=_cycalloc(sizeof(*_tmp58D));
_tmp58D[0]=({struct Cyc_Declarator_tok_struct _tmp58E;_tmp58E.tag=Cyc_Declarator_tok;
_tmp58E.f1=({struct Cyc_Parse_Declarator*_tmp58F=_cycalloc(sizeof(*_tmp58F));
_tmp58F->id=(Cyc_yyget_Declarator_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]))->id;_tmp58F->tms=Cyc_List_imp_append(Cyc_yyget_TypeModifierList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),(Cyc_yyget_Declarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->tms);_tmp58F;});
_tmp58E;});_tmp58D;});break;case 149: _LL284: yyval=(void*)({struct Cyc_Declarator_tok_struct*
_tmp590=_cycalloc(sizeof(*_tmp590));_tmp590[0]=({struct Cyc_Declarator_tok_struct
_tmp591;_tmp591.tag=Cyc_Declarator_tok;_tmp591.f1=({struct Cyc_Parse_Declarator*
_tmp592=_cycalloc(sizeof(*_tmp592));_tmp592->id=Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp592->tms=0;_tmp592;});
_tmp591;});_tmp590;});break;case 150: _LL285: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)];break;case 151: _LL286: {
struct Cyc_Parse_Declarator*_tmp593=Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp593->tms=({struct Cyc_List_List*
_tmp594=_cycalloc(sizeof(*_tmp594));_tmp594->hd=(void*)((void*)({struct Cyc_Absyn_Attributes_mod_struct*
_tmp595=_cycalloc(sizeof(*_tmp595));_tmp595[0]=({struct Cyc_Absyn_Attributes_mod_struct
_tmp596;_tmp596.tag=5;_tmp596.f1=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).last_line);_tmp596.f2=
Cyc_yyget_AttributeList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 2)]);_tmp596;});_tmp595;}));_tmp594->tl=_tmp593->tms;_tmp594;});
yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)];break;}case 152:
_LL287: yyval=(void*)({struct Cyc_Declarator_tok_struct*_tmp597=_cycalloc(sizeof(*
_tmp597));_tmp597[0]=({struct Cyc_Declarator_tok_struct _tmp598;_tmp598.tag=Cyc_Declarator_tok;
_tmp598.f1=({struct Cyc_Parse_Declarator*_tmp599=_cycalloc(sizeof(*_tmp599));
_tmp599->id=(Cyc_yyget_Declarator_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 3)]))->id;_tmp599->tms=({struct Cyc_List_List*_tmp59A=_cycalloc(
sizeof(*_tmp59A));_tmp59A->hd=(void*)((void*)({struct Cyc_Absyn_Carray_mod_struct*
_tmp59B=_cycalloc(sizeof(*_tmp59B));_tmp59B[0]=({struct Cyc_Absyn_Carray_mod_struct
_tmp59C;_tmp59C.tag=0;_tmp59C.f1=Cyc_yyget_YY4(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp59C;});_tmp59B;}));
_tmp59A->tl=(Cyc_yyget_Declarator_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 3)]))->tms;_tmp59A;});_tmp599;});_tmp598;});_tmp597;});break;case
153: _LL288: yyval=(void*)({struct Cyc_Declarator_tok_struct*_tmp59D=_cycalloc(
sizeof(*_tmp59D));_tmp59D[0]=({struct Cyc_Declarator_tok_struct _tmp59E;_tmp59E.tag=
Cyc_Declarator_tok;_tmp59E.f1=({struct Cyc_Parse_Declarator*_tmp59F=_cycalloc(
sizeof(*_tmp59F));_tmp59F->id=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]))->id;_tmp59F->tms=({
struct Cyc_List_List*_tmp5A0=_cycalloc(sizeof(*_tmp5A0));_tmp5A0->hd=(void*)((
void*)({struct Cyc_Absyn_ConstArray_mod_struct*_tmp5A1=_cycalloc(sizeof(*_tmp5A1));
_tmp5A1[0]=({struct Cyc_Absyn_ConstArray_mod_struct _tmp5A2;_tmp5A2.tag=1;_tmp5A2.f1=
Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);
_tmp5A2.f2=Cyc_yyget_YY4(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);
_tmp5A2;});_tmp5A1;}));_tmp5A0->tl=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]))->tms;_tmp5A0;});_tmp59F;});
_tmp59E;});_tmp59D;});break;case 154: _LL289: {struct _tuple14 _tmp5A4;struct Cyc_List_List*
_tmp5A5;int _tmp5A6;struct Cyc_Absyn_VarargInfo*_tmp5A7;struct Cyc_Core_Opt*_tmp5A8;
struct Cyc_List_List*_tmp5A9;struct _tuple14*_tmp5A3=Cyc_yyget_YY3(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp5A4=*_tmp5A3;_tmp5A5=
_tmp5A4.f1;_tmp5A6=_tmp5A4.f2;_tmp5A7=_tmp5A4.f3;_tmp5A8=_tmp5A4.f4;_tmp5A9=
_tmp5A4.f5;yyval=(void*)({struct Cyc_Declarator_tok_struct*_tmp5AA=_cycalloc(
sizeof(*_tmp5AA));_tmp5AA[0]=({struct Cyc_Declarator_tok_struct _tmp5AB;_tmp5AB.tag=
Cyc_Declarator_tok;_tmp5AB.f1=({struct Cyc_Parse_Declarator*_tmp5AC=_cycalloc(
sizeof(*_tmp5AC));_tmp5AC->id=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]))->id;_tmp5AC->tms=({
struct Cyc_List_List*_tmp5AD=_cycalloc(sizeof(*_tmp5AD));_tmp5AD->hd=(void*)((
void*)({struct Cyc_Absyn_Function_mod_struct*_tmp5AE=_cycalloc(sizeof(*_tmp5AE));
_tmp5AE[0]=({struct Cyc_Absyn_Function_mod_struct _tmp5AF;_tmp5AF.tag=3;_tmp5AF.f1=(
void*)((void*)({struct Cyc_Absyn_WithTypes_struct*_tmp5B0=_cycalloc(sizeof(*
_tmp5B0));_tmp5B0[0]=({struct Cyc_Absyn_WithTypes_struct _tmp5B1;_tmp5B1.tag=1;
_tmp5B1.f1=_tmp5A5;_tmp5B1.f2=_tmp5A6;_tmp5B1.f3=_tmp5A7;_tmp5B1.f4=_tmp5A8;
_tmp5B1.f5=_tmp5A9;_tmp5B1;});_tmp5B0;}));_tmp5AF;});_tmp5AE;}));_tmp5AD->tl=(
Cyc_yyget_Declarator_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 
3)]))->tms;_tmp5AD;});_tmp5AC;});_tmp5AB;});_tmp5AA;});break;}case 155: _LL28A:
yyval=(void*)({struct Cyc_Declarator_tok_struct*_tmp5B2=_cycalloc(sizeof(*_tmp5B2));
_tmp5B2[0]=({struct Cyc_Declarator_tok_struct _tmp5B3;_tmp5B3.tag=Cyc_Declarator_tok;
_tmp5B3.f1=({struct Cyc_Parse_Declarator*_tmp5B4=_cycalloc(sizeof(*_tmp5B4));
_tmp5B4->id=(Cyc_yyget_Declarator_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 4)]))->id;_tmp5B4->tms=({struct Cyc_List_List*_tmp5B5=_cycalloc(
sizeof(*_tmp5B5));_tmp5B5->hd=(void*)((void*)({struct Cyc_Absyn_Function_mod_struct*
_tmp5B6=_cycalloc(sizeof(*_tmp5B6));_tmp5B6[0]=({struct Cyc_Absyn_Function_mod_struct
_tmp5B7;_tmp5B7.tag=3;_tmp5B7.f1=(void*)((void*)({struct Cyc_Absyn_WithTypes_struct*
_tmp5B8=_cycalloc(sizeof(*_tmp5B8));_tmp5B8[0]=({struct Cyc_Absyn_WithTypes_struct
_tmp5B9;_tmp5B9.tag=1;_tmp5B9.f1=0;_tmp5B9.f2=0;_tmp5B9.f3=0;_tmp5B9.f4=Cyc_yyget_TypeOpt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp5B9.f5=Cyc_yyget_Rgnorder_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp5B9;});_tmp5B8;}));
_tmp5B7;});_tmp5B6;}));_tmp5B5->tl=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]))->tms;_tmp5B5;});_tmp5B4;});
_tmp5B3;});_tmp5B2;});break;case 156: _LL28B: yyval=(void*)({struct Cyc_Declarator_tok_struct*
_tmp5BA=_cycalloc(sizeof(*_tmp5BA));_tmp5BA[0]=({struct Cyc_Declarator_tok_struct
_tmp5BB;_tmp5BB.tag=Cyc_Declarator_tok;_tmp5BB.f1=({struct Cyc_Parse_Declarator*
_tmp5BC=_cycalloc(sizeof(*_tmp5BC));_tmp5BC->id=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]))->id;_tmp5BC->tms=({
struct Cyc_List_List*_tmp5BD=_cycalloc(sizeof(*_tmp5BD));_tmp5BD->hd=(void*)((
void*)({struct Cyc_Absyn_Function_mod_struct*_tmp5BE=_cycalloc(sizeof(*_tmp5BE));
_tmp5BE[0]=({struct Cyc_Absyn_Function_mod_struct _tmp5BF;_tmp5BF.tag=3;_tmp5BF.f1=(
void*)((void*)({struct Cyc_Absyn_NoTypes_struct*_tmp5C0=_cycalloc(sizeof(*_tmp5C0));
_tmp5C0[0]=({struct Cyc_Absyn_NoTypes_struct _tmp5C1;_tmp5C1.tag=0;_tmp5C1.f1=Cyc_yyget_IdList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp5C1.f2=Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp5C1;});
_tmp5C0;}));_tmp5BF;});_tmp5BE;}));_tmp5BD->tl=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]))->tms;_tmp5BD;});_tmp5BC;});
_tmp5BB;});_tmp5BA;});break;case 157: _LL28C: {struct Cyc_List_List*_tmp5C2=((
struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Position_Segment*,
void*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 2)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line),Cyc_List_imp_rev(Cyc_yyget_TypeList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)])));yyval=(void*)({struct
Cyc_Declarator_tok_struct*_tmp5C3=_cycalloc(sizeof(*_tmp5C3));_tmp5C3[0]=({
struct Cyc_Declarator_tok_struct _tmp5C4;_tmp5C4.tag=Cyc_Declarator_tok;_tmp5C4.f1=({
struct Cyc_Parse_Declarator*_tmp5C5=_cycalloc(sizeof(*_tmp5C5));_tmp5C5->id=(Cyc_yyget_Declarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]))->id;_tmp5C5->tms=({
struct Cyc_List_List*_tmp5C6=_cycalloc(sizeof(*_tmp5C6));_tmp5C6->hd=(void*)((
void*)({struct Cyc_Absyn_TypeParams_mod_struct*_tmp5C7=_cycalloc(sizeof(*_tmp5C7));
_tmp5C7[0]=({struct Cyc_Absyn_TypeParams_mod_struct _tmp5C8;_tmp5C8.tag=4;_tmp5C8.f1=
_tmp5C2;_tmp5C8.f2=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp5C8.f3=0;
_tmp5C8;});_tmp5C7;}));_tmp5C6->tl=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]))->tms;_tmp5C6;});_tmp5C5;});
_tmp5C4;});_tmp5C3;});break;}case 158: _LL28D: yyval=(void*)({struct Cyc_Declarator_tok_struct*
_tmp5C9=_cycalloc(sizeof(*_tmp5C9));_tmp5C9[0]=({struct Cyc_Declarator_tok_struct
_tmp5CA;_tmp5CA.tag=Cyc_Declarator_tok;_tmp5CA.f1=({struct Cyc_Parse_Declarator*
_tmp5CB=_cycalloc(sizeof(*_tmp5CB));_tmp5CB->id=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]))->id;_tmp5CB->tms=({
struct Cyc_List_List*_tmp5CC=_cycalloc(sizeof(*_tmp5CC));_tmp5CC->hd=(void*)((
void*)({struct Cyc_Absyn_Attributes_mod_struct*_tmp5CD=_cycalloc(sizeof(*_tmp5CD));
_tmp5CD[0]=({struct Cyc_Absyn_Attributes_mod_struct _tmp5CE;_tmp5CE.tag=5;_tmp5CE.f1=
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line);_tmp5CE.f2=Cyc_yyget_AttributeList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp5CE;});_tmp5CD;}));
_tmp5CC->tl=(Cyc_yyget_Declarator_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 1)]))->tms;_tmp5CC;});_tmp5CB;});_tmp5CA;});_tmp5C9;});break;case
159: _LL28E: yyval=(void*)({struct Cyc_Declarator_tok_struct*_tmp5CF=_cycalloc(
sizeof(*_tmp5CF));_tmp5CF[0]=({struct Cyc_Declarator_tok_struct _tmp5D0;_tmp5D0.tag=
Cyc_Declarator_tok;_tmp5D0.f1=({struct Cyc_Parse_Declarator*_tmp5D1=_cycalloc(
sizeof(*_tmp5D1));_tmp5D1->id=Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp5D1->tms=0;_tmp5D1;});
_tmp5D0;});_tmp5CF;});break;case 160: _LL28F: yyval=(void*)({struct Cyc_Declarator_tok_struct*
_tmp5D2=_cycalloc(sizeof(*_tmp5D2));_tmp5D2[0]=({struct Cyc_Declarator_tok_struct
_tmp5D3;_tmp5D3.tag=Cyc_Declarator_tok;_tmp5D3.f1=({struct Cyc_Parse_Declarator*
_tmp5D4=_cycalloc(sizeof(*_tmp5D4));_tmp5D4->id=Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp5D4->tms=0;_tmp5D4;});
_tmp5D3;});_tmp5D2;});break;case 161: _LL290: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)];break;case 162: _LL291: {
struct Cyc_Parse_Declarator*_tmp5D5=Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp5D5->tms=({struct Cyc_List_List*
_tmp5D6=_cycalloc(sizeof(*_tmp5D6));_tmp5D6->hd=(void*)((void*)({struct Cyc_Absyn_Attributes_mod_struct*
_tmp5D7=_cycalloc(sizeof(*_tmp5D7));_tmp5D7[0]=({struct Cyc_Absyn_Attributes_mod_struct
_tmp5D8;_tmp5D8.tag=5;_tmp5D8.f1=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).last_line);_tmp5D8.f2=
Cyc_yyget_AttributeList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 2)]);_tmp5D8;});_tmp5D7;}));_tmp5D6->tl=_tmp5D5->tms;_tmp5D6;});
yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)];break;}case 163:
_LL292: yyval=(void*)({struct Cyc_Declarator_tok_struct*_tmp5D9=_cycalloc(sizeof(*
_tmp5D9));_tmp5D9[0]=({struct Cyc_Declarator_tok_struct _tmp5DA;_tmp5DA.tag=Cyc_Declarator_tok;
_tmp5DA.f1=({struct Cyc_Parse_Declarator*_tmp5DB=_cycalloc(sizeof(*_tmp5DB));
_tmp5DB->id=(Cyc_yyget_Declarator_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 3)]))->id;_tmp5DB->tms=({struct Cyc_List_List*_tmp5DC=_cycalloc(
sizeof(*_tmp5DC));_tmp5DC->hd=(void*)((void*)({struct Cyc_Absyn_Carray_mod_struct*
_tmp5DD=_cycalloc(sizeof(*_tmp5DD));_tmp5DD[0]=({struct Cyc_Absyn_Carray_mod_struct
_tmp5DE;_tmp5DE.tag=0;_tmp5DE.f1=Cyc_yyget_YY4(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp5DE;});_tmp5DD;}));
_tmp5DC->tl=(Cyc_yyget_Declarator_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 3)]))->tms;_tmp5DC;});_tmp5DB;});_tmp5DA;});_tmp5D9;});break;case
164: _LL293: yyval=(void*)({struct Cyc_Declarator_tok_struct*_tmp5DF=_cycalloc(
sizeof(*_tmp5DF));_tmp5DF[0]=({struct Cyc_Declarator_tok_struct _tmp5E0;_tmp5E0.tag=
Cyc_Declarator_tok;_tmp5E0.f1=({struct Cyc_Parse_Declarator*_tmp5E1=_cycalloc(
sizeof(*_tmp5E1));_tmp5E1->id=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]))->id;_tmp5E1->tms=({
struct Cyc_List_List*_tmp5E2=_cycalloc(sizeof(*_tmp5E2));_tmp5E2->hd=(void*)((
void*)({struct Cyc_Absyn_ConstArray_mod_struct*_tmp5E3=_cycalloc(sizeof(*_tmp5E3));
_tmp5E3[0]=({struct Cyc_Absyn_ConstArray_mod_struct _tmp5E4;_tmp5E4.tag=1;_tmp5E4.f1=
Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);
_tmp5E4.f2=Cyc_yyget_YY4(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);
_tmp5E4;});_tmp5E3;}));_tmp5E2->tl=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]))->tms;_tmp5E2;});_tmp5E1;});
_tmp5E0;});_tmp5DF;});break;case 165: _LL294: {struct _tuple14 _tmp5E6;struct Cyc_List_List*
_tmp5E7;int _tmp5E8;struct Cyc_Absyn_VarargInfo*_tmp5E9;struct Cyc_Core_Opt*_tmp5EA;
struct Cyc_List_List*_tmp5EB;struct _tuple14*_tmp5E5=Cyc_yyget_YY3(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp5E6=*_tmp5E5;_tmp5E7=
_tmp5E6.f1;_tmp5E8=_tmp5E6.f2;_tmp5E9=_tmp5E6.f3;_tmp5EA=_tmp5E6.f4;_tmp5EB=
_tmp5E6.f5;yyval=(void*)({struct Cyc_Declarator_tok_struct*_tmp5EC=_cycalloc(
sizeof(*_tmp5EC));_tmp5EC[0]=({struct Cyc_Declarator_tok_struct _tmp5ED;_tmp5ED.tag=
Cyc_Declarator_tok;_tmp5ED.f1=({struct Cyc_Parse_Declarator*_tmp5EE=_cycalloc(
sizeof(*_tmp5EE));_tmp5EE->id=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]))->id;_tmp5EE->tms=({
struct Cyc_List_List*_tmp5EF=_cycalloc(sizeof(*_tmp5EF));_tmp5EF->hd=(void*)((
void*)({struct Cyc_Absyn_Function_mod_struct*_tmp5F0=_cycalloc(sizeof(*_tmp5F0));
_tmp5F0[0]=({struct Cyc_Absyn_Function_mod_struct _tmp5F1;_tmp5F1.tag=3;_tmp5F1.f1=(
void*)((void*)({struct Cyc_Absyn_WithTypes_struct*_tmp5F2=_cycalloc(sizeof(*
_tmp5F2));_tmp5F2[0]=({struct Cyc_Absyn_WithTypes_struct _tmp5F3;_tmp5F3.tag=1;
_tmp5F3.f1=_tmp5E7;_tmp5F3.f2=_tmp5E8;_tmp5F3.f3=_tmp5E9;_tmp5F3.f4=_tmp5EA;
_tmp5F3.f5=_tmp5EB;_tmp5F3;});_tmp5F2;}));_tmp5F1;});_tmp5F0;}));_tmp5EF->tl=(
Cyc_yyget_Declarator_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 
3)]))->tms;_tmp5EF;});_tmp5EE;});_tmp5ED;});_tmp5EC;});break;}case 166: _LL295:
yyval=(void*)({struct Cyc_Declarator_tok_struct*_tmp5F4=_cycalloc(sizeof(*_tmp5F4));
_tmp5F4[0]=({struct Cyc_Declarator_tok_struct _tmp5F5;_tmp5F5.tag=Cyc_Declarator_tok;
_tmp5F5.f1=({struct Cyc_Parse_Declarator*_tmp5F6=_cycalloc(sizeof(*_tmp5F6));
_tmp5F6->id=(Cyc_yyget_Declarator_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 4)]))->id;_tmp5F6->tms=({struct Cyc_List_List*_tmp5F7=_cycalloc(
sizeof(*_tmp5F7));_tmp5F7->hd=(void*)((void*)({struct Cyc_Absyn_Function_mod_struct*
_tmp5F8=_cycalloc(sizeof(*_tmp5F8));_tmp5F8[0]=({struct Cyc_Absyn_Function_mod_struct
_tmp5F9;_tmp5F9.tag=3;_tmp5F9.f1=(void*)((void*)({struct Cyc_Absyn_WithTypes_struct*
_tmp5FA=_cycalloc(sizeof(*_tmp5FA));_tmp5FA[0]=({struct Cyc_Absyn_WithTypes_struct
_tmp5FB;_tmp5FB.tag=1;_tmp5FB.f1=0;_tmp5FB.f2=0;_tmp5FB.f3=0;_tmp5FB.f4=Cyc_yyget_TypeOpt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp5FB.f5=Cyc_yyget_Rgnorder_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp5FB;});_tmp5FA;}));
_tmp5F9;});_tmp5F8;}));_tmp5F7->tl=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]))->tms;_tmp5F7;});_tmp5F6;});
_tmp5F5;});_tmp5F4;});break;case 167: _LL296: yyval=(void*)({struct Cyc_Declarator_tok_struct*
_tmp5FC=_cycalloc(sizeof(*_tmp5FC));_tmp5FC[0]=({struct Cyc_Declarator_tok_struct
_tmp5FD;_tmp5FD.tag=Cyc_Declarator_tok;_tmp5FD.f1=({struct Cyc_Parse_Declarator*
_tmp5FE=_cycalloc(sizeof(*_tmp5FE));_tmp5FE->id=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]))->id;_tmp5FE->tms=({
struct Cyc_List_List*_tmp5FF=_cycalloc(sizeof(*_tmp5FF));_tmp5FF->hd=(void*)((
void*)({struct Cyc_Absyn_Function_mod_struct*_tmp600=_cycalloc(sizeof(*_tmp600));
_tmp600[0]=({struct Cyc_Absyn_Function_mod_struct _tmp601;_tmp601.tag=3;_tmp601.f1=(
void*)((void*)({struct Cyc_Absyn_NoTypes_struct*_tmp602=_cycalloc(sizeof(*_tmp602));
_tmp602[0]=({struct Cyc_Absyn_NoTypes_struct _tmp603;_tmp603.tag=0;_tmp603.f1=Cyc_yyget_IdList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp603.f2=Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp603;});
_tmp602;}));_tmp601;});_tmp600;}));_tmp5FF->tl=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]))->tms;_tmp5FF;});_tmp5FE;});
_tmp5FD;});_tmp5FC;});break;case 168: _LL297: {struct Cyc_List_List*_tmp604=((
struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Position_Segment*,
void*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 2)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line),Cyc_List_imp_rev(Cyc_yyget_TypeList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)])));yyval=(void*)({struct
Cyc_Declarator_tok_struct*_tmp605=_cycalloc(sizeof(*_tmp605));_tmp605[0]=({
struct Cyc_Declarator_tok_struct _tmp606;_tmp606.tag=Cyc_Declarator_tok;_tmp606.f1=({
struct Cyc_Parse_Declarator*_tmp607=_cycalloc(sizeof(*_tmp607));_tmp607->id=(Cyc_yyget_Declarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]))->id;_tmp607->tms=({
struct Cyc_List_List*_tmp608=_cycalloc(sizeof(*_tmp608));_tmp608->hd=(void*)((
void*)({struct Cyc_Absyn_TypeParams_mod_struct*_tmp609=_cycalloc(sizeof(*_tmp609));
_tmp609[0]=({struct Cyc_Absyn_TypeParams_mod_struct _tmp60A;_tmp60A.tag=4;_tmp60A.f1=
_tmp604;_tmp60A.f2=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp60A.f3=0;
_tmp60A;});_tmp609;}));_tmp608->tl=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]))->tms;_tmp608;});_tmp607;});
_tmp606;});_tmp605;});break;}case 169: _LL298: yyval=(void*)({struct Cyc_Declarator_tok_struct*
_tmp60B=_cycalloc(sizeof(*_tmp60B));_tmp60B[0]=({struct Cyc_Declarator_tok_struct
_tmp60C;_tmp60C.tag=Cyc_Declarator_tok;_tmp60C.f1=({struct Cyc_Parse_Declarator*
_tmp60D=_cycalloc(sizeof(*_tmp60D));_tmp60D->id=(Cyc_yyget_Declarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]))->id;_tmp60D->tms=({
struct Cyc_List_List*_tmp60E=_cycalloc(sizeof(*_tmp60E));_tmp60E->hd=(void*)((
void*)({struct Cyc_Absyn_Attributes_mod_struct*_tmp60F=_cycalloc(sizeof(*_tmp60F));
_tmp60F[0]=({struct Cyc_Absyn_Attributes_mod_struct _tmp610;_tmp610.tag=5;_tmp610.f1=
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line);_tmp610.f2=Cyc_yyget_AttributeList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp610;});_tmp60F;}));
_tmp60E->tl=(Cyc_yyget_Declarator_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 1)]))->tms;_tmp60E;});_tmp60D;});_tmp60C;});_tmp60B;});break;case
170: _LL299: yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;
case 171: _LL29A: yyval=(void*)({struct Cyc_TypeModifierList_tok_struct*_tmp611=
_cycalloc(sizeof(*_tmp611));_tmp611[0]=({struct Cyc_TypeModifierList_tok_struct
_tmp612;_tmp612.tag=Cyc_TypeModifierList_tok;_tmp612.f1=Cyc_List_imp_append(Cyc_yyget_TypeModifierList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_yyget_TypeModifierList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]));_tmp612;});_tmp611;});
break;case 172: _LL29B: {struct Cyc_List_List*ans=0;if(Cyc_yyget_AttributeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)])!= 0)ans=({struct Cyc_List_List*
_tmp613=_cycalloc(sizeof(*_tmp613));_tmp613->hd=(void*)((void*)({struct Cyc_Absyn_Attributes_mod_struct*
_tmp614=_cycalloc(sizeof(*_tmp614));_tmp614[0]=({struct Cyc_Absyn_Attributes_mod_struct
_tmp615;_tmp615.tag=5;_tmp615.f1=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line);_tmp615.f2=
Cyc_yyget_AttributeList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 1)]);_tmp615;});_tmp614;}));_tmp613->tl=ans;_tmp613;});ans=({
struct Cyc_List_List*_tmp616=_cycalloc(sizeof(*_tmp616));_tmp616->hd=(void*)((
void*)({struct Cyc_Absyn_Pointer_mod_struct*_tmp617=_cycalloc(sizeof(*_tmp617));
_tmp617[0]=({struct Cyc_Absyn_Pointer_mod_struct _tmp618;_tmp618.tag=2;_tmp618.f1=({
struct Cyc_Absyn_PtrAtts _tmp619;_tmp619.rgn=(void*)Cyc_yyget_Type_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp619.nullable=(*Cyc_yyget_YY1(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 4)])).f1;_tmp619.bounds=(*
Cyc_yyget_YY1(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 4)])).f2;
_tmp619.zero_term=Cyc_yyget_YY4(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 3)]);_tmp619;});_tmp618.f2=Cyc_yyget_TypeQual_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp618;});_tmp617;}));
_tmp616->tl=ans;_tmp616;});yyval=(void*)({struct Cyc_TypeModifierList_tok_struct*
_tmp61A=_cycalloc(sizeof(*_tmp61A));_tmp61A[0]=({struct Cyc_TypeModifierList_tok_struct
_tmp61B;_tmp61B.tag=Cyc_TypeModifierList_tok;_tmp61B.f1=ans;_tmp61B;});_tmp61A;});
break;}case 173: _LL29C: yyval=(void*)({struct Cyc_YY1_struct*_tmp61C=_cycalloc(
sizeof(*_tmp61C));_tmp61C[0]=({struct Cyc_YY1_struct _tmp61D;_tmp61D.tag=Cyc_YY1;
_tmp61D.f1=({struct _tuple16*_tmp61E=_cycalloc(sizeof(*_tmp61E));_tmp61E->f1=Cyc_Absyn_true_conref;
_tmp61E->f2=Cyc_yyget_YY2(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);
_tmp61E;});_tmp61D;});_tmp61C;});break;case 174: _LL29D: yyval=(void*)({struct Cyc_YY1_struct*
_tmp61F=_cycalloc(sizeof(*_tmp61F));_tmp61F[0]=({struct Cyc_YY1_struct _tmp620;
_tmp620.tag=Cyc_YY1;_tmp620.f1=({struct _tuple16*_tmp621=_cycalloc(sizeof(*
_tmp621));_tmp621->f1=Cyc_Absyn_false_conref;_tmp621->f2=Cyc_yyget_YY2(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp621;});_tmp620;});
_tmp61F;});break;case 175: _LL29E: yyval=(void*)({struct Cyc_YY1_struct*_tmp622=
_cycalloc(sizeof(*_tmp622));_tmp622[0]=({struct Cyc_YY1_struct _tmp623;_tmp623.tag=
Cyc_YY1;_tmp623.f1=({struct _tuple16*_tmp624=_cycalloc(sizeof(*_tmp624));_tmp624->f1=
Cyc_Absyn_true_conref;_tmp624->f2=Cyc_Absyn_bounds_unknown_conref;_tmp624;});
_tmp623;});_tmp622;});break;case 176: _LL29F: yyval=(void*)({struct Cyc_YY2_struct*
_tmp625=_cycalloc(sizeof(*_tmp625));_tmp625[0]=({struct Cyc_YY2_struct _tmp626;
_tmp626.tag=Cyc_YY2;_tmp626.f1=Cyc_Absyn_bounds_one_conref;_tmp626;});_tmp625;});
break;case 177: _LL2A0: yyval=(void*)({struct Cyc_YY2_struct*_tmp627=_cycalloc(
sizeof(*_tmp627));_tmp627[0]=({struct Cyc_YY2_struct _tmp628;_tmp628.tag=Cyc_YY2;
_tmp628.f1=Cyc_Absyn_new_conref((void*)({struct Cyc_Absyn_Upper_b_struct*_tmp629=
_cycalloc(sizeof(*_tmp629));_tmp629[0]=({struct Cyc_Absyn_Upper_b_struct _tmp62A;
_tmp62A.tag=0;_tmp62A.f1=Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 1)]);_tmp62A;});_tmp629;}));_tmp628;});_tmp627;});break;case
178: _LL2A1: yyval=(void*)({struct Cyc_YY2_struct*_tmp62B=_cycalloc(sizeof(*_tmp62B));
_tmp62B[0]=({struct Cyc_YY2_struct _tmp62C;_tmp62C.tag=Cyc_YY2;_tmp62C.f1=Cyc_Absyn_new_conref((
void*)({struct Cyc_Absyn_AbsUpper_b_struct*_tmp62D=_cycalloc(sizeof(*_tmp62D));
_tmp62D[0]=({struct Cyc_Absyn_AbsUpper_b_struct _tmp62E;_tmp62E.tag=1;_tmp62E.f1=(
void*)Cyc_yyget_Type_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 
1)]);_tmp62E;});_tmp62D;}));_tmp62C;});_tmp62B;});break;case 179: _LL2A2: yyval=(
void*)({struct Cyc_YY4_struct*_tmp62F=_cycalloc(sizeof(*_tmp62F));_tmp62F[0]=({
struct Cyc_YY4_struct _tmp630;_tmp630.tag=Cyc_YY4;_tmp630.f1=((struct Cyc_Absyn_Conref*(*)())
Cyc_Absyn_empty_conref)();_tmp630;});_tmp62F;});break;case 180: _LL2A3: yyval=(void*)({
struct Cyc_YY4_struct*_tmp631=_cycalloc(sizeof(*_tmp631));_tmp631[0]=({struct Cyc_YY4_struct
_tmp632;_tmp632.tag=Cyc_YY4;_tmp632.f1=Cyc_Absyn_true_conref;_tmp632;});_tmp631;});
break;case 181: _LL2A4: yyval=(void*)({struct Cyc_YY4_struct*_tmp633=_cycalloc(
sizeof(*_tmp633));_tmp633[0]=({struct Cyc_YY4_struct _tmp634;_tmp634.tag=Cyc_YY4;
_tmp634.f1=Cyc_Absyn_false_conref;_tmp634;});_tmp633;});break;case 182: _LL2A5:
yyval=(void*)({struct Cyc_Type_tok_struct*_tmp635=_cycalloc(sizeof(*_tmp635));
_tmp635[0]=({struct Cyc_Type_tok_struct _tmp636;_tmp636.tag=Cyc_Type_tok;_tmp636.f1=(
void*)Cyc_Absyn_new_evar(({struct Cyc_Core_Opt*_tmp637=_cycalloc(sizeof(*_tmp637));
_tmp637->v=(void*)((void*)3);_tmp637;}),0);_tmp636;});_tmp635;});break;case 183:
_LL2A6: Cyc_Parse_set_vartyp_kind(Cyc_yyget_Type_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),(void*)3);yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 184: _LL2A7: yyval=(
void*)({struct Cyc_Type_tok_struct*_tmp638=_cycalloc(sizeof(*_tmp638));_tmp638[0]=({
struct Cyc_Type_tok_struct _tmp639;_tmp639.tag=Cyc_Type_tok;_tmp639.f1=(void*)Cyc_Absyn_new_evar(({
struct Cyc_Core_Opt*_tmp63A=_cycalloc(sizeof(*_tmp63A));_tmp63A->v=(void*)((void*)
3);_tmp63A;}),0);_tmp639;});_tmp638;});break;case 185: _LL2A8: yyval=(void*)({
struct Cyc_TypeQual_tok_struct*_tmp63B=_cycalloc_atomic(sizeof(*_tmp63B));_tmp63B[
0]=({struct Cyc_TypeQual_tok_struct _tmp63C;_tmp63C.tag=Cyc_TypeQual_tok;_tmp63C.f1=
Cyc_Absyn_empty_tqual();_tmp63C;});_tmp63B;});break;case 186: _LL2A9: yyval=(void*)({
struct Cyc_TypeQual_tok_struct*_tmp63D=_cycalloc_atomic(sizeof(*_tmp63D));_tmp63D[
0]=({struct Cyc_TypeQual_tok_struct _tmp63E;_tmp63E.tag=Cyc_TypeQual_tok;_tmp63E.f1=
Cyc_Absyn_combine_tqual(Cyc_yyget_TypeQual_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_yyget_TypeQual_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]));_tmp63E;});_tmp63D;});
break;case 187: _LL2AA: yyval=(void*)({struct Cyc_YY3_struct*_tmp63F=_cycalloc(
sizeof(*_tmp63F));_tmp63F[0]=({struct Cyc_YY3_struct _tmp640;_tmp640.tag=Cyc_YY3;
_tmp640.f1=({struct _tuple14*_tmp641=_cycalloc(sizeof(*_tmp641));_tmp641->f1=((
struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_ParamDeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]));_tmp641->f2=0;
_tmp641->f3=0;_tmp641->f4=Cyc_yyget_TypeOpt_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp641->f5=Cyc_yyget_Rgnorder_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp641;});_tmp640;});
_tmp63F;});break;case 188: _LL2AB: yyval=(void*)({struct Cyc_YY3_struct*_tmp642=
_cycalloc(sizeof(*_tmp642));_tmp642[0]=({struct Cyc_YY3_struct _tmp643;_tmp643.tag=
Cyc_YY3;_tmp643.f1=({struct _tuple14*_tmp644=_cycalloc(sizeof(*_tmp644));_tmp644->f1=((
struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_ParamDeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 4)]));_tmp644->f2=1;
_tmp644->f3=0;_tmp644->f4=Cyc_yyget_TypeOpt_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp644->f5=Cyc_yyget_Rgnorder_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp644;});_tmp643;});
_tmp642;});break;case 189: _LL2AC: {struct _tuple2 _tmp646;struct Cyc_Core_Opt*
_tmp647;struct Cyc_Absyn_Tqual _tmp648;void*_tmp649;struct _tuple2*_tmp645=Cyc_yyget_ParamDecl_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp646=*_tmp645;
_tmp647=_tmp646.f1;_tmp648=_tmp646.f2;_tmp649=_tmp646.f3;{struct Cyc_Absyn_VarargInfo*
_tmp64A=({struct Cyc_Absyn_VarargInfo*_tmp64E=_cycalloc(sizeof(*_tmp64E));_tmp64E->name=
_tmp647;_tmp64E->tq=_tmp648;_tmp64E->type=(void*)_tmp649;_tmp64E->inject=Cyc_yyget_Bool_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp64E;});yyval=(
void*)({struct Cyc_YY3_struct*_tmp64B=_cycalloc(sizeof(*_tmp64B));_tmp64B[0]=({
struct Cyc_YY3_struct _tmp64C;_tmp64C.tag=Cyc_YY3;_tmp64C.f1=({struct _tuple14*
_tmp64D=_cycalloc(sizeof(*_tmp64D));_tmp64D->f1=0;_tmp64D->f2=0;_tmp64D->f3=
_tmp64A;_tmp64D->f4=Cyc_yyget_TypeOpt_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 1)]);_tmp64D->f5=Cyc_yyget_Rgnorder_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp64D;});_tmp64C;});
_tmp64B;});break;}}case 190: _LL2AD: {struct _tuple2 _tmp650;struct Cyc_Core_Opt*
_tmp651;struct Cyc_Absyn_Tqual _tmp652;void*_tmp653;struct _tuple2*_tmp64F=Cyc_yyget_ParamDecl_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp650=*_tmp64F;
_tmp651=_tmp650.f1;_tmp652=_tmp650.f2;_tmp653=_tmp650.f3;{struct Cyc_Absyn_VarargInfo*
_tmp654=({struct Cyc_Absyn_VarargInfo*_tmp658=_cycalloc(sizeof(*_tmp658));_tmp658->name=
_tmp651;_tmp658->tq=_tmp652;_tmp658->type=(void*)_tmp653;_tmp658->inject=Cyc_yyget_Bool_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp658;});yyval=(
void*)({struct Cyc_YY3_struct*_tmp655=_cycalloc(sizeof(*_tmp655));_tmp655[0]=({
struct Cyc_YY3_struct _tmp656;_tmp656.tag=Cyc_YY3;_tmp656.f1=({struct _tuple14*
_tmp657=_cycalloc(sizeof(*_tmp657));_tmp657->f1=((struct Cyc_List_List*(*)(struct
Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_ParamDeclList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 6)]));_tmp657->f2=0;_tmp657->f3=
_tmp654;_tmp657->f4=Cyc_yyget_TypeOpt_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 1)]);_tmp657->f5=Cyc_yyget_Rgnorder_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp657;});_tmp656;});
_tmp655;});break;}}case 191: _LL2AE: yyval=(void*)({struct Cyc_Type_tok_struct*
_tmp659=_cycalloc(sizeof(*_tmp659));_tmp659[0]=({struct Cyc_Type_tok_struct
_tmp65A;_tmp65A.tag=Cyc_Type_tok;_tmp65A.f1=(void*)Cyc_Parse_id2type(Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),(void*)({struct Cyc_Absyn_Unknown_kb_struct*
_tmp65B=_cycalloc(sizeof(*_tmp65B));_tmp65B[0]=({struct Cyc_Absyn_Unknown_kb_struct
_tmp65C;_tmp65C.tag=1;_tmp65C.f1=0;_tmp65C;});_tmp65B;}));_tmp65A;});_tmp659;});
break;case 192: _LL2AF: yyval=(void*)({struct Cyc_Type_tok_struct*_tmp65D=_cycalloc(
sizeof(*_tmp65D));_tmp65D[0]=({struct Cyc_Type_tok_struct _tmp65E;_tmp65E.tag=Cyc_Type_tok;
_tmp65E.f1=(void*)Cyc_Parse_id2type(Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),(void*)({struct Cyc_Absyn_Eq_kb_struct*
_tmp65F=_cycalloc(sizeof(*_tmp65F));_tmp65F[0]=({struct Cyc_Absyn_Eq_kb_struct
_tmp660;_tmp660.tag=0;_tmp660.f1=(void*)Cyc_yyget_Kind_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp660;});_tmp65F;}));
_tmp65E;});_tmp65D;});break;case 193: _LL2B0: yyval=(void*)({struct Cyc_TypeOpt_tok_struct*
_tmp661=_cycalloc(sizeof(*_tmp661));_tmp661[0]=({struct Cyc_TypeOpt_tok_struct
_tmp662;_tmp662.tag=Cyc_TypeOpt_tok;_tmp662.f1=0;_tmp662;});_tmp661;});break;
case 194: _LL2B1: yyval=(void*)({struct Cyc_TypeOpt_tok_struct*_tmp663=_cycalloc(
sizeof(*_tmp663));_tmp663[0]=({struct Cyc_TypeOpt_tok_struct _tmp664;_tmp664.tag=
Cyc_TypeOpt_tok;_tmp664.f1=({struct Cyc_Core_Opt*_tmp665=_cycalloc(sizeof(*
_tmp665));_tmp665->v=(void*)((void*)({struct Cyc_Absyn_JoinEff_struct*_tmp666=
_cycalloc(sizeof(*_tmp666));_tmp666[0]=({struct Cyc_Absyn_JoinEff_struct _tmp667;
_tmp667.tag=20;_tmp667.f1=Cyc_yyget_TypeList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp667;});_tmp666;}));
_tmp665;});_tmp664;});_tmp663;});break;case 195: _LL2B2: yyval=(void*)({struct Cyc_Rgnorder_tok_struct*
_tmp668=_cycalloc(sizeof(*_tmp668));_tmp668[0]=({struct Cyc_Rgnorder_tok_struct
_tmp669;_tmp669.tag=Cyc_Rgnorder_tok;_tmp669.f1=0;_tmp669;});_tmp668;});break;
case 196: _LL2B3: yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];
break;case 197: _LL2B4: yyval=(void*)({struct Cyc_Rgnorder_tok_struct*_tmp66A=
_cycalloc(sizeof(*_tmp66A));_tmp66A[0]=({struct Cyc_Rgnorder_tok_struct _tmp66B;
_tmp66B.tag=Cyc_Rgnorder_tok;_tmp66B.f1=({struct Cyc_List_List*_tmp66C=_cycalloc(
sizeof(*_tmp66C));_tmp66C->hd=({struct _tuple4*_tmp66D=_cycalloc(sizeof(*_tmp66D));
_tmp66D->f1=(void*)({struct Cyc_Absyn_JoinEff_struct*_tmp670=_cycalloc(sizeof(*
_tmp670));_tmp670[0]=({struct Cyc_Absyn_JoinEff_struct _tmp671;_tmp671.tag=20;
_tmp671.f1=Cyc_yyget_TypeList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 2)]);_tmp671;});_tmp670;});_tmp66D->f2=Cyc_Parse_id2type(Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),(void*)({struct Cyc_Absyn_Eq_kb_struct*
_tmp66E=_cycalloc(sizeof(*_tmp66E));_tmp66E[0]=({struct Cyc_Absyn_Eq_kb_struct
_tmp66F;_tmp66F.tag=0;_tmp66F.f1=(void*)((void*)3);_tmp66F;});_tmp66E;}));
_tmp66D;});_tmp66C->tl=0;_tmp66C;});_tmp66B;});_tmp66A;});break;case 198: _LL2B5:
yyval=(void*)({struct Cyc_Rgnorder_tok_struct*_tmp672=_cycalloc(sizeof(*_tmp672));
_tmp672[0]=({struct Cyc_Rgnorder_tok_struct _tmp673;_tmp673.tag=Cyc_Rgnorder_tok;
_tmp673.f1=({struct Cyc_List_List*_tmp674=_cycalloc(sizeof(*_tmp674));_tmp674->hd=({
struct _tuple4*_tmp675=_cycalloc(sizeof(*_tmp675));_tmp675->f1=(void*)({struct Cyc_Absyn_JoinEff_struct*
_tmp678=_cycalloc(sizeof(*_tmp678));_tmp678[0]=({struct Cyc_Absyn_JoinEff_struct
_tmp679;_tmp679.tag=20;_tmp679.f1=Cyc_yyget_TypeList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]);_tmp679;});_tmp678;});
_tmp675->f2=Cyc_Parse_id2type(Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),(void*)({struct Cyc_Absyn_Eq_kb_struct*
_tmp676=_cycalloc(sizeof(*_tmp676));_tmp676[0]=({struct Cyc_Absyn_Eq_kb_struct
_tmp677;_tmp677.tag=0;_tmp677.f1=(void*)((void*)3);_tmp677;});_tmp676;}));
_tmp675;});_tmp674->tl=Cyc_yyget_Rgnorder_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp674;});_tmp673;});
_tmp672;});break;case 199: _LL2B6: yyval=(void*)({struct Cyc_Bool_tok_struct*_tmp67A=
_cycalloc_atomic(sizeof(*_tmp67A));_tmp67A[0]=({struct Cyc_Bool_tok_struct _tmp67B;
_tmp67B.tag=Cyc_Bool_tok;_tmp67B.f1=0;_tmp67B;});_tmp67A;});break;case 200: _LL2B7:
if(Cyc_zstrcmp((struct _tagged_arr)Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),({const char*_tmp67C="inject";
_tag_arr(_tmp67C,sizeof(char),_get_zero_arr_size(_tmp67C,7));}))!= 0)Cyc_Parse_err(({
const char*_tmp67D="missing type in function declaration";_tag_arr(_tmp67D,
sizeof(char),_get_zero_arr_size(_tmp67D,37));}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));yyval=(void*)({
struct Cyc_Bool_tok_struct*_tmp67E=_cycalloc_atomic(sizeof(*_tmp67E));_tmp67E[0]=({
struct Cyc_Bool_tok_struct _tmp67F;_tmp67F.tag=Cyc_Bool_tok;_tmp67F.f1=1;_tmp67F;});
_tmp67E;});break;case 201: _LL2B8: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 202: _LL2B9: yyval=(void*)({struct Cyc_TypeList_tok_struct*
_tmp680=_cycalloc(sizeof(*_tmp680));_tmp680[0]=({struct Cyc_TypeList_tok_struct
_tmp681;_tmp681.tag=Cyc_TypeList_tok;_tmp681.f1=Cyc_List_imp_append(Cyc_yyget_TypeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_TypeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]));_tmp681;});_tmp680;});
break;case 203: _LL2BA: yyval=(void*)({struct Cyc_TypeList_tok_struct*_tmp682=
_cycalloc(sizeof(*_tmp682));_tmp682[0]=({struct Cyc_TypeList_tok_struct _tmp683;
_tmp683.tag=Cyc_TypeList_tok;_tmp683.f1=0;_tmp683;});_tmp682;});break;case 204:
_LL2BB: yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)];break;
case 205: _LL2BC: yyval=(void*)({struct Cyc_TypeList_tok_struct*_tmp684=_cycalloc(
sizeof(*_tmp684));_tmp684[0]=({struct Cyc_TypeList_tok_struct _tmp685;_tmp685.tag=
Cyc_TypeList_tok;_tmp685.f1=({struct Cyc_List_List*_tmp686=_cycalloc(sizeof(*
_tmp686));_tmp686->hd=(void*)((void*)({struct Cyc_Absyn_RgnsEff_struct*_tmp687=
_cycalloc(sizeof(*_tmp687));_tmp687[0]=({struct Cyc_Absyn_RgnsEff_struct _tmp688;
_tmp688.tag=21;_tmp688.f1=(void*)Cyc_yyget_Type_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp688;});_tmp687;}));
_tmp686->tl=0;_tmp686;});_tmp685;});_tmp684;});break;case 206: _LL2BD: Cyc_Parse_set_vartyp_kind(
Cyc_yyget_Type_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),(
void*)4);yyval=(void*)({struct Cyc_TypeList_tok_struct*_tmp689=_cycalloc(sizeof(*
_tmp689));_tmp689[0]=({struct Cyc_TypeList_tok_struct _tmp68A;_tmp68A.tag=Cyc_TypeList_tok;
_tmp68A.f1=({struct Cyc_List_List*_tmp68B=_cycalloc(sizeof(*_tmp68B));_tmp68B->hd=(
void*)Cyc_yyget_Type_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);
_tmp68B->tl=0;_tmp68B;});_tmp68A;});_tmp689;});break;case 207: _LL2BE: Cyc_Parse_set_vartyp_kind(
Cyc_yyget_Type_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),(
void*)3);yyval=(void*)({struct Cyc_TypeList_tok_struct*_tmp68C=_cycalloc(sizeof(*
_tmp68C));_tmp68C[0]=({struct Cyc_TypeList_tok_struct _tmp68D;_tmp68D.tag=Cyc_TypeList_tok;
_tmp68D.f1=({struct Cyc_List_List*_tmp68E=_cycalloc(sizeof(*_tmp68E));_tmp68E->hd=(
void*)((void*)({struct Cyc_Absyn_AccessEff_struct*_tmp68F=_cycalloc(sizeof(*
_tmp68F));_tmp68F[0]=({struct Cyc_Absyn_AccessEff_struct _tmp690;_tmp690.tag=19;
_tmp690.f1=(void*)Cyc_yyget_Type_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]);_tmp690;});_tmp68F;}));_tmp68E->tl=0;_tmp68E;});_tmp68D;});
_tmp68C;});break;case 208: _LL2BF: Cyc_Parse_set_vartyp_kind(Cyc_yyget_Type_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),(void*)3);yyval=(
void*)({struct Cyc_TypeList_tok_struct*_tmp691=_cycalloc(sizeof(*_tmp691));
_tmp691[0]=({struct Cyc_TypeList_tok_struct _tmp692;_tmp692.tag=Cyc_TypeList_tok;
_tmp692.f1=({struct Cyc_List_List*_tmp693=_cycalloc(sizeof(*_tmp693));_tmp693->hd=(
void*)((void*)({struct Cyc_Absyn_AccessEff_struct*_tmp694=_cycalloc(sizeof(*
_tmp694));_tmp694[0]=({struct Cyc_Absyn_AccessEff_struct _tmp695;_tmp695.tag=19;
_tmp695.f1=(void*)Cyc_yyget_Type_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 2)]);_tmp695;});_tmp694;}));_tmp693->tl=Cyc_yyget_TypeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp693;});_tmp692;});
_tmp691;});break;case 209: _LL2C0: yyval=(void*)({struct Cyc_ParamDeclList_tok_struct*
_tmp696=_cycalloc(sizeof(*_tmp696));_tmp696[0]=({struct Cyc_ParamDeclList_tok_struct
_tmp697;_tmp697.tag=Cyc_ParamDeclList_tok;_tmp697.f1=({struct Cyc_List_List*
_tmp698=_cycalloc(sizeof(*_tmp698));_tmp698->hd=Cyc_yyget_ParamDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp698->tl=0;_tmp698;});
_tmp697;});_tmp696;});break;case 210: _LL2C1: yyval=(void*)({struct Cyc_ParamDeclList_tok_struct*
_tmp699=_cycalloc(sizeof(*_tmp699));_tmp699[0]=({struct Cyc_ParamDeclList_tok_struct
_tmp69A;_tmp69A.tag=Cyc_ParamDeclList_tok;_tmp69A.f1=({struct Cyc_List_List*
_tmp69B=_cycalloc(sizeof(*_tmp69B));_tmp69B->hd=Cyc_yyget_ParamDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp69B->tl=Cyc_yyget_ParamDeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp69B;});_tmp69A;});
_tmp699;});break;case 211: _LL2C2: {struct _tuple15 _tmp69D;struct Cyc_Absyn_Tqual
_tmp69E;struct Cyc_List_List*_tmp69F;struct Cyc_List_List*_tmp6A0;struct _tuple15*
_tmp69C=Cyc_yyget_QualSpecList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 1)]);_tmp69D=*_tmp69C;_tmp69E=_tmp69D.f1;_tmp69F=_tmp69D.f2;
_tmp6A0=_tmp69D.f3;{struct Cyc_Parse_Declarator _tmp6A2;struct _tuple1*_tmp6A3;
struct Cyc_List_List*_tmp6A4;struct Cyc_Parse_Declarator*_tmp6A1=Cyc_yyget_Declarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp6A2=*_tmp6A1;
_tmp6A3=_tmp6A2.id;_tmp6A4=_tmp6A2.tms;{void*_tmp6A5=Cyc_Parse_speclist2typ(
_tmp69F,Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).last_line));struct Cyc_Absyn_Tqual _tmp6A7;void*_tmp6A8;struct
Cyc_List_List*_tmp6A9;struct Cyc_List_List*_tmp6AA;struct _tuple6 _tmp6A6=Cyc_Parse_apply_tms(
_tmp69E,_tmp6A5,_tmp6A0,_tmp6A4);_tmp6A7=_tmp6A6.f1;_tmp6A8=_tmp6A6.f2;_tmp6A9=
_tmp6A6.f3;_tmp6AA=_tmp6A6.f4;if(_tmp6A9 != 0)Cyc_Parse_err(({const char*_tmp6AB="parameter with bad type params";
_tag_arr(_tmp6AB,sizeof(char),_get_zero_arr_size(_tmp6AB,31));}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));if(Cyc_Absyn_is_qvar_qualified(
_tmp6A3))Cyc_Parse_err(({const char*_tmp6AC="parameter cannot be qualified with a namespace";
_tag_arr(_tmp6AC,sizeof(char),_get_zero_arr_size(_tmp6AC,47));}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line));{struct Cyc_Core_Opt*
_tmp6AD=(struct Cyc_Core_Opt*)({struct Cyc_Core_Opt*_tmp6B3=_cycalloc(sizeof(*
_tmp6B3));_tmp6B3->v=(*_tmp6A3).f2;_tmp6B3;});if(_tmp6AA != 0)({void*_tmp6AE[0]={};
Cyc_Tcutil_warn(Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(
10000,yylsp_offset - 1)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line),({const char*_tmp6AF="extra attributes on parameter, ignoring";
_tag_arr(_tmp6AF,sizeof(char),_get_zero_arr_size(_tmp6AF,40));}),_tag_arr(
_tmp6AE,sizeof(void*),0));});yyval=(void*)({struct Cyc_ParamDecl_tok_struct*
_tmp6B0=_cycalloc(sizeof(*_tmp6B0));_tmp6B0[0]=({struct Cyc_ParamDecl_tok_struct
_tmp6B1;_tmp6B1.tag=Cyc_ParamDecl_tok;_tmp6B1.f1=({struct _tuple2*_tmp6B2=
_cycalloc(sizeof(*_tmp6B2));_tmp6B2->f1=_tmp6AD;_tmp6B2->f2=_tmp6A7;_tmp6B2->f3=
_tmp6A8;_tmp6B2;});_tmp6B1;});_tmp6B0;});break;}}}}case 212: _LL2C3: {struct
_tuple15 _tmp6B5;struct Cyc_Absyn_Tqual _tmp6B6;struct Cyc_List_List*_tmp6B7;struct
Cyc_List_List*_tmp6B8;struct _tuple15*_tmp6B4=Cyc_yyget_QualSpecList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp6B5=*_tmp6B4;_tmp6B6=
_tmp6B5.f1;_tmp6B7=_tmp6B5.f2;_tmp6B8=_tmp6B5.f3;{void*_tmp6B9=Cyc_Parse_speclist2typ(
_tmp6B7,Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));if(_tmp6B8 != 0)({void*_tmp6BA[0]={};Cyc_Tcutil_warn(
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line),({const char*_tmp6BB="bad attributes on parameter, ignoring";
_tag_arr(_tmp6BB,sizeof(char),_get_zero_arr_size(_tmp6BB,38));}),_tag_arr(
_tmp6BA,sizeof(void*),0));});yyval=(void*)({struct Cyc_ParamDecl_tok_struct*
_tmp6BC=_cycalloc(sizeof(*_tmp6BC));_tmp6BC[0]=({struct Cyc_ParamDecl_tok_struct
_tmp6BD;_tmp6BD.tag=Cyc_ParamDecl_tok;_tmp6BD.f1=({struct _tuple2*_tmp6BE=
_cycalloc(sizeof(*_tmp6BE));_tmp6BE->f1=0;_tmp6BE->f2=_tmp6B6;_tmp6BE->f3=
_tmp6B9;_tmp6BE;});_tmp6BD;});_tmp6BC;});break;}}case 213: _LL2C4: {struct _tuple15
_tmp6C0;struct Cyc_Absyn_Tqual _tmp6C1;struct Cyc_List_List*_tmp6C2;struct Cyc_List_List*
_tmp6C3;struct _tuple15*_tmp6BF=Cyc_yyget_QualSpecList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp6C0=*_tmp6BF;_tmp6C1=
_tmp6C0.f1;_tmp6C2=_tmp6C0.f2;_tmp6C3=_tmp6C0.f3;{void*_tmp6C4=Cyc_Parse_speclist2typ(
_tmp6C2,Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).last_line));struct Cyc_List_List*_tmp6C5=(Cyc_yyget_AbstractDeclarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->tms;struct Cyc_Absyn_Tqual
_tmp6C7;void*_tmp6C8;struct Cyc_List_List*_tmp6C9;struct Cyc_List_List*_tmp6CA;
struct _tuple6 _tmp6C6=Cyc_Parse_apply_tms(_tmp6C1,_tmp6C4,_tmp6C3,_tmp6C5);
_tmp6C7=_tmp6C6.f1;_tmp6C8=_tmp6C6.f2;_tmp6C9=_tmp6C6.f3;_tmp6CA=_tmp6C6.f4;if(
_tmp6C9 != 0)({void*_tmp6CB[0]={};Cyc_Tcutil_warn(Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line),({const char*
_tmp6CC="bad type parameters on formal argument, ignoring";_tag_arr(_tmp6CC,
sizeof(char),_get_zero_arr_size(_tmp6CC,49));}),_tag_arr(_tmp6CB,sizeof(void*),0));});
if(_tmp6CA != 0)({void*_tmp6CD[0]={};Cyc_Tcutil_warn(Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line),({const char*
_tmp6CE="bad attributes on parameter, ignoring";_tag_arr(_tmp6CE,sizeof(char),
_get_zero_arr_size(_tmp6CE,38));}),_tag_arr(_tmp6CD,sizeof(void*),0));});yyval=(
void*)({struct Cyc_ParamDecl_tok_struct*_tmp6CF=_cycalloc(sizeof(*_tmp6CF));
_tmp6CF[0]=({struct Cyc_ParamDecl_tok_struct _tmp6D0;_tmp6D0.tag=Cyc_ParamDecl_tok;
_tmp6D0.f1=({struct _tuple2*_tmp6D1=_cycalloc(sizeof(*_tmp6D1));_tmp6D1->f1=0;
_tmp6D1->f2=_tmp6C7;_tmp6D1->f3=_tmp6C8;_tmp6D1;});_tmp6D0;});_tmp6CF;});break;}}
case 214: _LL2C5: yyval=(void*)({struct Cyc_IdList_tok_struct*_tmp6D2=_cycalloc(
sizeof(*_tmp6D2));_tmp6D2[0]=({struct Cyc_IdList_tok_struct _tmp6D3;_tmp6D3.tag=
Cyc_IdList_tok;_tmp6D3.f1=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(
Cyc_yyget_IdList_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]));
_tmp6D3;});_tmp6D2;});break;case 215: _LL2C6: yyval=(void*)({struct Cyc_IdList_tok_struct*
_tmp6D4=_cycalloc(sizeof(*_tmp6D4));_tmp6D4[0]=({struct Cyc_IdList_tok_struct
_tmp6D5;_tmp6D5.tag=Cyc_IdList_tok;_tmp6D5.f1=({struct Cyc_List_List*_tmp6D6=
_cycalloc(sizeof(*_tmp6D6));_tmp6D6->hd=({struct _tagged_arr*_tmp6D7=_cycalloc(
sizeof(*_tmp6D7));_tmp6D7[0]=Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp6D7;});_tmp6D6->tl=0;
_tmp6D6;});_tmp6D5;});_tmp6D4;});break;case 216: _LL2C7: yyval=(void*)({struct Cyc_IdList_tok_struct*
_tmp6D8=_cycalloc(sizeof(*_tmp6D8));_tmp6D8[0]=({struct Cyc_IdList_tok_struct
_tmp6D9;_tmp6D9.tag=Cyc_IdList_tok;_tmp6D9.f1=({struct Cyc_List_List*_tmp6DA=
_cycalloc(sizeof(*_tmp6DA));_tmp6DA->hd=({struct _tagged_arr*_tmp6DB=_cycalloc(
sizeof(*_tmp6DB));_tmp6DB[0]=Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp6DB;});_tmp6DA->tl=Cyc_yyget_IdList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp6DA;});_tmp6D9;});
_tmp6D8;});break;case 217: _LL2C8: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 218: _LL2C9: yyval=yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)];break;case 219: _LL2CA: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp6DC=_cycalloc(sizeof(*_tmp6DC));_tmp6DC[0]=({struct Cyc_Exp_tok_struct _tmp6DD;
_tmp6DD.tag=Cyc_Exp_tok;_tmp6DD.f1=Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_UnresolvedMem_e_struct*
_tmp6DE=_cycalloc(sizeof(*_tmp6DE));_tmp6DE[0]=({struct Cyc_Absyn_UnresolvedMem_e_struct
_tmp6DF;_tmp6DF.tag=36;_tmp6DF.f1=0;_tmp6DF.f2=0;_tmp6DF;});_tmp6DE;}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp6DD;});
_tmp6DC;});break;case 220: _LL2CB: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp6E0=
_cycalloc(sizeof(*_tmp6E0));_tmp6E0[0]=({struct Cyc_Exp_tok_struct _tmp6E1;_tmp6E1.tag=
Cyc_Exp_tok;_tmp6E1.f1=Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_UnresolvedMem_e_struct*
_tmp6E2=_cycalloc(sizeof(*_tmp6E2));_tmp6E2[0]=({struct Cyc_Absyn_UnresolvedMem_e_struct
_tmp6E3;_tmp6E3.tag=36;_tmp6E3.f1=0;_tmp6E3.f2=((struct Cyc_List_List*(*)(struct
Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_InitializerList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]));_tmp6E3;});_tmp6E2;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 2)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp6E1;});_tmp6E0;});break;case 221: _LL2CC: yyval=(
void*)({struct Cyc_Exp_tok_struct*_tmp6E4=_cycalloc(sizeof(*_tmp6E4));_tmp6E4[0]=({
struct Cyc_Exp_tok_struct _tmp6E5;_tmp6E5.tag=Cyc_Exp_tok;_tmp6E5.f1=Cyc_Absyn_new_exp((
void*)({struct Cyc_Absyn_UnresolvedMem_e_struct*_tmp6E6=_cycalloc(sizeof(*_tmp6E6));
_tmp6E6[0]=({struct Cyc_Absyn_UnresolvedMem_e_struct _tmp6E7;_tmp6E7.tag=36;
_tmp6E7.f1=0;_tmp6E7.f2=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(
Cyc_yyget_InitializerList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 2)]));_tmp6E7;});_tmp6E6;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp6E5;});
_tmp6E4;});break;case 222: _LL2CD: {struct Cyc_Absyn_Vardecl*_tmp6E8=Cyc_Absyn_new_vardecl(({
struct _tuple1*_tmp6ED=_cycalloc(sizeof(*_tmp6ED));_tmp6ED->f1=(void*)0;_tmp6ED->f2=({
struct _tagged_arr*_tmp6EE=_cycalloc(sizeof(*_tmp6EE));_tmp6EE[0]=Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 5)]);_tmp6EE;});_tmp6ED;}),
Cyc_Absyn_uint_typ,(struct Cyc_Absyn_Exp*)Cyc_Absyn_uint_exp(0,Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 5)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 5)]).last_line)));(_tmp6E8->tq).real_const=
1;yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp6E9=_cycalloc(sizeof(*_tmp6E9));
_tmp6E9[0]=({struct Cyc_Exp_tok_struct _tmp6EA;_tmp6EA.tag=Cyc_Exp_tok;_tmp6EA.f1=
Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Comprehension_e_struct*_tmp6EB=
_cycalloc(sizeof(*_tmp6EB));_tmp6EB[0]=({struct Cyc_Absyn_Comprehension_e_struct
_tmp6EC;_tmp6EC.tag=29;_tmp6EC.f1=_tmp6E8;_tmp6EC.f2=Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp6EC.f3=Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp6EC.f4=0;
_tmp6EC;});_tmp6EB;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 7)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp6EA;});
_tmp6E9;});break;}case 223: _LL2CE: yyval=(void*)({struct Cyc_InitializerList_tok_struct*
_tmp6EF=_cycalloc(sizeof(*_tmp6EF));_tmp6EF[0]=({struct Cyc_InitializerList_tok_struct
_tmp6F0;_tmp6F0.tag=Cyc_InitializerList_tok;_tmp6F0.f1=({struct Cyc_List_List*
_tmp6F1=_cycalloc(sizeof(*_tmp6F1));_tmp6F1->hd=({struct _tuple19*_tmp6F2=
_cycalloc(sizeof(*_tmp6F2));_tmp6F2->f1=0;_tmp6F2->f2=Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp6F2;});_tmp6F1->tl=0;
_tmp6F1;});_tmp6F0;});_tmp6EF;});break;case 224: _LL2CF: yyval=(void*)({struct Cyc_InitializerList_tok_struct*
_tmp6F3=_cycalloc(sizeof(*_tmp6F3));_tmp6F3[0]=({struct Cyc_InitializerList_tok_struct
_tmp6F4;_tmp6F4.tag=Cyc_InitializerList_tok;_tmp6F4.f1=({struct Cyc_List_List*
_tmp6F5=_cycalloc(sizeof(*_tmp6F5));_tmp6F5->hd=({struct _tuple19*_tmp6F6=
_cycalloc(sizeof(*_tmp6F6));_tmp6F6->f1=Cyc_yyget_DesignatorList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp6F6->f2=Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp6F6;});_tmp6F5->tl=
0;_tmp6F5;});_tmp6F4;});_tmp6F3;});break;case 225: _LL2D0: yyval=(void*)({struct Cyc_InitializerList_tok_struct*
_tmp6F7=_cycalloc(sizeof(*_tmp6F7));_tmp6F7[0]=({struct Cyc_InitializerList_tok_struct
_tmp6F8;_tmp6F8.tag=Cyc_InitializerList_tok;_tmp6F8.f1=({struct Cyc_List_List*
_tmp6F9=_cycalloc(sizeof(*_tmp6F9));_tmp6F9->hd=({struct _tuple19*_tmp6FA=
_cycalloc(sizeof(*_tmp6FA));_tmp6FA->f1=0;_tmp6FA->f2=Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp6FA;});_tmp6F9->tl=Cyc_yyget_InitializerList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp6F9;});_tmp6F8;});
_tmp6F7;});break;case 226: _LL2D1: yyval=(void*)({struct Cyc_InitializerList_tok_struct*
_tmp6FB=_cycalloc(sizeof(*_tmp6FB));_tmp6FB[0]=({struct Cyc_InitializerList_tok_struct
_tmp6FC;_tmp6FC.tag=Cyc_InitializerList_tok;_tmp6FC.f1=({struct Cyc_List_List*
_tmp6FD=_cycalloc(sizeof(*_tmp6FD));_tmp6FD->hd=({struct _tuple19*_tmp6FE=
_cycalloc(sizeof(*_tmp6FE));_tmp6FE->f1=Cyc_yyget_DesignatorList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp6FE->f2=Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp6FE;});_tmp6FD->tl=
Cyc_yyget_InitializerList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 3)]);_tmp6FD;});_tmp6FC;});_tmp6FB;});break;case 227: _LL2D2: yyval=(
void*)({struct Cyc_DesignatorList_tok_struct*_tmp6FF=_cycalloc(sizeof(*_tmp6FF));
_tmp6FF[0]=({struct Cyc_DesignatorList_tok_struct _tmp700;_tmp700.tag=Cyc_DesignatorList_tok;
_tmp700.f1=Cyc_List_imp_rev(Cyc_yyget_DesignatorList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]));_tmp700;});_tmp6FF;});
break;case 228: _LL2D3: yyval=(void*)({struct Cyc_DesignatorList_tok_struct*_tmp701=
_cycalloc(sizeof(*_tmp701));_tmp701[0]=({struct Cyc_DesignatorList_tok_struct
_tmp702;_tmp702.tag=Cyc_DesignatorList_tok;_tmp702.f1=({struct Cyc_List_List*
_tmp703=_cycalloc(sizeof(*_tmp703));_tmp703->hd=(void*)Cyc_yyget_Designator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp703->tl=0;_tmp703;});
_tmp702;});_tmp701;});break;case 229: _LL2D4: yyval=(void*)({struct Cyc_DesignatorList_tok_struct*
_tmp704=_cycalloc(sizeof(*_tmp704));_tmp704[0]=({struct Cyc_DesignatorList_tok_struct
_tmp705;_tmp705.tag=Cyc_DesignatorList_tok;_tmp705.f1=({struct Cyc_List_List*
_tmp706=_cycalloc(sizeof(*_tmp706));_tmp706->hd=(void*)Cyc_yyget_Designator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp706->tl=Cyc_yyget_DesignatorList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp706;});_tmp705;});
_tmp704;});break;case 230: _LL2D5: yyval=(void*)({struct Cyc_Designator_tok_struct*
_tmp707=_cycalloc(sizeof(*_tmp707));_tmp707[0]=({struct Cyc_Designator_tok_struct
_tmp708;_tmp708.tag=Cyc_Designator_tok;_tmp708.f1=(void*)((void*)({struct Cyc_Absyn_ArrayElement_struct*
_tmp709=_cycalloc(sizeof(*_tmp709));_tmp709[0]=({struct Cyc_Absyn_ArrayElement_struct
_tmp70A;_tmp70A.tag=0;_tmp70A.f1=Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp70A;});_tmp709;}));
_tmp708;});_tmp707;});break;case 231: _LL2D6: yyval=(void*)({struct Cyc_Designator_tok_struct*
_tmp70B=_cycalloc(sizeof(*_tmp70B));_tmp70B[0]=({struct Cyc_Designator_tok_struct
_tmp70C;_tmp70C.tag=Cyc_Designator_tok;_tmp70C.f1=(void*)((void*)({struct Cyc_Absyn_FieldName_struct*
_tmp70D=_cycalloc(sizeof(*_tmp70D));_tmp70D[0]=({struct Cyc_Absyn_FieldName_struct
_tmp70E;_tmp70E.tag=1;_tmp70E.f1=({struct _tagged_arr*_tmp70F=_cycalloc(sizeof(*
_tmp70F));_tmp70F[0]=Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]);_tmp70F;});_tmp70E;});_tmp70D;}));_tmp70C;});_tmp70B;});
break;case 232: _LL2D7: {struct _tuple15 _tmp711;struct Cyc_Absyn_Tqual _tmp712;struct
Cyc_List_List*_tmp713;struct Cyc_List_List*_tmp714;struct _tuple15*_tmp710=Cyc_yyget_QualSpecList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp711=*_tmp710;
_tmp712=_tmp711.f1;_tmp713=_tmp711.f2;_tmp714=_tmp711.f3;{void*_tmp715=Cyc_Parse_speclist2typ(
_tmp713,Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));if(_tmp714 != 0)({void*_tmp716[0]={};Cyc_Tcutil_warn(
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line),({const char*_tmp717="ignoring attributes in type";
_tag_arr(_tmp717,sizeof(char),_get_zero_arr_size(_tmp717,28));}),_tag_arr(
_tmp716,sizeof(void*),0));});yyval=(void*)({struct Cyc_ParamDecl_tok_struct*
_tmp718=_cycalloc(sizeof(*_tmp718));_tmp718[0]=({struct Cyc_ParamDecl_tok_struct
_tmp719;_tmp719.tag=Cyc_ParamDecl_tok;_tmp719.f1=({struct _tuple2*_tmp71A=
_cycalloc(sizeof(*_tmp71A));_tmp71A->f1=0;_tmp71A->f2=_tmp712;_tmp71A->f3=
_tmp715;_tmp71A;});_tmp719;});_tmp718;});break;}}case 233: _LL2D8: {struct _tuple15
_tmp71C;struct Cyc_Absyn_Tqual _tmp71D;struct Cyc_List_List*_tmp71E;struct Cyc_List_List*
_tmp71F;struct _tuple15*_tmp71B=Cyc_yyget_QualSpecList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp71C=*_tmp71B;_tmp71D=
_tmp71C.f1;_tmp71E=_tmp71C.f2;_tmp71F=_tmp71C.f3;{void*_tmp720=Cyc_Parse_speclist2typ(
_tmp71E,Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).last_line));struct Cyc_List_List*_tmp721=(Cyc_yyget_AbstractDeclarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->tms;struct _tuple6
_tmp722=Cyc_Parse_apply_tms(_tmp71D,_tmp720,_tmp71F,_tmp721);if(_tmp722.f3 != 0)({
void*_tmp723[0]={};Cyc_Tcutil_warn(Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line),({const char*
_tmp724="bad type params, ignoring";_tag_arr(_tmp724,sizeof(char),
_get_zero_arr_size(_tmp724,26));}),_tag_arr(_tmp723,sizeof(void*),0));});if(
_tmp722.f4 != 0)({void*_tmp725[0]={};Cyc_Tcutil_warn(Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line),({const char*
_tmp726="bad specifiers, ignoring";_tag_arr(_tmp726,sizeof(char),
_get_zero_arr_size(_tmp726,25));}),_tag_arr(_tmp725,sizeof(void*),0));});yyval=(
void*)({struct Cyc_ParamDecl_tok_struct*_tmp727=_cycalloc(sizeof(*_tmp727));
_tmp727[0]=({struct Cyc_ParamDecl_tok_struct _tmp728;_tmp728.tag=Cyc_ParamDecl_tok;
_tmp728.f1=({struct _tuple2*_tmp729=_cycalloc(sizeof(*_tmp729));_tmp729->f1=0;
_tmp729->f2=_tmp722.f1;_tmp729->f3=_tmp722.f2;_tmp729;});_tmp728;});_tmp727;});
break;}}case 234: _LL2D9: yyval=(void*)({struct Cyc_Type_tok_struct*_tmp72A=
_cycalloc(sizeof(*_tmp72A));_tmp72A[0]=({struct Cyc_Type_tok_struct _tmp72B;
_tmp72B.tag=Cyc_Type_tok;_tmp72B.f1=(void*)(*Cyc_yyget_ParamDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)])).f3;_tmp72B;});_tmp72A;});
break;case 235: _LL2DA: yyval=(void*)({struct Cyc_Type_tok_struct*_tmp72C=_cycalloc(
sizeof(*_tmp72C));_tmp72C[0]=({struct Cyc_Type_tok_struct _tmp72D;_tmp72D.tag=Cyc_Type_tok;
_tmp72D.f1=(void*)((void*)({struct Cyc_Absyn_JoinEff_struct*_tmp72E=_cycalloc(
sizeof(*_tmp72E));_tmp72E[0]=({struct Cyc_Absyn_JoinEff_struct _tmp72F;_tmp72F.tag=
20;_tmp72F.f1=0;_tmp72F;});_tmp72E;}));_tmp72D;});_tmp72C;});break;case 236:
_LL2DB: yyval=(void*)({struct Cyc_Type_tok_struct*_tmp730=_cycalloc(sizeof(*
_tmp730));_tmp730[0]=({struct Cyc_Type_tok_struct _tmp731;_tmp731.tag=Cyc_Type_tok;
_tmp731.f1=(void*)((void*)({struct Cyc_Absyn_JoinEff_struct*_tmp732=_cycalloc(
sizeof(*_tmp732));_tmp732[0]=({struct Cyc_Absyn_JoinEff_struct _tmp733;_tmp733.tag=
20;_tmp733.f1=Cyc_yyget_TypeList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 1)]);_tmp733;});_tmp732;}));_tmp731;});_tmp730;});break;case 237:
_LL2DC: yyval=(void*)({struct Cyc_Type_tok_struct*_tmp734=_cycalloc(sizeof(*
_tmp734));_tmp734[0]=({struct Cyc_Type_tok_struct _tmp735;_tmp735.tag=Cyc_Type_tok;
_tmp735.f1=(void*)((void*)({struct Cyc_Absyn_RgnsEff_struct*_tmp736=_cycalloc(
sizeof(*_tmp736));_tmp736[0]=({struct Cyc_Absyn_RgnsEff_struct _tmp737;_tmp737.tag=
21;_tmp737.f1=(void*)Cyc_yyget_Type_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 1)]);_tmp737;});_tmp736;}));_tmp735;});_tmp734;});break;case
238: _LL2DD: yyval=(void*)({struct Cyc_Type_tok_struct*_tmp738=_cycalloc(sizeof(*
_tmp738));_tmp738[0]=({struct Cyc_Type_tok_struct _tmp739;_tmp739.tag=Cyc_Type_tok;
_tmp739.f1=(void*)((void*)({struct Cyc_Absyn_JoinEff_struct*_tmp73A=_cycalloc(
sizeof(*_tmp73A));_tmp73A[0]=({struct Cyc_Absyn_JoinEff_struct _tmp73B;_tmp73B.tag=
20;_tmp73B.f1=({struct Cyc_List_List*_tmp73C=_cycalloc(sizeof(*_tmp73C));_tmp73C->hd=(
void*)Cyc_yyget_Type_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 
2)]);_tmp73C->tl=Cyc_yyget_TypeList_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]);_tmp73C;});_tmp73B;});_tmp73A;}));_tmp739;});_tmp738;});
break;case 239: _LL2DE: yyval=(void*)({struct Cyc_TypeList_tok_struct*_tmp73D=
_cycalloc(sizeof(*_tmp73D));_tmp73D[0]=({struct Cyc_TypeList_tok_struct _tmp73E;
_tmp73E.tag=Cyc_TypeList_tok;_tmp73E.f1=({struct Cyc_List_List*_tmp73F=_cycalloc(
sizeof(*_tmp73F));_tmp73F->hd=(void*)Cyc_yyget_Type_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp73F->tl=0;_tmp73F;});
_tmp73E;});_tmp73D;});break;case 240: _LL2DF: yyval=(void*)({struct Cyc_TypeList_tok_struct*
_tmp740=_cycalloc(sizeof(*_tmp740));_tmp740[0]=({struct Cyc_TypeList_tok_struct
_tmp741;_tmp741.tag=Cyc_TypeList_tok;_tmp741.f1=({struct Cyc_List_List*_tmp742=
_cycalloc(sizeof(*_tmp742));_tmp742->hd=(void*)Cyc_yyget_Type_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp742->tl=Cyc_yyget_TypeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp742;});_tmp741;});
_tmp740;});break;case 241: _LL2E0: yyval=(void*)({struct Cyc_AbstractDeclarator_tok_struct*
_tmp743=_cycalloc(sizeof(*_tmp743));_tmp743[0]=({struct Cyc_AbstractDeclarator_tok_struct
_tmp744;_tmp744.tag=Cyc_AbstractDeclarator_tok;_tmp744.f1=({struct Cyc_Parse_Abstractdeclarator*
_tmp745=_cycalloc(sizeof(*_tmp745));_tmp745->tms=Cyc_yyget_TypeModifierList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp745;});_tmp744;});
_tmp743;});break;case 242: _LL2E1: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 243: _LL2E2: yyval=(void*)({struct Cyc_AbstractDeclarator_tok_struct*
_tmp746=_cycalloc(sizeof(*_tmp746));_tmp746[0]=({struct Cyc_AbstractDeclarator_tok_struct
_tmp747;_tmp747.tag=Cyc_AbstractDeclarator_tok;_tmp747.f1=({struct Cyc_Parse_Abstractdeclarator*
_tmp748=_cycalloc(sizeof(*_tmp748));_tmp748->tms=Cyc_List_imp_append(Cyc_yyget_TypeModifierList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),(Cyc_yyget_AbstractDeclarator_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]))->tms);_tmp748;});
_tmp747;});_tmp746;});break;case 244: _LL2E3: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)];break;case 245: _LL2E4:
yyval=(void*)({struct Cyc_AbstractDeclarator_tok_struct*_tmp749=_cycalloc(sizeof(*
_tmp749));_tmp749[0]=({struct Cyc_AbstractDeclarator_tok_struct _tmp74A;_tmp74A.tag=
Cyc_AbstractDeclarator_tok;_tmp74A.f1=({struct Cyc_Parse_Abstractdeclarator*
_tmp74B=_cycalloc(sizeof(*_tmp74B));_tmp74B->tms=({struct Cyc_List_List*_tmp74C=
_cycalloc(sizeof(*_tmp74C));_tmp74C->hd=(void*)((void*)({struct Cyc_Absyn_Carray_mod_struct*
_tmp74D=_cycalloc(sizeof(*_tmp74D));_tmp74D[0]=({struct Cyc_Absyn_Carray_mod_struct
_tmp74E;_tmp74E.tag=0;_tmp74E.f1=Cyc_yyget_YY4(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp74E;});_tmp74D;}));
_tmp74C->tl=0;_tmp74C;});_tmp74B;});_tmp74A;});_tmp749;});break;case 246: _LL2E5:
yyval=(void*)({struct Cyc_AbstractDeclarator_tok_struct*_tmp74F=_cycalloc(sizeof(*
_tmp74F));_tmp74F[0]=({struct Cyc_AbstractDeclarator_tok_struct _tmp750;_tmp750.tag=
Cyc_AbstractDeclarator_tok;_tmp750.f1=({struct Cyc_Parse_Abstractdeclarator*
_tmp751=_cycalloc(sizeof(*_tmp751));_tmp751->tms=({struct Cyc_List_List*_tmp752=
_cycalloc(sizeof(*_tmp752));_tmp752->hd=(void*)((void*)({struct Cyc_Absyn_Carray_mod_struct*
_tmp753=_cycalloc(sizeof(*_tmp753));_tmp753[0]=({struct Cyc_Absyn_Carray_mod_struct
_tmp754;_tmp754.tag=0;_tmp754.f1=Cyc_yyget_YY4(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp754;});_tmp753;}));
_tmp752->tl=(Cyc_yyget_AbstractDeclarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]))->tms;_tmp752;});_tmp751;});
_tmp750;});_tmp74F;});break;case 247: _LL2E6: yyval=(void*)({struct Cyc_AbstractDeclarator_tok_struct*
_tmp755=_cycalloc(sizeof(*_tmp755));_tmp755[0]=({struct Cyc_AbstractDeclarator_tok_struct
_tmp756;_tmp756.tag=Cyc_AbstractDeclarator_tok;_tmp756.f1=({struct Cyc_Parse_Abstractdeclarator*
_tmp757=_cycalloc(sizeof(*_tmp757));_tmp757->tms=({struct Cyc_List_List*_tmp758=
_cycalloc(sizeof(*_tmp758));_tmp758->hd=(void*)((void*)({struct Cyc_Absyn_ConstArray_mod_struct*
_tmp759=_cycalloc(sizeof(*_tmp759));_tmp759[0]=({struct Cyc_Absyn_ConstArray_mod_struct
_tmp75A;_tmp75A.tag=1;_tmp75A.f1=Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp75A.f2=Cyc_yyget_YY4(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp75A;});_tmp759;}));
_tmp758->tl=0;_tmp758;});_tmp757;});_tmp756;});_tmp755;});break;case 248: _LL2E7:
yyval=(void*)({struct Cyc_AbstractDeclarator_tok_struct*_tmp75B=_cycalloc(sizeof(*
_tmp75B));_tmp75B[0]=({struct Cyc_AbstractDeclarator_tok_struct _tmp75C;_tmp75C.tag=
Cyc_AbstractDeclarator_tok;_tmp75C.f1=({struct Cyc_Parse_Abstractdeclarator*
_tmp75D=_cycalloc(sizeof(*_tmp75D));_tmp75D->tms=({struct Cyc_List_List*_tmp75E=
_cycalloc(sizeof(*_tmp75E));_tmp75E->hd=(void*)((void*)({struct Cyc_Absyn_ConstArray_mod_struct*
_tmp75F=_cycalloc(sizeof(*_tmp75F));_tmp75F[0]=({struct Cyc_Absyn_ConstArray_mod_struct
_tmp760;_tmp760.tag=1;_tmp760.f1=Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp760.f2=Cyc_yyget_YY4(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp760;});_tmp75F;}));
_tmp75E->tl=(Cyc_yyget_AbstractDeclarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]))->tms;_tmp75E;});_tmp75D;});
_tmp75C;});_tmp75B;});break;case 249: _LL2E8: yyval=(void*)({struct Cyc_AbstractDeclarator_tok_struct*
_tmp761=_cycalloc(sizeof(*_tmp761));_tmp761[0]=({struct Cyc_AbstractDeclarator_tok_struct
_tmp762;_tmp762.tag=Cyc_AbstractDeclarator_tok;_tmp762.f1=({struct Cyc_Parse_Abstractdeclarator*
_tmp763=_cycalloc(sizeof(*_tmp763));_tmp763->tms=({struct Cyc_List_List*_tmp764=
_cycalloc(sizeof(*_tmp764));_tmp764->hd=(void*)((void*)({struct Cyc_Absyn_Function_mod_struct*
_tmp765=_cycalloc(sizeof(*_tmp765));_tmp765[0]=({struct Cyc_Absyn_Function_mod_struct
_tmp766;_tmp766.tag=3;_tmp766.f1=(void*)((void*)({struct Cyc_Absyn_WithTypes_struct*
_tmp767=_cycalloc(sizeof(*_tmp767));_tmp767[0]=({struct Cyc_Absyn_WithTypes_struct
_tmp768;_tmp768.tag=1;_tmp768.f1=0;_tmp768.f2=0;_tmp768.f3=0;_tmp768.f4=Cyc_yyget_TypeOpt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp768.f5=Cyc_yyget_Rgnorder_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp768;});_tmp767;}));
_tmp766;});_tmp765;}));_tmp764->tl=0;_tmp764;});_tmp763;});_tmp762;});_tmp761;});
break;case 250: _LL2E9: {struct _tuple14 _tmp76A;struct Cyc_List_List*_tmp76B;int
_tmp76C;struct Cyc_Absyn_VarargInfo*_tmp76D;struct Cyc_Core_Opt*_tmp76E;struct Cyc_List_List*
_tmp76F;struct _tuple14*_tmp769=Cyc_yyget_YY3(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 1)]);_tmp76A=*_tmp769;_tmp76B=_tmp76A.f1;_tmp76C=_tmp76A.f2;
_tmp76D=_tmp76A.f3;_tmp76E=_tmp76A.f4;_tmp76F=_tmp76A.f5;yyval=(void*)({struct
Cyc_AbstractDeclarator_tok_struct*_tmp770=_cycalloc(sizeof(*_tmp770));_tmp770[0]=({
struct Cyc_AbstractDeclarator_tok_struct _tmp771;_tmp771.tag=Cyc_AbstractDeclarator_tok;
_tmp771.f1=({struct Cyc_Parse_Abstractdeclarator*_tmp772=_cycalloc(sizeof(*
_tmp772));_tmp772->tms=({struct Cyc_List_List*_tmp773=_cycalloc(sizeof(*_tmp773));
_tmp773->hd=(void*)((void*)({struct Cyc_Absyn_Function_mod_struct*_tmp774=
_cycalloc(sizeof(*_tmp774));_tmp774[0]=({struct Cyc_Absyn_Function_mod_struct
_tmp775;_tmp775.tag=3;_tmp775.f1=(void*)((void*)({struct Cyc_Absyn_WithTypes_struct*
_tmp776=_cycalloc(sizeof(*_tmp776));_tmp776[0]=({struct Cyc_Absyn_WithTypes_struct
_tmp777;_tmp777.tag=1;_tmp777.f1=_tmp76B;_tmp777.f2=_tmp76C;_tmp777.f3=_tmp76D;
_tmp777.f4=_tmp76E;_tmp777.f5=_tmp76F;_tmp777;});_tmp776;}));_tmp775;});_tmp774;}));
_tmp773->tl=0;_tmp773;});_tmp772;});_tmp771;});_tmp770;});break;}case 251: _LL2EA:
yyval=(void*)({struct Cyc_AbstractDeclarator_tok_struct*_tmp778=_cycalloc(sizeof(*
_tmp778));_tmp778[0]=({struct Cyc_AbstractDeclarator_tok_struct _tmp779;_tmp779.tag=
Cyc_AbstractDeclarator_tok;_tmp779.f1=({struct Cyc_Parse_Abstractdeclarator*
_tmp77A=_cycalloc(sizeof(*_tmp77A));_tmp77A->tms=({struct Cyc_List_List*_tmp77B=
_cycalloc(sizeof(*_tmp77B));_tmp77B->hd=(void*)((void*)({struct Cyc_Absyn_Function_mod_struct*
_tmp77C=_cycalloc(sizeof(*_tmp77C));_tmp77C[0]=({struct Cyc_Absyn_Function_mod_struct
_tmp77D;_tmp77D.tag=3;_tmp77D.f1=(void*)((void*)({struct Cyc_Absyn_WithTypes_struct*
_tmp77E=_cycalloc(sizeof(*_tmp77E));_tmp77E[0]=({struct Cyc_Absyn_WithTypes_struct
_tmp77F;_tmp77F.tag=1;_tmp77F.f1=0;_tmp77F.f2=0;_tmp77F.f3=0;_tmp77F.f4=Cyc_yyget_TypeOpt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp77F.f5=Cyc_yyget_Rgnorder_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp77F;});_tmp77E;}));
_tmp77D;});_tmp77C;}));_tmp77B->tl=(Cyc_yyget_AbstractDeclarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]))->tms;_tmp77B;});_tmp77A;});
_tmp779;});_tmp778;});break;case 252: _LL2EB: {struct _tuple14 _tmp781;struct Cyc_List_List*
_tmp782;int _tmp783;struct Cyc_Absyn_VarargInfo*_tmp784;struct Cyc_Core_Opt*_tmp785;
struct Cyc_List_List*_tmp786;struct _tuple14*_tmp780=Cyc_yyget_YY3(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp781=*_tmp780;_tmp782=
_tmp781.f1;_tmp783=_tmp781.f2;_tmp784=_tmp781.f3;_tmp785=_tmp781.f4;_tmp786=
_tmp781.f5;yyval=(void*)({struct Cyc_AbstractDeclarator_tok_struct*_tmp787=
_cycalloc(sizeof(*_tmp787));_tmp787[0]=({struct Cyc_AbstractDeclarator_tok_struct
_tmp788;_tmp788.tag=Cyc_AbstractDeclarator_tok;_tmp788.f1=({struct Cyc_Parse_Abstractdeclarator*
_tmp789=_cycalloc(sizeof(*_tmp789));_tmp789->tms=({struct Cyc_List_List*_tmp78A=
_cycalloc(sizeof(*_tmp78A));_tmp78A->hd=(void*)((void*)({struct Cyc_Absyn_Function_mod_struct*
_tmp78B=_cycalloc(sizeof(*_tmp78B));_tmp78B[0]=({struct Cyc_Absyn_Function_mod_struct
_tmp78C;_tmp78C.tag=3;_tmp78C.f1=(void*)((void*)({struct Cyc_Absyn_WithTypes_struct*
_tmp78D=_cycalloc(sizeof(*_tmp78D));_tmp78D[0]=({struct Cyc_Absyn_WithTypes_struct
_tmp78E;_tmp78E.tag=1;_tmp78E.f1=_tmp782;_tmp78E.f2=_tmp783;_tmp78E.f3=_tmp784;
_tmp78E.f4=_tmp785;_tmp78E.f5=_tmp786;_tmp78E;});_tmp78D;}));_tmp78C;});_tmp78B;}));
_tmp78A->tl=(Cyc_yyget_AbstractDeclarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]))->tms;_tmp78A;});_tmp789;});
_tmp788;});_tmp787;});break;}case 253: _LL2EC: {struct Cyc_List_List*_tmp78F=((
struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Position_Segment*,
void*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 2)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line),Cyc_List_imp_rev(Cyc_yyget_TypeList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)])));yyval=(void*)({struct
Cyc_AbstractDeclarator_tok_struct*_tmp790=_cycalloc(sizeof(*_tmp790));_tmp790[0]=({
struct Cyc_AbstractDeclarator_tok_struct _tmp791;_tmp791.tag=Cyc_AbstractDeclarator_tok;
_tmp791.f1=({struct Cyc_Parse_Abstractdeclarator*_tmp792=_cycalloc(sizeof(*
_tmp792));_tmp792->tms=({struct Cyc_List_List*_tmp793=_cycalloc(sizeof(*_tmp793));
_tmp793->hd=(void*)((void*)({struct Cyc_Absyn_TypeParams_mod_struct*_tmp794=
_cycalloc(sizeof(*_tmp794));_tmp794[0]=({struct Cyc_Absyn_TypeParams_mod_struct
_tmp795;_tmp795.tag=4;_tmp795.f1=_tmp78F;_tmp795.f2=Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp795.f3=0;
_tmp795;});_tmp794;}));_tmp793->tl=(Cyc_yyget_AbstractDeclarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]))->tms;_tmp793;});_tmp792;});
_tmp791;});_tmp790;});break;}case 254: _LL2ED: yyval=(void*)({struct Cyc_AbstractDeclarator_tok_struct*
_tmp796=_cycalloc(sizeof(*_tmp796));_tmp796[0]=({struct Cyc_AbstractDeclarator_tok_struct
_tmp797;_tmp797.tag=Cyc_AbstractDeclarator_tok;_tmp797.f1=({struct Cyc_Parse_Abstractdeclarator*
_tmp798=_cycalloc(sizeof(*_tmp798));_tmp798->tms=({struct Cyc_List_List*_tmp799=
_cycalloc(sizeof(*_tmp799));_tmp799->hd=(void*)((void*)({struct Cyc_Absyn_Attributes_mod_struct*
_tmp79A=_cycalloc(sizeof(*_tmp79A));_tmp79A[0]=({struct Cyc_Absyn_Attributes_mod_struct
_tmp79B;_tmp79B.tag=5;_tmp79B.f1=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp79B.f2=Cyc_yyget_AttributeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp79B;});_tmp79A;}));
_tmp799->tl=(Cyc_yyget_AbstractDeclarator_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]))->tms;_tmp799;});_tmp798;});
_tmp797;});_tmp796;});break;case 255: _LL2EE: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 256: _LL2EF: yyval=
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 257: _LL2F0:
yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 258:
_LL2F1: yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;case
259: _LL2F2: yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;
case 260: _LL2F3: yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];
break;case 261: _LL2F4: if(Cyc_zstrcmp((struct _tagged_arr)Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]),({const char*_tmp79C="`H";
_tag_arr(_tmp79C,sizeof(char),_get_zero_arr_size(_tmp79C,3));}))== 0)Cyc_Parse_err(({
const char*_tmp79D="bad occurrence of heap region `H";_tag_arr(_tmp79D,sizeof(
char),_get_zero_arr_size(_tmp79D,33));}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).last_line));{struct Cyc_Absyn_Tvar*
tv=({struct Cyc_Absyn_Tvar*_tmp7A8=_cycalloc(sizeof(*_tmp7A8));_tmp7A8->name=({
struct _tagged_arr*_tmp7AB=_cycalloc(sizeof(*_tmp7AB));_tmp7AB[0]=Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp7AB;});_tmp7A8->identity=
0;_tmp7A8->kind=(void*)((void*)({struct Cyc_Absyn_Eq_kb_struct*_tmp7A9=_cycalloc(
sizeof(*_tmp7A9));_tmp7A9[0]=({struct Cyc_Absyn_Eq_kb_struct _tmp7AA;_tmp7AA.tag=0;
_tmp7AA.f1=(void*)((void*)3);_tmp7AA;});_tmp7A9;}));_tmp7A8;});void*t=(void*)({
struct Cyc_Absyn_VarType_struct*_tmp7A6=_cycalloc(sizeof(*_tmp7A6));_tmp7A6[0]=({
struct Cyc_Absyn_VarType_struct _tmp7A7;_tmp7A7.tag=1;_tmp7A7.f1=tv;_tmp7A7;});
_tmp7A6;});yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp79E=_cycalloc(sizeof(*
_tmp79E));_tmp79E[0]=({struct Cyc_Stmt_tok_struct _tmp79F;_tmp79F.tag=Cyc_Stmt_tok;
_tmp79F.f1=Cyc_Absyn_new_stmt((void*)({struct Cyc_Absyn_Region_s_struct*_tmp7A0=
_cycalloc(sizeof(*_tmp7A0));_tmp7A0[0]=({struct Cyc_Absyn_Region_s_struct _tmp7A1;
_tmp7A1.tag=15;_tmp7A1.f1=tv;_tmp7A1.f2=Cyc_Absyn_new_vardecl(({struct _tuple1*
_tmp7A2=_cycalloc(sizeof(*_tmp7A2));_tmp7A2->f1=(void*)0;_tmp7A2->f2=({struct
_tagged_arr*_tmp7A3=_cycalloc(sizeof(*_tmp7A3));_tmp7A3[0]=Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp7A3;});_tmp7A2;}),(
void*)({struct Cyc_Absyn_RgnHandleType_struct*_tmp7A4=_cycalloc(sizeof(*_tmp7A4));
_tmp7A4[0]=({struct Cyc_Absyn_RgnHandleType_struct _tmp7A5;_tmp7A5.tag=15;_tmp7A5.f1=(
void*)t;_tmp7A5;});_tmp7A4;}),0);_tmp7A1.f3=0;_tmp7A1.f4=Cyc_yyget_Stmt_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp7A1;});_tmp7A0;}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 5)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp79F;});
_tmp79E;});break;}case 262: _LL2F5: if(Cyc_zstrcmp((struct _tagged_arr)Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),({const char*_tmp7AC="H";
_tag_arr(_tmp7AC,sizeof(char),_get_zero_arr_size(_tmp7AC,2));}))== 0)Cyc_Parse_err(({
const char*_tmp7AD="bad occurrence of heap region `H";_tag_arr(_tmp7AD,sizeof(
char),_get_zero_arr_size(_tmp7AD,33));}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line));{struct Cyc_Absyn_Tvar*
tv=({struct Cyc_Absyn_Tvar*_tmp7B8=_cycalloc(sizeof(*_tmp7B8));_tmp7B8->name=({
struct _tagged_arr*_tmp7BB=_cycalloc(sizeof(*_tmp7BB));_tmp7BB[0]=(struct
_tagged_arr)({struct Cyc_String_pa_struct _tmp7BE;_tmp7BE.tag=0;_tmp7BE.f1=(struct
_tagged_arr)((struct _tagged_arr)Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]));{void*_tmp7BC[1]={&
_tmp7BE};Cyc_aprintf(({const char*_tmp7BD="`%s";_tag_arr(_tmp7BD,sizeof(char),
_get_zero_arr_size(_tmp7BD,4));}),_tag_arr(_tmp7BC,sizeof(void*),1));}});_tmp7BB;});
_tmp7B8->identity=0;_tmp7B8->kind=(void*)((void*)({struct Cyc_Absyn_Eq_kb_struct*
_tmp7B9=_cycalloc(sizeof(*_tmp7B9));_tmp7B9[0]=({struct Cyc_Absyn_Eq_kb_struct
_tmp7BA;_tmp7BA.tag=0;_tmp7BA.f1=(void*)((void*)3);_tmp7BA;});_tmp7B9;}));
_tmp7B8;});void*t=(void*)({struct Cyc_Absyn_VarType_struct*_tmp7B6=_cycalloc(
sizeof(*_tmp7B6));_tmp7B6[0]=({struct Cyc_Absyn_VarType_struct _tmp7B7;_tmp7B7.tag=
1;_tmp7B7.f1=tv;_tmp7B7;});_tmp7B6;});yyval=(void*)({struct Cyc_Stmt_tok_struct*
_tmp7AE=_cycalloc(sizeof(*_tmp7AE));_tmp7AE[0]=({struct Cyc_Stmt_tok_struct
_tmp7AF;_tmp7AF.tag=Cyc_Stmt_tok;_tmp7AF.f1=Cyc_Absyn_new_stmt((void*)({struct
Cyc_Absyn_Region_s_struct*_tmp7B0=_cycalloc(sizeof(*_tmp7B0));_tmp7B0[0]=({
struct Cyc_Absyn_Region_s_struct _tmp7B1;_tmp7B1.tag=15;_tmp7B1.f1=tv;_tmp7B1.f2=
Cyc_Absyn_new_vardecl(({struct _tuple1*_tmp7B2=_cycalloc(sizeof(*_tmp7B2));
_tmp7B2->f1=(void*)0;_tmp7B2->f2=({struct _tagged_arr*_tmp7B3=_cycalloc(sizeof(*
_tmp7B3));_tmp7B3[0]=Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 1)]);_tmp7B3;});_tmp7B2;}),(void*)({struct Cyc_Absyn_RgnHandleType_struct*
_tmp7B4=_cycalloc(sizeof(*_tmp7B4));_tmp7B4[0]=({struct Cyc_Absyn_RgnHandleType_struct
_tmp7B5;_tmp7B5.tag=15;_tmp7B5.f1=(void*)t;_tmp7B5;});_tmp7B4;}),0);_tmp7B1.f3=0;
_tmp7B1.f4=Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]);_tmp7B1;});_tmp7B0;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp7AF;});
_tmp7AE;});break;}case 263: _LL2F6: if(Cyc_zstrcmp((struct _tagged_arr)Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 6)]),({const char*_tmp7BF="resetable";
_tag_arr(_tmp7BF,sizeof(char),_get_zero_arr_size(_tmp7BF,10));}))!= 0)Cyc_Parse_err(({
const char*_tmp7C0="expecting [resetable]";_tag_arr(_tmp7C0,sizeof(char),
_get_zero_arr_size(_tmp7C0,22));}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 6)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 6)]).last_line));if(Cyc_zstrcmp((
struct _tagged_arr)Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 3)]),({const char*_tmp7C1="`H";_tag_arr(_tmp7C1,sizeof(char),
_get_zero_arr_size(_tmp7C1,3));}))== 0)Cyc_Parse_err(({const char*_tmp7C2="bad occurrence of heap region `H";
_tag_arr(_tmp7C2,sizeof(char),_get_zero_arr_size(_tmp7C2,33));}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).last_line));{struct Cyc_Absyn_Tvar*
tv=({struct Cyc_Absyn_Tvar*_tmp7CD=_cycalloc(sizeof(*_tmp7CD));_tmp7CD->name=({
struct _tagged_arr*_tmp7D0=_cycalloc(sizeof(*_tmp7D0));_tmp7D0[0]=Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp7D0;});_tmp7CD->identity=
0;_tmp7CD->kind=(void*)((void*)({struct Cyc_Absyn_Eq_kb_struct*_tmp7CE=_cycalloc(
sizeof(*_tmp7CE));_tmp7CE[0]=({struct Cyc_Absyn_Eq_kb_struct _tmp7CF;_tmp7CF.tag=0;
_tmp7CF.f1=(void*)((void*)3);_tmp7CF;});_tmp7CE;}));_tmp7CD;});void*t=(void*)({
struct Cyc_Absyn_VarType_struct*_tmp7CB=_cycalloc(sizeof(*_tmp7CB));_tmp7CB[0]=({
struct Cyc_Absyn_VarType_struct _tmp7CC;_tmp7CC.tag=1;_tmp7CC.f1=tv;_tmp7CC;});
_tmp7CB;});yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp7C3=_cycalloc(sizeof(*
_tmp7C3));_tmp7C3[0]=({struct Cyc_Stmt_tok_struct _tmp7C4;_tmp7C4.tag=Cyc_Stmt_tok;
_tmp7C4.f1=Cyc_Absyn_new_stmt((void*)({struct Cyc_Absyn_Region_s_struct*_tmp7C5=
_cycalloc(sizeof(*_tmp7C5));_tmp7C5[0]=({struct Cyc_Absyn_Region_s_struct _tmp7C6;
_tmp7C6.tag=15;_tmp7C6.f1=tv;_tmp7C6.f2=Cyc_Absyn_new_vardecl(({struct _tuple1*
_tmp7C7=_cycalloc(sizeof(*_tmp7C7));_tmp7C7->f1=(void*)0;_tmp7C7->f2=({struct
_tagged_arr*_tmp7C8=_cycalloc(sizeof(*_tmp7C8));_tmp7C8[0]=Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp7C8;});_tmp7C7;}),(
void*)({struct Cyc_Absyn_RgnHandleType_struct*_tmp7C9=_cycalloc(sizeof(*_tmp7C9));
_tmp7C9[0]=({struct Cyc_Absyn_RgnHandleType_struct _tmp7CA;_tmp7CA.tag=15;_tmp7CA.f1=(
void*)t;_tmp7CA;});_tmp7C9;}),0);_tmp7C6.f3=1;_tmp7C6.f4=Cyc_yyget_Stmt_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp7C6;});_tmp7C5;}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 8)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp7C4;});
_tmp7C3;});break;}case 264: _LL2F7: if(Cyc_zstrcmp((struct _tagged_arr)Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]),({const char*_tmp7D1="resetable";
_tag_arr(_tmp7D1,sizeof(char),_get_zero_arr_size(_tmp7D1,10));}))!= 0)Cyc_Parse_err(({
const char*_tmp7D2="expecting `resetable'";_tag_arr(_tmp7D2,sizeof(char),
_get_zero_arr_size(_tmp7D2,22));}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).last_line));if(Cyc_zstrcmp((
struct _tagged_arr)Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 1)]),({const char*_tmp7D3="H";_tag_arr(_tmp7D3,sizeof(char),
_get_zero_arr_size(_tmp7D3,2));}))== 0)Cyc_Parse_err(({const char*_tmp7D4="bad occurrence of heap region `H";
_tag_arr(_tmp7D4,sizeof(char),_get_zero_arr_size(_tmp7D4,33));}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line));{struct Cyc_Absyn_Tvar*
tv=({struct Cyc_Absyn_Tvar*_tmp7DF=_cycalloc(sizeof(*_tmp7DF));_tmp7DF->name=({
struct _tagged_arr*_tmp7E2=_cycalloc(sizeof(*_tmp7E2));_tmp7E2[0]=(struct
_tagged_arr)({struct Cyc_String_pa_struct _tmp7E5;_tmp7E5.tag=0;_tmp7E5.f1=(struct
_tagged_arr)((struct _tagged_arr)Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]));{void*_tmp7E3[1]={&
_tmp7E5};Cyc_aprintf(({const char*_tmp7E4="`%s";_tag_arr(_tmp7E4,sizeof(char),
_get_zero_arr_size(_tmp7E4,4));}),_tag_arr(_tmp7E3,sizeof(void*),1));}});_tmp7E2;});
_tmp7DF->identity=0;_tmp7DF->kind=(void*)((void*)({struct Cyc_Absyn_Eq_kb_struct*
_tmp7E0=_cycalloc(sizeof(*_tmp7E0));_tmp7E0[0]=({struct Cyc_Absyn_Eq_kb_struct
_tmp7E1;_tmp7E1.tag=0;_tmp7E1.f1=(void*)((void*)3);_tmp7E1;});_tmp7E0;}));
_tmp7DF;});void*t=(void*)({struct Cyc_Absyn_VarType_struct*_tmp7DD=_cycalloc(
sizeof(*_tmp7DD));_tmp7DD[0]=({struct Cyc_Absyn_VarType_struct _tmp7DE;_tmp7DE.tag=
1;_tmp7DE.f1=tv;_tmp7DE;});_tmp7DD;});yyval=(void*)({struct Cyc_Stmt_tok_struct*
_tmp7D5=_cycalloc(sizeof(*_tmp7D5));_tmp7D5[0]=({struct Cyc_Stmt_tok_struct
_tmp7D6;_tmp7D6.tag=Cyc_Stmt_tok;_tmp7D6.f1=Cyc_Absyn_new_stmt((void*)({struct
Cyc_Absyn_Region_s_struct*_tmp7D7=_cycalloc(sizeof(*_tmp7D7));_tmp7D7[0]=({
struct Cyc_Absyn_Region_s_struct _tmp7D8;_tmp7D8.tag=15;_tmp7D8.f1=tv;_tmp7D8.f2=
Cyc_Absyn_new_vardecl(({struct _tuple1*_tmp7D9=_cycalloc(sizeof(*_tmp7D9));
_tmp7D9->f1=(void*)0;_tmp7D9->f2=({struct _tagged_arr*_tmp7DA=_cycalloc(sizeof(*
_tmp7DA));_tmp7DA[0]=Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 1)]);_tmp7DA;});_tmp7D9;}),(void*)({struct Cyc_Absyn_RgnHandleType_struct*
_tmp7DB=_cycalloc(sizeof(*_tmp7DB));_tmp7DB[0]=({struct Cyc_Absyn_RgnHandleType_struct
_tmp7DC;_tmp7DC.tag=15;_tmp7DC.f1=(void*)t;_tmp7DC;});_tmp7DB;}),0);_tmp7D8.f3=1;
_tmp7D8.f4=Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]);_tmp7D8;});_tmp7D7;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 5)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp7D6;});
_tmp7D5;});break;}case 265: _LL2F8: yyval=(void*)({struct Cyc_Stmt_tok_struct*
_tmp7E6=_cycalloc(sizeof(*_tmp7E6));_tmp7E6[0]=({struct Cyc_Stmt_tok_struct
_tmp7E7;_tmp7E7.tag=Cyc_Stmt_tok;_tmp7E7.f1=Cyc_Absyn_new_stmt((void*)({struct
Cyc_Absyn_ResetRegion_s_struct*_tmp7E8=_cycalloc(sizeof(*_tmp7E8));_tmp7E8[0]=({
struct Cyc_Absyn_ResetRegion_s_struct _tmp7E9;_tmp7E9.tag=16;_tmp7E9.f1=Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp7E9;});_tmp7E8;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 4)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp7E7;});_tmp7E6;});break;case 266: _LL2F9: yyval=(
void*)({struct Cyc_Stmt_tok_struct*_tmp7EA=_cycalloc(sizeof(*_tmp7EA));_tmp7EA[0]=({
struct Cyc_Stmt_tok_struct _tmp7EB;_tmp7EB.tag=Cyc_Stmt_tok;_tmp7EB.f1=Cyc_Absyn_new_stmt((
void*)({struct Cyc_Absyn_Label_s_struct*_tmp7EC=_cycalloc(sizeof(*_tmp7EC));
_tmp7EC[0]=({struct Cyc_Absyn_Label_s_struct _tmp7ED;_tmp7ED.tag=12;_tmp7ED.f1=({
struct _tagged_arr*_tmp7EE=_cycalloc(sizeof(*_tmp7EE));_tmp7EE[0]=Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp7EE;});_tmp7ED.f2=
Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);
_tmp7ED;});_tmp7EC;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp7EB;});
_tmp7EA;});break;case 267: _LL2FA: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp7EF=
_cycalloc(sizeof(*_tmp7EF));_tmp7EF[0]=({struct Cyc_Stmt_tok_struct _tmp7F0;
_tmp7F0.tag=Cyc_Stmt_tok;_tmp7F0.f1=Cyc_Absyn_skip_stmt(Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp7F0;});
_tmp7EF;});break;case 268: _LL2FB: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp7F1=
_cycalloc(sizeof(*_tmp7F1));_tmp7F1[0]=({struct Cyc_Stmt_tok_struct _tmp7F2;
_tmp7F2.tag=Cyc_Stmt_tok;_tmp7F2.f1=Cyc_Absyn_exp_stmt(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp7F2;});
_tmp7F1;});break;case 269: _LL2FC: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp7F3=
_cycalloc(sizeof(*_tmp7F3));_tmp7F3[0]=({struct Cyc_Stmt_tok_struct _tmp7F4;
_tmp7F4.tag=Cyc_Stmt_tok;_tmp7F4.f1=Cyc_Absyn_skip_stmt(Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp7F4;});
_tmp7F3;});break;case 270: _LL2FD: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 1)];break;case 271: _LL2FE: yyval=(void*)({struct Cyc_Stmt_tok_struct*
_tmp7F5=_cycalloc(sizeof(*_tmp7F5));_tmp7F5[0]=({struct Cyc_Stmt_tok_struct
_tmp7F6;_tmp7F6.tag=Cyc_Stmt_tok;_tmp7F6.f1=Cyc_Parse_flatten_declarations(Cyc_yyget_DeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Absyn_skip_stmt(0));
_tmp7F6;});_tmp7F5;});break;case 272: _LL2FF: yyval=(void*)({struct Cyc_Stmt_tok_struct*
_tmp7F7=_cycalloc(sizeof(*_tmp7F7));_tmp7F7[0]=({struct Cyc_Stmt_tok_struct
_tmp7F8;_tmp7F8.tag=Cyc_Stmt_tok;_tmp7F8.f1=Cyc_Parse_flatten_declarations(Cyc_yyget_DeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]));_tmp7F8;});_tmp7F7;});
break;case 273: _LL300: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 274: _LL301: yyval=(void*)({struct Cyc_Stmt_tok_struct*
_tmp7F9=_cycalloc(sizeof(*_tmp7F9));_tmp7F9[0]=({struct Cyc_Stmt_tok_struct
_tmp7FA;_tmp7FA.tag=Cyc_Stmt_tok;_tmp7FA.f1=Cyc_Absyn_seq_stmt(Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp7FA;});
_tmp7F9;});break;case 275: _LL302: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp7FB=
_cycalloc(sizeof(*_tmp7FB));_tmp7FB[0]=({struct Cyc_Stmt_tok_struct _tmp7FC;
_tmp7FC.tag=Cyc_Stmt_tok;_tmp7FC.f1=Cyc_Parse_flatten_decl(Cyc_Absyn_new_decl((
void*)({struct Cyc_Absyn_Fn_d_struct*_tmp7FD=_cycalloc(sizeof(*_tmp7FD));_tmp7FD[
0]=({struct Cyc_Absyn_Fn_d_struct _tmp7FE;_tmp7FE.tag=1;_tmp7FE.f1=Cyc_yyget_FnDecl_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp7FE;});_tmp7FD;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line)),Cyc_Absyn_skip_stmt(0));_tmp7FC;});_tmp7FB;});break;
case 276: _LL303: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp7FF=_cycalloc(
sizeof(*_tmp7FF));_tmp7FF[0]=({struct Cyc_Stmt_tok_struct _tmp800;_tmp800.tag=Cyc_Stmt_tok;
_tmp800.f1=Cyc_Parse_flatten_decl(Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Fn_d_struct*
_tmp801=_cycalloc(sizeof(*_tmp801));_tmp801[0]=({struct Cyc_Absyn_Fn_d_struct
_tmp802;_tmp802.tag=1;_tmp802.f1=Cyc_yyget_FnDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp802;});_tmp801;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).last_line)),Cyc_yyget_Stmt_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]));_tmp800;});_tmp7FF;});
break;case 277: _LL304: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp803=_cycalloc(
sizeof(*_tmp803));_tmp803[0]=({struct Cyc_Stmt_tok_struct _tmp804;_tmp804.tag=Cyc_Stmt_tok;
_tmp804.f1=Cyc_Absyn_ifthenelse_stmt(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Stmt_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Absyn_skip_stmt(0),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp804;});
_tmp803;});break;case 278: _LL305: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp805=
_cycalloc(sizeof(*_tmp805));_tmp805[0]=({struct Cyc_Stmt_tok_struct _tmp806;
_tmp806.tag=Cyc_Stmt_tok;_tmp806.f1=Cyc_Absyn_ifthenelse_stmt(Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 4)]),Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 6)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp806;});
_tmp805;});break;case 279: _LL306: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp807=
_cycalloc(sizeof(*_tmp807));_tmp807[0]=({struct Cyc_Stmt_tok_struct _tmp808;
_tmp808.tag=Cyc_Stmt_tok;_tmp808.f1=Cyc_Absyn_switch_stmt(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]),Cyc_yyget_SwitchClauseList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 6)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp808;});
_tmp807;});break;case 280: _LL307: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp809=
_cycalloc(sizeof(*_tmp809));_tmp809[0]=({struct Cyc_Stmt_tok_struct _tmp80A;
_tmp80A.tag=Cyc_Stmt_tok;_tmp80A.f1=Cyc_Absyn_trycatch_stmt(Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 4)]),Cyc_yyget_SwitchClauseList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 5)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp80A;});
_tmp809;});break;case 281: _LL308: yyval=(void*)({struct Cyc_SwitchClauseList_tok_struct*
_tmp80B=_cycalloc(sizeof(*_tmp80B));_tmp80B[0]=({struct Cyc_SwitchClauseList_tok_struct
_tmp80C;_tmp80C.tag=Cyc_SwitchClauseList_tok;_tmp80C.f1=0;_tmp80C;});_tmp80B;});
break;case 282: _LL309: yyval=(void*)({struct Cyc_SwitchClauseList_tok_struct*
_tmp80D=_cycalloc(sizeof(*_tmp80D));_tmp80D[0]=({struct Cyc_SwitchClauseList_tok_struct
_tmp80E;_tmp80E.tag=Cyc_SwitchClauseList_tok;_tmp80E.f1=({struct Cyc_List_List*
_tmp80F=_cycalloc(sizeof(*_tmp80F));_tmp80F->hd=({struct Cyc_Absyn_Switch_clause*
_tmp810=_cycalloc(sizeof(*_tmp810));_tmp810->pattern=Cyc_Absyn_new_pat((void*)0,
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 2)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 2)]).last_line));_tmp810->pat_vars=0;_tmp810->where_clause=0;
_tmp810->body=Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]);_tmp810->loc=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp810;});
_tmp80F->tl=0;_tmp80F;});_tmp80E;});_tmp80D;});break;case 283: _LL30A: yyval=(void*)({
struct Cyc_SwitchClauseList_tok_struct*_tmp811=_cycalloc(sizeof(*_tmp811));
_tmp811[0]=({struct Cyc_SwitchClauseList_tok_struct _tmp812;_tmp812.tag=Cyc_SwitchClauseList_tok;
_tmp812.f1=({struct Cyc_List_List*_tmp813=_cycalloc(sizeof(*_tmp813));_tmp813->hd=({
struct Cyc_Absyn_Switch_clause*_tmp814=_cycalloc(sizeof(*_tmp814));_tmp814->pattern=
Cyc_yyget_Pattern_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);
_tmp814->pat_vars=0;_tmp814->where_clause=0;_tmp814->body=Cyc_Absyn_fallthru_stmt(
0,Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).last_line));_tmp814->loc=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp814;});
_tmp813->tl=Cyc_yyget_SwitchClauseList_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]);_tmp813;});_tmp812;});_tmp811;});break;case 284: _LL30B:
yyval=(void*)({struct Cyc_SwitchClauseList_tok_struct*_tmp815=_cycalloc(sizeof(*
_tmp815));_tmp815[0]=({struct Cyc_SwitchClauseList_tok_struct _tmp816;_tmp816.tag=
Cyc_SwitchClauseList_tok;_tmp816.f1=({struct Cyc_List_List*_tmp817=_cycalloc(
sizeof(*_tmp817));_tmp817->hd=({struct Cyc_Absyn_Switch_clause*_tmp818=_cycalloc(
sizeof(*_tmp818));_tmp818->pattern=Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp818->pat_vars=0;
_tmp818->where_clause=0;_tmp818->body=Cyc_yyget_Stmt_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp818->loc=Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line);_tmp818;});
_tmp817->tl=Cyc_yyget_SwitchClauseList_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]);_tmp817;});_tmp816;});_tmp815;});break;case 285: _LL30C:
yyval=(void*)({struct Cyc_SwitchClauseList_tok_struct*_tmp819=_cycalloc(sizeof(*
_tmp819));_tmp819[0]=({struct Cyc_SwitchClauseList_tok_struct _tmp81A;_tmp81A.tag=
Cyc_SwitchClauseList_tok;_tmp81A.f1=({struct Cyc_List_List*_tmp81B=_cycalloc(
sizeof(*_tmp81B));_tmp81B->hd=({struct Cyc_Absyn_Switch_clause*_tmp81C=_cycalloc(
sizeof(*_tmp81C));_tmp81C->pattern=Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]);_tmp81C->pat_vars=0;
_tmp81C->where_clause=(struct Cyc_Absyn_Exp*)Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp81C->body=Cyc_Absyn_fallthru_stmt(
0,Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).last_line));_tmp81C->loc=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 5)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp81C;});
_tmp81B->tl=Cyc_yyget_SwitchClauseList_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]);_tmp81B;});_tmp81A;});_tmp819;});break;case 286: _LL30D:
yyval=(void*)({struct Cyc_SwitchClauseList_tok_struct*_tmp81D=_cycalloc(sizeof(*
_tmp81D));_tmp81D[0]=({struct Cyc_SwitchClauseList_tok_struct _tmp81E;_tmp81E.tag=
Cyc_SwitchClauseList_tok;_tmp81E.f1=({struct Cyc_List_List*_tmp81F=_cycalloc(
sizeof(*_tmp81F));_tmp81F->hd=({struct Cyc_Absyn_Switch_clause*_tmp820=_cycalloc(
sizeof(*_tmp820));_tmp820->pattern=Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 5)]);_tmp820->pat_vars=0;
_tmp820->where_clause=(struct Cyc_Absyn_Exp*)Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp820->body=Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp820->loc=Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 6)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);_tmp820;});
_tmp81F->tl=Cyc_yyget_SwitchClauseList_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]);_tmp81F;});_tmp81E;});_tmp81D;});break;case 287: _LL30E:
yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp821=_cycalloc(sizeof(*_tmp821));
_tmp821[0]=({struct Cyc_Stmt_tok_struct _tmp822;_tmp822.tag=Cyc_Stmt_tok;_tmp822.f1=
Cyc_Absyn_while_stmt(Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 2)]),Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(
10000,yylsp_offset - 4)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp822;});_tmp821;});break;case 288: _LL30F: yyval=(
void*)({struct Cyc_Stmt_tok_struct*_tmp823=_cycalloc(sizeof(*_tmp823));_tmp823[0]=({
struct Cyc_Stmt_tok_struct _tmp824;_tmp824.tag=Cyc_Stmt_tok;_tmp824.f1=Cyc_Absyn_do_stmt(
Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 5)]),
Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 6)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp824;});_tmp823;});break;case 289: _LL310: yyval=(
void*)({struct Cyc_Stmt_tok_struct*_tmp825=_cycalloc(sizeof(*_tmp825));_tmp825[0]=({
struct Cyc_Stmt_tok_struct _tmp826;_tmp826.tag=Cyc_Stmt_tok;_tmp826.f1=Cyc_Absyn_for_stmt(
Cyc_Absyn_false_exp(0),Cyc_Absyn_true_exp(0),Cyc_Absyn_false_exp(0),Cyc_yyget_Stmt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 5)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp826;});
_tmp825;});break;case 290: _LL311: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp827=
_cycalloc(sizeof(*_tmp827));_tmp827[0]=({struct Cyc_Stmt_tok_struct _tmp828;
_tmp828.tag=Cyc_Stmt_tok;_tmp828.f1=Cyc_Absyn_for_stmt(Cyc_Absyn_false_exp(0),
Cyc_Absyn_true_exp(0),Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 2)]),Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 6)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp828;});
_tmp827;});break;case 291: _LL312: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp829=
_cycalloc(sizeof(*_tmp829));_tmp829[0]=({struct Cyc_Stmt_tok_struct _tmp82A;
_tmp82A.tag=Cyc_Stmt_tok;_tmp82A.f1=Cyc_Absyn_for_stmt(Cyc_Absyn_false_exp(0),
Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]),
Cyc_Absyn_false_exp(0),Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 6)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp82A;});
_tmp829;});break;case 292: _LL313: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp82B=
_cycalloc(sizeof(*_tmp82B));_tmp82B[0]=({struct Cyc_Stmt_tok_struct _tmp82C;
_tmp82C.tag=Cyc_Stmt_tok;_tmp82C.f1=Cyc_Absyn_for_stmt(Cyc_Absyn_false_exp(0),
Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 4)]),
Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),
Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 7)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line));_tmp82C;});
_tmp82B;});break;case 293: _LL314: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp82D=
_cycalloc(sizeof(*_tmp82D));_tmp82D[0]=({struct Cyc_Stmt_tok_struct _tmp82E;
_tmp82E.tag=Cyc_Stmt_tok;_tmp82E.f1=Cyc_Absyn_for_stmt(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]),Cyc_Absyn_true_exp(0),
Cyc_Absyn_false_exp(0),Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 6)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp82E;});
_tmp82D;});break;case 294: _LL315: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp82F=
_cycalloc(sizeof(*_tmp82F));_tmp82F[0]=({struct Cyc_Stmt_tok_struct _tmp830;
_tmp830.tag=Cyc_Stmt_tok;_tmp830.f1=Cyc_Absyn_for_stmt(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 5)]),Cyc_Absyn_true_exp(0),
Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),
Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 7)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp830;});
_tmp82F;});break;case 295: _LL316: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp831=
_cycalloc(sizeof(*_tmp831));_tmp831[0]=({struct Cyc_Stmt_tok_struct _tmp832;
_tmp832.tag=Cyc_Stmt_tok;_tmp832.f1=Cyc_Absyn_for_stmt(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 5)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]),Cyc_Absyn_false_exp(0),
Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 7)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp832;});
_tmp831;});break;case 296: _LL317: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp833=
_cycalloc(sizeof(*_tmp833));_tmp833[0]=({struct Cyc_Stmt_tok_struct _tmp834;
_tmp834.tag=Cyc_Stmt_tok;_tmp834.f1=Cyc_Absyn_for_stmt(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 6)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Stmt_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 8)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp834;});
_tmp833;});break;case 297: _LL318: {struct Cyc_List_List*_tmp835=Cyc_yyget_DeclList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);struct Cyc_Absyn_Stmt*
_tmp836=Cyc_Absyn_for_stmt(Cyc_Absyn_false_exp(0),Cyc_Absyn_true_exp(0),Cyc_Absyn_false_exp(
0),Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 5)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp837=
_cycalloc(sizeof(*_tmp837));_tmp837[0]=({struct Cyc_Stmt_tok_struct _tmp838;
_tmp838.tag=Cyc_Stmt_tok;_tmp838.f1=Cyc_Parse_flatten_declarations(_tmp835,
_tmp836);_tmp838;});_tmp837;});break;}case 298: _LL319: {struct Cyc_List_List*
_tmp839=Cyc_yyget_DeclList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 4)]);struct Cyc_Absyn_Stmt*_tmp83A=Cyc_Absyn_for_stmt(Cyc_Absyn_false_exp(
0),Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]),
Cyc_Absyn_false_exp(0),Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 6)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));yyval=(void*)({
struct Cyc_Stmt_tok_struct*_tmp83B=_cycalloc(sizeof(*_tmp83B));_tmp83B[0]=({
struct Cyc_Stmt_tok_struct _tmp83C;_tmp83C.tag=Cyc_Stmt_tok;_tmp83C.f1=Cyc_Parse_flatten_declarations(
_tmp839,_tmp83A);_tmp83C;});_tmp83B;});break;}case 299: _LL31A: {struct Cyc_List_List*
_tmp83D=Cyc_yyget_DeclList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 4)]);struct Cyc_Absyn_Stmt*_tmp83E=Cyc_Absyn_for_stmt(Cyc_Absyn_false_exp(
0),Cyc_Absyn_true_exp(0),Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 2)]),Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 6)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));yyval=(void*)({
struct Cyc_Stmt_tok_struct*_tmp83F=_cycalloc(sizeof(*_tmp83F));_tmp83F[0]=({
struct Cyc_Stmt_tok_struct _tmp840;_tmp840.tag=Cyc_Stmt_tok;_tmp840.f1=Cyc_Parse_flatten_declarations(
_tmp83D,_tmp83E);_tmp840;});_tmp83F;});break;}case 300: _LL31B: {struct Cyc_List_List*
_tmp841=Cyc_yyget_DeclList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 5)]);struct Cyc_Absyn_Stmt*_tmp842=Cyc_Absyn_for_stmt(Cyc_Absyn_false_exp(
0),Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 4)]),
Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),
Cyc_yyget_Stmt_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 7)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));yyval=(void*)({
struct Cyc_Stmt_tok_struct*_tmp843=_cycalloc(sizeof(*_tmp843));_tmp843[0]=({
struct Cyc_Stmt_tok_struct _tmp844;_tmp844.tag=Cyc_Stmt_tok;_tmp844.f1=Cyc_Parse_flatten_declarations(
_tmp841,_tmp842);_tmp844;});_tmp843;});break;}case 301: _LL31C: yyval=(void*)({
struct Cyc_Stmt_tok_struct*_tmp845=_cycalloc(sizeof(*_tmp845));_tmp845[0]=({
struct Cyc_Stmt_tok_struct _tmp846;_tmp846.tag=Cyc_Stmt_tok;_tmp846.f1=Cyc_Absyn_goto_stmt(({
struct _tagged_arr*_tmp847=_cycalloc(sizeof(*_tmp847));_tmp847[0]=Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp847;}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line));_tmp846;});
_tmp845;});break;case 302: _LL31D: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp848=
_cycalloc(sizeof(*_tmp848));_tmp848[0]=({struct Cyc_Stmt_tok_struct _tmp849;
_tmp849.tag=Cyc_Stmt_tok;_tmp849.f1=Cyc_Absyn_continue_stmt(Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line));_tmp849;});
_tmp848;});break;case 303: _LL31E: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp84A=
_cycalloc(sizeof(*_tmp84A));_tmp84A[0]=({struct Cyc_Stmt_tok_struct _tmp84B;
_tmp84B.tag=Cyc_Stmt_tok;_tmp84B.f1=Cyc_Absyn_break_stmt(Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line));_tmp84B;});
_tmp84A;});break;case 304: _LL31F: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp84C=
_cycalloc(sizeof(*_tmp84C));_tmp84C[0]=({struct Cyc_Stmt_tok_struct _tmp84D;
_tmp84D.tag=Cyc_Stmt_tok;_tmp84D.f1=Cyc_Absyn_return_stmt(0,Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line));_tmp84D;});
_tmp84C;});break;case 305: _LL320: yyval=(void*)({struct Cyc_Stmt_tok_struct*_tmp84E=
_cycalloc(sizeof(*_tmp84E));_tmp84E[0]=({struct Cyc_Stmt_tok_struct _tmp84F;
_tmp84F.tag=Cyc_Stmt_tok;_tmp84F.f1=Cyc_Absyn_return_stmt((struct Cyc_Absyn_Exp*)
Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 2)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).last_line));_tmp84F;});_tmp84E;});break;case 306: _LL321: yyval=(
void*)({struct Cyc_Stmt_tok_struct*_tmp850=_cycalloc(sizeof(*_tmp850));_tmp850[0]=({
struct Cyc_Stmt_tok_struct _tmp851;_tmp851.tag=Cyc_Stmt_tok;_tmp851.f1=Cyc_Absyn_fallthru_stmt(
0,Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 1)]).last_line));_tmp851;});_tmp850;});break;case 307: _LL322: yyval=(
void*)({struct Cyc_Stmt_tok_struct*_tmp852=_cycalloc(sizeof(*_tmp852));_tmp852[0]=({
struct Cyc_Stmt_tok_struct _tmp853;_tmp853.tag=Cyc_Stmt_tok;_tmp853.f1=Cyc_Absyn_fallthru_stmt(
0,Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).last_line));_tmp853;});_tmp852;});break;case 308: _LL323: yyval=(
void*)({struct Cyc_Stmt_tok_struct*_tmp854=_cycalloc(sizeof(*_tmp854));_tmp854[0]=({
struct Cyc_Stmt_tok_struct _tmp855;_tmp855.tag=Cyc_Stmt_tok;_tmp855.f1=Cyc_Absyn_fallthru_stmt(
Cyc_yyget_ExpList_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 4)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 4)]).last_line));_tmp855;});_tmp854;});break;case 309: _LL324: yyval=
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 310: _LL325:
yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 311:
_LL326: yyval=(void*)({struct Cyc_Pattern_tok_struct*_tmp856=_cycalloc(sizeof(*
_tmp856));_tmp856[0]=({struct Cyc_Pattern_tok_struct _tmp857;_tmp857.tag=Cyc_Pattern_tok;
_tmp857.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_conditional_exp(Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 4)])),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp857;});
_tmp856;});break;case 312: _LL327: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 313: _LL328: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp858=_cycalloc(sizeof(*_tmp858));_tmp858[0]=({struct Cyc_Pattern_tok_struct
_tmp859;_tmp859.tag=Cyc_Pattern_tok;_tmp859.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_or_exp(
Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp859;});
_tmp858;});break;case 314: _LL329: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 315: _LL32A: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp85A=_cycalloc(sizeof(*_tmp85A));_tmp85A[0]=({struct Cyc_Pattern_tok_struct
_tmp85B;_tmp85B.tag=Cyc_Pattern_tok;_tmp85B.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_and_exp(
Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp85B;});
_tmp85A;});break;case 316: _LL32B: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 317: _LL32C: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp85C=_cycalloc(sizeof(*_tmp85C));_tmp85C[0]=({struct Cyc_Pattern_tok_struct
_tmp85D;_tmp85D.tag=Cyc_Pattern_tok;_tmp85D.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_prim2_exp((
void*)14,Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp85D;});
_tmp85C;});break;case 318: _LL32D: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 319: _LL32E: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp85E=_cycalloc(sizeof(*_tmp85E));_tmp85E[0]=({struct Cyc_Pattern_tok_struct
_tmp85F;_tmp85F.tag=Cyc_Pattern_tok;_tmp85F.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_prim2_exp((
void*)15,Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp85F;});
_tmp85E;});break;case 320: _LL32F: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 321: _LL330: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp860=_cycalloc(sizeof(*_tmp860));_tmp860[0]=({struct Cyc_Pattern_tok_struct
_tmp861;_tmp861.tag=Cyc_Pattern_tok;_tmp861.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_prim2_exp((
void*)13,Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp861;});
_tmp860;});break;case 322: _LL331: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 323: _LL332: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp862=_cycalloc(sizeof(*_tmp862));_tmp862[0]=({struct Cyc_Pattern_tok_struct
_tmp863;_tmp863.tag=Cyc_Pattern_tok;_tmp863.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_eq_exp(
Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp863;});
_tmp862;});break;case 324: _LL333: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp864=_cycalloc(sizeof(*_tmp864));_tmp864[0]=({struct Cyc_Pattern_tok_struct
_tmp865;_tmp865.tag=Cyc_Pattern_tok;_tmp865.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_neq_exp(
Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp865;});
_tmp864;});break;case 325: _LL334: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 326: _LL335: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp866=_cycalloc(sizeof(*_tmp866));_tmp866[0]=({struct Cyc_Pattern_tok_struct
_tmp867;_tmp867.tag=Cyc_Pattern_tok;_tmp867.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_lt_exp(
Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp867;});
_tmp866;});break;case 327: _LL336: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp868=_cycalloc(sizeof(*_tmp868));_tmp868[0]=({struct Cyc_Pattern_tok_struct
_tmp869;_tmp869.tag=Cyc_Pattern_tok;_tmp869.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_gt_exp(
Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp869;});
_tmp868;});break;case 328: _LL337: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp86A=_cycalloc(sizeof(*_tmp86A));_tmp86A[0]=({struct Cyc_Pattern_tok_struct
_tmp86B;_tmp86B.tag=Cyc_Pattern_tok;_tmp86B.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_lte_exp(
Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp86B;});
_tmp86A;});break;case 329: _LL338: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp86C=_cycalloc(sizeof(*_tmp86C));_tmp86C[0]=({struct Cyc_Pattern_tok_struct
_tmp86D;_tmp86D.tag=Cyc_Pattern_tok;_tmp86D.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_gte_exp(
Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp86D;});
_tmp86C;});break;case 330: _LL339: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 331: _LL33A: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp86E=_cycalloc(sizeof(*_tmp86E));_tmp86E[0]=({struct Cyc_Pattern_tok_struct
_tmp86F;_tmp86F.tag=Cyc_Pattern_tok;_tmp86F.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_prim2_exp((
void*)16,Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp86F;});
_tmp86E;});break;case 332: _LL33B: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp870=_cycalloc(sizeof(*_tmp870));_tmp870[0]=({struct Cyc_Pattern_tok_struct
_tmp871;_tmp871.tag=Cyc_Pattern_tok;_tmp871.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_prim2_exp((
void*)17,Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp871;});
_tmp870;});break;case 333: _LL33C: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 334: _LL33D: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp872=_cycalloc(sizeof(*_tmp872));_tmp872[0]=({struct Cyc_Pattern_tok_struct
_tmp873;_tmp873.tag=Cyc_Pattern_tok;_tmp873.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_prim2_exp((
void*)0,Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp873;});
_tmp872;});break;case 335: _LL33E: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp874=_cycalloc(sizeof(*_tmp874));_tmp874[0]=({struct Cyc_Pattern_tok_struct
_tmp875;_tmp875.tag=Cyc_Pattern_tok;_tmp875.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_prim2_exp((
void*)2,Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp875;});
_tmp874;});break;case 336: _LL33F: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 337: _LL340: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp876=_cycalloc(sizeof(*_tmp876));_tmp876[0]=({struct Cyc_Pattern_tok_struct
_tmp877;_tmp877.tag=Cyc_Pattern_tok;_tmp877.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_prim2_exp((
void*)1,Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp877;});
_tmp876;});break;case 338: _LL341: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp878=_cycalloc(sizeof(*_tmp878));_tmp878[0]=({struct Cyc_Pattern_tok_struct
_tmp879;_tmp879.tag=Cyc_Pattern_tok;_tmp879.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_prim2_exp((
void*)3,Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp879;});
_tmp878;});break;case 339: _LL342: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp87A=_cycalloc(sizeof(*_tmp87A));_tmp87A[0]=({struct Cyc_Pattern_tok_struct
_tmp87B;_tmp87B.tag=Cyc_Pattern_tok;_tmp87B.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_prim2_exp((
void*)4,Cyc_Parse_pat2exp(Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)])),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp87B;});
_tmp87A;});break;case 340: _LL343: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 341: _LL344: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp87C=_cycalloc(sizeof(*_tmp87C));_tmp87C[0]=({struct Cyc_Pattern_tok_struct
_tmp87D;_tmp87D.tag=Cyc_Pattern_tok;_tmp87D.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_cast_exp((*
Cyc_yyget_ParamDecl_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 
2)])).f3,Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]),1,(void*)0,Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp87D;});
_tmp87C;});break;case 342: _LL345: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 343: _LL346: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp87E=_cycalloc(sizeof(*_tmp87E));_tmp87E[0]=({struct Cyc_Pattern_tok_struct
_tmp87F;_tmp87F.tag=Cyc_Pattern_tok;_tmp87F.f1=Cyc_Absyn_exp_pat(Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]));_tmp87F;});_tmp87E;});
break;case 344: _LL347: yyval=(void*)({struct Cyc_Pattern_tok_struct*_tmp880=
_cycalloc(sizeof(*_tmp880));_tmp880[0]=({struct Cyc_Pattern_tok_struct _tmp881;
_tmp881.tag=Cyc_Pattern_tok;_tmp881.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_prim1_exp(Cyc_yyget_Primop_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp881;});
_tmp880;});break;case 345: _LL348: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp882=_cycalloc(sizeof(*_tmp882));_tmp882[0]=({struct Cyc_Pattern_tok_struct
_tmp883;_tmp883.tag=Cyc_Pattern_tok;_tmp883.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_sizeoftyp_exp((*
Cyc_yyget_ParamDecl_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 
1)])).f3,Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line)));_tmp883;});_tmp882;});break;case 346: _LL349: yyval=(
void*)({struct Cyc_Pattern_tok_struct*_tmp884=_cycalloc(sizeof(*_tmp884));_tmp884[
0]=({struct Cyc_Pattern_tok_struct _tmp885;_tmp885.tag=Cyc_Pattern_tok;_tmp885.f1=
Cyc_Absyn_exp_pat(Cyc_Absyn_sizeofexp_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp885;});
_tmp884;});break;case 347: _LL34A: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp886=_cycalloc(sizeof(*_tmp886));_tmp886[0]=({struct Cyc_Pattern_tok_struct
_tmp887;_tmp887.tag=Cyc_Pattern_tok;_tmp887.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_offsetof_exp((*
Cyc_yyget_ParamDecl_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 
3)])).f3,(void*)({struct Cyc_Absyn_StructField_struct*_tmp888=_cycalloc(sizeof(*
_tmp888));_tmp888[0]=({struct Cyc_Absyn_StructField_struct _tmp889;_tmp889.tag=0;
_tmp889.f1=({struct _tagged_arr*_tmp88A=_cycalloc(sizeof(*_tmp88A));_tmp88A[0]=
Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);
_tmp88A;});_tmp889;});_tmp888;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 5)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line)));_tmp887;});
_tmp886;});break;case 348: _LL34B: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp88B=_cycalloc(sizeof(*_tmp88B));_tmp88B[0]=({struct Cyc_Pattern_tok_struct
_tmp88C;_tmp88C.tag=Cyc_Pattern_tok;_tmp88C.f1=Cyc_Absyn_exp_pat(Cyc_Absyn_offsetof_exp((*
Cyc_yyget_ParamDecl_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 
3)])).f3,(void*)({struct Cyc_Absyn_TupleIndex_struct*_tmp88D=_cycalloc_atomic(
sizeof(*_tmp88D));_tmp88D[0]=({struct Cyc_Absyn_TupleIndex_struct _tmp88E;_tmp88E.tag=
1;_tmp88E.f1=(unsigned int)(*Cyc_yyget_Int_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)])).f2;_tmp88E;});_tmp88D;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 5)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line)));_tmp88C;});_tmp88B;});break;case 349: _LL34C: yyval=
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 350: _LL34D:
yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 351:
_LL34E: yyval=(void*)({struct Cyc_Pattern_tok_struct*_tmp88F=_cycalloc(sizeof(*
_tmp88F));_tmp88F[0]=({struct Cyc_Pattern_tok_struct _tmp890;_tmp890.tag=Cyc_Pattern_tok;
_tmp890.f1=Cyc_Absyn_new_pat((void*)0,Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp890;});
_tmp88F;});break;case 352: _LL34F: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp891=_cycalloc(sizeof(*_tmp891));_tmp891[0]=({struct Cyc_Pattern_tok_struct
_tmp892;_tmp892.tag=Cyc_Pattern_tok;_tmp892.f1=Cyc_Absyn_exp_pat(Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]));_tmp892;});_tmp891;});
break;case 353: _LL350: {struct Cyc_Absyn_Exp*e=Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);{void*_tmp893=(void*)e->r;
void*_tmp894;void*_tmp895;char _tmp896;void*_tmp897;void*_tmp898;short _tmp899;
void*_tmp89A;void*_tmp89B;int _tmp89C;void*_tmp89D;struct _tagged_arr _tmp89E;void*
_tmp89F;void*_tmp8A0;void*_tmp8A1;_LL353: if(*((int*)_tmp893)!= 0)goto _LL355;
_tmp894=(void*)((struct Cyc_Absyn_Const_e_struct*)_tmp893)->f1;if(_tmp894 <= (void*)
1  || *((int*)_tmp894)!= 0)goto _LL355;_tmp895=(void*)((struct Cyc_Absyn_Char_c_struct*)
_tmp894)->f1;_tmp896=((struct Cyc_Absyn_Char_c_struct*)_tmp894)->f2;_LL354: yyval=(
void*)({struct Cyc_Pattern_tok_struct*_tmp8A2=_cycalloc(sizeof(*_tmp8A2));_tmp8A2[
0]=({struct Cyc_Pattern_tok_struct _tmp8A3;_tmp8A3.tag=Cyc_Pattern_tok;_tmp8A3.f1=
Cyc_Absyn_new_pat((void*)({struct Cyc_Absyn_Char_p_struct*_tmp8A4=
_cycalloc_atomic(sizeof(*_tmp8A4));_tmp8A4[0]=({struct Cyc_Absyn_Char_p_struct
_tmp8A5;_tmp8A5.tag=8;_tmp8A5.f1=_tmp896;_tmp8A5;});_tmp8A4;}),e->loc);_tmp8A3;});
_tmp8A2;});goto _LL352;_LL355: if(*((int*)_tmp893)!= 0)goto _LL357;_tmp897=(void*)((
struct Cyc_Absyn_Const_e_struct*)_tmp893)->f1;if(_tmp897 <= (void*)1  || *((int*)
_tmp897)!= 1)goto _LL357;_tmp898=(void*)((struct Cyc_Absyn_Short_c_struct*)_tmp897)->f1;
_tmp899=((struct Cyc_Absyn_Short_c_struct*)_tmp897)->f2;_LL356: yyval=(void*)({
struct Cyc_Pattern_tok_struct*_tmp8A6=_cycalloc(sizeof(*_tmp8A6));_tmp8A6[0]=({
struct Cyc_Pattern_tok_struct _tmp8A7;_tmp8A7.tag=Cyc_Pattern_tok;_tmp8A7.f1=Cyc_Absyn_new_pat((
void*)({struct Cyc_Absyn_Int_p_struct*_tmp8A8=_cycalloc(sizeof(*_tmp8A8));_tmp8A8[
0]=({struct Cyc_Absyn_Int_p_struct _tmp8A9;_tmp8A9.tag=7;_tmp8A9.f1=(void*)_tmp898;
_tmp8A9.f2=(int)_tmp899;_tmp8A9;});_tmp8A8;}),e->loc);_tmp8A7;});_tmp8A6;});goto
_LL352;_LL357: if(*((int*)_tmp893)!= 0)goto _LL359;_tmp89A=(void*)((struct Cyc_Absyn_Const_e_struct*)
_tmp893)->f1;if(_tmp89A <= (void*)1  || *((int*)_tmp89A)!= 2)goto _LL359;_tmp89B=(
void*)((struct Cyc_Absyn_Int_c_struct*)_tmp89A)->f1;_tmp89C=((struct Cyc_Absyn_Int_c_struct*)
_tmp89A)->f2;_LL358: yyval=(void*)({struct Cyc_Pattern_tok_struct*_tmp8AA=
_cycalloc(sizeof(*_tmp8AA));_tmp8AA[0]=({struct Cyc_Pattern_tok_struct _tmp8AB;
_tmp8AB.tag=Cyc_Pattern_tok;_tmp8AB.f1=Cyc_Absyn_new_pat((void*)({struct Cyc_Absyn_Int_p_struct*
_tmp8AC=_cycalloc(sizeof(*_tmp8AC));_tmp8AC[0]=({struct Cyc_Absyn_Int_p_struct
_tmp8AD;_tmp8AD.tag=7;_tmp8AD.f1=(void*)_tmp89B;_tmp8AD.f2=_tmp89C;_tmp8AD;});
_tmp8AC;}),e->loc);_tmp8AB;});_tmp8AA;});goto _LL352;_LL359: if(*((int*)_tmp893)!= 
0)goto _LL35B;_tmp89D=(void*)((struct Cyc_Absyn_Const_e_struct*)_tmp893)->f1;if(
_tmp89D <= (void*)1  || *((int*)_tmp89D)!= 4)goto _LL35B;_tmp89E=((struct Cyc_Absyn_Float_c_struct*)
_tmp89D)->f1;_LL35A: yyval=(void*)({struct Cyc_Pattern_tok_struct*_tmp8AE=
_cycalloc(sizeof(*_tmp8AE));_tmp8AE[0]=({struct Cyc_Pattern_tok_struct _tmp8AF;
_tmp8AF.tag=Cyc_Pattern_tok;_tmp8AF.f1=Cyc_Absyn_new_pat((void*)({struct Cyc_Absyn_Float_p_struct*
_tmp8B0=_cycalloc(sizeof(*_tmp8B0));_tmp8B0[0]=({struct Cyc_Absyn_Float_p_struct
_tmp8B1;_tmp8B1.tag=9;_tmp8B1.f1=_tmp89E;_tmp8B1;});_tmp8B0;}),e->loc);_tmp8AF;});
_tmp8AE;});goto _LL352;_LL35B: if(*((int*)_tmp893)!= 0)goto _LL35D;_tmp89F=(void*)((
struct Cyc_Absyn_Const_e_struct*)_tmp893)->f1;if((int)_tmp89F != 0)goto _LL35D;
_LL35C: yyval=(void*)({struct Cyc_Pattern_tok_struct*_tmp8B2=_cycalloc(sizeof(*
_tmp8B2));_tmp8B2[0]=({struct Cyc_Pattern_tok_struct _tmp8B3;_tmp8B3.tag=Cyc_Pattern_tok;
_tmp8B3.f1=Cyc_Absyn_new_pat((void*)1,e->loc);_tmp8B3;});_tmp8B2;});goto _LL352;
_LL35D: if(*((int*)_tmp893)!= 0)goto _LL35F;_tmp8A0=(void*)((struct Cyc_Absyn_Const_e_struct*)
_tmp893)->f1;if(_tmp8A0 <= (void*)1  || *((int*)_tmp8A0)!= 5)goto _LL35F;_LL35E: Cyc_Parse_err(({
const char*_tmp8B4="strings cannot occur within patterns";_tag_arr(_tmp8B4,
sizeof(char),_get_zero_arr_size(_tmp8B4,37));}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));goto _LL352;
_LL35F: if(*((int*)_tmp893)!= 0)goto _LL361;_tmp8A1=(void*)((struct Cyc_Absyn_Const_e_struct*)
_tmp893)->f1;if(_tmp8A1 <= (void*)1  || *((int*)_tmp8A1)!= 3)goto _LL361;_LL360: Cyc_Parse_unimp(({
const char*_tmp8B5="long long's in patterns";_tag_arr(_tmp8B5,sizeof(char),
_get_zero_arr_size(_tmp8B5,24));}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));goto _LL352;
_LL361:;_LL362: Cyc_Parse_err(({const char*_tmp8B6="bad constant in case";_tag_arr(
_tmp8B6,sizeof(char),_get_zero_arr_size(_tmp8B6,21));}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_LL352:;}break;}
case 354: _LL351: yyval=(void*)({struct Cyc_Pattern_tok_struct*_tmp8B7=_cycalloc(
sizeof(*_tmp8B7));_tmp8B7[0]=({struct Cyc_Pattern_tok_struct _tmp8B8;_tmp8B8.tag=
Cyc_Pattern_tok;_tmp8B8.f1=Cyc_Absyn_new_pat((void*)({struct Cyc_Absyn_UnknownId_p_struct*
_tmp8B9=_cycalloc(sizeof(*_tmp8B9));_tmp8B9[0]=({struct Cyc_Absyn_UnknownId_p_struct
_tmp8BA;_tmp8BA.tag=12;_tmp8BA.f1=Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp8BA;});_tmp8B9;}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp8B8;});
_tmp8B7;});break;case 355: _LL363: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp8BB=_cycalloc(sizeof(*_tmp8BB));_tmp8BB[0]=({struct Cyc_Pattern_tok_struct
_tmp8BC;_tmp8BC.tag=Cyc_Pattern_tok;_tmp8BC.f1=Cyc_Absyn_new_pat((void*)({struct
Cyc_Absyn_Tuple_p_struct*_tmp8BD=_cycalloc(sizeof(*_tmp8BD));_tmp8BD[0]=({struct
Cyc_Absyn_Tuple_p_struct _tmp8BE;_tmp8BE.tag=3;_tmp8BE.f1=((struct Cyc_List_List*(*)(
struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_PatternList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]));_tmp8BE;});_tmp8BD;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp8BC;});_tmp8BB;});break;case 356: _LL364: yyval=(
void*)({struct Cyc_Pattern_tok_struct*_tmp8BF=_cycalloc(sizeof(*_tmp8BF));_tmp8BF[
0]=({struct Cyc_Pattern_tok_struct _tmp8C0;_tmp8C0.tag=Cyc_Pattern_tok;_tmp8C0.f1=
Cyc_Absyn_new_pat((void*)({struct Cyc_Absyn_UnknownCall_p_struct*_tmp8C1=
_cycalloc(sizeof(*_tmp8C1));_tmp8C1[0]=({struct Cyc_Absyn_UnknownCall_p_struct
_tmp8C2;_tmp8C2.tag=13;_tmp8C2.f1=Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp8C2.f2=((struct Cyc_List_List*(*)(
struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_PatternList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]));_tmp8C2;});_tmp8C1;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp8C0;});_tmp8BF;});break;case 357: _LL365: {struct
Cyc_List_List*_tmp8C3=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct
Cyc_Position_Segment*,void*),struct Cyc_Position_Segment*env,struct Cyc_List_List*
x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 2)]).last_line),Cyc_yyget_TypeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]));yyval=(void*)({
struct Cyc_Pattern_tok_struct*_tmp8C4=_cycalloc(sizeof(*_tmp8C4));_tmp8C4[0]=({
struct Cyc_Pattern_tok_struct _tmp8C5;_tmp8C5.tag=Cyc_Pattern_tok;_tmp8C5.f1=Cyc_Absyn_new_pat((
void*)({struct Cyc_Absyn_Aggr_p_struct*_tmp8C6=_cycalloc(sizeof(*_tmp8C6));
_tmp8C6[0]=({struct Cyc_Absyn_Aggr_p_struct _tmp8C7;_tmp8C7.tag=5;_tmp8C7.f1=({
struct Cyc_Absyn_AggrInfo _tmp8C8;_tmp8C8.aggr_info=(void*)((void*)({struct Cyc_Absyn_UnknownAggr_struct*
_tmp8C9=_cycalloc(sizeof(*_tmp8C9));_tmp8C9[0]=({struct Cyc_Absyn_UnknownAggr_struct
_tmp8CA;_tmp8CA.tag=0;_tmp8CA.f1=(void*)((void*)0);_tmp8CA.f2=Cyc_yyget_QualId_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 4)]);_tmp8CA;});_tmp8C9;}));
_tmp8C8.targs=0;_tmp8C8;});_tmp8C7.f2=_tmp8C3;_tmp8C7.f3=((struct Cyc_List_List*(*)(
struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_FieldPatternList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]));_tmp8C7;});_tmp8C6;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 4)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp8C5;});_tmp8C4;});break;}case 358: _LL366: yyval=(
void*)({struct Cyc_Pattern_tok_struct*_tmp8CB=_cycalloc(sizeof(*_tmp8CB));_tmp8CB[
0]=({struct Cyc_Pattern_tok_struct _tmp8CC;_tmp8CC.tag=Cyc_Pattern_tok;_tmp8CC.f1=
Cyc_Absyn_new_pat((void*)({struct Cyc_Absyn_Pointer_p_struct*_tmp8CD=_cycalloc(
sizeof(*_tmp8CD));_tmp8CD[0]=({struct Cyc_Absyn_Pointer_p_struct _tmp8CE;_tmp8CE.tag=
4;_tmp8CE.f1=Cyc_yyget_Pattern_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)]);_tmp8CE;});_tmp8CD;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp8CC;});
_tmp8CB;});break;case 359: _LL367: yyval=(void*)({struct Cyc_Pattern_tok_struct*
_tmp8CF=_cycalloc(sizeof(*_tmp8CF));_tmp8CF[0]=({struct Cyc_Pattern_tok_struct
_tmp8D0;_tmp8D0.tag=Cyc_Pattern_tok;_tmp8D0.f1=Cyc_Absyn_new_pat((void*)({struct
Cyc_Absyn_Pointer_p_struct*_tmp8D1=_cycalloc(sizeof(*_tmp8D1));_tmp8D1[0]=({
struct Cyc_Absyn_Pointer_p_struct _tmp8D2;_tmp8D2.tag=4;_tmp8D2.f1=Cyc_Absyn_new_pat((
void*)({struct Cyc_Absyn_Pointer_p_struct*_tmp8D3=_cycalloc(sizeof(*_tmp8D3));
_tmp8D3[0]=({struct Cyc_Absyn_Pointer_p_struct _tmp8D4;_tmp8D4.tag=4;_tmp8D4.f1=
Cyc_yyget_Pattern_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);
_tmp8D4;});_tmp8D3;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp8D2;});
_tmp8D1;}),Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(
10000,yylsp_offset - 1)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp8D0;});_tmp8CF;});break;case 360: _LL368: yyval=(
void*)({struct Cyc_Pattern_tok_struct*_tmp8D5=_cycalloc(sizeof(*_tmp8D5));_tmp8D5[
0]=({struct Cyc_Pattern_tok_struct _tmp8D6;_tmp8D6.tag=Cyc_Pattern_tok;_tmp8D6.f1=
Cyc_Absyn_new_pat((void*)({struct Cyc_Absyn_Reference_p_struct*_tmp8D7=_cycalloc(
sizeof(*_tmp8D7));_tmp8D7[0]=({struct Cyc_Absyn_Reference_p_struct _tmp8D8;_tmp8D8.tag=
1;_tmp8D8.f1=Cyc_Absyn_new_vardecl(({struct _tuple1*_tmp8D9=_cycalloc(sizeof(*
_tmp8D9));_tmp8D9->f1=(void*)0;_tmp8D9->f2=({struct _tagged_arr*_tmp8DA=_cycalloc(
sizeof(*_tmp8DA));_tmp8DA[0]=Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp8DA;});_tmp8D9;}),(void*)
0,0);_tmp8D8;});_tmp8D7;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp8D6;});
_tmp8D5;});break;case 361: _LL369: {void*_tmp8DB=Cyc_Parse_id2type(Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),(void*)({struct Cyc_Absyn_Eq_kb_struct*
_tmp8E4=_cycalloc(sizeof(*_tmp8E4));_tmp8E4[0]=({struct Cyc_Absyn_Eq_kb_struct
_tmp8E5;_tmp8E5.tag=0;_tmp8E5.f1=(void*)((void*)5);_tmp8E5;});_tmp8E4;}));yyval=(
void*)({struct Cyc_Pattern_tok_struct*_tmp8DC=_cycalloc(sizeof(*_tmp8DC));_tmp8DC[
0]=({struct Cyc_Pattern_tok_struct _tmp8DD;_tmp8DD.tag=Cyc_Pattern_tok;_tmp8DD.f1=
Cyc_Absyn_new_pat((void*)({struct Cyc_Absyn_TagInt_p_struct*_tmp8DE=_cycalloc(
sizeof(*_tmp8DE));_tmp8DE[0]=({struct Cyc_Absyn_TagInt_p_struct _tmp8DF;_tmp8DF.tag=
2;_tmp8DF.f1=Cyc_Parse_typ2tvar(Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line),_tmp8DB);
_tmp8DF.f2=Cyc_Absyn_new_vardecl(({struct _tuple1*_tmp8E0=_cycalloc(sizeof(*
_tmp8E0));_tmp8E0->f1=(void*)0;_tmp8E0->f2=({struct _tagged_arr*_tmp8E1=_cycalloc(
sizeof(*_tmp8E1));_tmp8E1[0]=Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp8E1;});_tmp8E0;}),(
void*)({struct Cyc_Absyn_TagType_struct*_tmp8E2=_cycalloc(sizeof(*_tmp8E2));
_tmp8E2[0]=({struct Cyc_Absyn_TagType_struct _tmp8E3;_tmp8E3.tag=17;_tmp8E3.f1=(
void*)_tmp8DB;_tmp8E3;});_tmp8E2;}),0);_tmp8DF;});_tmp8DE;}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp8DD;});
_tmp8DC;});break;}case 362: _LL36A: {struct Cyc_Absyn_Tvar*_tmp8E6=Cyc_Tcutil_new_tvar((
void*)({struct Cyc_Absyn_Eq_kb_struct*_tmp8F1=_cycalloc(sizeof(*_tmp8F1));_tmp8F1[
0]=({struct Cyc_Absyn_Eq_kb_struct _tmp8F2;_tmp8F2.tag=0;_tmp8F2.f1=(void*)((void*)
5);_tmp8F2;});_tmp8F1;}));yyval=(void*)({struct Cyc_Pattern_tok_struct*_tmp8E7=
_cycalloc(sizeof(*_tmp8E7));_tmp8E7[0]=({struct Cyc_Pattern_tok_struct _tmp8E8;
_tmp8E8.tag=Cyc_Pattern_tok;_tmp8E8.f1=Cyc_Absyn_new_pat((void*)({struct Cyc_Absyn_TagInt_p_struct*
_tmp8E9=_cycalloc(sizeof(*_tmp8E9));_tmp8E9[0]=({struct Cyc_Absyn_TagInt_p_struct
_tmp8EA;_tmp8EA.tag=2;_tmp8EA.f1=_tmp8E6;_tmp8EA.f2=Cyc_Absyn_new_vardecl(({
struct _tuple1*_tmp8EB=_cycalloc(sizeof(*_tmp8EB));_tmp8EB->f1=(void*)0;_tmp8EB->f2=({
struct _tagged_arr*_tmp8EC=_cycalloc(sizeof(*_tmp8EC));_tmp8EC[0]=Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp8EC;});_tmp8EB;}),(
void*)({struct Cyc_Absyn_TagType_struct*_tmp8ED=_cycalloc(sizeof(*_tmp8ED));
_tmp8ED[0]=({struct Cyc_Absyn_TagType_struct _tmp8EE;_tmp8EE.tag=17;_tmp8EE.f1=(
void*)((void*)({struct Cyc_Absyn_VarType_struct*_tmp8EF=_cycalloc(sizeof(*_tmp8EF));
_tmp8EF[0]=({struct Cyc_Absyn_VarType_struct _tmp8F0;_tmp8F0.tag=1;_tmp8F0.f1=
_tmp8E6;_tmp8F0;});_tmp8EF;}));_tmp8EE;});_tmp8ED;}),0);_tmp8EA;});_tmp8E9;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 3)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp8E8;});_tmp8E7;});break;}case 363: _LL36B: yyval=(
void*)({struct Cyc_PatternList_tok_struct*_tmp8F3=_cycalloc(sizeof(*_tmp8F3));
_tmp8F3[0]=({struct Cyc_PatternList_tok_struct _tmp8F4;_tmp8F4.tag=Cyc_PatternList_tok;
_tmp8F4.f1=({struct Cyc_List_List*_tmp8F5=_cycalloc(sizeof(*_tmp8F5));_tmp8F5->hd=
Cyc_yyget_Pattern_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);
_tmp8F5->tl=0;_tmp8F5;});_tmp8F4;});_tmp8F3;});break;case 364: _LL36C: yyval=(void*)({
struct Cyc_PatternList_tok_struct*_tmp8F6=_cycalloc(sizeof(*_tmp8F6));_tmp8F6[0]=({
struct Cyc_PatternList_tok_struct _tmp8F7;_tmp8F7.tag=Cyc_PatternList_tok;_tmp8F7.f1=({
struct Cyc_List_List*_tmp8F8=_cycalloc(sizeof(*_tmp8F8));_tmp8F8->hd=Cyc_yyget_Pattern_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp8F8->tl=Cyc_yyget_PatternList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp8F8;});_tmp8F7;});
_tmp8F6;});break;case 365: _LL36D: yyval=(void*)({struct Cyc_FieldPattern_tok_struct*
_tmp8F9=_cycalloc(sizeof(*_tmp8F9));_tmp8F9[0]=({struct Cyc_FieldPattern_tok_struct
_tmp8FA;_tmp8FA.tag=Cyc_FieldPattern_tok;_tmp8FA.f1=({struct _tuple11*_tmp8FB=
_cycalloc(sizeof(*_tmp8FB));_tmp8FB->f1=0;_tmp8FB->f2=Cyc_yyget_Pattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp8FB;});_tmp8FA;});
_tmp8F9;});break;case 366: _LL36E: yyval=(void*)({struct Cyc_FieldPattern_tok_struct*
_tmp8FC=_cycalloc(sizeof(*_tmp8FC));_tmp8FC[0]=({struct Cyc_FieldPattern_tok_struct
_tmp8FD;_tmp8FD.tag=Cyc_FieldPattern_tok;_tmp8FD.f1=({struct _tuple11*_tmp8FE=
_cycalloc(sizeof(*_tmp8FE));_tmp8FE->f1=Cyc_yyget_DesignatorList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp8FE->f2=Cyc_yyget_Pattern_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp8FE;});_tmp8FD;});
_tmp8FC;});break;case 367: _LL36F: yyval=(void*)({struct Cyc_FieldPatternList_tok_struct*
_tmp8FF=_cycalloc(sizeof(*_tmp8FF));_tmp8FF[0]=({struct Cyc_FieldPatternList_tok_struct
_tmp900;_tmp900.tag=Cyc_FieldPatternList_tok;_tmp900.f1=({struct Cyc_List_List*
_tmp901=_cycalloc(sizeof(*_tmp901));_tmp901->hd=Cyc_yyget_FieldPattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp901->tl=0;_tmp901;});
_tmp900;});_tmp8FF;});break;case 368: _LL370: yyval=(void*)({struct Cyc_FieldPatternList_tok_struct*
_tmp902=_cycalloc(sizeof(*_tmp902));_tmp902[0]=({struct Cyc_FieldPatternList_tok_struct
_tmp903;_tmp903.tag=Cyc_FieldPatternList_tok;_tmp903.f1=({struct Cyc_List_List*
_tmp904=_cycalloc(sizeof(*_tmp904));_tmp904->hd=Cyc_yyget_FieldPattern_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp904->tl=Cyc_yyget_FieldPatternList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp904;});_tmp903;});
_tmp902;});break;case 369: _LL371: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 370: _LL372: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp905=_cycalloc(sizeof(*_tmp905));_tmp905[0]=({struct Cyc_Exp_tok_struct _tmp906;
_tmp906.tag=Cyc_Exp_tok;_tmp906.f1=Cyc_Absyn_seq_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp906;});
_tmp905;});break;case 371: _LL373: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 372: _LL374: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp907=_cycalloc(sizeof(*_tmp907));_tmp907[0]=({struct Cyc_Exp_tok_struct _tmp908;
_tmp908.tag=Cyc_Exp_tok;_tmp908.f1=Cyc_Absyn_assignop_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Primopopt_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp908;});
_tmp907;});break;case 373: _LL375: yyval=(void*)({struct Cyc_Primopopt_tok_struct*
_tmp909=_cycalloc(sizeof(*_tmp909));_tmp909[0]=({struct Cyc_Primopopt_tok_struct
_tmp90A;_tmp90A.tag=Cyc_Primopopt_tok;_tmp90A.f1=0;_tmp90A;});_tmp909;});break;
case 374: _LL376: yyval=(void*)({struct Cyc_Primopopt_tok_struct*_tmp90B=_cycalloc(
sizeof(*_tmp90B));_tmp90B[0]=({struct Cyc_Primopopt_tok_struct _tmp90C;_tmp90C.tag=
Cyc_Primopopt_tok;_tmp90C.f1=({struct Cyc_Core_Opt*_tmp90D=_cycalloc(sizeof(*
_tmp90D));_tmp90D->v=(void*)((void*)1);_tmp90D;});_tmp90C;});_tmp90B;});break;
case 375: _LL377: yyval=(void*)({struct Cyc_Primopopt_tok_struct*_tmp90E=_cycalloc(
sizeof(*_tmp90E));_tmp90E[0]=({struct Cyc_Primopopt_tok_struct _tmp90F;_tmp90F.tag=
Cyc_Primopopt_tok;_tmp90F.f1=({struct Cyc_Core_Opt*_tmp910=_cycalloc(sizeof(*
_tmp910));_tmp910->v=(void*)((void*)3);_tmp910;});_tmp90F;});_tmp90E;});break;
case 376: _LL378: yyval=(void*)({struct Cyc_Primopopt_tok_struct*_tmp911=_cycalloc(
sizeof(*_tmp911));_tmp911[0]=({struct Cyc_Primopopt_tok_struct _tmp912;_tmp912.tag=
Cyc_Primopopt_tok;_tmp912.f1=({struct Cyc_Core_Opt*_tmp913=_cycalloc(sizeof(*
_tmp913));_tmp913->v=(void*)((void*)4);_tmp913;});_tmp912;});_tmp911;});break;
case 377: _LL379: yyval=(void*)({struct Cyc_Primopopt_tok_struct*_tmp914=_cycalloc(
sizeof(*_tmp914));_tmp914[0]=({struct Cyc_Primopopt_tok_struct _tmp915;_tmp915.tag=
Cyc_Primopopt_tok;_tmp915.f1=({struct Cyc_Core_Opt*_tmp916=_cycalloc(sizeof(*
_tmp916));_tmp916->v=(void*)((void*)0);_tmp916;});_tmp915;});_tmp914;});break;
case 378: _LL37A: yyval=(void*)({struct Cyc_Primopopt_tok_struct*_tmp917=_cycalloc(
sizeof(*_tmp917));_tmp917[0]=({struct Cyc_Primopopt_tok_struct _tmp918;_tmp918.tag=
Cyc_Primopopt_tok;_tmp918.f1=({struct Cyc_Core_Opt*_tmp919=_cycalloc(sizeof(*
_tmp919));_tmp919->v=(void*)((void*)2);_tmp919;});_tmp918;});_tmp917;});break;
case 379: _LL37B: yyval=(void*)({struct Cyc_Primopopt_tok_struct*_tmp91A=_cycalloc(
sizeof(*_tmp91A));_tmp91A[0]=({struct Cyc_Primopopt_tok_struct _tmp91B;_tmp91B.tag=
Cyc_Primopopt_tok;_tmp91B.f1=({struct Cyc_Core_Opt*_tmp91C=_cycalloc(sizeof(*
_tmp91C));_tmp91C->v=(void*)((void*)16);_tmp91C;});_tmp91B;});_tmp91A;});break;
case 380: _LL37C: yyval=(void*)({struct Cyc_Primopopt_tok_struct*_tmp91D=_cycalloc(
sizeof(*_tmp91D));_tmp91D[0]=({struct Cyc_Primopopt_tok_struct _tmp91E;_tmp91E.tag=
Cyc_Primopopt_tok;_tmp91E.f1=({struct Cyc_Core_Opt*_tmp91F=_cycalloc(sizeof(*
_tmp91F));_tmp91F->v=(void*)((void*)17);_tmp91F;});_tmp91E;});_tmp91D;});break;
case 381: _LL37D: yyval=(void*)({struct Cyc_Primopopt_tok_struct*_tmp920=_cycalloc(
sizeof(*_tmp920));_tmp920[0]=({struct Cyc_Primopopt_tok_struct _tmp921;_tmp921.tag=
Cyc_Primopopt_tok;_tmp921.f1=({struct Cyc_Core_Opt*_tmp922=_cycalloc(sizeof(*
_tmp922));_tmp922->v=(void*)((void*)13);_tmp922;});_tmp921;});_tmp920;});break;
case 382: _LL37E: yyval=(void*)({struct Cyc_Primopopt_tok_struct*_tmp923=_cycalloc(
sizeof(*_tmp923));_tmp923[0]=({struct Cyc_Primopopt_tok_struct _tmp924;_tmp924.tag=
Cyc_Primopopt_tok;_tmp924.f1=({struct Cyc_Core_Opt*_tmp925=_cycalloc(sizeof(*
_tmp925));_tmp925->v=(void*)((void*)15);_tmp925;});_tmp924;});_tmp923;});break;
case 383: _LL37F: yyval=(void*)({struct Cyc_Primopopt_tok_struct*_tmp926=_cycalloc(
sizeof(*_tmp926));_tmp926[0]=({struct Cyc_Primopopt_tok_struct _tmp927;_tmp927.tag=
Cyc_Primopopt_tok;_tmp927.f1=({struct Cyc_Core_Opt*_tmp928=_cycalloc(sizeof(*
_tmp928));_tmp928->v=(void*)((void*)14);_tmp928;});_tmp927;});_tmp926;});break;
case 384: _LL380: yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];
break;case 385: _LL381: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp929=_cycalloc(
sizeof(*_tmp929));_tmp929[0]=({struct Cyc_Exp_tok_struct _tmp92A;_tmp92A.tag=Cyc_Exp_tok;
_tmp92A.f1=Cyc_Absyn_conditional_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp92A;});
_tmp929;});break;case 386: _LL382: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp92B=
_cycalloc(sizeof(*_tmp92B));_tmp92B[0]=({struct Cyc_Exp_tok_struct _tmp92C;_tmp92C.tag=
Cyc_Exp_tok;_tmp92C.f1=Cyc_Absyn_throw_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp92C;});
_tmp92B;});break;case 387: _LL383: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp92D=
_cycalloc(sizeof(*_tmp92D));_tmp92D[0]=({struct Cyc_Exp_tok_struct _tmp92E;_tmp92E.tag=
Cyc_Exp_tok;_tmp92E.f1=Cyc_Absyn_New_exp(0,Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp92E;});
_tmp92D;});break;case 388: _LL384: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp92F=
_cycalloc(sizeof(*_tmp92F));_tmp92F[0]=({struct Cyc_Exp_tok_struct _tmp930;_tmp930.tag=
Cyc_Exp_tok;_tmp930.f1=Cyc_Absyn_New_exp(0,Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp930;});
_tmp92F;});break;case 389: _LL385: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp931=
_cycalloc(sizeof(*_tmp931));_tmp931[0]=({struct Cyc_Exp_tok_struct _tmp932;_tmp932.tag=
Cyc_Exp_tok;_tmp932.f1=Cyc_Absyn_New_exp((struct Cyc_Absyn_Exp*)Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp932;});
_tmp931;});break;case 390: _LL386: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp933=
_cycalloc(sizeof(*_tmp933));_tmp933[0]=({struct Cyc_Exp_tok_struct _tmp934;_tmp934.tag=
Cyc_Exp_tok;_tmp934.f1=Cyc_Absyn_New_exp((struct Cyc_Absyn_Exp*)Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp934;});
_tmp933;});break;case 391: _LL387: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 392: _LL388: yyval=yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset)];break;case 393: _LL389: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp935=_cycalloc(sizeof(*_tmp935));_tmp935[0]=({struct Cyc_Exp_tok_struct _tmp936;
_tmp936.tag=Cyc_Exp_tok;_tmp936.f1=Cyc_Absyn_or_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp936;});
_tmp935;});break;case 394: _LL38A: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 395: _LL38B: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp937=_cycalloc(sizeof(*_tmp937));_tmp937[0]=({struct Cyc_Exp_tok_struct _tmp938;
_tmp938.tag=Cyc_Exp_tok;_tmp938.f1=Cyc_Absyn_and_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp938;});
_tmp937;});break;case 396: _LL38C: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 397: _LL38D: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp939=_cycalloc(sizeof(*_tmp939));_tmp939[0]=({struct Cyc_Exp_tok_struct _tmp93A;
_tmp93A.tag=Cyc_Exp_tok;_tmp93A.f1=Cyc_Absyn_prim2_exp((void*)14,Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp93A;});
_tmp939;});break;case 398: _LL38E: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 399: _LL38F: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp93B=_cycalloc(sizeof(*_tmp93B));_tmp93B[0]=({struct Cyc_Exp_tok_struct _tmp93C;
_tmp93C.tag=Cyc_Exp_tok;_tmp93C.f1=Cyc_Absyn_prim2_exp((void*)15,Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp93C;});
_tmp93B;});break;case 400: _LL390: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 401: _LL391: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp93D=_cycalloc(sizeof(*_tmp93D));_tmp93D[0]=({struct Cyc_Exp_tok_struct _tmp93E;
_tmp93E.tag=Cyc_Exp_tok;_tmp93E.f1=Cyc_Absyn_prim2_exp((void*)13,Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp93E;});
_tmp93D;});break;case 402: _LL392: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 403: _LL393: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp93F=_cycalloc(sizeof(*_tmp93F));_tmp93F[0]=({struct Cyc_Exp_tok_struct _tmp940;
_tmp940.tag=Cyc_Exp_tok;_tmp940.f1=Cyc_Absyn_eq_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp940;});
_tmp93F;});break;case 404: _LL394: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp941=
_cycalloc(sizeof(*_tmp941));_tmp941[0]=({struct Cyc_Exp_tok_struct _tmp942;_tmp942.tag=
Cyc_Exp_tok;_tmp942.f1=Cyc_Absyn_neq_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp942;});
_tmp941;});break;case 405: _LL395: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 406: _LL396: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp943=_cycalloc(sizeof(*_tmp943));_tmp943[0]=({struct Cyc_Exp_tok_struct _tmp944;
_tmp944.tag=Cyc_Exp_tok;_tmp944.f1=Cyc_Absyn_lt_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp944;});
_tmp943;});break;case 407: _LL397: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp945=
_cycalloc(sizeof(*_tmp945));_tmp945[0]=({struct Cyc_Exp_tok_struct _tmp946;_tmp946.tag=
Cyc_Exp_tok;_tmp946.f1=Cyc_Absyn_gt_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp946;});
_tmp945;});break;case 408: _LL398: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp947=
_cycalloc(sizeof(*_tmp947));_tmp947[0]=({struct Cyc_Exp_tok_struct _tmp948;_tmp948.tag=
Cyc_Exp_tok;_tmp948.f1=Cyc_Absyn_lte_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp948;});
_tmp947;});break;case 409: _LL399: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp949=
_cycalloc(sizeof(*_tmp949));_tmp949[0]=({struct Cyc_Exp_tok_struct _tmp94A;_tmp94A.tag=
Cyc_Exp_tok;_tmp94A.f1=Cyc_Absyn_gte_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp94A;});
_tmp949;});break;case 410: _LL39A: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 411: _LL39B: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp94B=_cycalloc(sizeof(*_tmp94B));_tmp94B[0]=({struct Cyc_Exp_tok_struct _tmp94C;
_tmp94C.tag=Cyc_Exp_tok;_tmp94C.f1=Cyc_Absyn_prim2_exp((void*)16,Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp94C;});
_tmp94B;});break;case 412: _LL39C: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp94D=
_cycalloc(sizeof(*_tmp94D));_tmp94D[0]=({struct Cyc_Exp_tok_struct _tmp94E;_tmp94E.tag=
Cyc_Exp_tok;_tmp94E.f1=Cyc_Absyn_prim2_exp((void*)17,Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp94E;});
_tmp94D;});break;case 413: _LL39D: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 414: _LL39E: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp94F=_cycalloc(sizeof(*_tmp94F));_tmp94F[0]=({struct Cyc_Exp_tok_struct _tmp950;
_tmp950.tag=Cyc_Exp_tok;_tmp950.f1=Cyc_Absyn_prim2_exp((void*)0,Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp950;});
_tmp94F;});break;case 415: _LL39F: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp951=
_cycalloc(sizeof(*_tmp951));_tmp951[0]=({struct Cyc_Exp_tok_struct _tmp952;_tmp952.tag=
Cyc_Exp_tok;_tmp952.f1=Cyc_Absyn_prim2_exp((void*)2,Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp952;});
_tmp951;});break;case 416: _LL3A0: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 417: _LL3A1: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp953=_cycalloc(sizeof(*_tmp953));_tmp953[0]=({struct Cyc_Exp_tok_struct _tmp954;
_tmp954.tag=Cyc_Exp_tok;_tmp954.f1=Cyc_Absyn_prim2_exp((void*)1,Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp954;});
_tmp953;});break;case 418: _LL3A2: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp955=
_cycalloc(sizeof(*_tmp955));_tmp955[0]=({struct Cyc_Exp_tok_struct _tmp956;_tmp956.tag=
Cyc_Exp_tok;_tmp956.f1=Cyc_Absyn_prim2_exp((void*)3,Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp956;});
_tmp955;});break;case 419: _LL3A3: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp957=
_cycalloc(sizeof(*_tmp957));_tmp957[0]=({struct Cyc_Exp_tok_struct _tmp958;_tmp958.tag=
Cyc_Exp_tok;_tmp958.f1=Cyc_Absyn_prim2_exp((void*)4,Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp958;});
_tmp957;});break;case 420: _LL3A4: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 421: _LL3A5: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp959=_cycalloc(sizeof(*_tmp959));_tmp959[0]=({struct Cyc_Exp_tok_struct _tmp95A;
_tmp95A.tag=Cyc_Exp_tok;_tmp95A.f1=Cyc_Absyn_cast_exp((*Cyc_yyget_ParamDecl_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)])).f3,Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),1,(void*)0,Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp95A;});
_tmp959;});break;case 422: _LL3A6: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 423: _LL3A7: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp95B=_cycalloc(sizeof(*_tmp95B));_tmp95B[0]=({struct Cyc_Exp_tok_struct _tmp95C;
_tmp95C.tag=Cyc_Exp_tok;_tmp95C.f1=Cyc_Absyn_pre_inc_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp95C;});
_tmp95B;});break;case 424: _LL3A8: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp95D=
_cycalloc(sizeof(*_tmp95D));_tmp95D[0]=({struct Cyc_Exp_tok_struct _tmp95E;_tmp95E.tag=
Cyc_Exp_tok;_tmp95E.f1=Cyc_Absyn_pre_dec_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp95E;});
_tmp95D;});break;case 425: _LL3A9: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp95F=
_cycalloc(sizeof(*_tmp95F));_tmp95F[0]=({struct Cyc_Exp_tok_struct _tmp960;_tmp960.tag=
Cyc_Exp_tok;_tmp960.f1=Cyc_Absyn_address_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp960;});
_tmp95F;});break;case 426: _LL3AA: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp961=
_cycalloc(sizeof(*_tmp961));_tmp961[0]=({struct Cyc_Exp_tok_struct _tmp962;_tmp962.tag=
Cyc_Exp_tok;_tmp962.f1=Cyc_Absyn_deref_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp962;});
_tmp961;});break;case 427: _LL3AB: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 428: _LL3AC: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp963=_cycalloc(sizeof(*_tmp963));_tmp963[0]=({struct Cyc_Exp_tok_struct _tmp964;
_tmp964.tag=Cyc_Exp_tok;_tmp964.f1=Cyc_Absyn_prim1_exp(Cyc_yyget_Primop_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp964;});
_tmp963;});break;case 429: _LL3AD: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp965=
_cycalloc(sizeof(*_tmp965));_tmp965[0]=({struct Cyc_Exp_tok_struct _tmp966;_tmp966.tag=
Cyc_Exp_tok;_tmp966.f1=Cyc_Absyn_sizeoftyp_exp((*Cyc_yyget_ParamDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)])).f3,Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp966;});
_tmp965;});break;case 430: _LL3AE: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp967=
_cycalloc(sizeof(*_tmp967));_tmp967[0]=({struct Cyc_Exp_tok_struct _tmp968;_tmp968.tag=
Cyc_Exp_tok;_tmp968.f1=Cyc_Absyn_sizeofexp_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp968;});
_tmp967;});break;case 431: _LL3AF: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp969=
_cycalloc(sizeof(*_tmp969));_tmp969[0]=({struct Cyc_Exp_tok_struct _tmp96A;_tmp96A.tag=
Cyc_Exp_tok;_tmp96A.f1=Cyc_Absyn_offsetof_exp((*Cyc_yyget_ParamDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)])).f3,(void*)({struct Cyc_Absyn_StructField_struct*
_tmp96B=_cycalloc(sizeof(*_tmp96B));_tmp96B[0]=({struct Cyc_Absyn_StructField_struct
_tmp96C;_tmp96C.tag=0;_tmp96C.f1=({struct _tagged_arr*_tmp96D=_cycalloc(sizeof(*
_tmp96D));_tmp96D[0]=Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(
10000,yyvsp_offset - 1)]);_tmp96D;});_tmp96C;});_tmp96B;}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 5)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp96A;});
_tmp969;});break;case 432: _LL3B0: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp96E=
_cycalloc(sizeof(*_tmp96E));_tmp96E[0]=({struct Cyc_Exp_tok_struct _tmp96F;_tmp96F.tag=
Cyc_Exp_tok;_tmp96F.f1=Cyc_Absyn_offsetof_exp((*Cyc_yyget_ParamDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)])).f3,(void*)({struct Cyc_Absyn_TupleIndex_struct*
_tmp970=_cycalloc_atomic(sizeof(*_tmp970));_tmp970[0]=({struct Cyc_Absyn_TupleIndex_struct
_tmp971;_tmp971.tag=1;_tmp971.f1=(unsigned int)(*Cyc_yyget_Int_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)])).f2;_tmp971;});_tmp970;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 5)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp96F;});_tmp96E;});break;case 433: _LL3B1: {struct
Cyc_Position_Segment*_tmp972=Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line);struct Cyc_List_List*
_tmp973=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Position_Segment*,
void*),struct Cyc_Position_Segment*env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,
_tmp972,Cyc_yyget_TypeList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 3)]));yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp974=_cycalloc(
sizeof(*_tmp974));_tmp974[0]=({struct Cyc_Exp_tok_struct _tmp975;_tmp975.tag=Cyc_Exp_tok;
_tmp975.f1=Cyc_Absyn_gentyp_exp(_tmp973,(*Cyc_yyget_ParamDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)])).f3,Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp975;});
_tmp974;});break;}case 434: _LL3B2: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp976=
_cycalloc(sizeof(*_tmp976));_tmp976[0]=({struct Cyc_Exp_tok_struct _tmp977;_tmp977.tag=
Cyc_Exp_tok;_tmp977.f1=Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Malloc_e_struct*
_tmp978=_cycalloc(sizeof(*_tmp978));_tmp978[0]=({struct Cyc_Absyn_Malloc_e_struct
_tmp979;_tmp979.tag=35;_tmp979.f1=({struct Cyc_Absyn_MallocInfo _tmp97A;_tmp97A.is_calloc=
0;_tmp97A.rgn=0;_tmp97A.elt_type=0;_tmp97A.num_elts=Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]);_tmp97A.fat_result=0;
_tmp97A;});_tmp979;});_tmp978;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp977;});
_tmp976;});break;case 435: _LL3B3: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp97B=
_cycalloc(sizeof(*_tmp97B));_tmp97B[0]=({struct Cyc_Exp_tok_struct _tmp97C;_tmp97C.tag=
Cyc_Exp_tok;_tmp97C.f1=Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Malloc_e_struct*
_tmp97D=_cycalloc(sizeof(*_tmp97D));_tmp97D[0]=({struct Cyc_Absyn_Malloc_e_struct
_tmp97E;_tmp97E.tag=35;_tmp97E.f1=({struct Cyc_Absyn_MallocInfo _tmp97F;_tmp97F.is_calloc=
0;_tmp97F.rgn=(struct Cyc_Absyn_Exp*)Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]);_tmp97F.elt_type=0;
_tmp97F.num_elts=Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 1)]);_tmp97F.fat_result=0;_tmp97F;});_tmp97E;});_tmp97D;}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 5)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp97C;});
_tmp97B;});break;case 436: _LL3B4: {void*_tmp981;struct _tuple2 _tmp980=*Cyc_yyget_ParamDecl_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp981=_tmp980.f3;
yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp982=_cycalloc(sizeof(*_tmp982));
_tmp982[0]=({struct Cyc_Exp_tok_struct _tmp983;_tmp983.tag=Cyc_Exp_tok;_tmp983.f1=
Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Malloc_e_struct*_tmp984=_cycalloc(
sizeof(*_tmp984));_tmp984[0]=({struct Cyc_Absyn_Malloc_e_struct _tmp985;_tmp985.tag=
35;_tmp985.f1=({struct Cyc_Absyn_MallocInfo _tmp986;_tmp986.is_calloc=1;_tmp986.rgn=
0;_tmp986.elt_type=({void**_tmp987=_cycalloc(sizeof(*_tmp987));_tmp987[0]=
_tmp981;_tmp987;});_tmp986.num_elts=Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 6)]);_tmp986.fat_result=0;
_tmp986;});_tmp985;});_tmp984;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 8)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp983;});
_tmp982;});break;}case 437: _LL3B5: {void*_tmp989;struct _tuple2 _tmp988=*Cyc_yyget_ParamDecl_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp989=_tmp988.f3;
yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp98A=_cycalloc(sizeof(*_tmp98A));
_tmp98A[0]=({struct Cyc_Exp_tok_struct _tmp98B;_tmp98B.tag=Cyc_Exp_tok;_tmp98B.f1=
Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Malloc_e_struct*_tmp98C=_cycalloc(
sizeof(*_tmp98C));_tmp98C[0]=({struct Cyc_Absyn_Malloc_e_struct _tmp98D;_tmp98D.tag=
35;_tmp98D.f1=({struct Cyc_Absyn_MallocInfo _tmp98E;_tmp98E.is_calloc=1;_tmp98E.rgn=(
struct Cyc_Absyn_Exp*)Cyc_yyget_Exp_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 8)]);_tmp98E.elt_type=({void**_tmp98F=_cycalloc(sizeof(*_tmp98F));
_tmp98F[0]=_tmp989;_tmp98F;});_tmp98E.num_elts=Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 6)]);_tmp98E.fat_result=0;
_tmp98E;});_tmp98D;});_tmp98C;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 10)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp98B;});
_tmp98A;});break;}case 438: _LL3B6: yyval=(void*)({struct Cyc_Primop_tok_struct*
_tmp990=_cycalloc(sizeof(*_tmp990));_tmp990[0]=({struct Cyc_Primop_tok_struct
_tmp991;_tmp991.tag=Cyc_Primop_tok;_tmp991.f1=(void*)((void*)12);_tmp991;});
_tmp990;});break;case 439: _LL3B7: yyval=(void*)({struct Cyc_Primop_tok_struct*
_tmp992=_cycalloc(sizeof(*_tmp992));_tmp992[0]=({struct Cyc_Primop_tok_struct
_tmp993;_tmp993.tag=Cyc_Primop_tok;_tmp993.f1=(void*)((void*)11);_tmp993;});
_tmp992;});break;case 440: _LL3B8: yyval=(void*)({struct Cyc_Primop_tok_struct*
_tmp994=_cycalloc(sizeof(*_tmp994));_tmp994[0]=({struct Cyc_Primop_tok_struct
_tmp995;_tmp995.tag=Cyc_Primop_tok;_tmp995.f1=(void*)((void*)2);_tmp995;});
_tmp994;});break;case 441: _LL3B9: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 442: _LL3BA: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp996=_cycalloc(sizeof(*_tmp996));_tmp996[0]=({struct Cyc_Exp_tok_struct _tmp997;
_tmp997.tag=Cyc_Exp_tok;_tmp997.f1=Cyc_Absyn_subscript_exp(Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 3)]),Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp997;});
_tmp996;});break;case 443: _LL3BB: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp998=
_cycalloc(sizeof(*_tmp998));_tmp998[0]=({struct Cyc_Exp_tok_struct _tmp999;_tmp999.tag=
Cyc_Exp_tok;_tmp999.f1=Cyc_Absyn_unknowncall_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),0,Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp999;});
_tmp998;});break;case 444: _LL3BC: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp99A=
_cycalloc(sizeof(*_tmp99A));_tmp99A[0]=({struct Cyc_Exp_tok_struct _tmp99B;_tmp99B.tag=
Cyc_Exp_tok;_tmp99B.f1=Cyc_Absyn_unknowncall_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 3)]),Cyc_yyget_ExpList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp99B;});
_tmp99A;});break;case 445: _LL3BD: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp99C=
_cycalloc(sizeof(*_tmp99C));_tmp99C[0]=({struct Cyc_Exp_tok_struct _tmp99D;_tmp99D.tag=
Cyc_Exp_tok;_tmp99D.f1=Cyc_Absyn_aggrmember_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),({struct _tagged_arr*
_tmp99E=_cycalloc(sizeof(*_tmp99E));_tmp99E[0]=Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp99E;}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp99D;});
_tmp99C;});break;case 446: _LL3BE: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp99F=
_cycalloc(sizeof(*_tmp99F));_tmp99F[0]=({struct Cyc_Exp_tok_struct _tmp9A0;_tmp9A0.tag=
Cyc_Exp_tok;_tmp9A0.f1=Cyc_Absyn_aggrarrow_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),({struct _tagged_arr*
_tmp9A1=_cycalloc(sizeof(*_tmp9A1));_tmp9A1[0]=Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp9A1;}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 2)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9A0;});
_tmp99F;});break;case 447: _LL3BF: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp9A2=
_cycalloc(sizeof(*_tmp9A2));_tmp9A2[0]=({struct Cyc_Exp_tok_struct _tmp9A3;_tmp9A3.tag=
Cyc_Exp_tok;_tmp9A3.f1=Cyc_Absyn_post_inc_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9A3;});
_tmp9A2;});break;case 448: _LL3C0: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp9A4=
_cycalloc(sizeof(*_tmp9A4));_tmp9A4[0]=({struct Cyc_Exp_tok_struct _tmp9A5;_tmp9A5.tag=
Cyc_Exp_tok;_tmp9A5.f1=Cyc_Absyn_post_dec_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9A5;});
_tmp9A4;});break;case 449: _LL3C1: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp9A6=
_cycalloc(sizeof(*_tmp9A6));_tmp9A6[0]=({struct Cyc_Exp_tok_struct _tmp9A7;_tmp9A7.tag=
Cyc_Exp_tok;_tmp9A7.f1=Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_CompoundLit_e_struct*
_tmp9A8=_cycalloc(sizeof(*_tmp9A8));_tmp9A8[0]=({struct Cyc_Absyn_CompoundLit_e_struct
_tmp9A9;_tmp9A9.tag=27;_tmp9A9.f1=Cyc_yyget_ParamDecl_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]);_tmp9A9.f2=((struct Cyc_List_List*(*)(
struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_InitializerList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]));_tmp9A9;});_tmp9A8;}),
Cyc_Position_segment_of_abs((yyls[_check_known_subscript_notnull(10000,
yylsp_offset - 5)]).first_line,(yyls[_check_known_subscript_notnull(10000,
yylsp_offset)]).last_line));_tmp9A7;});_tmp9A6;});break;case 450: _LL3C2: yyval=(
void*)({struct Cyc_Exp_tok_struct*_tmp9AA=_cycalloc(sizeof(*_tmp9AA));_tmp9AA[0]=({
struct Cyc_Exp_tok_struct _tmp9AB;_tmp9AB.tag=Cyc_Exp_tok;_tmp9AB.f1=Cyc_Absyn_new_exp((
void*)({struct Cyc_Absyn_CompoundLit_e_struct*_tmp9AC=_cycalloc(sizeof(*_tmp9AC));
_tmp9AC[0]=({struct Cyc_Absyn_CompoundLit_e_struct _tmp9AD;_tmp9AD.tag=27;_tmp9AD.f1=
Cyc_yyget_ParamDecl_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 
5)]);_tmp9AD.f2=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(
Cyc_yyget_InitializerList_tok(yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 2)]));_tmp9AD;});_tmp9AC;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 6)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9AB;});
_tmp9AA;});break;case 451: _LL3C3: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp9AE=
_cycalloc(sizeof(*_tmp9AE));_tmp9AE[0]=({struct Cyc_Exp_tok_struct _tmp9AF;_tmp9AF.tag=
Cyc_Exp_tok;_tmp9AF.f1=Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_UnknownId_e_struct*
_tmp9B0=_cycalloc(sizeof(*_tmp9B0));_tmp9B0[0]=({struct Cyc_Absyn_UnknownId_e_struct
_tmp9B1;_tmp9B1.tag=2;_tmp9B1.f1=Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp9B1;});_tmp9B0;}),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9AF;});
_tmp9AE;});break;case 452: _LL3C4: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset)];break;case 453: _LL3C5: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp9B2=_cycalloc(sizeof(*_tmp9B2));_tmp9B2[0]=({struct Cyc_Exp_tok_struct _tmp9B3;
_tmp9B3.tag=Cyc_Exp_tok;_tmp9B3.f1=Cyc_Absyn_string_exp(Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9B3;});
_tmp9B2;});break;case 454: _LL3C6: yyval=yyvs[_check_known_subscript_notnull(10000,
yyvsp_offset - 1)];break;case 455: _LL3C7: yyval=(void*)({struct Cyc_Exp_tok_struct*
_tmp9B4=_cycalloc(sizeof(*_tmp9B4));_tmp9B4[0]=({struct Cyc_Exp_tok_struct _tmp9B5;
_tmp9B5.tag=Cyc_Exp_tok;_tmp9B5.f1=Cyc_Absyn_noinstantiate_exp(Cyc_yyget_Exp_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9B5;});
_tmp9B4;});break;case 456: _LL3C8: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp9B6=
_cycalloc(sizeof(*_tmp9B6));_tmp9B6[0]=({struct Cyc_Exp_tok_struct _tmp9B7;_tmp9B7.tag=
Cyc_Exp_tok;_tmp9B7.f1=Cyc_Absyn_instantiate_exp(Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]),Cyc_List_imp_rev(Cyc_yyget_TypeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)])),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9B7;});
_tmp9B6;});break;case 457: _LL3C9: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp9B8=
_cycalloc(sizeof(*_tmp9B8));_tmp9B8[0]=({struct Cyc_Exp_tok_struct _tmp9B9;_tmp9B9.tag=
Cyc_Exp_tok;_tmp9B9.f1=Cyc_Absyn_tuple_exp(Cyc_yyget_ExpList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 1)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 3)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9B9;});
_tmp9B8;});break;case 458: _LL3CA: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp9BA=
_cycalloc(sizeof(*_tmp9BA));_tmp9BA[0]=({struct Cyc_Exp_tok_struct _tmp9BB;_tmp9BB.tag=
Cyc_Exp_tok;_tmp9BB.f1=Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Struct_e_struct*
_tmp9BC=_cycalloc(sizeof(*_tmp9BC));_tmp9BC[0]=({struct Cyc_Absyn_Struct_e_struct
_tmp9BD;_tmp9BD.tag=30;_tmp9BD.f1=Cyc_yyget_QualId_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 4)]);_tmp9BD.f2=Cyc_yyget_TypeList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp9BD.f3=((struct
Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_InitializerList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 1)]));_tmp9BD.f4=0;
_tmp9BD;});_tmp9BC;}),Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9BB;});
_tmp9BA;});break;case 459: _LL3CB: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp9BE=
_cycalloc(sizeof(*_tmp9BE));_tmp9BE[0]=({struct Cyc_Exp_tok_struct _tmp9BF;_tmp9BF.tag=
Cyc_Exp_tok;_tmp9BF.f1=Cyc_Absyn_stmt_exp(Cyc_yyget_Stmt_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset - 2)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 4)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9BF;});
_tmp9BE;});break;case 460: _LL3CC: yyval=(void*)({struct Cyc_ExpList_tok_struct*
_tmp9C0=_cycalloc(sizeof(*_tmp9C0));_tmp9C0[0]=({struct Cyc_ExpList_tok_struct
_tmp9C1;_tmp9C1.tag=Cyc_ExpList_tok;_tmp9C1.f1=((struct Cyc_List_List*(*)(struct
Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_ExpList_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]));_tmp9C1;});_tmp9C0;});
break;case 461: _LL3CD: yyval=(void*)({struct Cyc_ExpList_tok_struct*_tmp9C2=
_cycalloc(sizeof(*_tmp9C2));_tmp9C2[0]=({struct Cyc_ExpList_tok_struct _tmp9C3;
_tmp9C3.tag=Cyc_ExpList_tok;_tmp9C3.f1=({struct Cyc_List_List*_tmp9C4=_cycalloc(
sizeof(*_tmp9C4));_tmp9C4->hd=Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp9C4->tl=0;_tmp9C4;});
_tmp9C3;});_tmp9C2;});break;case 462: _LL3CE: yyval=(void*)({struct Cyc_ExpList_tok_struct*
_tmp9C5=_cycalloc(sizeof(*_tmp9C5));_tmp9C5[0]=({struct Cyc_ExpList_tok_struct
_tmp9C6;_tmp9C6.tag=Cyc_ExpList_tok;_tmp9C6.f1=({struct Cyc_List_List*_tmp9C7=
_cycalloc(sizeof(*_tmp9C7));_tmp9C7->hd=Cyc_yyget_Exp_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp9C7->tl=Cyc_yyget_ExpList_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset - 2)]);_tmp9C7;});_tmp9C6;});
_tmp9C5;});break;case 463: _LL3CF: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp9C8=
_cycalloc(sizeof(*_tmp9C8));_tmp9C8[0]=({struct Cyc_Exp_tok_struct _tmp9C9;_tmp9C9.tag=
Cyc_Exp_tok;_tmp9C9.f1=Cyc_Absyn_int_exp((*Cyc_yyget_Int_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)])).f1,(*Cyc_yyget_Int_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)])).f2,Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9C9;});
_tmp9C8;});break;case 464: _LL3D0: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp9CA=
_cycalloc(sizeof(*_tmp9CA));_tmp9CA[0]=({struct Cyc_Exp_tok_struct _tmp9CB;_tmp9CB.tag=
Cyc_Exp_tok;_tmp9CB.f1=Cyc_Absyn_char_exp(Cyc_yyget_Char_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9CB;});
_tmp9CA;});break;case 465: _LL3D1: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp9CC=
_cycalloc(sizeof(*_tmp9CC));_tmp9CC[0]=({struct Cyc_Exp_tok_struct _tmp9CD;_tmp9CD.tag=
Cyc_Exp_tok;_tmp9CD.f1=Cyc_Absyn_float_exp(Cyc_yyget_String_tok(yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)]),Cyc_Position_segment_of_abs((
yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9CD;});
_tmp9CC;});break;case 466: _LL3D2: yyval=(void*)({struct Cyc_Exp_tok_struct*_tmp9CE=
_cycalloc(sizeof(*_tmp9CE));_tmp9CE[0]=({struct Cyc_Exp_tok_struct _tmp9CF;_tmp9CF.tag=
Cyc_Exp_tok;_tmp9CF.f1=Cyc_Absyn_null_exp(Cyc_Position_segment_of_abs((yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).first_line,(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line));_tmp9CF;});
_tmp9CE;});break;case 467: _LL3D3: yyval=(void*)({struct Cyc_QualId_tok_struct*
_tmp9D0=_cycalloc(sizeof(*_tmp9D0));_tmp9D0[0]=({struct Cyc_QualId_tok_struct
_tmp9D1;_tmp9D1.tag=Cyc_QualId_tok;_tmp9D1.f1=({struct _tuple1*_tmp9D2=_cycalloc(
sizeof(*_tmp9D2));_tmp9D2->f1=Cyc_Absyn_rel_ns_null;_tmp9D2->f2=({struct
_tagged_arr*_tmp9D3=_cycalloc(sizeof(*_tmp9D3));_tmp9D3[0]=Cyc_yyget_String_tok(
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);_tmp9D3;});_tmp9D2;});
_tmp9D1;});_tmp9D0;});break;case 468: _LL3D4: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 469: _LL3D5: yyval=(
void*)({struct Cyc_QualId_tok_struct*_tmp9D4=_cycalloc(sizeof(*_tmp9D4));_tmp9D4[
0]=({struct Cyc_QualId_tok_struct _tmp9D5;_tmp9D5.tag=Cyc_QualId_tok;_tmp9D5.f1=({
struct _tuple1*_tmp9D6=_cycalloc(sizeof(*_tmp9D6));_tmp9D6->f1=Cyc_Absyn_rel_ns_null;
_tmp9D6->f2=({struct _tagged_arr*_tmp9D7=_cycalloc(sizeof(*_tmp9D7));_tmp9D7[0]=
Cyc_yyget_String_tok(yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)]);
_tmp9D7;});_tmp9D6;});_tmp9D5;});_tmp9D4;});break;case 470: _LL3D6: yyval=yyvs[
_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 471: _LL3D7: yyval=
yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 472: _LL3D8:
yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;case 473:
_LL3D9: yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;case
474: _LL3DA: yyval=yyvs[_check_known_subscript_notnull(10000,yyvsp_offset)];break;
case 475: _LL3DB: break;case 476: _LL3DC:((struct Cyc_Lexing_lexbuf*)((struct Cyc_Core_Opt*)
_check_null(Cyc_Parse_lbuf))->v)->lex_curr_pos -=1;break;default: _LL3DD: break;}
yyvsp_offset -=yylen;yyssp_offset -=yylen;yylsp_offset -=yylen;yyvs[
_check_known_subscript_notnull(10000,++ yyvsp_offset)]=yyval;yylsp_offset ++;if(
yylen == 0){(yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_line=
Cyc_yylloc.first_line;(yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).first_column=
Cyc_yylloc.first_column;(yyls[_check_known_subscript_notnull(10000,yylsp_offset)]).last_line=(
yyls[_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_line;(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_column=(yyls[
_check_known_subscript_notnull(10000,yylsp_offset - 1)]).last_column;}else{(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_line=(yyls[
_check_known_subscript_notnull(10000,(yylsp_offset + yylen)- 1)]).last_line;(yyls[
_check_known_subscript_notnull(10000,yylsp_offset)]).last_column=(yyls[
_check_known_subscript_notnull(10000,(yylsp_offset + yylen)- 1)]).last_column;}
yyn=(int)Cyc_yyr1[_check_known_subscript_notnull(477,yyn)];yystate=Cyc_yypgoto[
_check_known_subscript_notnull(137,yyn - 127)]+ yyss[
_check_known_subscript_notnull(10000,yyssp_offset)];if((yystate >= 0  && yystate <= 
5735) && Cyc_yycheck[_check_known_subscript_notnull(5736,yystate)]== yyss[
_check_known_subscript_notnull(10000,yyssp_offset)])yystate=(int)Cyc_yytable[
_check_known_subscript_notnull(5736,yystate)];else{yystate=(int)Cyc_yydefgoto[
_check_known_subscript_notnull(137,yyn - 127)];}goto yynewstate;yyerrlab: if(
yyerrstatus == 0){++ Cyc_yynerrs;yyn=(int)Cyc_yypact[
_check_known_subscript_notnull(947,yystate)];if(yyn > - 32768  && yyn < 5735){int sze=
0;struct _tagged_arr msg;int x;int count;count=0;for(x=yyn < 0?- yyn: 0;x < 264 / sizeof(
char*);x ++){if(Cyc_yycheck[_check_known_subscript_notnull(5736,x + yyn)]== x)(sze
+=Cyc_strlen((struct _tagged_arr)Cyc_yytname[_check_known_subscript_notnull(264,x)])
+ 15,count ++);}msg=({unsigned int _tmp9D8=(unsigned int)(sze + 15);char*_tmp9D9=(
char*)_cycalloc_atomic(_check_times(sizeof(char),_tmp9D8 + 1));struct _tagged_arr
_tmp9DB=_tag_arr(_tmp9D9,sizeof(char),_tmp9D8 + 1);{unsigned int _tmp9DA=_tmp9D8;
unsigned int i;for(i=0;i < _tmp9DA;i ++){_tmp9D9[i]='\000';}_tmp9D9[_tmp9DA]=(char)
0;}_tmp9DB;});Cyc_strcpy(msg,({const char*_tmp9DC="parse error";_tag_arr(_tmp9DC,
sizeof(char),_get_zero_arr_size(_tmp9DC,12));}));if(count < 5){count=0;for(x=yyn < 
0?- yyn: 0;x < 264 / sizeof(char*);x ++){if(Cyc_yycheck[
_check_known_subscript_notnull(5736,x + yyn)]== x){Cyc_strcat(msg,(struct
_tagged_arr)(count == 0?(struct _tagged_arr)({const char*_tmp9DD=", expecting `";
_tag_arr(_tmp9DD,sizeof(char),_get_zero_arr_size(_tmp9DD,14));}):(struct
_tagged_arr)({const char*_tmp9DE=" or `";_tag_arr(_tmp9DE,sizeof(char),
_get_zero_arr_size(_tmp9DE,6));})));Cyc_strcat(msg,(struct _tagged_arr)Cyc_yytname[
_check_known_subscript_notnull(264,x)]);Cyc_strcat(msg,({const char*_tmp9DF="'";
_tag_arr(_tmp9DF,sizeof(char),_get_zero_arr_size(_tmp9DF,2));}));count ++;}}}Cyc_yyerror((
struct _tagged_arr)msg);}else{Cyc_yyerror(({const char*_tmp9E0="parse error";
_tag_arr(_tmp9E0,sizeof(char),_get_zero_arr_size(_tmp9E0,12));}));}}goto
yyerrlab1;yyerrlab1: if(yyerrstatus == 3){if(Cyc_yychar == 0)return 1;Cyc_yychar=- 2;}
yyerrstatus=3;goto yyerrhandle;yyerrdefault: yyerrpop: if(yyssp_offset == 0)return 1;
yyvsp_offset --;yystate=(int)yyss[_check_known_subscript_notnull(10000,--
yyssp_offset)];yylsp_offset --;yyerrhandle: yyn=(int)Cyc_yypact[
_check_known_subscript_notnull(947,yystate)];if(yyn == - 32768)goto yyerrdefault;
yyn +=1;if((yyn < 0  || yyn > 5735) || Cyc_yycheck[_check_known_subscript_notnull(
5736,yyn)]!= 1)goto yyerrdefault;yyn=(int)Cyc_yytable[
_check_known_subscript_notnull(5736,yyn)];if(yyn < 0){if(yyn == - 32768)goto
yyerrpop;yyn=- yyn;goto yyreduce;}else{if(yyn == 0)goto yyerrpop;}if(yyn == 946)
return 0;yyvs[_check_known_subscript_notnull(10000,++ yyvsp_offset)]=Cyc_yylval;
yyls[_check_known_subscript_notnull(10000,++ yylsp_offset)]=Cyc_yylloc;goto
yynewstate;}void Cyc_yyprint(int i,void*v){void*_tmp9EA=v;struct _tuple13*_tmp9EB;
struct _tuple13 _tmp9EC;int _tmp9ED;char _tmp9EE;short _tmp9EF;struct _tagged_arr
_tmp9F0;struct Cyc_Core_Opt*_tmp9F1;struct Cyc_Core_Opt*_tmp9F2;struct Cyc_Core_Opt
_tmp9F3;struct _tagged_arr*_tmp9F4;struct _tuple1*_tmp9F5;struct _tuple1 _tmp9F6;
void*_tmp9F7;struct _tagged_arr*_tmp9F8;_LL3E0: if(*((void**)_tmp9EA)!= Cyc_Int_tok)
goto _LL3E2;_tmp9EB=((struct Cyc_Int_tok_struct*)_tmp9EA)->f1;_tmp9EC=*_tmp9EB;
_tmp9ED=_tmp9EC.f2;_LL3E1:({struct Cyc_Int_pa_struct _tmp9FB;_tmp9FB.tag=1;_tmp9FB.f1=(
unsigned int)_tmp9ED;{void*_tmp9F9[1]={& _tmp9FB};Cyc_fprintf(Cyc_stderr,({const
char*_tmp9FA="%d";_tag_arr(_tmp9FA,sizeof(char),_get_zero_arr_size(_tmp9FA,3));}),
_tag_arr(_tmp9F9,sizeof(void*),1));}});goto _LL3DF;_LL3E2: if(*((void**)_tmp9EA)!= 
Cyc_Char_tok)goto _LL3E4;_tmp9EE=((struct Cyc_Char_tok_struct*)_tmp9EA)->f1;_LL3E3:({
struct Cyc_Int_pa_struct _tmp9FE;_tmp9FE.tag=1;_tmp9FE.f1=(unsigned int)((int)
_tmp9EE);{void*_tmp9FC[1]={& _tmp9FE};Cyc_fprintf(Cyc_stderr,({const char*_tmp9FD="%c";
_tag_arr(_tmp9FD,sizeof(char),_get_zero_arr_size(_tmp9FD,3));}),_tag_arr(_tmp9FC,
sizeof(void*),1));}});goto _LL3DF;_LL3E4: if(*((void**)_tmp9EA)!= Cyc_Short_tok)
goto _LL3E6;_tmp9EF=((struct Cyc_Short_tok_struct*)_tmp9EA)->f1;_LL3E5:({struct Cyc_Int_pa_struct
_tmpA01;_tmpA01.tag=1;_tmpA01.f1=(unsigned int)((int)_tmp9EF);{void*_tmp9FF[1]={&
_tmpA01};Cyc_fprintf(Cyc_stderr,({const char*_tmpA00="%ds";_tag_arr(_tmpA00,
sizeof(char),_get_zero_arr_size(_tmpA00,4));}),_tag_arr(_tmp9FF,sizeof(void*),1));}});
goto _LL3DF;_LL3E6: if(*((void**)_tmp9EA)!= Cyc_String_tok)goto _LL3E8;_tmp9F0=((
struct Cyc_String_tok_struct*)_tmp9EA)->f1;_LL3E7:({struct Cyc_String_pa_struct
_tmpA04;_tmpA04.tag=0;_tmpA04.f1=(struct _tagged_arr)((struct _tagged_arr)_tmp9F0);{
void*_tmpA02[1]={& _tmpA04};Cyc_fprintf(Cyc_stderr,({const char*_tmpA03="\"%s\"";
_tag_arr(_tmpA03,sizeof(char),_get_zero_arr_size(_tmpA03,5));}),_tag_arr(_tmpA02,
sizeof(void*),1));}});goto _LL3DF;_LL3E8: if(*((void**)_tmp9EA)!= Cyc_Stringopt_tok)
goto _LL3EA;_tmp9F1=((struct Cyc_Stringopt_tok_struct*)_tmp9EA)->f1;if(_tmp9F1 != 0)
goto _LL3EA;_LL3E9:({void*_tmpA05[0]={};Cyc_fprintf(Cyc_stderr,({const char*
_tmpA06="NULL";_tag_arr(_tmpA06,sizeof(char),_get_zero_arr_size(_tmpA06,5));}),
_tag_arr(_tmpA05,sizeof(void*),0));});goto _LL3DF;_LL3EA: if(*((void**)_tmp9EA)!= 
Cyc_Stringopt_tok)goto _LL3EC;_tmp9F2=((struct Cyc_Stringopt_tok_struct*)_tmp9EA)->f1;
if(_tmp9F2 == 0)goto _LL3EC;_tmp9F3=*_tmp9F2;_tmp9F4=(struct _tagged_arr*)_tmp9F3.v;
_LL3EB:({struct Cyc_String_pa_struct _tmpA09;_tmpA09.tag=0;_tmpA09.f1=(struct
_tagged_arr)((struct _tagged_arr)*_tmp9F4);{void*_tmpA07[1]={& _tmpA09};Cyc_fprintf(
Cyc_stderr,({const char*_tmpA08="\"%s\"";_tag_arr(_tmpA08,sizeof(char),
_get_zero_arr_size(_tmpA08,5));}),_tag_arr(_tmpA07,sizeof(void*),1));}});goto
_LL3DF;_LL3EC: if(*((void**)_tmp9EA)!= Cyc_QualId_tok)goto _LL3EE;_tmp9F5=((struct
Cyc_QualId_tok_struct*)_tmp9EA)->f1;_tmp9F6=*_tmp9F5;_tmp9F7=_tmp9F6.f1;_tmp9F8=
_tmp9F6.f2;_LL3ED: {struct Cyc_List_List*_tmpA0A=0;{void*_tmpA0B=_tmp9F7;struct
Cyc_List_List*_tmpA0C;struct Cyc_List_List*_tmpA0D;_LL3F1: if(_tmpA0B <= (void*)1
 || *((int*)_tmpA0B)!= 0)goto _LL3F3;_tmpA0C=((struct Cyc_Absyn_Rel_n_struct*)
_tmpA0B)->f1;_LL3F2: _tmpA0A=_tmpA0C;goto _LL3F0;_LL3F3: if(_tmpA0B <= (void*)1  || *((
int*)_tmpA0B)!= 1)goto _LL3F5;_tmpA0D=((struct Cyc_Absyn_Abs_n_struct*)_tmpA0B)->f1;
_LL3F4: _tmpA0A=_tmpA0D;goto _LL3F0;_LL3F5: if((int)_tmpA0B != 0)goto _LL3F0;_LL3F6:
goto _LL3F0;_LL3F0:;}for(0;_tmpA0A != 0;_tmpA0A=_tmpA0A->tl){({struct Cyc_String_pa_struct
_tmpA10;_tmpA10.tag=0;_tmpA10.f1=(struct _tagged_arr)((struct _tagged_arr)*((
struct _tagged_arr*)_tmpA0A->hd));{void*_tmpA0E[1]={& _tmpA10};Cyc_fprintf(Cyc_stderr,({
const char*_tmpA0F="%s::";_tag_arr(_tmpA0F,sizeof(char),_get_zero_arr_size(
_tmpA0F,5));}),_tag_arr(_tmpA0E,sizeof(void*),1));}});}({struct Cyc_String_pa_struct
_tmpA13;_tmpA13.tag=0;_tmpA13.f1=(struct _tagged_arr)((struct _tagged_arr)*_tmp9F8);{
void*_tmpA11[1]={& _tmpA13};Cyc_fprintf(Cyc_stderr,({const char*_tmpA12="%s::";
_tag_arr(_tmpA12,sizeof(char),_get_zero_arr_size(_tmpA12,5));}),_tag_arr(_tmpA11,
sizeof(void*),1));}});goto _LL3DF;}_LL3EE:;_LL3EF:({void*_tmpA14[0]={};Cyc_fprintf(
Cyc_stderr,({const char*_tmpA15="?";_tag_arr(_tmpA15,sizeof(char),
_get_zero_arr_size(_tmpA15,2));}),_tag_arr(_tmpA14,sizeof(void*),0));});goto
_LL3DF;_LL3DF:;}struct Cyc_List_List*Cyc_Parse_parse_file(struct Cyc___cycFILE*f){
Cyc_Parse_parse_result=0;Cyc_Parse_lbuf=({struct Cyc_Core_Opt*_tmpA16=_cycalloc(
sizeof(*_tmpA16));_tmpA16->v=Cyc_Lexing_from_file(f);_tmpA16;});Cyc_yyparse();
return Cyc_Parse_parse_result;}
