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
  unsigned int total_bytes;
  unsigned int free_bytes;
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
#ifndef NO_CYC_NULL_CHECKS
  if(!ptr)
    _throw_null();
#endif
  return ptr;
}
static inline 
char * _check_known_subscript_null(void * ptr, unsigned bound, 
				   unsigned elt_sz, unsigned index) {
#ifndef NO_CYC_NULL_CHECKS
  if(!ptr)
    _throw_null();
#endif
#ifndef NO_CYC_BOUNDS_CHECKS
  if(index >= bound)
    _throw_arraybounds();
#endif
  return ((char *)ptr) + elt_sz*index;
}
static inline 
unsigned _check_known_subscript_notnull(unsigned bound, unsigned index) {
#ifndef NO_CYC_BOUNDS_CHECKS
  if(index >= bound)
    _throw_arraybounds();
#endif
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
#ifndef NO_CYC_NULL_CHECKS
  if(!arr.base) 
    _throw_null();
#endif
#ifndef NO_CYC_BOUNDS_CHECKS
  if(ans < arr.base || ans >= arr.last_plus_one)
    _throw_arraybounds();
#endif
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
#ifndef NO_CYC_BOUNDS_CHECKS
  if(arr.curr < arr.base || arr.curr + elt_sz * num_elts > arr.last_plus_one)
    _throw_arraybounds();
#endif
  return arr.curr;
}
static inline 
unsigned _get_arr_size(struct _tagged_arr arr, unsigned elt_sz) {
  return (arr.curr<arr.base || arr.curr>=arr.last_plus_one) ? 0 : ((arr.last_plus_one - arr.curr) / elt_sz);
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
extern void * GC_calloc(unsigned int,unsigned int);
extern void * GC_calloc_atomic(unsigned int,unsigned int);

static inline void * _cycalloc(int n) {
  void * ans = (void *)GC_malloc(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static inline void * _cycalloc_atomic(int n) {
  void * ans = (void *)GC_malloc_atomic(n);
  if(!ans)
    _throw_badalloc();
  return ans;
}
static inline void * _cyccalloc(unsigned int n, unsigned int s) {
  void * ans = (void *)GC_calloc(n,s);
  if (!ans)
    _throw_badalloc();
  return ans;
}
static inline void * _cyccalloc_atomic(unsigned int n, unsigned int s) {
  void * ans = (void *)GC_calloc_atomic(n,s);
  if (!ans)
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

#if defined(CYC_REGION_PROFILE) 
extern void * _profile_GC_malloc(int,char *file,int lineno);
extern void * _profile_GC_malloc_atomic(int,char *file,int lineno);
extern void * _profile_region_malloc(struct _RegionHandle *, unsigned int,
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
 struct Cyc_Core_Opt{void*v;};extern struct _tagged_arr Cyc_Core_new_string(
unsigned int);extern unsigned char Cyc_Core_Invalid_argument[21];struct Cyc_Core_Invalid_argument_struct{
unsigned char*tag;struct _tagged_arr f1;};extern unsigned char Cyc_Core_Failure[12];
struct Cyc_Core_Failure_struct{unsigned char*tag;struct _tagged_arr f1;};extern
unsigned char Cyc_Core_Impossible[15];struct Cyc_Core_Impossible_struct{
unsigned char*tag;struct _tagged_arr f1;};extern unsigned char Cyc_Core_Not_found[14];
extern unsigned char Cyc_Core_Unreachable[16];struct Cyc_Core_Unreachable_struct{
unsigned char*tag;struct _tagged_arr f1;};struct Cyc_List_List{void*hd;struct Cyc_List_List*
tl;};extern unsigned char Cyc_List_List_mismatch[18];extern struct Cyc_List_List*Cyc_List_imp_rev(
struct Cyc_List_List*x);extern unsigned char Cyc_List_Nth[8];struct Cyc_Cstdio___sFILE;
struct Cyc_Std___sFILE;extern struct Cyc_Std___sFILE*Cyc_Std_stdout;extern struct Cyc_Std___sFILE*
Cyc_Std_stderr;extern int Cyc_Std_fflush(struct Cyc_Std___sFILE*);extern
unsigned char Cyc_Std_FileCloseError[19];extern unsigned char Cyc_Std_FileOpenError[
18];struct Cyc_Std_FileOpenError_struct{unsigned char*tag;struct _tagged_arr f1;};
static const int Cyc_Std_String_pa=0;struct Cyc_Std_String_pa_struct{int tag;struct
_tagged_arr f1;};static const int Cyc_Std_Int_pa=1;struct Cyc_Std_Int_pa_struct{int
tag;unsigned int f1;};static const int Cyc_Std_Double_pa=2;struct Cyc_Std_Double_pa_struct{
int tag;double f1;};static const int Cyc_Std_ShortPtr_pa=3;struct Cyc_Std_ShortPtr_pa_struct{
int tag;short*f1;};static const int Cyc_Std_IntPtr_pa=4;struct Cyc_Std_IntPtr_pa_struct{
int tag;unsigned int*f1;};extern int Cyc_Std_fprintf(struct Cyc_Std___sFILE*,struct
_tagged_arr fmt,struct _tagged_arr);extern struct _tagged_arr Cyc_Std_aprintf(struct
_tagged_arr fmt,struct _tagged_arr);static const int Cyc_Std_ShortPtr_sa=0;struct Cyc_Std_ShortPtr_sa_struct{
int tag;short*f1;};static const int Cyc_Std_UShortPtr_sa=1;struct Cyc_Std_UShortPtr_sa_struct{
int tag;unsigned short*f1;};static const int Cyc_Std_IntPtr_sa=2;struct Cyc_Std_IntPtr_sa_struct{
int tag;int*f1;};static const int Cyc_Std_UIntPtr_sa=3;struct Cyc_Std_UIntPtr_sa_struct{
int tag;unsigned int*f1;};static const int Cyc_Std_StringPtr_sa=4;struct Cyc_Std_StringPtr_sa_struct{
int tag;struct _tagged_arr f1;};static const int Cyc_Std_DoublePtr_sa=5;struct Cyc_Std_DoublePtr_sa_struct{
int tag;double*f1;};static const int Cyc_Std_FloatPtr_sa=6;struct Cyc_Std_FloatPtr_sa_struct{
int tag;float*f1;};struct Cyc_Lineno_Pos{struct _tagged_arr logical_file;struct
_tagged_arr line;int line_no;int col;};extern struct Cyc_Lineno_Pos*Cyc_Lineno_pos_of_abs(
struct _tagged_arr,int);extern void Cyc_Lineno_poss_of_abss(struct _tagged_arr
filename,struct Cyc_List_List*places);extern unsigned char Cyc_Position_Exit[9];
extern void Cyc_Position_reset_position(struct _tagged_arr);extern void Cyc_Position_set_position_file(
struct _tagged_arr);extern struct _tagged_arr Cyc_Position_get_position_file();
struct Cyc_Position_Segment;extern struct Cyc_Position_Segment*Cyc_Position_segment_of_abs(
int,int);extern struct Cyc_Position_Segment*Cyc_Position_segment_join(struct Cyc_Position_Segment*,
struct Cyc_Position_Segment*);extern struct _tagged_arr Cyc_Position_string_of_loc(
int);extern struct _tagged_arr Cyc_Position_string_of_segment(struct Cyc_Position_Segment*);
extern struct Cyc_List_List*Cyc_Position_strings_of_segments(struct Cyc_List_List*);
static const int Cyc_Position_Lex=0;static const int Cyc_Position_Parse=1;static const
int Cyc_Position_Elab=2;struct Cyc_Position_Error{struct _tagged_arr source;struct
Cyc_Position_Segment*seg;void*kind;struct _tagged_arr desc;};extern struct Cyc_Position_Error*
Cyc_Position_mk_err_lex(struct Cyc_Position_Segment*,struct _tagged_arr);extern
struct Cyc_Position_Error*Cyc_Position_mk_err_parse(struct Cyc_Position_Segment*,
struct _tagged_arr);extern struct Cyc_Position_Error*Cyc_Position_mk_err_elab(
struct Cyc_Position_Segment*,struct _tagged_arr);extern unsigned char Cyc_Position_Nocontext[
14];extern int Cyc_Position_print_context;extern int Cyc_Position_num_errors;extern
int Cyc_Position_max_errors;extern void Cyc_Position_post_error(struct Cyc_Position_Error*);
extern int Cyc_Position_error_p();extern struct _tagged_arr Cyc_Position_get_line_directive(
struct Cyc_Position_Segment*loc);extern unsigned int Cyc_Std_strlen(struct
_tagged_arr s);extern int Cyc_Std_strcmp(struct _tagged_arr s1,struct _tagged_arr s2);
extern struct _tagged_arr Cyc_Std_strncpy(struct _tagged_arr,struct _tagged_arr,
unsigned int);extern struct _tagged_arr Cyc_Std_substring(struct _tagged_arr,int ofs,
unsigned int n);static const int Cyc_Typerep_Var=0;struct Cyc_Typerep_Var_struct{int
tag;struct _tagged_arr*f1;};static const int Cyc_Typerep_Int=1;struct Cyc_Typerep_Int_struct{
int tag;unsigned int f1;};static const int Cyc_Typerep_Float=0;static const int Cyc_Typerep_Double=
1;static const int Cyc_Typerep_ThinPtr=2;struct Cyc_Typerep_ThinPtr_struct{int tag;
unsigned int f1;void*f2;};static const int Cyc_Typerep_FatPtr=3;struct Cyc_Typerep_FatPtr_struct{
int tag;void*f1;};static const int Cyc_Typerep_Tuple=4;struct _tuple0{unsigned int f1;
void*f2;};struct Cyc_Typerep_Tuple_struct{int tag;unsigned int f1;struct _tagged_arr
f2;};static const int Cyc_Typerep_TUnion=5;struct Cyc_Typerep_TUnion_struct{int tag;
struct _tagged_arr f1;};static const int Cyc_Typerep_XTUnion=6;struct _tuple1{struct
_tagged_arr f1;void*f2;};struct Cyc_Typerep_XTUnion_struct{int tag;struct
_tagged_arr f1;};static const int Cyc_Typerep_Union=7;struct Cyc_Typerep_Union_struct{
int tag;struct _tagged_arr f1;};static const int Cyc_Typerep_Forall=8;struct Cyc_Typerep_Forall_struct{
int tag;struct _tagged_arr f1;void**f2;};static const int Cyc_Typerep_App=9;struct Cyc_Typerep_App_struct{
int tag;void*f1;struct _tagged_arr f2;};unsigned int Cyc_Typerep_size_type(void*rep);
extern struct Cyc_Typerep_Tuple_struct Cyc_struct_Position_Segment_rep;
unsigned char Cyc_Position_Exit[9]="\000\000\000\000Exit";static unsigned char
_tmp0[1]="";static struct _tagged_arr Cyc_Position_source={_tmp0,_tmp0,_tmp0 + 1};
struct Cyc_Position_Segment{int start;int end;};struct Cyc_Position_Segment*Cyc_Position_segment_of_abs(
int start,int end){return({struct Cyc_Position_Segment*_tmp1=_cycalloc_atomic(
sizeof(struct Cyc_Position_Segment));_tmp1->start=start;_tmp1->end=end;_tmp1;});}
struct Cyc_Position_Segment*Cyc_Position_segment_join(struct Cyc_Position_Segment*
s1,struct Cyc_Position_Segment*s2){if(s1 == 0){return s2;}if(s2 == 0){return s1;}
return({struct Cyc_Position_Segment*_tmp2=_cycalloc_atomic(sizeof(struct Cyc_Position_Segment));
_tmp2->start=s1->start;_tmp2->end=s2->end;_tmp2;});}struct _tagged_arr Cyc_Position_string_of_loc(
int loc){struct Cyc_Lineno_Pos*pos=Cyc_Lineno_pos_of_abs(Cyc_Position_source,loc);
return({struct Cyc_Std_Int_pa_struct _tmp6;_tmp6.tag=Cyc_Std_Int_pa;_tmp6.f1=(int)((
unsigned int)pos->col);{struct Cyc_Std_Int_pa_struct _tmp5;_tmp5.tag=Cyc_Std_Int_pa;
_tmp5.f1=(int)((unsigned int)pos->line_no);{struct Cyc_Std_String_pa_struct _tmp4;
_tmp4.tag=Cyc_Std_String_pa;_tmp4.f1=(struct _tagged_arr)pos->logical_file;{void*
_tmp3[3]={& _tmp4,& _tmp5,& _tmp6};Cyc_Std_aprintf(_tag_arr("%s (%d:%d)",sizeof(
unsigned char),11),_tag_arr(_tmp3,sizeof(void*),3));}}}});}static struct
_tagged_arr Cyc_Position_string_of_pos_pr(struct Cyc_Lineno_Pos*pos_s,struct Cyc_Lineno_Pos*
pos_e){if(Cyc_Std_strcmp(pos_s->logical_file,pos_e->logical_file)== 0){return({
struct Cyc_Std_Int_pa_struct _tmpC;_tmpC.tag=Cyc_Std_Int_pa;_tmpC.f1=(int)((
unsigned int)pos_e->col);{struct Cyc_Std_Int_pa_struct _tmpB;_tmpB.tag=Cyc_Std_Int_pa;
_tmpB.f1=(int)((unsigned int)pos_e->line_no);{struct Cyc_Std_Int_pa_struct _tmpA;
_tmpA.tag=Cyc_Std_Int_pa;_tmpA.f1=(int)((unsigned int)pos_s->col);{struct Cyc_Std_Int_pa_struct
_tmp9;_tmp9.tag=Cyc_Std_Int_pa;_tmp9.f1=(int)((unsigned int)pos_s->line_no);{
struct Cyc_Std_String_pa_struct _tmp8;_tmp8.tag=Cyc_Std_String_pa;_tmp8.f1=(struct
_tagged_arr)pos_s->logical_file;{void*_tmp7[5]={& _tmp8,& _tmp9,& _tmpA,& _tmpB,&
_tmpC};Cyc_Std_aprintf(_tag_arr("%s(%d:%d-%d:%d)",sizeof(unsigned char),16),
_tag_arr(_tmp7,sizeof(void*),5));}}}}}});}else{return({struct Cyc_Std_Int_pa_struct
_tmp13;_tmp13.tag=Cyc_Std_Int_pa;_tmp13.f1=(int)((unsigned int)pos_e->col);{
struct Cyc_Std_Int_pa_struct _tmp12;_tmp12.tag=Cyc_Std_Int_pa;_tmp12.f1=(int)((
unsigned int)pos_e->line_no);{struct Cyc_Std_String_pa_struct _tmp11;_tmp11.tag=
Cyc_Std_String_pa;_tmp11.f1=(struct _tagged_arr)pos_e->logical_file;{struct Cyc_Std_Int_pa_struct
_tmp10;_tmp10.tag=Cyc_Std_Int_pa;_tmp10.f1=(int)((unsigned int)pos_s->col);{
struct Cyc_Std_Int_pa_struct _tmpF;_tmpF.tag=Cyc_Std_Int_pa;_tmpF.f1=(int)((
unsigned int)pos_s->line_no);{struct Cyc_Std_String_pa_struct _tmpE;_tmpE.tag=Cyc_Std_String_pa;
_tmpE.f1=(struct _tagged_arr)pos_s->logical_file;{void*_tmpD[6]={& _tmpE,& _tmpF,&
_tmp10,& _tmp11,& _tmp12,& _tmp13};Cyc_Std_aprintf(_tag_arr("%s(%d:%d)-%s(%d:%d)",
sizeof(unsigned char),20),_tag_arr(_tmpD,sizeof(void*),6));}}}}}}});}}struct
_tagged_arr Cyc_Position_string_of_segment(struct Cyc_Position_Segment*s){if(s == 0){
return({struct Cyc_Std_String_pa_struct _tmp15;_tmp15.tag=Cyc_Std_String_pa;_tmp15.f1=(
struct _tagged_arr)Cyc_Position_source;{void*_tmp14[1]={& _tmp15};Cyc_Std_aprintf(
_tag_arr("%s",sizeof(unsigned char),3),_tag_arr(_tmp14,sizeof(void*),1));}});}{
struct Cyc_Lineno_Pos*pos_s=Cyc_Lineno_pos_of_abs(Cyc_Position_source,s->start);
struct Cyc_Lineno_Pos*pos_e=Cyc_Lineno_pos_of_abs(Cyc_Position_source,s->end);
return Cyc_Position_string_of_pos_pr(pos_s,pos_e);}}static struct Cyc_Lineno_Pos*
Cyc_Position_new_pos(){return({struct Cyc_Lineno_Pos*_tmp16=_cycalloc(sizeof(
struct Cyc_Lineno_Pos));_tmp16->logical_file=_tag_arr("",sizeof(unsigned char),1);
_tmp16->line=Cyc_Core_new_string(0);_tmp16->line_no=0;_tmp16->col=0;_tmp16;});}
struct _tuple2{int f1;struct Cyc_Lineno_Pos*f2;};struct Cyc_List_List*Cyc_Position_strings_of_segments(
struct Cyc_List_List*segs){struct Cyc_List_List*places=0;{struct Cyc_List_List*
_tmp17=segs;for(0;_tmp17 != 0;_tmp17=_tmp17->tl){if((struct Cyc_Position_Segment*)
_tmp17->hd == 0){continue;}places=({struct Cyc_List_List*_tmp18=_cycalloc(sizeof(
struct Cyc_List_List));_tmp18->hd=({struct _tuple2*_tmp1B=_cycalloc(sizeof(struct
_tuple2));_tmp1B->f1=((struct Cyc_Position_Segment*)_check_null((struct Cyc_Position_Segment*)
_tmp17->hd))->end;_tmp1B->f2=Cyc_Position_new_pos();_tmp1B;});_tmp18->tl=({
struct Cyc_List_List*_tmp19=_cycalloc(sizeof(struct Cyc_List_List));_tmp19->hd=({
struct _tuple2*_tmp1A=_cycalloc(sizeof(struct _tuple2));_tmp1A->f1=((struct Cyc_Position_Segment*)
_check_null((struct Cyc_Position_Segment*)_tmp17->hd))->start;_tmp1A->f2=Cyc_Position_new_pos();
_tmp1A;});_tmp19->tl=places;_tmp19;});_tmp18;});}}Cyc_Lineno_poss_of_abss(Cyc_Position_source,
places);{struct Cyc_List_List*ans=0;places=((struct Cyc_List_List*(*)(struct Cyc_List_List*
x))Cyc_List_imp_rev)(places);for(0;segs != 0;segs=segs->tl){if((struct Cyc_Position_Segment*)
segs->hd == 0){ans=({struct Cyc_List_List*_tmp1C=_cycalloc(sizeof(struct Cyc_List_List));
_tmp1C->hd=({struct _tagged_arr*_tmp1D=_cycalloc(sizeof(struct _tagged_arr));
_tmp1D[0]=({struct Cyc_Std_String_pa_struct _tmp1F;_tmp1F.tag=Cyc_Std_String_pa;
_tmp1F.f1=(struct _tagged_arr)Cyc_Position_source;{void*_tmp1E[1]={& _tmp1F};Cyc_Std_aprintf(
_tag_arr("%s(unknown)",sizeof(unsigned char),12),_tag_arr(_tmp1E,sizeof(void*),1));}});
_tmp1D;});_tmp1C->tl=ans;_tmp1C;});}else{ans=({struct Cyc_List_List*_tmp20=
_cycalloc(sizeof(struct Cyc_List_List));_tmp20->hd=({struct _tagged_arr*_tmp21=
_cycalloc(sizeof(struct _tagged_arr));_tmp21[0]=Cyc_Position_string_of_pos_pr((*((
struct _tuple2*)((struct Cyc_List_List*)_check_null(places))->hd)).f2,(*((struct
_tuple2*)((struct Cyc_List_List*)_check_null(((struct Cyc_List_List*)_check_null(
places))->tl))->hd)).f2);_tmp21;});_tmp20->tl=ans;_tmp20;});places=((struct Cyc_List_List*)
_check_null(places->tl))->tl;}}return ans;}}struct Cyc_Position_Error;struct Cyc_Position_Error*
Cyc_Position_mk_err_lex(struct Cyc_Position_Segment*l,struct _tagged_arr desc){
return({struct Cyc_Position_Error*_tmp22=_cycalloc(sizeof(struct Cyc_Position_Error));
_tmp22->source=Cyc_Position_source;_tmp22->seg=l;_tmp22->kind=(void*)((void*)Cyc_Position_Lex);
_tmp22->desc=desc;_tmp22;});}struct Cyc_Position_Error*Cyc_Position_mk_err_parse(
struct Cyc_Position_Segment*l,struct _tagged_arr desc){return({struct Cyc_Position_Error*
_tmp23=_cycalloc(sizeof(struct Cyc_Position_Error));_tmp23->source=Cyc_Position_source;
_tmp23->seg=l;_tmp23->kind=(void*)((void*)Cyc_Position_Parse);_tmp23->desc=desc;
_tmp23;});}struct Cyc_Position_Error*Cyc_Position_mk_err_elab(struct Cyc_Position_Segment*
l,struct _tagged_arr desc){return({struct Cyc_Position_Error*_tmp24=_cycalloc(
sizeof(struct Cyc_Position_Error));_tmp24->source=Cyc_Position_source;_tmp24->seg=
l;_tmp24->kind=(void*)((void*)Cyc_Position_Elab);_tmp24->desc=desc;_tmp24;});}
unsigned char Cyc_Position_Nocontext[14]="\000\000\000\000Nocontext";static struct
_tagged_arr Cyc_Position_trunc(int n,struct _tagged_arr s){int len=(int)Cyc_Std_strlen((
struct _tagged_arr)s);if(len < n){return s;}{int len_one=(n - 3)/ 2;int len_two=(n - 3)
- len_one;struct _tagged_arr ans=Cyc_Core_new_string((unsigned int)(n + 1));Cyc_Std_strncpy(
ans,(struct _tagged_arr)s,(unsigned int)len_one);Cyc_Std_strncpy(_tagged_arr_plus(
ans,sizeof(unsigned char),len_one),_tag_arr("...",sizeof(unsigned char),4),3);
Cyc_Std_strncpy(_tagged_arr_plus(_tagged_arr_plus(ans,sizeof(unsigned char),
len_one),sizeof(unsigned char),3),(struct _tagged_arr)_tagged_arr_plus(
_tagged_arr_plus(s,sizeof(unsigned char),len),sizeof(unsigned char),- len_two),(
unsigned int)len_two);return ans;}}static int Cyc_Position_line_length=76;struct
_tuple3{struct _tagged_arr f1;int f2;int f3;};static struct _tuple3*Cyc_Position_get_context(
struct Cyc_Position_Segment*seg){if(seg == 0){(int)_throw((void*)Cyc_Position_Nocontext);}{
struct Cyc_Lineno_Pos*pos_s;struct Cyc_Lineno_Pos*pos_e;{struct _handler_cons _tmp25;
_push_handler(& _tmp25);{int _tmp27=0;if(setjmp(_tmp25.handler)){_tmp27=1;}if(!
_tmp27){pos_s=Cyc_Lineno_pos_of_abs(Cyc_Position_source,seg->start);pos_e=Cyc_Lineno_pos_of_abs(
Cyc_Position_source,seg->end);;_pop_handler();}else{void*_tmp26=(void*)
_exn_thrown;void*_tmp29=_tmp26;_LL43: goto _LL44;_LL45: goto _LL46;_LL44:(int)_throw((
void*)Cyc_Position_Nocontext);_LL46:(void)_throw(_tmp29);_LL42:;}}}{struct Cyc_Lineno_Pos
_tmp31;int _tmp32;int _tmp34;struct _tagged_arr _tmp36;struct Cyc_Lineno_Pos*_tmp2F=
pos_s;_tmp31=*_tmp2F;_LL55: _tmp36=_tmp31.line;goto _LL53;_LL53: _tmp34=_tmp31.line_no;
goto _LL51;_LL51: _tmp32=_tmp31.col;goto _LL48;_LL48: {struct Cyc_Lineno_Pos _tmp3A;
int _tmp3B;int _tmp3D;struct _tagged_arr _tmp3F;struct Cyc_Lineno_Pos*_tmp38=pos_e;
_tmp3A=*_tmp38;_LL64: _tmp3F=_tmp3A.line;goto _LL62;_LL62: _tmp3D=_tmp3A.line_no;
goto _LL60;_LL60: _tmp3B=_tmp3A.col;goto _LL57;_LL57: if(_tmp34 == _tmp3D){int n=Cyc_Position_line_length
/ 3;struct _tagged_arr sec_one=Cyc_Position_trunc(n,Cyc_Std_substring((struct
_tagged_arr)_tmp36,0,(unsigned int)_tmp32));struct _tagged_arr sec_two=Cyc_Position_trunc(
n,Cyc_Std_substring((struct _tagged_arr)_tmp36,_tmp32,(unsigned int)(_tmp3B - 
_tmp32)));struct _tagged_arr sec_three=Cyc_Position_trunc(n,Cyc_Std_substring((
struct _tagged_arr)_tmp36,_tmp32,Cyc_Std_strlen((struct _tagged_arr)_tmp36)- 
_tmp3B));return({struct _tuple3*_tmp41=_cycalloc(sizeof(struct _tuple3));_tmp41->f1=({
struct Cyc_Std_String_pa_struct _tmp45;_tmp45.tag=Cyc_Std_String_pa;_tmp45.f1=(
struct _tagged_arr)sec_three;{struct Cyc_Std_String_pa_struct _tmp44;_tmp44.tag=Cyc_Std_String_pa;
_tmp44.f1=(struct _tagged_arr)sec_two;{struct Cyc_Std_String_pa_struct _tmp43;
_tmp43.tag=Cyc_Std_String_pa;_tmp43.f1=(struct _tagged_arr)sec_one;{void*_tmp42[3]={&
_tmp43,& _tmp44,& _tmp45};Cyc_Std_aprintf(_tag_arr("%s%s%s",sizeof(unsigned char),
7),_tag_arr(_tmp42,sizeof(void*),3));}}}});_tmp41->f2=(int)Cyc_Std_strlen((
struct _tagged_arr)sec_one);_tmp41->f3=(int)(Cyc_Std_strlen((struct _tagged_arr)
sec_one)+ Cyc_Std_strlen((struct _tagged_arr)sec_two));_tmp41;});}else{int n=(Cyc_Position_line_length
- 3)/ 4;struct _tagged_arr sec_one=Cyc_Position_trunc(n,Cyc_Std_substring((struct
_tagged_arr)_tmp36,0,(unsigned int)_tmp32));struct _tagged_arr sec_two=Cyc_Position_trunc(
n,Cyc_Std_substring((struct _tagged_arr)_tmp36,_tmp32,Cyc_Std_strlen((struct
_tagged_arr)_tmp36)- _tmp32));struct _tagged_arr sec_three=Cyc_Position_trunc(n,
Cyc_Std_substring((struct _tagged_arr)_tmp3F,0,(unsigned int)_tmp3B));struct
_tagged_arr sec_four=Cyc_Position_trunc(n,Cyc_Std_substring((struct _tagged_arr)
_tmp3F,_tmp3B,Cyc_Std_strlen((struct _tagged_arr)_tmp3F)- _tmp3B));return({struct
_tuple3*_tmp46=_cycalloc(sizeof(struct _tuple3));_tmp46->f1=({struct Cyc_Std_String_pa_struct
_tmp4B;_tmp4B.tag=Cyc_Std_String_pa;_tmp4B.f1=(struct _tagged_arr)sec_four;{
struct Cyc_Std_String_pa_struct _tmp4A;_tmp4A.tag=Cyc_Std_String_pa;_tmp4A.f1=(
struct _tagged_arr)sec_three;{struct Cyc_Std_String_pa_struct _tmp49;_tmp49.tag=Cyc_Std_String_pa;
_tmp49.f1=(struct _tagged_arr)sec_two;{struct Cyc_Std_String_pa_struct _tmp48;
_tmp48.tag=Cyc_Std_String_pa;_tmp48.f1=(struct _tagged_arr)sec_one;{void*_tmp47[4]={&
_tmp48,& _tmp49,& _tmp4A,& _tmp4B};Cyc_Std_aprintf(_tag_arr("%s%s.\\.%s%s",sizeof(
unsigned char),12),_tag_arr(_tmp47,sizeof(void*),4));}}}}});_tmp46->f2=(int)Cyc_Std_strlen((
struct _tagged_arr)sec_one);_tmp46->f3=(int)(((Cyc_Std_strlen((struct _tagged_arr)
sec_one)+ Cyc_Std_strlen((struct _tagged_arr)sec_two))+ 3)+ Cyc_Std_strlen((
struct _tagged_arr)sec_three));_tmp46;});}}}}}static int Cyc_Position_error_b=0;int
Cyc_Position_error_p(){return Cyc_Position_error_b;}unsigned char Cyc_Position_Error[
10]="\000\000\000\000Error";struct Cyc_Position_Error_struct{unsigned char*tag;
struct Cyc_Position_Error*f1;};int Cyc_Position_print_context=0;int Cyc_Position_first_error=
1;int Cyc_Position_num_errors=0;int Cyc_Position_max_errors=10;void Cyc_Position_post_error(
struct Cyc_Position_Error*e){Cyc_Position_error_b=1;Cyc_Std_fflush((struct Cyc_Std___sFILE*)
Cyc_Std_stdout);if(Cyc_Position_first_error){({void*_tmp4C[0]={};Cyc_Std_fprintf(
Cyc_Std_stderr,_tag_arr("\n",sizeof(unsigned char),2),_tag_arr(_tmp4C,sizeof(
void*),0));});Cyc_Position_first_error=0;}if(Cyc_Position_num_errors <= Cyc_Position_max_errors){({
struct Cyc_Std_String_pa_struct _tmp4F;_tmp4F.tag=Cyc_Std_String_pa;_tmp4F.f1=(
struct _tagged_arr)e->desc;{struct Cyc_Std_String_pa_struct _tmp4E;_tmp4E.tag=Cyc_Std_String_pa;
_tmp4E.f1=(struct _tagged_arr)Cyc_Position_string_of_segment(e->seg);{void*_tmp4D[
2]={& _tmp4E,& _tmp4F};Cyc_Std_fprintf(Cyc_Std_stderr,_tag_arr("%s: %s\n",sizeof(
unsigned char),8),_tag_arr(_tmp4D,sizeof(void*),2));}}});if(Cyc_Position_print_context){
struct _handler_cons _tmp50;_push_handler(& _tmp50);{int _tmp52=0;if(setjmp(_tmp50.handler)){
_tmp52=1;}if(! _tmp52){{struct _tuple3*x=Cyc_Position_get_context(e->seg);struct
_tagged_arr marker_str=({unsigned int _tmp56=(unsigned int)((*x).f3 + 1);
unsigned char*_tmp57=(unsigned char*)_cycalloc_atomic(_check_times(sizeof(
unsigned char),_tmp56));struct _tagged_arr _tmp59=_tag_arr(_tmp57,sizeof(
unsigned char),(unsigned int)((*x).f3 + 1));{unsigned int _tmp58=_tmp56;
unsigned int i;for(i=0;i < _tmp58;i ++){_tmp57[i]='\000';}};_tmp59;});int i=- 1;
while(++ i < (*x).f2){((unsigned char*)marker_str.curr)[i]=' ';}while(++ i < (*x).f3){((
unsigned char*)marker_str.curr)[i]='^';}({struct Cyc_Std_String_pa_struct _tmp55;
_tmp55.tag=Cyc_Std_String_pa;_tmp55.f1=(struct _tagged_arr)marker_str;{struct Cyc_Std_String_pa_struct
_tmp54;_tmp54.tag=Cyc_Std_String_pa;_tmp54.f1=(struct _tagged_arr)(*x).f1;{void*
_tmp53[2]={& _tmp54,& _tmp55};Cyc_Std_fprintf(Cyc_Std_stderr,_tag_arr("  %s\n  %s\n",
sizeof(unsigned char),11),_tag_arr(_tmp53,sizeof(void*),2));}}});};_pop_handler();}
else{void*_tmp51=(void*)_exn_thrown;void*_tmp5B=_tmp51;_LL93: if(_tmp5B == Cyc_Position_Nocontext){
goto _LL94;}else{goto _LL95;}_LL95: goto _LL96;_LL94: goto _LL92;_LL96:(void)_throw(
_tmp5B);_LL92:;}}}}if(Cyc_Position_num_errors == Cyc_Position_max_errors){({void*
_tmp61[0]={};Cyc_Std_fprintf(Cyc_Std_stderr,_tag_arr("Too many error messages!\n",
sizeof(unsigned char),26),_tag_arr(_tmp61,sizeof(void*),0));});}Cyc_Std_fflush((
struct Cyc_Std___sFILE*)Cyc_Std_stderr);Cyc_Position_num_errors ++;}void Cyc_Position_reset_position(
struct _tagged_arr s){Cyc_Position_source=s;Cyc_Position_error_b=0;}void Cyc_Position_set_position_file(
struct _tagged_arr s){Cyc_Position_source=s;Cyc_Position_error_b=0;}struct
_tagged_arr Cyc_Position_get_position_file(){return Cyc_Position_source;}struct
_tagged_arr Cyc_Position_get_line_directive(struct Cyc_Position_Segment*s){struct
Cyc_Lineno_Pos*pos_s=Cyc_Lineno_pos_of_abs(Cyc_Position_source,((struct Cyc_Position_Segment*)
_check_null(s))->start);if((struct Cyc_Lineno_Pos*)pos_s != 0){return(struct
_tagged_arr)({struct Cyc_Std_String_pa_struct _tmp64;_tmp64.tag=Cyc_Std_String_pa;
_tmp64.f1=(struct _tagged_arr)pos_s->logical_file;{struct Cyc_Std_Int_pa_struct
_tmp63;_tmp63.tag=Cyc_Std_Int_pa;_tmp63.f1=(int)((unsigned int)pos_s->line_no);{
void*_tmp62[2]={& _tmp63,& _tmp64};Cyc_Std_aprintf(_tag_arr("\n#line %d \"%s\"\n",
sizeof(unsigned char),16),_tag_arr(_tmp62,sizeof(void*),2));}}});}else{return(
struct _tagged_arr)_tag_arr(0,0,0);}}extern struct Cyc_Typerep_Tuple_struct Cyc_struct_Position_Segment_rep;
static struct Cyc_Typerep_Int_struct Cyc__genrep_0={1,32};static struct _tuple0 Cyc__gentuple_1={
offsetof(struct Cyc_Position_Segment,start),(void*)& Cyc__genrep_0};static struct
_tuple0 Cyc__gentuple_2={offsetof(struct Cyc_Position_Segment,end),(void*)& Cyc__genrep_0};
static struct _tuple0*Cyc__genarr_3[2]={& Cyc__gentuple_1,& Cyc__gentuple_2};struct
Cyc_Typerep_Tuple_struct Cyc_struct_Position_Segment_rep={4,sizeof(struct Cyc_Position_Segment),{(
void*)((struct _tuple0**)Cyc__genarr_3),(void*)((struct _tuple0**)Cyc__genarr_3),(
void*)((struct _tuple0**)Cyc__genarr_3 + 2)}};void*Cyc_segment_rep=(void*)& Cyc_struct_Position_Segment_rep;
