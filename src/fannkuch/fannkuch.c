 // INCLUDES / DEFINES / ENUMS
        //#define CEU_DEBUG
        #include <stdio.h>
        #include <stdlib.h>
        #include <stddef.h>
        #include <stdint.h>
        #include <string.h>
        #include <assert.h>
        #include <stdarg.h>
        #include <time.h>
        #include <math.h>

        #undef MAX
        #undef MIN
        #define MAX(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
        #define MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

        typedef enum CEU_HOLD {
            CEU_HOLD_FLEET = 0,     // not assigned, dst assigns
            CEU_HOLD_MUTAB,         // set and assignable to narrow 
            CEU_HOLD_IMMUT,         // set but not assignable (nested fun)
            CEU_HOLD_MAX
        } __attribute__ ((__packed__)) CEU_HOLD;
        _Static_assert(sizeof(CEU_HOLD) == 1);
       // CEU_Value, CEU_Dyn
        union CEU_Dyn;
        struct CEU_Block;
        struct CEU_Any;
        struct CEU_Tags_List;

        typedef enum CEU_VALUE {
            CEU_VALUE_NIL = 0,
            CEU_VALUE_ERROR,
            CEU_VALUE_TAG,
            CEU_VALUE_BOOL,
            CEU_VALUE_CHAR,
            CEU_VALUE_NUMBER,
            CEU_VALUE_POINTER,
            CEU_VALUE_DYNAMIC,    // all below are dynamic
            CEU_VALUE_CLOSURE,
            CEU_VALUE_TUPLE,
            CEU_VALUE_VECTOR,
            CEU_VALUE_DICT
        } __attribute__ ((__packed__)) CEU_VALUE;
        _Static_assert(sizeof(CEU_VALUE) == 1);
        
        typedef struct CEU_Value {
            CEU_VALUE type;
            union {
                //void nil;
                char* Error;
                unsigned int Tag;
                int Bool;
                char Char;
                double Number;
                void* Pointer;
                union CEU_Dyn* Dyn;    // Func/Task/Tuple/Dict/Coro/Tasks: allocates memory
            };
        } CEU_Value;

        #define _CEU_Dyn_                   \
            CEU_VALUE type;                 \
            uint8_t refs;                   \
            CEU_HOLD hld_type;              \
            CEU_HOLD hld_depth;             \
            union CEU_Dyn** hld_prev;       \
            union CEU_Dyn* hld_next;        \
            struct CEU_Tags_List* tags;
            
        typedef struct CEU_Any {
            _CEU_Dyn_
        } CEU_Any;

        typedef struct CEU_Tuple {
            _CEU_Dyn_
            int its;                // number of items
            CEU_Value buf[0];       // beginning of CEU_Value[n]
        } CEU_Tuple;

        typedef struct CEU_Vector {
            _CEU_Dyn_
            CEU_VALUE unit;         // type of each element
            int max;                // size of buf
            int its;                // number of items
            char* buf;              // resizable Unknown[n]
        } CEU_Vector;
        
        typedef struct CEU_Dict {
            _CEU_Dyn_
            int max;                // size of buf
            CEU_Value (*buf)[0][2]; // resizable CEU_Value[n][2]
        } CEU_Dict;
        
        struct CEU_Frame;
        typedef CEU_Value (*CEU_Proto) (
            struct CEU_Frame* frame,
            int n,
            struct CEU_Value args[]
        );

        typedef struct CEU_Closure {  // lexical func/task
            _CEU_Dyn_
            struct CEU_Frame* up_frame;   // points to active frame
            CEU_Proto proto;
            struct {
                int its;     // number of upvals
                CEU_Value* buf;
            } upvs;
        } CEU_Closure;
        
        typedef union CEU_Dyn {                                                                 
            struct CEU_Any     Any;
            struct CEU_Tuple   Tuple;
            struct CEU_Vector  Vector;
            struct CEU_Dict    Dict;
            struct CEU_Closure Closure;
        } CEU_Dyn;        
     // CEU_Frame, CEU_Block
        typedef struct CEU_Frame {          // call func / create task
            struct CEU_Closure* closure;
            struct CEU_Block* up_block;     // block enclosing this call/coroutine
        } CEU_Frame;

        typedef struct CEU_Block {
            uint16_t depth;
            uint8_t  istop;
            union {
                struct CEU_Frame* frame;    // istop = 1
                struct CEU_Block* block;    // istop = 0
            } up;
            union CEU_Dyn* dyns;            // list of allocated data to bcast/free
        } CEU_Block;
     // CEU_Tags
        typedef struct CEU_Tags_Names {
            int tag;
            char* name;
            struct CEU_Tags_Names* next;
        } CEU_Tags_Names;
        
        typedef struct CEU_Tags_List {
            int tag;
            struct CEU_Tags_List* next;
        } CEU_Tags_List;
     // PROTOS
        CEU_Value ceu_type_f (CEU_Frame* _1, int n, CEU_Value args[]);
        int ceu_as_bool (CEU_Value v);
        
        CEU_Value ceu_tags_f (CEU_Frame* _1, int n, CEU_Value args[]);
        char* ceu_tag_to_string (int tag);
        int ceu_tag_to_size (int type);
                
        void ceu_dyn_free (CEU_Dyn* dyn);
        void ceu_block_free (CEU_Block* blk);
        
        void ceu_gc_inc (CEU_Value v);
        void ceu_gc_dec (CEU_Value v, int chk);
        void ceu_gc_chk_args (int n, CEU_Value args[]);

        void ceu_hold_add (CEU_Dyn* dyn, CEU_Dyn** blk);
        void ceu_hold_rem (CEU_Dyn* dyn);

        int ceu_hold_set (CEU_Dyn** dst, int depth, CEU_HOLD tphold, CEU_Dyn* src);
        
        CEU_Value ceu_tuple_create   (CEU_Block* hld, int n);
        CEU_Value ceu_vector_create  (CEU_Block* hld);
        CEU_Value ceu_dict_create    (CEU_Block* hld);
        CEU_Value ceu_closure_create (CEU_Block* hld, CEU_HOLD tphold, CEU_Frame* frame, CEU_Proto proto, int upvs);

        CEU_Value ceu_tuple_set (CEU_Tuple* tup, int i, CEU_Value v);

        CEU_Value ceu_vector_get (CEU_Vector* vec, int i);
        CEU_Value ceu_vector_set (CEU_Vector* vec, int i, CEU_Value v);
        CEU_Value ceu_vector_from_c_string (CEU_Block* hld, const char* str);
        
        int ceu_dict_key_to_index (CEU_Dict* col, CEU_Value key, int* idx);
        CEU_Value ceu_dict_get (CEU_Dict* col, CEU_Value key);
        CEU_Value ceu_dict_set (CEU_Dict* col, CEU_Value key, CEU_Value val);
        
        CEU_Value ceu_col_check (CEU_Value col, CEU_Value idx);

        void ceu_print1 (CEU_Frame* _1, CEU_Value v);
        CEU_Value _ceu_op_equals_equals_f_ (CEU_Frame* _1, int n, CEU_Value args[]);
     // GLOBALS
        int ceu_gc_count = 0;
        
        
                #define CEU_TAG_nil (0)
                CEU_Tags_Names ceu_tag_nil = { CEU_TAG_nil, ":nil", NULL };
                
                #define CEU_TAG_error (1)
                CEU_Tags_Names ceu_tag_error = { CEU_TAG_error, ":error", &ceu_tag_nil };
                
                #define CEU_TAG_tag (2)
                CEU_Tags_Names ceu_tag_tag = { CEU_TAG_tag, ":tag", &ceu_tag_error };
                
                #define CEU_TAG_bool (3)
                CEU_Tags_Names ceu_tag_bool = { CEU_TAG_bool, ":bool", &ceu_tag_tag };
                
                #define CEU_TAG_char (4)
                CEU_Tags_Names ceu_tag_char = { CEU_TAG_char, ":char", &ceu_tag_bool };
                
                #define CEU_TAG_number (5)
                CEU_Tags_Names ceu_tag_number = { CEU_TAG_number, ":number", &ceu_tag_char };
                
                #define CEU_TAG_pointer (6)
                CEU_Tags_Names ceu_tag_pointer = { CEU_TAG_pointer, ":pointer", &ceu_tag_number };
                
                #define CEU_TAG_dynamic (7)
                CEU_Tags_Names ceu_tag_dynamic = { CEU_TAG_dynamic, ":dynamic", &ceu_tag_pointer };
                
                #define CEU_TAG_func (8)
                CEU_Tags_Names ceu_tag_func = { CEU_TAG_func, ":func", &ceu_tag_dynamic };
                
                #define CEU_TAG_tuple (9)
                CEU_Tags_Names ceu_tag_tuple = { CEU_TAG_tuple, ":tuple", &ceu_tag_func };
                
                #define CEU_TAG_vector (10)
                CEU_Tags_Names ceu_tag_vector = { CEU_TAG_vector, ":vector", &ceu_tag_tuple };
                
                #define CEU_TAG_dict (11)
                CEU_Tags_Names ceu_tag_dict = { CEU_TAG_dict, ":dict", &ceu_tag_vector };
                
                #define CEU_TAG_ceu (12)
                CEU_Tags_Names ceu_tag_ceu = { CEU_TAG_ceu, ":ceu", &ceu_tag_dict };
                
                #define CEU_TAG_tmp (13)
                CEU_Tags_Names ceu_tag_tmp = { CEU_TAG_tmp, ":tmp", &ceu_tag_ceu };
                
                #define CEU_TAG_string (14)
                CEU_Tags_Names ceu_tag_string = { CEU_TAG_string, ":string", &ceu_tag_tmp };
                
                CEU_Tags_Names* CEU_TAGS = &ceu_tag_string;
            
     // IMPLS
        void ceu_exit (CEU_Block* blk) {
            if (blk == NULL) {
                exit(0);
            }
            CEU_Block* up = (blk->istop) ? blk->up.frame->up_block : blk->up.block;
            ceu_block_free(blk);
            return ceu_exit(up);
        }
        void ceu_ferror (CEU_Block* blk, CEU_Value err) {
            fprintf(stderr, "%s\n", err.Error);
            ceu_exit(blk);
        }
        void ceu_ferror_pre (CEU_Block* blk, char* pre, CEU_Value err) {
            fprintf(stderr, "%s : %s\n", pre, err.Error);
            ceu_exit(blk);
        }
        CEU_Value ceu_assert (CEU_Block* blk, CEU_Value v) {
            if (v.type == CEU_VALUE_ERROR) {
                ceu_ferror(blk, v);
            }
            return v;
        }
        CEU_Value ceu_assert_pre (CEU_Block* blk, CEU_Value v, char* pre) {
            if (v.type == CEU_VALUE_ERROR) {
                ceu_ferror_pre(blk, pre, v);
            }
            return v;
        }
        CEU_Value ceu_error_f (CEU_Frame* _1, int n, CEU_Value args[]) {
            assert(n==1 && args[0].type==CEU_VALUE_TAG);
            return (CEU_Value) { CEU_VALUE_ERROR, {.Error=ceu_tag_to_string(args[0].Tag)} };
        }

        CEU_Value ceu_dyn_to_val (CEU_Dyn* dyn) {
            return (CEU_Value) { dyn->Any.type, {.Dyn=dyn} };
        }
        
        void _ceu_dump_ (CEU_Value v) {
            puts(">>>>>>>>>>>");
            ceu_print1(NULL, v);
            puts(" <<<");
            if (v.type > CEU_VALUE_DYNAMIC) {
                printf("    dyn   = %p\n", v.Dyn);
                printf("    refs  = %d\n", v.Dyn->Any.refs);
                printf("    hold  = %d\n", v.Dyn->Any.hld_type);
                printf("    depth = %d\n", v.Dyn->Any.hld_depth);
                printf("    prev  = %p\n", v.Dyn->Any.hld_prev);
                printf("    &next = %p\n", &v.Dyn->Any.hld_next);
                printf("    next  = %p\n", v.Dyn->Any.hld_next);
            }
            puts("<<<<<<<<<<<");
        }
        CEU_Value ceu_dump_f (CEU_Frame* _1, int n, CEU_Value args[]) {
            assert(n == 1);
            _ceu_dump_(args[0]);
            ceu_gc_chk_args(n, args);
            return (CEU_Value) { CEU_VALUE_NIL };
        }

        int ceu_as_bool (CEU_Value v) {
            return !(v.type==CEU_VALUE_NIL || (v.type==CEU_VALUE_BOOL && !v.Bool));
        }
        CEU_Value ceu_type_f (CEU_Frame* _1, int n, CEU_Value args[]) {
            assert(n == 1 && "bug found");
            ceu_gc_chk_args(n, args);
            return (CEU_Value) { CEU_VALUE_TAG, {.Tag=args[0].type} };
        }
        CEU_Value ceu_sup_question__f (CEU_Frame* _1, int n, CEU_Value args[]) {
            assert(n >= 2);
            CEU_Value sup = args[0];
            CEU_Value sub = args[1];
            assert(sup.type == CEU_VALUE_TAG);
            assert(sub.type == CEU_VALUE_TAG);
            
            //printf("sup=0x%08X vs sub=0x%08X\n", sup->Tag, sub->Tag);
            int sup0 = sup.Tag & 0x000000FF;
            int sup1 = sup.Tag & 0x0000FF00;
            int sup2 = sup.Tag & 0x00FF0000;
            int sup3 = sup.Tag & 0xFF000000;
            int sub0 = sub.Tag & 0x000000FF;
            int sub1 = sub.Tag & 0x0000FF00;
            int sub2 = sub.Tag & 0x00FF0000;
            int sub3 = sub.Tag & 0xFF000000;
            
            ceu_gc_chk_args(n, args);

            return (CEU_Value) { CEU_VALUE_BOOL, { .Bool =
                (sup0 == sub0) && ((sup1 == 0) || (
                    (sup1 == sub1) && ((sup2 == 0) || (
                        (sup2 == sub2) && ((sup3 == 0) || (
                            (sup3 == sub3)
                        ))
                    ))
                ))
            } };
        }
        CEU_Value ceu_tags_f (CEU_Frame* frame, int n, CEU_Value args[]) {
            assert(n >= 1);
            CEU_Value dyn = args[0];
            CEU_Tags_List* tags = (dyn.type < CEU_VALUE_DYNAMIC) ? NULL : dyn.Dyn->Any.tags;
            CEU_Value tag; // = (CEU_Value) { CEU_VALUE_NIL };
            if (n >= 2) {
                tag = args[1];
                assert(tag.type == CEU_VALUE_TAG);
            }
            switch (n) {
                case 1: {
                    int len = 0; {
                        CEU_Tags_List* cur = tags;
                        while (cur != NULL) {
                            len++;
                            cur = cur->next;
                        }
                    }
                    CEU_Value tup = ceu_tuple_create(frame->up_block, len);
                    {
                        CEU_Tags_List* cur = tags;
                        int i = 0;
                        while (cur != NULL) {
                            assert(ceu_tuple_set(&tup.Dyn->Tuple, i++, (CEU_Value) { CEU_VALUE_TAG, {.Tag=cur->tag} }).type != CEU_VALUE_ERROR);
                            cur = cur->next;
                        }
                    }                    
                    return tup;
                }
                case 2: {   // check
                    CEU_Value ret = (CEU_Value) { CEU_VALUE_BOOL, {.Bool=0} };
                    CEU_Tags_List* cur = tags;
                    while (cur != NULL) {
                        CEU_Value args[] = {
                            tag,
                            (CEU_Value) { CEU_VALUE_TAG, {.Tag=cur->tag} }
                        };
                        ret = ceu_assert(frame->up_block, ceu_sup_question__f(frame, 2, args));
                        if (ret.Bool) {
                            break;
                        }
                        cur = cur->next;
                    }
                    return ret;
                }
                case 3: {   // add/rem
                    assert(dyn.type > CEU_VALUE_DYNAMIC);
                    CEU_Value bool = args[2];
                    assert(bool.type == CEU_VALUE_BOOL);
                    if (bool.Bool) {   // add
                        CEU_Value chk = ceu_tags_f(frame, 2, args);
                        if (chk.Bool) {
                            return (CEU_Value) { CEU_VALUE_NIL };
                        } else {
                            CEU_Tags_List* v = malloc(sizeof(CEU_Tags_List));
                            assert(v != NULL);
                            v->tag = tag.Tag;
                            v->next = dyn.Dyn->Any.tags;
                            dyn.Dyn->Any.tags = v;
                            return dyn;
                        }
                    } else {            // rem
                        CEU_Value ret = (CEU_Value) { CEU_VALUE_NIL };
                        CEU_Tags_List** cur = &dyn.Dyn->Any.tags;
                        while (*cur != NULL) {
                            if ((*cur)->tag == tag.Tag) {
                                CEU_Tags_List* v = *cur;
                                *cur = v->next;
                                free(v);
                                ret = dyn;
                                break;
                            }
                            cur = &(*cur)->next;
                        }
                        return ret;
                    }
                }
            }
        }
        char* ceu_tag_to_string (int tag) {
            CEU_Tags_Names* cur = CEU_TAGS;
            while (cur != NULL) {
                if (cur->tag == tag) {
                    return cur->name;
                }
                cur = cur->next;
            }
            assert(0 && "bug found");
        }
        CEU_Value ceu_string_dash_to_dash_tag_f (CEU_Frame* _1, int n, CEU_Value args[]) {
            assert(n == 1);
            CEU_Value str = args[0];
            assert(str.type==CEU_VALUE_VECTOR && str.Dyn->Vector.unit==CEU_VALUE_CHAR);
            CEU_Tags_Names* cur = CEU_TAGS;
            CEU_Value ret = (CEU_Value) { CEU_VALUE_NIL };
            while (cur != NULL) {
                if (!strcmp(cur->name,str.Dyn->Vector.buf)) {
                    ret = (CEU_Value) { CEU_VALUE_TAG, {.Tag=cur->tag} };
                    break;
                }
                cur = cur->next;
            }
            ceu_gc_chk_args(n, args);
            return ret;
        }
     // GC
        void ceu_gc_free (CEU_Dyn* dyn) {
            switch (dyn->Any.type) {
                case CEU_VALUE_CLOSURE:
                    for (int i=0; i<dyn->Closure.upvs.its; i++) {
                        ceu_gc_dec(dyn->Closure.upvs.buf[i], 1);
                    }
                    break;
                case CEU_VALUE_TUPLE:
                    for (int i=0; i<dyn->Tuple.its; i++) {
                        ceu_gc_dec(dyn->Tuple.buf[i], 1);
                    }
                    break;
                case CEU_VALUE_VECTOR:
                    for (int i=0; i<dyn->Vector.its; i++) {
                        CEU_Value ret = ceu_vector_get(&dyn->Vector, i);
                        assert(ret.type != CEU_VALUE_ERROR);
                        ceu_gc_dec(ret, 1);
                    }
                    break;
                case CEU_VALUE_DICT:
                    for (int i=0; i<dyn->Dict.max; i++) {
                        ceu_gc_dec((*dyn->Dict.buf)[i][0], 1);
                        ceu_gc_dec((*dyn->Dict.buf)[i][1], 1);
                    }
                    break;
                default:
                    assert(0);
                    break;
            }
            ceu_gc_count++;
            ceu_hold_rem(dyn);
            ceu_dyn_free(dyn);
        }
        
        void ceu_gc_chk (CEU_Dyn* dyn) {
            assert(dyn->Any.type > CEU_VALUE_DYNAMIC);
            if (dyn->Any.refs == 0) {
                ceu_gc_free(dyn);
            }
        }

        // var x = ?        // var source
        // set x = ?        // set source
        // set x[...] = ?   //      - index value
        // set x[?] = ?     //      - index key
        // set x.pub = ?    //      - pub
        // [...?...]        // constructor argument     // TODO
        // f(?)             // call argument
        // closure
        
        void ceu_gc_inc (CEU_Value new) {
            if (new.type > CEU_VALUE_DYNAMIC) {
                new.Dyn->Any.refs++;
            }
        }
        void ceu_gc_dec (CEU_Value old, int chk) {
            if (old.type > CEU_VALUE_DYNAMIC) {
                old.Dyn->Any.refs--;
                if (chk) {
                    ceu_gc_chk(old.Dyn);
                }
            }
        }
        void ceu_gc_chk_args (int n, CEU_Value args[]) {
            for (int i=0; i<n; i++) {
                if (args[i].type > CEU_VALUE_DYNAMIC) {
                    ceu_gc_chk(args[i].Dyn);
                }
            }
        }
        void ceu_gc_inc_args (int n, CEU_Value args[]) {
            for (int i=0; i<n; i++) {
                ceu_gc_inc(args[i]);
            }
        }
     // BLOCK
        void ceu_dyn_free (CEU_Dyn* dyn) {
            while (dyn->Any.tags != NULL) {
                CEU_Tags_List* tag = dyn->Any.tags;
                dyn->Any.tags = tag->next;
                free(tag);
            }
            switch (dyn->Any.type) {
                case CEU_VALUE_CLOSURE:
                    free(dyn->Closure.upvs.buf);
                    break;
                case CEU_VALUE_TUPLE:       // buf w/ dyn
                    break;
                case CEU_VALUE_VECTOR:
                    free(dyn->Vector.buf);
                    break;
                case CEU_VALUE_DICT:
                    free(dyn->Dict.buf);
                    break;
                default:
                    assert(0 && "bug found");
            }
            free(dyn);
        }
        
        void ceu_block_free (CEU_Block* blk) {
            CEU_Dyn* cur = blk->dyns;
            while (cur != NULL) {
                CEU_Dyn* old = cur;
                cur = old->Any.hld_next;
                ceu_dyn_free((CEU_Dyn*)old);
            }
            blk->dyns = NULL;
        }
     // HOLD
        void ceu_hold_add (CEU_Dyn* dyn, CEU_Dyn** nxt) {
            dyn->Any.hld_prev = nxt;
            dyn->Any.hld_next = *nxt;
            if (*nxt != NULL) {
                (*nxt)->Any.hld_prev = &dyn->Any.hld_next;
            }
            *nxt = dyn;
        }
        void ceu_hold_rem (CEU_Dyn* dyn) {
            *(dyn->Any.hld_prev) = dyn->Any.hld_next;
            if (dyn->Any.hld_next != NULL) {
                dyn->Any.hld_next->Any.hld_prev = dyn->Any.hld_prev;
            }
            dyn->Any.hld_prev = NULL;
            dyn->Any.hld_next = NULL;
        }
        void ceu_hold_chg (CEU_Dyn* dyn, CEU_Dyn** nxt, int depth) {
            dyn->Any.hld_depth = depth;
            ceu_hold_rem(dyn);
            ceu_hold_add(dyn, nxt);
        }

        CEU_Value ceu_hold_chk_set (CEU_Dyn** dst, int depth, CEU_HOLD type, CEU_Value src, int nest, char* pre) {
            static char msg[256];
            if (src.type < CEU_VALUE_DYNAMIC) {
                return (CEU_Value) { CEU_VALUE_NIL };
            } else if (src.Dyn->Any.hld_type == CEU_HOLD_FLEET) {
                if (src.Dyn->Any.refs-nest>0 && depth>src.Dyn->Any.hld_depth) {
                    strncpy(msg, pre, 256);
                    strcat(msg, " : cannot move to deeper scope with pending references");
                    return (CEU_Value) { CEU_VALUE_ERROR, {.Error=msg} };
                } else {
                    // continue below
                }
            } else if (depth >= src.Dyn->Any.hld_depth) {
                return (CEU_Value) { CEU_VALUE_NIL };
            } else {
                strncpy(msg, pre, 256);
                strcat(msg, " : cannot copy reference to outer scope");
                return (CEU_Value) { CEU_VALUE_ERROR, {.Error=msg} };
            };

            int src_depth = src.Dyn->Any.hld_depth;
            int src_type  = src.Dyn->Any.hld_type;

            src.Dyn->Any.hld_type = MAX(src.Dyn->Any.hld_type,type);
            if (depth != src.Dyn->Any.hld_depth) {
                ceu_hold_chg(src.Dyn, dst, depth);
            }
            if (src.Dyn->Any.hld_type==src_type && depth>=src_depth) {
                return (CEU_Value) { CEU_VALUE_NIL };
            }
            
            #define CEU_CHECK_ERROR_RETURN(v) { CEU_Value ret=v; if (ret.type==CEU_VALUE_ERROR) { return ret; } }

            switch (src.Dyn->Any.type) {
                case CEU_VALUE_CLOSURE:
                    for (int i=0; i<src.Dyn->Closure.upvs.its; i++) {
                        CEU_CHECK_ERROR_RETURN(ceu_hold_chk_set(dst, depth, type, src.Dyn->Closure.upvs.buf[i], 1, pre));
                    }
                    break;
                case CEU_VALUE_TUPLE:
                    for (int i=0; i<src.Dyn->Tuple.its; i++) {
                        CEU_CHECK_ERROR_RETURN(ceu_hold_chk_set(dst, depth, type, src.Dyn->Tuple.buf[i], 1, pre));
                    }
                    break;
                case CEU_VALUE_VECTOR:
                    for (int i=0; i<src.Dyn->Vector.its; i++) {
                        CEU_CHECK_ERROR_RETURN(ceu_hold_chk_set(dst, depth, type, ceu_vector_get(&src.Dyn->Vector,i), 1, pre));
                    }
                    break;
                case CEU_VALUE_DICT:
                    for (int i=0; i<src.Dyn->Dict.max; i++) {
                        CEU_CHECK_ERROR_RETURN(ceu_hold_chk_set(dst, depth, type, (*src.Dyn->Dict.buf)[i][0], 1, pre));
                        CEU_CHECK_ERROR_RETURN(ceu_hold_chk_set(dst, depth, type, (*src.Dyn->Dict.buf)[i][1], 1, pre));
                    }
                    break;
            }
            return (CEU_Value) { CEU_VALUE_NIL };
        }
        
        CEU_Value ceu_hold_chk_set_col (CEU_Dyn* col, CEU_Value v) {
            if (v.type < CEU_VALUE_DYNAMIC) {
                return (CEU_Value) { CEU_VALUE_NIL };
            }
            
            // col affects v:
            // [x,[1]] <-- moves v=[1] to v
            CEU_Value err = ceu_hold_chk_set(&col->Any.hld_next, col->Any.hld_depth, col->Any.hld_type, v, 0, "set error");
            if (err.type==CEU_VALUE_ERROR && col->Any.hld_type!=CEU_HOLD_FLEET) {
                // must be second b/c chk_set above may modify v
                return err;
            }
                     
            // v affects fleeting col with innermost scope
            if (col->Any.hld_type == CEU_HOLD_FLEET) {
                if (v.Dyn->Any.hld_depth < col->Any.hld_depth) {
                    return (CEU_Value) { CEU_VALUE_NIL };
                } else {
                    col->Any.hld_type = MAX(col->Any.hld_type, MIN(CEU_HOLD_FLEET,v.Dyn->Any.hld_type));
                    if (v.Dyn->Any.hld_depth > col->Any.hld_depth) {
                        ceu_hold_chg(col, v.Dyn->Any.hld_prev, v.Dyn->Any.hld_depth);
                    }
                    return (CEU_Value) { CEU_VALUE_NIL };
                }
            } else {
                return (CEU_Value) { CEU_VALUE_NIL };
            }
        }

        CEU_Value _ceu_drop_f_ (CEU_Frame* frame, int n, CEU_Value args[]) {
            assert(n == 1);
            CEU_Value src = args[0];
            CEU_Dyn* dyn = src.Dyn;
            
            // do not drop non-dyn or globals
            if (src.type < CEU_VALUE_DYNAMIC) {
                return (CEU_Value) { CEU_VALUE_NIL };
            } else if (dyn->Any.hld_depth == 1) {
                return (CEU_Value) { CEU_VALUE_NIL };
            } else if (dyn->Any.hld_type == CEU_HOLD_FLEET) {
                return (CEU_Value) { CEU_VALUE_NIL };
            }
            
            //printf(">>> %d\n", dyn->Any.refs);
            if (dyn->Any.hld_type == CEU_HOLD_IMMUT) {
                return (CEU_Value) { CEU_VALUE_ERROR, {.Error="drop error : value is not movable"} };
            }
            //if (dyn->Any.refs > 1) {
            //    return (CEU_Value) { CEU_VALUE_ERROR, {.Error="drop error : multiple references"} };
            //}
            dyn->Any.hld_type = CEU_HOLD_FLEET;
            //ceu_hold_chg(dyn, &frame->up_block->dyns, frame->up_block->depth);

            switch (src.type) {
                case CEU_VALUE_CLOSURE:
                    for (int i=0; i<dyn->Closure.upvs.its; i++) {
                        CEU_Value ret = _ceu_drop_f_(frame, 1, &dyn->Closure.upvs.buf[i]);
                        if (ret.type == CEU_VALUE_ERROR) {
                            return ret;
                        }
                    }
                    break;
                case CEU_VALUE_TUPLE: {
                    for (int i=0; i<dyn->Tuple.its; i++) {
                        CEU_Value ret = _ceu_drop_f_(frame, 1, &dyn->Tuple.buf[i]);
                        if (ret.type == CEU_VALUE_ERROR) {
                            return ret;
                        }
                    }
                    break;
                }
                case CEU_VALUE_VECTOR: {
                    for (int i=0; i<dyn->Vector.its; i++) {
                        CEU_Value ret1 = ceu_vector_get(&dyn->Vector, i);
                        assert(ret1.type != CEU_VALUE_ERROR);
                        CEU_Value args[1] = { ret1 };
                        CEU_Value ret2 = _ceu_drop_f_(frame, 1, args);
                        if (ret2.type == CEU_VALUE_ERROR) {
                            return ret2;
                        }
                    }
                    break;
                }
                case CEU_VALUE_DICT: {
                    for (int i=0; i<dyn->Dict.max; i++) {
                        CEU_Value ret0 = _ceu_drop_f_(frame, 1, &(*dyn->Dict.buf)[i][0]);
                        if (ret0.type == CEU_VALUE_ERROR) {
                            return ret0;
                        }
                        CEU_Value ret1 = _ceu_drop_f_(frame, 1, &(*dyn->Dict.buf)[i][1]);
                        if (ret1.type == CEU_VALUE_ERROR) {
                            return ret1;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            return (CEU_Value) { CEU_VALUE_NIL };;
        }        
        CEU_Value ceu_drop_f (CEU_Frame* frame, int n, CEU_Value args[]) {
            CEU_Value ret = _ceu_drop_f_(frame, n, args);
            ceu_gc_chk_args(n, args);
            return ret;
        }
     // TUPLE / VECTOR / DICT
        #define ceu_sizeof(type, member) sizeof(((type *)0)->member)
        int ceu_tag_to_size (int type) {
            switch (type) {
                case CEU_VALUE_NIL:
                    return 0;
                case CEU_VALUE_TAG:
                    return ceu_sizeof(CEU_Value, Tag);
                case CEU_VALUE_BOOL:
                    return ceu_sizeof(CEU_Value, Bool);
                case CEU_VALUE_CHAR:
                    return ceu_sizeof(CEU_Value, Char);
                case CEU_VALUE_NUMBER:
                    return ceu_sizeof(CEU_Value, Number);
                case CEU_VALUE_POINTER:
                    return ceu_sizeof(CEU_Value, Pointer);
                case CEU_VALUE_CLOSURE:
                case CEU_VALUE_TUPLE:
                case CEU_VALUE_VECTOR:
                case CEU_VALUE_DICT:
                    return ceu_sizeof(CEU_Value, Dyn);
                default:
                    assert(0 && "bug found");
            }
        }
        
        CEU_Value ceu_tuple_set (CEU_Tuple* tup, int i, CEU_Value v) {
            ceu_gc_inc(v);
            ceu_gc_dec(tup->buf[i], 1);
            tup->buf[i] = v;
            return ceu_hold_chk_set_col((CEU_Dyn*)tup, v);
        }
        
        CEU_Value ceu_vector_get (CEU_Vector* vec, int i) {
            if (i<0 || i>=vec->its) {
                return (CEU_Value) { CEU_VALUE_ERROR, {.Error="index error : out of bounds"} };
            }
            int sz = ceu_tag_to_size(vec->unit);
            CEU_Value ret = (CEU_Value) { vec->unit };
            memcpy(&ret.Number, vec->buf+i*sz, sz);
            return ret;
        }
        
        CEU_Value ceu_vector_set (CEU_Vector* vec, int i, CEU_Value v) {
            if (v.type == CEU_VALUE_NIL) {           // pop
                assert(i == vec->its-1);
                CEU_Value ret = ceu_vector_get(vec, i);
                assert(ret.type != CEU_VALUE_ERROR);
                ceu_gc_dec(ret, 1);
                vec->its--;
                return ret;
            } else {
                CEU_Value err = ceu_hold_chk_set_col((CEU_Dyn*)vec, v);
                if (err.type == CEU_VALUE_ERROR) {
                    return err;
                }
                if (vec->its == 0) {
                    vec->unit = v.type;
                } else {
                    assert(v.type == vec->unit);
                }
                int sz = ceu_tag_to_size(vec->unit);
                if (i == vec->its) {           // push
                    if (i == vec->max) {
                        vec->max = vec->max*2 + 1;    // +1 if max=0
                        vec->buf = realloc(vec->buf, vec->max*sz + 1);
                        assert(vec->buf != NULL);
                    }
                    ceu_gc_inc(v);
                    vec->its++;
                    vec->buf[sz*vec->its] = '\0';
                } else {                            // set
                    CEU_Value ret = ceu_vector_get(vec, i);
                    assert(ret.type != CEU_VALUE_ERROR);
                    ceu_gc_inc(v);
                    ceu_gc_dec(ret, 1);
                    assert(i < vec->its);
                }
                memcpy(vec->buf + i*sz, (char*)&v.Number, sz);
                return (CEU_Value) { CEU_VALUE_NIL };
            }
        }
        
        CEU_Value ceu_vector_from_c_string (CEU_Block* hld, const char* str) {
            CEU_Value vec = ceu_vector_create(hld);
            int N = strlen(str);
            for (int i=0; i<N; i++) {
                assert(ceu_vector_set(&vec.Dyn->Vector, vec.Dyn->Vector.its, (CEU_Value) { CEU_VALUE_CHAR, {.Char=str[i]} }).type != CEU_VALUE_ERROR);
            }
            return vec;
        }

        CEU_Value _ceu_next_f_ (CEU_Frame* _1, int n, CEU_Value args[]) {
            assert(n==1 || n==2);
            CEU_Value col = args[0];
            if (col.type != CEU_VALUE_DICT) {
                return (CEU_Value) { CEU_VALUE_ERROR, {.Error="next error : expected dict"} };
            }
            CEU_Value key = (n == 1) ? ((CEU_Value) { CEU_VALUE_NIL }) : args[1];
            if (key.type == CEU_VALUE_NIL) {
                return (*col.Dyn->Dict.buf)[0][0];
            }
            for (int i=0; i<col.Dyn->Dict.max-1; i++) {     // -1: last element has no next
                CEU_Value args[] = { key, (*col.Dyn->Dict.buf)[i][0] };
                CEU_Value ret = _ceu_op_equals_equals_f_(NULL, 2, args);
                assert(ret.type != CEU_VALUE_ERROR);
                if (ret.Bool) {
                    return (*col.Dyn->Dict.buf)[i+1][0];
                }
            }
            return (CEU_Value) { CEU_VALUE_NIL };
        }        
        CEU_Value ceu_next_f (CEU_Frame* _1, int n, CEU_Value args[]) {
            CEU_Value ret = _ceu_next_f_(_1, n, args);
            ceu_gc_chk_args(n, args);
            return ret;
        }
        int ceu_dict_key_to_index (CEU_Dict* col, CEU_Value key, int* idx) {
            *idx = -1;
            for (int i=0; i<col->max; i++) {
                CEU_Value cur = (*col->buf)[i][0];
                CEU_Value args[] = { key, cur };
                CEU_Value ret = _ceu_op_equals_equals_f_(NULL, 2, args);
                assert(ret.type != CEU_VALUE_ERROR);
                if (ret.Bool) {
                    *idx = i;
                    return 1;
                } else {
                    if (*idx==-1 && cur.type==CEU_VALUE_NIL) {
                        *idx = i;
                    }
                }
            }
            return 0;
        }        
        CEU_Value ceu_dict_get (CEU_Dict* col, CEU_Value key) {
            int i;
            int ok = ceu_dict_key_to_index(col, key, &i);
            if (ok) {
                return (*col->buf)[i][1];
            } else {
                return (CEU_Value) { CEU_VALUE_NIL };
            }
        }        
        CEU_Value ceu_dict_set (CEU_Dict* col, CEU_Value key, CEU_Value val) {
            if (key.type == CEU_VALUE_NIL) {
                return (CEU_Value) { CEU_VALUE_ERROR, {.Error="dict error : index cannot be nil"} };
            }
            int old;
            ceu_dict_key_to_index(col, key, &old);
            if (old == -1) {
                old = col->max;
                int new = MAX(5, old * 2);
                col->max = new;
                col->buf = realloc(col->buf, new*2*sizeof(CEU_Value));
                assert(col->buf != NULL);
                memset(&(*col->buf)[old], 0, (new-old)*2*sizeof(CEU_Value));  // x[i]=nil
            }
            assert(old != -1);
            
            CEU_Value vv = ceu_dict_get(col, key);
            
            if (val.type == CEU_VALUE_NIL) {
                ceu_gc_dec(vv, 1);
                ceu_gc_dec(key, 1);
                (*col->buf)[old][0] = (CEU_Value) { CEU_VALUE_NIL };
                return (CEU_Value) { CEU_VALUE_NIL };
            } else {
                CEU_Value err1 = ceu_hold_chk_set_col((CEU_Dyn*)col, key);
                if (err1.type == CEU_VALUE_ERROR) {
                    return err1;
                }
                CEU_Value err2 = ceu_hold_chk_set_col((CEU_Dyn*)col, val);
                if (err2.type == CEU_VALUE_ERROR) {
                    return err2;
                }

                ceu_gc_inc(val);
                ceu_gc_dec(vv, 1);
                if (vv.type == CEU_VALUE_NIL) {
                    ceu_gc_inc(key);
                }
                (*col->buf)[old][0] = key;
                (*col->buf)[old][1] = val;
                return (CEU_Value) { CEU_VALUE_NIL };
            }
        }        
        
        CEU_Value ceu_col_check (CEU_Value col, CEU_Value idx) {
            if (col.type<CEU_VALUE_TUPLE || col.type>CEU_VALUE_DICT) {                
                return (CEU_Value) { CEU_VALUE_ERROR, {.Error="index error : expected collection"} };
            }
            if (col.type != CEU_VALUE_DICT) {
                if (idx.type != CEU_VALUE_NUMBER) {
                    return (CEU_Value) { CEU_VALUE_ERROR, {.Error="index error : expected number"} };
                }
                if (col.type==CEU_VALUE_TUPLE && (idx.Number<0 || idx.Number>=col.Dyn->Tuple.its)) {                
                    return (CEU_Value) { CEU_VALUE_ERROR, {.Error="index error : out of bounds"} };
                }
                if (col.type==CEU_VALUE_VECTOR && (idx.Number<0 || idx.Number>col.Dyn->Vector.its)) {                
                    return (CEU_Value) { CEU_VALUE_ERROR, {.Error="index error : out of bounds"} };
                }
            }
            return (CEU_Value) { CEU_VALUE_NIL };
        }
     // CREATES
        CEU_Value ceu_tuple_create (CEU_Block* blk, int n) {
            CEU_Tuple* ret = malloc(sizeof(CEU_Tuple) + n*sizeof(CEU_Value));
            assert(ret != NULL);
            *ret = (CEU_Tuple) {
                CEU_VALUE_TUPLE, 0, CEU_HOLD_FLEET, blk->depth, NULL, NULL, NULL,
                n, {}
            };
            memset(ret->buf, 0, n*sizeof(CEU_Value));
            ceu_hold_add((CEU_Dyn*)ret, &blk->dyns);
            return (CEU_Value) { CEU_VALUE_TUPLE, {.Dyn=(CEU_Dyn*)ret} };
        }
        
        CEU_Value ceu_tuple_f (CEU_Frame* frame, int n, CEU_Value args[]) {
            assert(n==1 && args[0].type==CEU_VALUE_NUMBER);
            return ceu_tuple_create(frame->up_block, args[0].Number);
        }
        
        CEU_Value ceu_vector_create (CEU_Block* blk) {
            CEU_Vector* ret = malloc(sizeof(CEU_Vector));
            assert(ret != NULL);
            char* buf = malloc(1);  // because of '\0' in empty strings
            assert(buf != NULL);
            buf[0] = '\0';
            *ret = (CEU_Vector) {
                CEU_VALUE_VECTOR, 0, CEU_HOLD_FLEET, blk->depth, NULL, NULL, NULL,
                0, 0, CEU_VALUE_NIL, buf
            };
            ceu_hold_add((CEU_Dyn*)ret, &blk->dyns);
            return (CEU_Value) { CEU_VALUE_VECTOR, {.Dyn=(CEU_Dyn*)ret} };
        }
        
        CEU_Value ceu_dict_create (CEU_Block* blk) {
            CEU_Dict* ret = malloc(sizeof(CEU_Dict));
            assert(ret != NULL);
            *ret = (CEU_Dict) {
                CEU_VALUE_DICT, 0, CEU_HOLD_FLEET, blk->depth, NULL, NULL, NULL,
                0, NULL
            };
            ceu_hold_add((CEU_Dyn*)ret, &blk->dyns);
            return (CEU_Value) { CEU_VALUE_DICT, {.Dyn=(CEU_Dyn*)ret} };
        }
        
        CEU_Value ceu_closure_create (CEU_Block* blk, CEU_HOLD tphold, CEU_Frame* frame, CEU_Proto proto, int upvs) {
            CEU_Closure* ret = malloc(sizeof(CEU_Closure));
            assert(ret != NULL);
            CEU_Value* buf = malloc(upvs * sizeof(CEU_Value));
            assert(buf != NULL);
            for (int i=0; i<upvs; i++) {
                buf[i] = (CEU_Value) { CEU_VALUE_NIL };
            }
            *ret = (CEU_Closure) {
                CEU_VALUE_CLOSURE, 0, tphold, blk->depth, NULL, NULL, NULL,
                frame, proto, { upvs, buf }
            };
            ceu_hold_add((CEU_Dyn*)ret, &blk->dyns);
            return (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)ret} };
        }        
     // PRINT
        void ceu_print1 (CEU_Frame* _1, CEU_Value v) {
            // no tags when _1==NULL (ceu_error_list_print)
            if (_1!=NULL && v.type>CEU_VALUE_DYNAMIC) {  // TAGS
                CEU_Value tup = ceu_tags_f(_1, 1, &v);
                assert(tup.type != CEU_VALUE_ERROR);
                int N = tup.Dyn->Tuple.its;
                if (N > 0) {
                    if (N > 1) {
                        printf("[");
                    }
                    for (int i=0; i<N; i++) {
                        ceu_print1(_1, tup.Dyn->Tuple.buf[i]);
                        if (i < N-1) {
                            printf(",");
                        }
                    }
                    if (N > 1) {
                        printf("]");
                    }
                    printf(" ");
                }
                ceu_hold_rem(tup.Dyn);
                ceu_dyn_free(tup.Dyn);
            }
            switch (v.type) {
                case CEU_VALUE_NIL:
                    printf("nil");
                    break;
                case CEU_VALUE_TAG:
                    printf("%s", ceu_tag_to_string(v.Tag));
                    break;
                case CEU_VALUE_BOOL:
                    if (v.Bool) {
                        printf("true");
                    } else {
                        printf("false");
                    }
                    break;
                case CEU_VALUE_CHAR:
                    putchar(v.Char);
                    break;
                case CEU_VALUE_NUMBER:
                    printf("%g", v.Number);
                    break;
                case CEU_VALUE_POINTER:
                    printf("pointer: %p", v.Pointer);
                    break;
                case CEU_VALUE_TUPLE:
                    printf("[");
                    for (int i=0; i<v.Dyn->Tuple.its; i++) {
                        if (i > 0) {
                            printf(",");
                        }
                        ceu_print1(_1, v.Dyn->Tuple.buf[i]);
                    }                    
                    printf("]");
                    break;
                case CEU_VALUE_VECTOR:
                    if (v.Dyn->Vector.unit == CEU_VALUE_CHAR) {
                        printf("%s", v.Dyn->Vector.buf);
                    } else {
                        printf("#[");
                        for (int i=0; i<v.Dyn->Vector.its; i++) {
                            if (i > 0) {
                                printf(",");
                            }
                            CEU_Value ret = ceu_vector_get(&v.Dyn->Vector, i);
                            assert(ret.type != CEU_VALUE_ERROR);
                            ceu_print1(_1, ret);
                        }                    
                        printf("]");
                    }
                    break;
                case CEU_VALUE_DICT:
                    printf("@[");
                    int comma = 0;
                    for (int i=0; i<v.Dyn->Dict.max; i++) {
                        if ((*v.Dyn->Dict.buf)[i][0].type != CEU_VALUE_NIL) {
                            if (comma != 0) {
                                printf(",");
                            }
                            comma = 1;
                            printf("(");
                            ceu_print1(_1, (*v.Dyn->Dict.buf)[i][0]);
                            printf(",");
                            ceu_print1(_1, (*v.Dyn->Dict.buf)[i][1]);
                            printf(")");
                        }
                    }                    
                    printf("]");
                    break;
                case CEU_VALUE_CLOSURE:
                    printf("func: %p", v.Dyn);
                    break;
                default:
                    assert(0 && "bug found");
            }
        }
        CEU_Value ceu_print_f (CEU_Frame* _1, int n, CEU_Value args[]) {
            for (int i=0; i<n; i++) {
                if (i > 0) {
                    printf("\t");
                }
                ceu_print1(_1, args[i]);
            }
            ceu_gc_chk_args(n, args);
            return (CEU_Value) { CEU_VALUE_NIL };
        }
        CEU_Value ceu_println_f (CEU_Frame* _1, int n, CEU_Value args[]) {
            ceu_print_f(_1, n, args);
            printf("\n");
            return (CEU_Value) { CEU_VALUE_NIL };
        }
    
        // EQ / NEQ / LEN
        CEU_Value _ceu_op_equals_equals_f_ (CEU_Frame* _1, int n, CEU_Value args[]) {
            assert(n == 2);
            CEU_Value e1 = args[0];
            CEU_Value e2 = args[1];
            int v = (e1.type == e2.type);
            if (v) {
                switch (e1.type) {
                    case CEU_VALUE_NIL:
                        v = 1;
                        break;
                    case CEU_VALUE_TAG:
                        v = (e1.Tag == e2.Tag);
                        break;
                    case CEU_VALUE_BOOL:
                        v = (e1.Bool == e2.Bool);
                        break;
                    case CEU_VALUE_CHAR:
                        v = (e1.Char == e2.Char);
                        break;
                    case CEU_VALUE_NUMBER:
                        v = (e1.Number == e2.Number);
                        break;
                    case CEU_VALUE_POINTER:
                        v = (e1.Pointer == e2.Pointer);
                        break;
                    case CEU_VALUE_TUPLE:
                    case CEU_VALUE_VECTOR:
                    case CEU_VALUE_DICT:
                    case CEU_VALUE_CLOSURE:
                        v = (e1.Dyn == e2.Dyn);
                        break;
                    default:
                        assert(0 && "bug found");
                }
            }
            return (CEU_Value) { CEU_VALUE_BOOL, {.Bool=v} };
        }
        CEU_Value ceu_op_equals_equals_f (CEU_Frame* _1, int n, CEU_Value args[]) {
            CEU_Value ret = _ceu_op_equals_equals_f_(_1, n, args);
            ceu_gc_chk_args(n, args);
            return ret;
        }
        CEU_Value ceu_op_slash_equals_f (CEU_Frame* _1, int n, CEU_Value args[]) {
            CEU_Value ret = ceu_op_equals_equals_f(_1, n, args);
            ret.Bool = !ret.Bool;
            return ret;
        }
        
        CEU_Value ceu_op_hash_f (CEU_Frame* _1, int n, CEU_Value args[]) {
            assert(n == 1);
            CEU_Value ret;
            if (args[0].type == CEU_VALUE_VECTOR) {
                ret = (CEU_Value) { CEU_VALUE_NUMBER, {.Number=args[0].Dyn->Vector.its} };
            } else if (args[0].type == CEU_VALUE_TUPLE) {
                ret = (CEU_Value) { CEU_VALUE_NUMBER, {.Number=args[0].Dyn->Tuple.its} };
            } else {
                ret = (CEU_Value) { CEU_VALUE_ERROR, {.Error="length error : not a vector"} };
            }
            ceu_gc_chk_args(n, args);
            return ret;
        }        
     // GLOBALS
        CEU_Block _ceu_block_ = { 0, 0, {.block=NULL}, NULL };
        CEU_Frame _ceu_frame_ = { NULL, &_ceu_block_ };
        CEU_Frame* ceu_frame = &_ceu_frame_;

        CEU_Closure ceu_dump = { 
            CEU_VALUE_CLOSURE, 1, CEU_HOLD_MUTAB, 1, NULL, NULL, NULL,
            &_ceu_frame_, ceu_dump_f, {0,NULL}
        };
        CEU_Closure ceu_error = { 
            CEU_VALUE_CLOSURE, 1, CEU_HOLD_MUTAB, 1, NULL, NULL, NULL,
            &_ceu_frame_, ceu_error_f, {0,NULL}
        };
        CEU_Closure ceu_next = { 
            CEU_VALUE_CLOSURE, 1, CEU_HOLD_MUTAB, 1, NULL, NULL, NULL,
            &_ceu_frame_, ceu_next_f, {0,NULL}
        };
        CEU_Closure ceu_print = { 
            CEU_VALUE_CLOSURE, 1, CEU_HOLD_MUTAB, 1, NULL, NULL, NULL,
            &_ceu_frame_, ceu_print_f, {0,NULL}
        };
        CEU_Closure ceu_println = { 
            CEU_VALUE_CLOSURE, 1, CEU_HOLD_MUTAB, 1, NULL, NULL, NULL,
            &_ceu_frame_, ceu_println_f, {0,NULL}
        };
        CEU_Closure ceu_sup_question_ = { 
            CEU_VALUE_CLOSURE, 1, CEU_HOLD_MUTAB, 1, NULL, NULL, NULL,
            &_ceu_frame_, ceu_sup_question__f, {0,NULL}
        };
        CEU_Closure ceu_tags = { 
            CEU_VALUE_CLOSURE, 1, CEU_HOLD_MUTAB, 1, NULL, NULL, NULL,
            &_ceu_frame_, ceu_tags_f, {0,NULL}
        };
        CEU_Closure ceu_tuple = { 
            CEU_VALUE_CLOSURE, 1, CEU_HOLD_MUTAB, 1, NULL, NULL, NULL,
            &_ceu_frame_, ceu_tuple_f, {0,NULL}
        };
        CEU_Closure ceu_type = { 
            CEU_VALUE_CLOSURE, 1, CEU_HOLD_MUTAB, 1, NULL, NULL, NULL,
            &_ceu_frame_, ceu_type_f, {0,NULL}
        };
        CEU_Closure ceu_op_equals_equals = { 
            CEU_VALUE_CLOSURE, 1, CEU_HOLD_MUTAB, 1, NULL, NULL, NULL,
            &_ceu_frame_, ceu_op_equals_equals_f, {0,NULL}
        };
        CEU_Closure ceu_op_hash = { 
            CEU_VALUE_CLOSURE, 1, CEU_HOLD_MUTAB, 1, NULL, NULL, NULL,
            &_ceu_frame_, ceu_op_hash_f, {0,NULL}
        };
        CEU_Closure ceu_op_slash_equals = { 
            CEU_VALUE_CLOSURE, 1, CEU_HOLD_MUTAB, 1, NULL, NULL, NULL,
            &_ceu_frame_, ceu_op_slash_equals_f, {0,NULL}
        };
        CEU_Closure ceu_string_dash_to_dash_tag = { 
            CEU_VALUE_CLOSURE, 1, CEU_HOLD_MUTAB, 1, NULL, NULL, NULL,
            &_ceu_frame_, ceu_string_dash_to_dash_tag_f, {0,NULL}
        };

        CEU_Value id_dump                    = (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)&ceu_dump}                    };
        CEU_Value id_error                   = (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)&ceu_error}                   };
        CEU_Value id_next                    = (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)&ceu_next}                    };
        CEU_Value id_print                   = (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)&ceu_print}                   };
        CEU_Value id_println                 = (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)&ceu_println}                 };
        CEU_Value id_tags                    = (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)&ceu_tags}                    };
        CEU_Value id_type                    = (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)&ceu_type}                    };
        CEU_Value id_tuple                   = (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)&ceu_tuple}                   };
        CEU_Value op_hash                    = (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)&ceu_op_hash}                 };
        CEU_Value id_sup_question_           = (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)&ceu_sup_question_}           };
        CEU_Value op_equals_equals           = (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)&ceu_op_equals_equals}        };
        CEU_Value op_slash_equals            = (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)&ceu_op_slash_equals}         };
        CEU_Value id_string_dash_to_dash_tag = (CEU_Value) { CEU_VALUE_CLOSURE, {.Dyn=(CEU_Dyn*)&ceu_string_dash_to_dash_tag} };
     // MAIN
        int main (int ceu_argc, char** ceu_argv) {
            assert(CEU_TAG_nil == CEU_VALUE_NIL);
            CEU_Value ceu_acc;        
            
                    { // BLOCK | 
                        CEU_Block _ceu_block_2005 = (CEU_Block) { 1, 1, {.frame=&_ceu_frame_}, NULL };
                        CEU_Block* ceu_block_2005 = &_ceu_block_2005; 
                        
                            // main block varargs (...)
                            CEU_Value id__dot__dot__dot_ = ceu_tuple_create(ceu_block_2005, ceu_argc);
                            for (int i=0; i<ceu_argc; i++) {
                                CEU_Value vec = ceu_vector_from_c_string(ceu_block_2005, ceu_argv[i]);
                                assert(ceu_tuple_set(&id__dot__dot__dot_.Dyn->Tuple, i, vec).type != CEU_VALUE_ERROR);
                            }
                        
                        
                        
                            CEU_Value op_ampersand_ampersand = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value op_bar_bar = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value op_plus = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value op_minus = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value op_asterisk = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value op_asterisk_asterisk = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value op_slash = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value op_slash_slash = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value op_null = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value op_greater_equals = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value op_greater = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value op_less_equals = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value op_less = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_to_dash_string = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_to_dash_number = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_to_dash_tag = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_n = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_fannkuch = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_main = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_31 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v1;
                            CEU_Block* _id_v1_;
                            
                            CEU_Value id_v2;
                            CEU_Block* _id_v2_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_30 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_30 = &_ceu_block_30; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_30,
                                            ceu_hold_chk_set(&ceu_block_30->dyns, ceu_block_30->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 3, col 28)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_30,
                                            ceu_hold_chk_set(&ceu_block_30->dyns, ceu_block_30->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 3, col 28)"
                                        );
                                        id_v2 = ceu_args[1];
                                    } else {
                                        id_v2 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        
                { // IF | 
                    ceu_acc = id_v1;

                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_21 = ceu_block_30;
                    // >>> block
                    ceu_acc = id_v2;

                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_27 = ceu_block_30;
                    // >>> block
                    ceu_acc = ((CEU_Value) { CEU_VALUE_BOOL, {.Bool=0} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_30, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 3, col 28)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_30);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_31 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_31,
                    0
                );
                ceu_acc = ceu_ret_31;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 3, col 1)"
                        );
                    
                
                    op_ampersand_ampersand = ceu_acc;
                    ceu_gc_inc(op_ampersand_ampersand);
                    ceu_acc = op_ampersand_ampersand;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_60 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v1;
                            CEU_Block* _id_v1_;
                            
                            CEU_Value id_v2;
                            CEU_Block* _id_v2_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_59 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_59 = &_ceu_block_59; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_59,
                                            ceu_hold_chk_set(&ceu_block_59->dyns, ceu_block_59->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 11, col 28)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_59,
                                            ceu_hold_chk_set(&ceu_block_59->dyns, ceu_block_59->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 11, col 28)"
                                        );
                                        id_v2 = ceu_args[1];
                                    } else {
                                        id_v2 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        
                { // IF | 
                    ceu_acc = id_v1;

                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_50 = ceu_block_59;
                    // >>> block
                    ceu_acc = ((CEU_Value) { CEU_VALUE_BOOL, {.Bool=1} });
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_56 = ceu_block_59;
                    // >>> block
                    ceu_acc = id_v2;

                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_59, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 11, col 28)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_59);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_60 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_60,
                    0
                );
                ceu_acc = ceu_ret_60;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 11, col 1)"
                        );
                    
                
                    op_bar_bar = ceu_acc;
                    ceu_gc_inc(op_bar_bar);
                    ceu_acc = op_bar_bar;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_76 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v1;
                            CEU_Block* _id_v1_;
                            
                            CEU_Value id_v2;
                            CEU_Block* _id_v2_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_75 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_75 = &_ceu_block_75; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_75,
                                            ceu_hold_chk_set(&ceu_block_75->dyns, ceu_block_75->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 21, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_75,
                                            ceu_hold_chk_set(&ceu_block_75->dyns, ceu_block_75->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 21, col 27)"
                                        );
                                        id_v2 = ceu_args[1];
                                    } else {
                                        id_v2 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( ((id_v1).Number + (id_v2).Number))} });
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_75, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 21, col 27)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_75);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_76 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_76,
                    0
                );
                ceu_acc = ceu_ret_76;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 21, col 1)"
                        );
                    
                
                    op_plus = ceu_acc;
                    ceu_gc_inc(op_plus);
                    ceu_acc = op_plus;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_189 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v1;
                            CEU_Block* _id_v1_;
                            
                            CEU_Value id_v2;
                            CEU_Block* _id_v2_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_188 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_188 = &_ceu_block_188; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_188,
                                            ceu_hold_chk_set(&ceu_block_188->dyns, ceu_block_188->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 25, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_188,
                                            ceu_hold_chk_set(&ceu_block_188->dyns, ceu_block_188->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 25, col 27)"
                                        );
                                        id_v2 = ceu_args[1];
                                    } else {
                                        id_v2 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                            CEU_Value id_t1 = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_t2 = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_97 = ceu_acc;
                    if (ceu_closure_97.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_188, "prelude.ceu : (lin 26, col 14)", err);
                    }
                    CEU_Frame ceu_frame_97 = { &ceu_closure_97.Dyn->Closure, ceu_block_188 };
                    
                    CEU_Value ceu_args_97[1];
                    
                    ceu_acc = id_v1;
ceu_args_97[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_97.closure->proto (
                        &ceu_frame_97,
                        1,
                        ceu_args_97
                    );
                    ceu_assert_pre(ceu_block_188, ceu_acc, "prelude.ceu : (lin 26, col 14) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_188,
                            ceu_hold_chk_set(&ceu_block_188->dyns, ceu_block_188->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 26, col 5)"
                        );
                    
                
                    id_t1 = ceu_acc;
                    ceu_gc_inc(id_t1);
                    ceu_acc = id_t1;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_108 = ceu_acc;
                    if (ceu_closure_108.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_188, "prelude.ceu : (lin 27, col 14)", err);
                    }
                    CEU_Frame ceu_frame_108 = { &ceu_closure_108.Dyn->Closure, ceu_block_188 };
                    
                    CEU_Value ceu_args_108[1];
                    
                    ceu_acc = id_v2;
ceu_args_108[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_108.closure->proto (
                        &ceu_frame_108,
                        1,
                        ceu_args_108
                    );
                    ceu_assert_pre(ceu_block_188, ceu_acc, "prelude.ceu : (lin 27, col 14) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_188,
                            ceu_hold_chk_set(&ceu_block_188->dyns, ceu_block_188->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 27, col 5)"
                        );
                    
                
                    id_t2 = ceu_acc;
                    ceu_gc_inc(id_t2);
                    ceu_acc = id_t2;
                    
                
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_ampersand_ampersand;

                    CEU_Value ceu_closure_134 = ceu_acc;
                    if (ceu_closure_134.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_188, "prelude.ceu : (lin 28, col 24)", err);
                    }
                    CEU_Frame ceu_frame_134 = { &ceu_closure_134.Dyn->Closure, ceu_block_188 };
                    
                    CEU_Value ceu_args_134[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_119 = ceu_acc;
                    if (ceu_closure_119.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_188, "prelude.ceu : (lin 28, col 12)", err);
                    }
                    CEU_Frame ceu_frame_119 = { &ceu_closure_119.Dyn->Closure, ceu_block_188 };
                    
                    CEU_Value ceu_args_119[2];
                    
                    ceu_acc = id_t1;
ceu_args_119[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_119[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_119.closure->proto (
                        &ceu_frame_119,
                        2,
                        ceu_args_119
                    );
                    ceu_assert_pre(ceu_block_188, ceu_acc, "prelude.ceu : (lin 28, col 12) : call error");
                } // CALL
                ceu_args_134[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_130 = ceu_acc;
                    if (ceu_closure_130.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_188, "prelude.ceu : (lin 28, col 31)", err);
                    }
                    CEU_Frame ceu_frame_130 = { &ceu_closure_130.Dyn->Closure, ceu_block_188 };
                    
                    CEU_Value ceu_args_130[2];
                    
                    ceu_acc = id_t2;
ceu_args_130[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_nil} });ceu_args_130[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_130.closure->proto (
                        &ceu_frame_130,
                        2,
                        ceu_args_130
                    );
                    ceu_assert_pre(ceu_block_188, ceu_acc, "prelude.ceu : (lin 28, col 31) : call error");
                } // CALL
                ceu_args_134[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_134.closure->proto (
                        &ceu_frame_134,
                        2,
                        ceu_args_134
                    );
                    ceu_assert_pre(ceu_block_188, ceu_acc, "prelude.ceu : (lin 28, col 24) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_139 = ceu_block_188;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( (- (id_v1).Number))} });
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_185 = ceu_block_188;
                    // >>> block
                    
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_166 = ceu_acc;
                    if (ceu_closure_166.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_185, "prelude.ceu : (lin 31, col 28)", err);
                    }
                    CEU_Frame ceu_frame_166 = { &ceu_closure_166.Dyn->Closure, ceu_block_185 };
                    
                    CEU_Value ceu_args_166[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_151 = ceu_acc;
                    if (ceu_closure_151.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_185, "prelude.ceu : (lin 31, col 16)", err);
                    }
                    CEU_Frame ceu_frame_151 = { &ceu_closure_151.Dyn->Closure, ceu_block_185 };
                    
                    CEU_Value ceu_args_151[2];
                    
                    ceu_acc = id_t1;
ceu_args_151[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_151[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_151.closure->proto (
                        &ceu_frame_151,
                        2,
                        ceu_args_151
                    );
                    ceu_assert_pre(ceu_block_185, ceu_acc, "prelude.ceu : (lin 31, col 16) : call error");
                } // CALL
                ceu_args_166[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_162 = ceu_acc;
                    if (ceu_closure_162.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_185, "prelude.ceu : (lin 31, col 35)", err);
                    }
                    CEU_Frame ceu_frame_162 = { &ceu_closure_162.Dyn->Closure, ceu_block_185 };
                    
                    CEU_Value ceu_args_162[2];
                    
                    ceu_acc = id_t2;
ceu_args_162[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_162[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_162.closure->proto (
                        &ceu_frame_162,
                        2,
                        ceu_args_162
                    );
                    ceu_assert_pre(ceu_block_185, ceu_acc, "prelude.ceu : (lin 31, col 35) : call error");
                } // CALL
                ceu_args_166[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_166.closure->proto (
                        &ceu_frame_166,
                        2,
                        ceu_args_166
                    );
                    ceu_assert_pre(ceu_block_185, ceu_acc, "prelude.ceu : (lin 31, col 28) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_176 = ceu_block_185;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_174 = ceu_acc;
                    if (ceu_closure_174.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_176, "prelude.ceu : (lin 32, col 13)", err);
                    }
                    CEU_Frame ceu_frame_174 = { &ceu_closure_174.Dyn->Closure, ceu_block_176 };
                    
                    CEU_Value ceu_args_174[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_174[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_174.closure->proto (
                        &ceu_frame_174,
                        1,
                        ceu_args_174
                    );
                    ceu_assert_pre(ceu_block_176, ceu_acc, "prelude.ceu : (lin 32, col 13) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_182 = ceu_block_185;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( ((id_v1).Number - (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_188, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 25, col 27)"
                            );
                            
                        
                            if (id_t1.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_t1, (id_t1.Dyn->Any.hld_depth == ceu_block_188->depth));
                            }
                        
                            if (id_t2.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_t2, (id_t2.Dyn->Any.hld_depth == ceu_block_188->depth));
                            }
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_188);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_189 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_189,
                    0
                );
                ceu_acc = ceu_ret_189;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 25, col 1)"
                        );
                    
                
                    op_minus = ceu_acc;
                    ceu_gc_inc(op_minus);
                    ceu_acc = op_minus;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_255 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v1;
                            CEU_Block* _id_v1_;
                            
                            CEU_Value id_v2;
                            CEU_Block* _id_v2_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_254 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_254 = &_ceu_block_254; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_254,
                                            ceu_hold_chk_set(&ceu_block_254->dyns, ceu_block_254->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 39, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_254,
                                            ceu_hold_chk_set(&ceu_block_254->dyns, ceu_block_254->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 39, col 27)"
                                        );
                                        id_v2 = ceu_args[1];
                                    } else {
                                        id_v2 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_235 = ceu_acc;
                    if (ceu_closure_235.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_254, "prelude.ceu : (lin 40, col 30)", err);
                    }
                    CEU_Frame ceu_frame_235 = { &ceu_closure_235.Dyn->Closure, ceu_block_254 };
                    
                    CEU_Value ceu_args_235[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_215 = ceu_acc;
                    if (ceu_closure_215.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_254, "prelude.ceu : (lin 40, col 18)", err);
                    }
                    CEU_Frame ceu_frame_215 = { &ceu_closure_215.Dyn->Closure, ceu_block_254 };
                    
                    CEU_Value ceu_args_215[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_209 = ceu_acc;
                    if (ceu_closure_209.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_254, "prelude.ceu : (lin 40, col 9)", err);
                    }
                    CEU_Frame ceu_frame_209 = { &ceu_closure_209.Dyn->Closure, ceu_block_254 };
                    
                    CEU_Value ceu_args_209[1];
                    
                    ceu_acc = id_v1;
ceu_args_209[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_209.closure->proto (
                        &ceu_frame_209,
                        1,
                        ceu_args_209
                    );
                    ceu_assert_pre(ceu_block_254, ceu_acc, "prelude.ceu : (lin 40, col 9) : call error");
                } // CALL
                ceu_args_215[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_215[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_215.closure->proto (
                        &ceu_frame_215,
                        2,
                        ceu_args_215
                    );
                    ceu_assert_pre(ceu_block_254, ceu_acc, "prelude.ceu : (lin 40, col 18) : call error");
                } // CALL
                ceu_args_235[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_231 = ceu_acc;
                    if (ceu_closure_231.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_254, "prelude.ceu : (lin 40, col 43)", err);
                    }
                    CEU_Frame ceu_frame_231 = { &ceu_closure_231.Dyn->Closure, ceu_block_254 };
                    
                    CEU_Value ceu_args_231[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_225 = ceu_acc;
                    if (ceu_closure_225.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_254, "prelude.ceu : (lin 40, col 34)", err);
                    }
                    CEU_Frame ceu_frame_225 = { &ceu_closure_225.Dyn->Closure, ceu_block_254 };
                    
                    CEU_Value ceu_args_225[1];
                    
                    ceu_acc = id_v2;
ceu_args_225[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_225.closure->proto (
                        &ceu_frame_225,
                        1,
                        ceu_args_225
                    );
                    ceu_assert_pre(ceu_block_254, ceu_acc, "prelude.ceu : (lin 40, col 34) : call error");
                } // CALL
                ceu_args_231[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_231[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_231.closure->proto (
                        &ceu_frame_231,
                        2,
                        ceu_args_231
                    );
                    ceu_assert_pre(ceu_block_254, ceu_acc, "prelude.ceu : (lin 40, col 43) : call error");
                } // CALL
                ceu_args_235[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_235.closure->proto (
                        &ceu_frame_235,
                        2,
                        ceu_args_235
                    );
                    ceu_assert_pre(ceu_block_254, ceu_acc, "prelude.ceu : (lin 40, col 30) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_245 = ceu_block_254;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_243 = ceu_acc;
                    if (ceu_closure_243.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_245, "prelude.ceu : (lin 41, col 9)", err);
                    }
                    CEU_Frame ceu_frame_243 = { &ceu_closure_243.Dyn->Closure, ceu_block_245 };
                    
                    CEU_Value ceu_args_243[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_243[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_243.closure->proto (
                        &ceu_frame_243,
                        1,
                        ceu_args_243
                    );
                    ceu_assert_pre(ceu_block_245, ceu_acc, "prelude.ceu : (lin 41, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_251 = ceu_block_254;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( ((id_v1).Number * (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_254, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 39, col 27)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_254);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_255 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_255,
                    0
                );
                ceu_acc = ceu_ret_255;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 39, col 1)"
                        );
                    
                
                    op_asterisk = ceu_acc;
                    ceu_gc_inc(op_asterisk);
                    ceu_acc = op_asterisk;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_321 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v1;
                            CEU_Block* _id_v1_;
                            
                            CEU_Value id_v2;
                            CEU_Block* _id_v2_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_320 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_320 = &_ceu_block_320; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_320,
                                            ceu_hold_chk_set(&ceu_block_320->dyns, ceu_block_320->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 47, col 28)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_320,
                                            ceu_hold_chk_set(&ceu_block_320->dyns, ceu_block_320->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 47, col 28)"
                                        );
                                        id_v2 = ceu_args[1];
                                    } else {
                                        id_v2 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_301 = ceu_acc;
                    if (ceu_closure_301.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_320, "prelude.ceu : (lin 48, col 30)", err);
                    }
                    CEU_Frame ceu_frame_301 = { &ceu_closure_301.Dyn->Closure, ceu_block_320 };
                    
                    CEU_Value ceu_args_301[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_281 = ceu_acc;
                    if (ceu_closure_281.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_320, "prelude.ceu : (lin 48, col 18)", err);
                    }
                    CEU_Frame ceu_frame_281 = { &ceu_closure_281.Dyn->Closure, ceu_block_320 };
                    
                    CEU_Value ceu_args_281[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_275 = ceu_acc;
                    if (ceu_closure_275.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_320, "prelude.ceu : (lin 48, col 9)", err);
                    }
                    CEU_Frame ceu_frame_275 = { &ceu_closure_275.Dyn->Closure, ceu_block_320 };
                    
                    CEU_Value ceu_args_275[1];
                    
                    ceu_acc = id_v1;
ceu_args_275[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_275.closure->proto (
                        &ceu_frame_275,
                        1,
                        ceu_args_275
                    );
                    ceu_assert_pre(ceu_block_320, ceu_acc, "prelude.ceu : (lin 48, col 9) : call error");
                } // CALL
                ceu_args_281[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_281[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_281.closure->proto (
                        &ceu_frame_281,
                        2,
                        ceu_args_281
                    );
                    ceu_assert_pre(ceu_block_320, ceu_acc, "prelude.ceu : (lin 48, col 18) : call error");
                } // CALL
                ceu_args_301[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_297 = ceu_acc;
                    if (ceu_closure_297.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_320, "prelude.ceu : (lin 48, col 43)", err);
                    }
                    CEU_Frame ceu_frame_297 = { &ceu_closure_297.Dyn->Closure, ceu_block_320 };
                    
                    CEU_Value ceu_args_297[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_291 = ceu_acc;
                    if (ceu_closure_291.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_320, "prelude.ceu : (lin 48, col 34)", err);
                    }
                    CEU_Frame ceu_frame_291 = { &ceu_closure_291.Dyn->Closure, ceu_block_320 };
                    
                    CEU_Value ceu_args_291[1];
                    
                    ceu_acc = id_v2;
ceu_args_291[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_291.closure->proto (
                        &ceu_frame_291,
                        1,
                        ceu_args_291
                    );
                    ceu_assert_pre(ceu_block_320, ceu_acc, "prelude.ceu : (lin 48, col 34) : call error");
                } // CALL
                ceu_args_297[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_297[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_297.closure->proto (
                        &ceu_frame_297,
                        2,
                        ceu_args_297
                    );
                    ceu_assert_pre(ceu_block_320, ceu_acc, "prelude.ceu : (lin 48, col 43) : call error");
                } // CALL
                ceu_args_301[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_301.closure->proto (
                        &ceu_frame_301,
                        2,
                        ceu_args_301
                    );
                    ceu_assert_pre(ceu_block_320, ceu_acc, "prelude.ceu : (lin 48, col 30) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_311 = ceu_block_320;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_309 = ceu_acc;
                    if (ceu_closure_309.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_311, "prelude.ceu : (lin 49, col 9)", err);
                    }
                    CEU_Frame ceu_frame_309 = { &ceu_closure_309.Dyn->Closure, ceu_block_311 };
                    
                    CEU_Value ceu_args_309[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_309[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_309.closure->proto (
                        &ceu_frame_309,
                        1,
                        ceu_args_309
                    );
                    ceu_assert_pre(ceu_block_311, ceu_acc, "prelude.ceu : (lin 49, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_317 = ceu_block_320;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( powf((id_v1).Number, (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_320, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 47, col 28)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_320);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_321 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_321,
                    0
                );
                ceu_acc = ceu_ret_321;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 47, col 1)"
                        );
                    
                
                    op_asterisk_asterisk = ceu_acc;
                    ceu_gc_inc(op_asterisk_asterisk);
                    ceu_acc = op_asterisk_asterisk;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_387 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v1;
                            CEU_Block* _id_v1_;
                            
                            CEU_Value id_v2;
                            CEU_Block* _id_v2_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_386 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_386 = &_ceu_block_386; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_386,
                                            ceu_hold_chk_set(&ceu_block_386->dyns, ceu_block_386->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 55, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_386,
                                            ceu_hold_chk_set(&ceu_block_386->dyns, ceu_block_386->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 55, col 27)"
                                        );
                                        id_v2 = ceu_args[1];
                                    } else {
                                        id_v2 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_367 = ceu_acc;
                    if (ceu_closure_367.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_386, "prelude.ceu : (lin 56, col 30)", err);
                    }
                    CEU_Frame ceu_frame_367 = { &ceu_closure_367.Dyn->Closure, ceu_block_386 };
                    
                    CEU_Value ceu_args_367[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_347 = ceu_acc;
                    if (ceu_closure_347.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_386, "prelude.ceu : (lin 56, col 18)", err);
                    }
                    CEU_Frame ceu_frame_347 = { &ceu_closure_347.Dyn->Closure, ceu_block_386 };
                    
                    CEU_Value ceu_args_347[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_341 = ceu_acc;
                    if (ceu_closure_341.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_386, "prelude.ceu : (lin 56, col 9)", err);
                    }
                    CEU_Frame ceu_frame_341 = { &ceu_closure_341.Dyn->Closure, ceu_block_386 };
                    
                    CEU_Value ceu_args_341[1];
                    
                    ceu_acc = id_v1;
ceu_args_341[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_341.closure->proto (
                        &ceu_frame_341,
                        1,
                        ceu_args_341
                    );
                    ceu_assert_pre(ceu_block_386, ceu_acc, "prelude.ceu : (lin 56, col 9) : call error");
                } // CALL
                ceu_args_347[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_347[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_347.closure->proto (
                        &ceu_frame_347,
                        2,
                        ceu_args_347
                    );
                    ceu_assert_pre(ceu_block_386, ceu_acc, "prelude.ceu : (lin 56, col 18) : call error");
                } // CALL
                ceu_args_367[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_363 = ceu_acc;
                    if (ceu_closure_363.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_386, "prelude.ceu : (lin 56, col 43)", err);
                    }
                    CEU_Frame ceu_frame_363 = { &ceu_closure_363.Dyn->Closure, ceu_block_386 };
                    
                    CEU_Value ceu_args_363[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_357 = ceu_acc;
                    if (ceu_closure_357.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_386, "prelude.ceu : (lin 56, col 34)", err);
                    }
                    CEU_Frame ceu_frame_357 = { &ceu_closure_357.Dyn->Closure, ceu_block_386 };
                    
                    CEU_Value ceu_args_357[1];
                    
                    ceu_acc = id_v2;
ceu_args_357[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_357.closure->proto (
                        &ceu_frame_357,
                        1,
                        ceu_args_357
                    );
                    ceu_assert_pre(ceu_block_386, ceu_acc, "prelude.ceu : (lin 56, col 34) : call error");
                } // CALL
                ceu_args_363[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_363[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_363.closure->proto (
                        &ceu_frame_363,
                        2,
                        ceu_args_363
                    );
                    ceu_assert_pre(ceu_block_386, ceu_acc, "prelude.ceu : (lin 56, col 43) : call error");
                } // CALL
                ceu_args_367[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_367.closure->proto (
                        &ceu_frame_367,
                        2,
                        ceu_args_367
                    );
                    ceu_assert_pre(ceu_block_386, ceu_acc, "prelude.ceu : (lin 56, col 30) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_377 = ceu_block_386;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_375 = ceu_acc;
                    if (ceu_closure_375.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_377, "prelude.ceu : (lin 57, col 9)", err);
                    }
                    CEU_Frame ceu_frame_375 = { &ceu_closure_375.Dyn->Closure, ceu_block_377 };
                    
                    CEU_Value ceu_args_375[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_375[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_375.closure->proto (
                        &ceu_frame_375,
                        1,
                        ceu_args_375
                    );
                    ceu_assert_pre(ceu_block_377, ceu_acc, "prelude.ceu : (lin 57, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_383 = ceu_block_386;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( ((id_v1).Number / (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_386, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 55, col 27)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_386);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_387 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_387,
                    0
                );
                ceu_acc = ceu_ret_387;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 55, col 1)"
                        );
                    
                
                    op_slash = ceu_acc;
                    ceu_gc_inc(op_slash);
                    ceu_acc = op_slash;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_453 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v1;
                            CEU_Block* _id_v1_;
                            
                            CEU_Value id_v2;
                            CEU_Block* _id_v2_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_452 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_452 = &_ceu_block_452; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_452,
                                            ceu_hold_chk_set(&ceu_block_452->dyns, ceu_block_452->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 63, col 28)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_452,
                                            ceu_hold_chk_set(&ceu_block_452->dyns, ceu_block_452->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 63, col 28)"
                                        );
                                        id_v2 = ceu_args[1];
                                    } else {
                                        id_v2 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_433 = ceu_acc;
                    if (ceu_closure_433.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_452, "prelude.ceu : (lin 64, col 30)", err);
                    }
                    CEU_Frame ceu_frame_433 = { &ceu_closure_433.Dyn->Closure, ceu_block_452 };
                    
                    CEU_Value ceu_args_433[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_413 = ceu_acc;
                    if (ceu_closure_413.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_452, "prelude.ceu : (lin 64, col 18)", err);
                    }
                    CEU_Frame ceu_frame_413 = { &ceu_closure_413.Dyn->Closure, ceu_block_452 };
                    
                    CEU_Value ceu_args_413[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_407 = ceu_acc;
                    if (ceu_closure_407.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_452, "prelude.ceu : (lin 64, col 9)", err);
                    }
                    CEU_Frame ceu_frame_407 = { &ceu_closure_407.Dyn->Closure, ceu_block_452 };
                    
                    CEU_Value ceu_args_407[1];
                    
                    ceu_acc = id_v1;
ceu_args_407[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_407.closure->proto (
                        &ceu_frame_407,
                        1,
                        ceu_args_407
                    );
                    ceu_assert_pre(ceu_block_452, ceu_acc, "prelude.ceu : (lin 64, col 9) : call error");
                } // CALL
                ceu_args_413[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_413[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_413.closure->proto (
                        &ceu_frame_413,
                        2,
                        ceu_args_413
                    );
                    ceu_assert_pre(ceu_block_452, ceu_acc, "prelude.ceu : (lin 64, col 18) : call error");
                } // CALL
                ceu_args_433[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_429 = ceu_acc;
                    if (ceu_closure_429.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_452, "prelude.ceu : (lin 64, col 43)", err);
                    }
                    CEU_Frame ceu_frame_429 = { &ceu_closure_429.Dyn->Closure, ceu_block_452 };
                    
                    CEU_Value ceu_args_429[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_423 = ceu_acc;
                    if (ceu_closure_423.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_452, "prelude.ceu : (lin 64, col 34)", err);
                    }
                    CEU_Frame ceu_frame_423 = { &ceu_closure_423.Dyn->Closure, ceu_block_452 };
                    
                    CEU_Value ceu_args_423[1];
                    
                    ceu_acc = id_v2;
ceu_args_423[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_423.closure->proto (
                        &ceu_frame_423,
                        1,
                        ceu_args_423
                    );
                    ceu_assert_pre(ceu_block_452, ceu_acc, "prelude.ceu : (lin 64, col 34) : call error");
                } // CALL
                ceu_args_429[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_429[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_429.closure->proto (
                        &ceu_frame_429,
                        2,
                        ceu_args_429
                    );
                    ceu_assert_pre(ceu_block_452, ceu_acc, "prelude.ceu : (lin 64, col 43) : call error");
                } // CALL
                ceu_args_433[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_433.closure->proto (
                        &ceu_frame_433,
                        2,
                        ceu_args_433
                    );
                    ceu_assert_pre(ceu_block_452, ceu_acc, "prelude.ceu : (lin 64, col 30) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_443 = ceu_block_452;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_441 = ceu_acc;
                    if (ceu_closure_441.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_443, "prelude.ceu : (lin 65, col 9)", err);
                    }
                    CEU_Frame ceu_frame_441 = { &ceu_closure_441.Dyn->Closure, ceu_block_443 };
                    
                    CEU_Value ceu_args_441[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_441[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_441.closure->proto (
                        &ceu_frame_441,
                        1,
                        ceu_args_441
                    );
                    ceu_assert_pre(ceu_block_443, ceu_acc, "prelude.ceu : (lin 65, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_449 = ceu_block_452;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( powf((id_v1).Number, 1/(id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_452, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 63, col 28)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_452);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_453 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_453,
                    0
                );
                ceu_acc = ceu_ret_453;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 63, col 1)"
                        );
                    
                
                    op_slash_slash = ceu_acc;
                    ceu_gc_inc(op_slash_slash);
                    ceu_acc = op_slash_slash;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_519 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v1;
                            CEU_Block* _id_v1_;
                            
                            CEU_Value id_v2;
                            CEU_Block* _id_v2_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_518 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_518 = &_ceu_block_518; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_518,
                                            ceu_hold_chk_set(&ceu_block_518->dyns, ceu_block_518->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 71, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_518,
                                            ceu_hold_chk_set(&ceu_block_518->dyns, ceu_block_518->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 71, col 27)"
                                        );
                                        id_v2 = ceu_args[1];
                                    } else {
                                        id_v2 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_499 = ceu_acc;
                    if (ceu_closure_499.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_518, "prelude.ceu : (lin 72, col 30)", err);
                    }
                    CEU_Frame ceu_frame_499 = { &ceu_closure_499.Dyn->Closure, ceu_block_518 };
                    
                    CEU_Value ceu_args_499[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_479 = ceu_acc;
                    if (ceu_closure_479.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_518, "prelude.ceu : (lin 72, col 18)", err);
                    }
                    CEU_Frame ceu_frame_479 = { &ceu_closure_479.Dyn->Closure, ceu_block_518 };
                    
                    CEU_Value ceu_args_479[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_473 = ceu_acc;
                    if (ceu_closure_473.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_518, "prelude.ceu : (lin 72, col 9)", err);
                    }
                    CEU_Frame ceu_frame_473 = { &ceu_closure_473.Dyn->Closure, ceu_block_518 };
                    
                    CEU_Value ceu_args_473[1];
                    
                    ceu_acc = id_v1;
ceu_args_473[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_473.closure->proto (
                        &ceu_frame_473,
                        1,
                        ceu_args_473
                    );
                    ceu_assert_pre(ceu_block_518, ceu_acc, "prelude.ceu : (lin 72, col 9) : call error");
                } // CALL
                ceu_args_479[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_479[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_479.closure->proto (
                        &ceu_frame_479,
                        2,
                        ceu_args_479
                    );
                    ceu_assert_pre(ceu_block_518, ceu_acc, "prelude.ceu : (lin 72, col 18) : call error");
                } // CALL
                ceu_args_499[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_495 = ceu_acc;
                    if (ceu_closure_495.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_518, "prelude.ceu : (lin 72, col 43)", err);
                    }
                    CEU_Frame ceu_frame_495 = { &ceu_closure_495.Dyn->Closure, ceu_block_518 };
                    
                    CEU_Value ceu_args_495[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_489 = ceu_acc;
                    if (ceu_closure_489.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_518, "prelude.ceu : (lin 72, col 34)", err);
                    }
                    CEU_Frame ceu_frame_489 = { &ceu_closure_489.Dyn->Closure, ceu_block_518 };
                    
                    CEU_Value ceu_args_489[1];
                    
                    ceu_acc = id_v2;
ceu_args_489[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_489.closure->proto (
                        &ceu_frame_489,
                        1,
                        ceu_args_489
                    );
                    ceu_assert_pre(ceu_block_518, ceu_acc, "prelude.ceu : (lin 72, col 34) : call error");
                } // CALL
                ceu_args_495[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_495[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_495.closure->proto (
                        &ceu_frame_495,
                        2,
                        ceu_args_495
                    );
                    ceu_assert_pre(ceu_block_518, ceu_acc, "prelude.ceu : (lin 72, col 43) : call error");
                } // CALL
                ceu_args_499[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_499.closure->proto (
                        &ceu_frame_499,
                        2,
                        ceu_args_499
                    );
                    ceu_assert_pre(ceu_block_518, ceu_acc, "prelude.ceu : (lin 72, col 30) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_509 = ceu_block_518;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_507 = ceu_acc;
                    if (ceu_closure_507.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_509, "prelude.ceu : (lin 73, col 9)", err);
                    }
                    CEU_Frame ceu_frame_507 = { &ceu_closure_507.Dyn->Closure, ceu_block_509 };
                    
                    CEU_Value ceu_args_507[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_507[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_507.closure->proto (
                        &ceu_frame_507,
                        1,
                        ceu_args_507
                    );
                    ceu_assert_pre(ceu_block_509, ceu_acc, "prelude.ceu : (lin 73, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_515 = ceu_block_518;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( fmod((id_v1).Number, (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_518, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 71, col 27)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_518);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_519 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_519,
                    0
                );
                ceu_acc = ceu_ret_519;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 71, col 1)"
                        );
                    
                
                    op_null = ceu_acc;
                    ceu_gc_inc(op_null);
                    ceu_acc = op_null;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_591 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v1;
                            CEU_Block* _id_v1_;
                            
                            CEU_Value id_v2;
                            CEU_Block* _id_v2_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_590 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_590 = &_ceu_block_590; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_590,
                                            ceu_hold_chk_set(&ceu_block_590->dyns, ceu_block_590->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 81, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_590,
                                            ceu_hold_chk_set(&ceu_block_590->dyns, ceu_block_590->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 81, col 27)"
                                        );
                                        id_v2 = ceu_args[1];
                                    } else {
                                        id_v2 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                            CEU_Value id_t1 = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_540 = ceu_acc;
                    if (ceu_closure_540.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_590, "prelude.ceu : (lin 82, col 14)", err);
                    }
                    CEU_Frame ceu_frame_540 = { &ceu_closure_540.Dyn->Closure, ceu_block_590 };
                    
                    CEU_Value ceu_args_540[1];
                    
                    ceu_acc = id_v1;
ceu_args_540[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_540.closure->proto (
                        &ceu_frame_540,
                        1,
                        ceu_args_540
                    );
                    ceu_assert_pre(ceu_block_590, ceu_acc, "prelude.ceu : (lin 82, col 14) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_590,
                            ceu_hold_chk_set(&ceu_block_590->dyns, ceu_block_590->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 82, col 5)"
                        );
                    
                
                    id_t1 = ceu_acc;
                    ceu_gc_inc(id_t1);
                    ceu_acc = id_t1;
                    
                
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_571 = ceu_acc;
                    if (ceu_closure_571.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_590, "prelude.ceu : (lin 83, col 25)", err);
                    }
                    CEU_Frame ceu_frame_571 = { &ceu_closure_571.Dyn->Closure, ceu_block_590 };
                    
                    CEU_Value ceu_args_571[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_556 = ceu_acc;
                    if (ceu_closure_556.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_590, "prelude.ceu : (lin 83, col 12)", err);
                    }
                    CEU_Frame ceu_frame_556 = { &ceu_closure_556.Dyn->Closure, ceu_block_590 };
                    
                    CEU_Value ceu_args_556[2];
                    
                    ceu_acc = id_t1;
ceu_args_556[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_553 = ceu_acc;
                    if (ceu_closure_553.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_590, "prelude.ceu : (lin 83, col 15)", err);
                    }
                    CEU_Frame ceu_frame_553 = { &ceu_closure_553.Dyn->Closure, ceu_block_590 };
                    
                    CEU_Value ceu_args_553[1];
                    
                    ceu_acc = id_v2;
ceu_args_553[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_553.closure->proto (
                        &ceu_frame_553,
                        1,
                        ceu_args_553
                    );
                    ceu_assert_pre(ceu_block_590, ceu_acc, "prelude.ceu : (lin 83, col 15) : call error");
                } // CALL
                ceu_args_556[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_556.closure->proto (
                        &ceu_frame_556,
                        2,
                        ceu_args_556
                    );
                    ceu_assert_pre(ceu_block_590, ceu_acc, "prelude.ceu : (lin 83, col 12) : call error");
                } // CALL
                ceu_args_571[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_567 = ceu_acc;
                    if (ceu_closure_567.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_590, "prelude.ceu : (lin 83, col 32)", err);
                    }
                    CEU_Frame ceu_frame_567 = { &ceu_closure_567.Dyn->Closure, ceu_block_590 };
                    
                    CEU_Value ceu_args_567[2];
                    
                    ceu_acc = id_t1;
ceu_args_567[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_567[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_567.closure->proto (
                        &ceu_frame_567,
                        2,
                        ceu_args_567
                    );
                    ceu_assert_pre(ceu_block_590, ceu_acc, "prelude.ceu : (lin 83, col 32) : call error");
                } // CALL
                ceu_args_571[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_571.closure->proto (
                        &ceu_frame_571,
                        2,
                        ceu_args_571
                    );
                    ceu_assert_pre(ceu_block_590, ceu_acc, "prelude.ceu : (lin 83, col 25) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_581 = ceu_block_590;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_579 = ceu_acc;
                    if (ceu_closure_579.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_581, "prelude.ceu : (lin 84, col 9)", err);
                    }
                    CEU_Frame ceu_frame_579 = { &ceu_closure_579.Dyn->Closure, ceu_block_581 };
                    
                    CEU_Value ceu_args_579[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_579[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_579.closure->proto (
                        &ceu_frame_579,
                        1,
                        ceu_args_579
                    );
                    ceu_assert_pre(ceu_block_581, ceu_acc, "prelude.ceu : (lin 84, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_587 = ceu_block_590;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_BOOL, {.Bool=( ((id_v1).Number >= (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_590, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 81, col 27)"
                            );
                            
                        
                            if (id_t1.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_t1, (id_t1.Dyn->Any.hld_depth == ceu_block_590->depth));
                            }
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_590);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_591 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_591,
                    0
                );
                ceu_acc = ceu_ret_591;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 81, col 1)"
                        );
                    
                
                    op_greater_equals = ceu_acc;
                    ceu_gc_inc(op_greater_equals);
                    ceu_acc = op_greater_equals;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_663 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v1;
                            CEU_Block* _id_v1_;
                            
                            CEU_Value id_v2;
                            CEU_Block* _id_v2_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_662 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_662 = &_ceu_block_662; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_662,
                                            ceu_hold_chk_set(&ceu_block_662->dyns, ceu_block_662->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 90, col 26)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_662,
                                            ceu_hold_chk_set(&ceu_block_662->dyns, ceu_block_662->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 90, col 26)"
                                        );
                                        id_v2 = ceu_args[1];
                                    } else {
                                        id_v2 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                            CEU_Value id_t1 = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_612 = ceu_acc;
                    if (ceu_closure_612.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_662, "prelude.ceu : (lin 91, col 14)", err);
                    }
                    CEU_Frame ceu_frame_612 = { &ceu_closure_612.Dyn->Closure, ceu_block_662 };
                    
                    CEU_Value ceu_args_612[1];
                    
                    ceu_acc = id_v1;
ceu_args_612[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_612.closure->proto (
                        &ceu_frame_612,
                        1,
                        ceu_args_612
                    );
                    ceu_assert_pre(ceu_block_662, ceu_acc, "prelude.ceu : (lin 91, col 14) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_662,
                            ceu_hold_chk_set(&ceu_block_662->dyns, ceu_block_662->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 91, col 5)"
                        );
                    
                
                    id_t1 = ceu_acc;
                    ceu_gc_inc(id_t1);
                    ceu_acc = id_t1;
                    
                
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_643 = ceu_acc;
                    if (ceu_closure_643.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_662, "prelude.ceu : (lin 92, col 25)", err);
                    }
                    CEU_Frame ceu_frame_643 = { &ceu_closure_643.Dyn->Closure, ceu_block_662 };
                    
                    CEU_Value ceu_args_643[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_628 = ceu_acc;
                    if (ceu_closure_628.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_662, "prelude.ceu : (lin 92, col 12)", err);
                    }
                    CEU_Frame ceu_frame_628 = { &ceu_closure_628.Dyn->Closure, ceu_block_662 };
                    
                    CEU_Value ceu_args_628[2];
                    
                    ceu_acc = id_t1;
ceu_args_628[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_625 = ceu_acc;
                    if (ceu_closure_625.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_662, "prelude.ceu : (lin 92, col 15)", err);
                    }
                    CEU_Frame ceu_frame_625 = { &ceu_closure_625.Dyn->Closure, ceu_block_662 };
                    
                    CEU_Value ceu_args_625[1];
                    
                    ceu_acc = id_v2;
ceu_args_625[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_625.closure->proto (
                        &ceu_frame_625,
                        1,
                        ceu_args_625
                    );
                    ceu_assert_pre(ceu_block_662, ceu_acc, "prelude.ceu : (lin 92, col 15) : call error");
                } // CALL
                ceu_args_628[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_628.closure->proto (
                        &ceu_frame_628,
                        2,
                        ceu_args_628
                    );
                    ceu_assert_pre(ceu_block_662, ceu_acc, "prelude.ceu : (lin 92, col 12) : call error");
                } // CALL
                ceu_args_643[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_639 = ceu_acc;
                    if (ceu_closure_639.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_662, "prelude.ceu : (lin 92, col 32)", err);
                    }
                    CEU_Frame ceu_frame_639 = { &ceu_closure_639.Dyn->Closure, ceu_block_662 };
                    
                    CEU_Value ceu_args_639[2];
                    
                    ceu_acc = id_t1;
ceu_args_639[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_639[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_639.closure->proto (
                        &ceu_frame_639,
                        2,
                        ceu_args_639
                    );
                    ceu_assert_pre(ceu_block_662, ceu_acc, "prelude.ceu : (lin 92, col 32) : call error");
                } // CALL
                ceu_args_643[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_643.closure->proto (
                        &ceu_frame_643,
                        2,
                        ceu_args_643
                    );
                    ceu_assert_pre(ceu_block_662, ceu_acc, "prelude.ceu : (lin 92, col 25) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_653 = ceu_block_662;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_651 = ceu_acc;
                    if (ceu_closure_651.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_653, "prelude.ceu : (lin 93, col 9)", err);
                    }
                    CEU_Frame ceu_frame_651 = { &ceu_closure_651.Dyn->Closure, ceu_block_653 };
                    
                    CEU_Value ceu_args_651[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_651[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_651.closure->proto (
                        &ceu_frame_651,
                        1,
                        ceu_args_651
                    );
                    ceu_assert_pre(ceu_block_653, ceu_acc, "prelude.ceu : (lin 93, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_659 = ceu_block_662;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_BOOL, {.Bool=( ((id_v1).Number > (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_662, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 90, col 26)"
                            );
                            
                        
                            if (id_t1.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_t1, (id_t1.Dyn->Any.hld_depth == ceu_block_662->depth));
                            }
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_662);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_663 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_663,
                    0
                );
                ceu_acc = ceu_ret_663;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 90, col 1)"
                        );
                    
                
                    op_greater = ceu_acc;
                    ceu_gc_inc(op_greater);
                    ceu_acc = op_greater;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_735 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v1;
                            CEU_Block* _id_v1_;
                            
                            CEU_Value id_v2;
                            CEU_Block* _id_v2_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_734 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_734 = &_ceu_block_734; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_734,
                                            ceu_hold_chk_set(&ceu_block_734->dyns, ceu_block_734->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 99, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_734,
                                            ceu_hold_chk_set(&ceu_block_734->dyns, ceu_block_734->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 99, col 27)"
                                        );
                                        id_v2 = ceu_args[1];
                                    } else {
                                        id_v2 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                            CEU_Value id_t1 = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_684 = ceu_acc;
                    if (ceu_closure_684.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_734, "prelude.ceu : (lin 100, col 14)", err);
                    }
                    CEU_Frame ceu_frame_684 = { &ceu_closure_684.Dyn->Closure, ceu_block_734 };
                    
                    CEU_Value ceu_args_684[1];
                    
                    ceu_acc = id_v1;
ceu_args_684[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_684.closure->proto (
                        &ceu_frame_684,
                        1,
                        ceu_args_684
                    );
                    ceu_assert_pre(ceu_block_734, ceu_acc, "prelude.ceu : (lin 100, col 14) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_734,
                            ceu_hold_chk_set(&ceu_block_734->dyns, ceu_block_734->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 100, col 5)"
                        );
                    
                
                    id_t1 = ceu_acc;
                    ceu_gc_inc(id_t1);
                    ceu_acc = id_t1;
                    
                
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_715 = ceu_acc;
                    if (ceu_closure_715.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_734, "prelude.ceu : (lin 101, col 25)", err);
                    }
                    CEU_Frame ceu_frame_715 = { &ceu_closure_715.Dyn->Closure, ceu_block_734 };
                    
                    CEU_Value ceu_args_715[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_700 = ceu_acc;
                    if (ceu_closure_700.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_734, "prelude.ceu : (lin 101, col 12)", err);
                    }
                    CEU_Frame ceu_frame_700 = { &ceu_closure_700.Dyn->Closure, ceu_block_734 };
                    
                    CEU_Value ceu_args_700[2];
                    
                    ceu_acc = id_t1;
ceu_args_700[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_697 = ceu_acc;
                    if (ceu_closure_697.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_734, "prelude.ceu : (lin 101, col 15)", err);
                    }
                    CEU_Frame ceu_frame_697 = { &ceu_closure_697.Dyn->Closure, ceu_block_734 };
                    
                    CEU_Value ceu_args_697[1];
                    
                    ceu_acc = id_v2;
ceu_args_697[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_697.closure->proto (
                        &ceu_frame_697,
                        1,
                        ceu_args_697
                    );
                    ceu_assert_pre(ceu_block_734, ceu_acc, "prelude.ceu : (lin 101, col 15) : call error");
                } // CALL
                ceu_args_700[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_700.closure->proto (
                        &ceu_frame_700,
                        2,
                        ceu_args_700
                    );
                    ceu_assert_pre(ceu_block_734, ceu_acc, "prelude.ceu : (lin 101, col 12) : call error");
                } // CALL
                ceu_args_715[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_711 = ceu_acc;
                    if (ceu_closure_711.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_734, "prelude.ceu : (lin 101, col 32)", err);
                    }
                    CEU_Frame ceu_frame_711 = { &ceu_closure_711.Dyn->Closure, ceu_block_734 };
                    
                    CEU_Value ceu_args_711[2];
                    
                    ceu_acc = id_t1;
ceu_args_711[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_711[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_711.closure->proto (
                        &ceu_frame_711,
                        2,
                        ceu_args_711
                    );
                    ceu_assert_pre(ceu_block_734, ceu_acc, "prelude.ceu : (lin 101, col 32) : call error");
                } // CALL
                ceu_args_715[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_715.closure->proto (
                        &ceu_frame_715,
                        2,
                        ceu_args_715
                    );
                    ceu_assert_pre(ceu_block_734, ceu_acc, "prelude.ceu : (lin 101, col 25) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_725 = ceu_block_734;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_723 = ceu_acc;
                    if (ceu_closure_723.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_725, "prelude.ceu : (lin 102, col 9)", err);
                    }
                    CEU_Frame ceu_frame_723 = { &ceu_closure_723.Dyn->Closure, ceu_block_725 };
                    
                    CEU_Value ceu_args_723[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_723[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_723.closure->proto (
                        &ceu_frame_723,
                        1,
                        ceu_args_723
                    );
                    ceu_assert_pre(ceu_block_725, ceu_acc, "prelude.ceu : (lin 102, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_731 = ceu_block_734;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_BOOL, {.Bool=( ((id_v1).Number <= (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_734, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 99, col 27)"
                            );
                            
                        
                            if (id_t1.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_t1, (id_t1.Dyn->Any.hld_depth == ceu_block_734->depth));
                            }
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_734);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_735 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_735,
                    0
                );
                ceu_acc = ceu_ret_735;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 99, col 1)"
                        );
                    
                
                    op_less_equals = ceu_acc;
                    ceu_gc_inc(op_less_equals);
                    ceu_acc = op_less_equals;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_807 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v1;
                            CEU_Block* _id_v1_;
                            
                            CEU_Value id_v2;
                            CEU_Block* _id_v2_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_806 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_806 = &_ceu_block_806; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_806,
                                            ceu_hold_chk_set(&ceu_block_806->dyns, ceu_block_806->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 108, col 26)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_806,
                                            ceu_hold_chk_set(&ceu_block_806->dyns, ceu_block_806->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 108, col 26)"
                                        );
                                        id_v2 = ceu_args[1];
                                    } else {
                                        id_v2 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                            CEU_Value id_t1 = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_756 = ceu_acc;
                    if (ceu_closure_756.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_806, "prelude.ceu : (lin 109, col 14)", err);
                    }
                    CEU_Frame ceu_frame_756 = { &ceu_closure_756.Dyn->Closure, ceu_block_806 };
                    
                    CEU_Value ceu_args_756[1];
                    
                    ceu_acc = id_v1;
ceu_args_756[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_756.closure->proto (
                        &ceu_frame_756,
                        1,
                        ceu_args_756
                    );
                    ceu_assert_pre(ceu_block_806, ceu_acc, "prelude.ceu : (lin 109, col 14) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_806,
                            ceu_hold_chk_set(&ceu_block_806->dyns, ceu_block_806->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 109, col 5)"
                        );
                    
                
                    id_t1 = ceu_acc;
                    ceu_gc_inc(id_t1);
                    ceu_acc = id_t1;
                    
                
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_787 = ceu_acc;
                    if (ceu_closure_787.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_806, "prelude.ceu : (lin 110, col 25)", err);
                    }
                    CEU_Frame ceu_frame_787 = { &ceu_closure_787.Dyn->Closure, ceu_block_806 };
                    
                    CEU_Value ceu_args_787[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_772 = ceu_acc;
                    if (ceu_closure_772.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_806, "prelude.ceu : (lin 110, col 12)", err);
                    }
                    CEU_Frame ceu_frame_772 = { &ceu_closure_772.Dyn->Closure, ceu_block_806 };
                    
                    CEU_Value ceu_args_772[2];
                    
                    ceu_acc = id_t1;
ceu_args_772[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_769 = ceu_acc;
                    if (ceu_closure_769.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_806, "prelude.ceu : (lin 110, col 15)", err);
                    }
                    CEU_Frame ceu_frame_769 = { &ceu_closure_769.Dyn->Closure, ceu_block_806 };
                    
                    CEU_Value ceu_args_769[1];
                    
                    ceu_acc = id_v2;
ceu_args_769[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_769.closure->proto (
                        &ceu_frame_769,
                        1,
                        ceu_args_769
                    );
                    ceu_assert_pre(ceu_block_806, ceu_acc, "prelude.ceu : (lin 110, col 15) : call error");
                } // CALL
                ceu_args_772[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_772.closure->proto (
                        &ceu_frame_772,
                        2,
                        ceu_args_772
                    );
                    ceu_assert_pre(ceu_block_806, ceu_acc, "prelude.ceu : (lin 110, col 12) : call error");
                } // CALL
                ceu_args_787[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_783 = ceu_acc;
                    if (ceu_closure_783.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_806, "prelude.ceu : (lin 110, col 32)", err);
                    }
                    CEU_Frame ceu_frame_783 = { &ceu_closure_783.Dyn->Closure, ceu_block_806 };
                    
                    CEU_Value ceu_args_783[2];
                    
                    ceu_acc = id_t1;
ceu_args_783[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_783[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_783.closure->proto (
                        &ceu_frame_783,
                        2,
                        ceu_args_783
                    );
                    ceu_assert_pre(ceu_block_806, ceu_acc, "prelude.ceu : (lin 110, col 32) : call error");
                } // CALL
                ceu_args_787[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_787.closure->proto (
                        &ceu_frame_787,
                        2,
                        ceu_args_787
                    );
                    ceu_assert_pre(ceu_block_806, ceu_acc, "prelude.ceu : (lin 110, col 25) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_797 = ceu_block_806;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_795 = ceu_acc;
                    if (ceu_closure_795.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_797, "prelude.ceu : (lin 111, col 9)", err);
                    }
                    CEU_Frame ceu_frame_795 = { &ceu_closure_795.Dyn->Closure, ceu_block_797 };
                    
                    CEU_Value ceu_args_795[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_795[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_795.closure->proto (
                        &ceu_frame_795,
                        1,
                        ceu_args_795
                    );
                    ceu_assert_pre(ceu_block_797, ceu_acc, "prelude.ceu : (lin 111, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_803 = ceu_block_806;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_BOOL, {.Bool=( ((id_v1).Number < (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_806, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 108, col 26)"
                            );
                            
                        
                            if (id_t1.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_t1, (id_t1.Dyn->Any.hld_depth == ceu_block_806->depth));
                            }
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_806);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_807 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_807,
                    0
                );
                ceu_acc = ceu_ret_807;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 108, col 1)"
                        );
                    
                
                    op_less = ceu_acc;
                    ceu_gc_inc(op_less);
                    ceu_acc = op_less;
                    
                
                // DCL | 
                
                
                    
                    ceu_gc_inc(id_to_dash_string);
                    ceu_acc = id_to_dash_string;
                    
                
                { // SET | 
                     // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_951 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v;
                            CEU_Block* _id_v_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_950 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_950 = &_ceu_block_950; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_950,
                                            ceu_hold_chk_set(&ceu_block_950->dyns, ceu_block_950->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 120, col 26)"
                                        );
                                        id_v = ceu_args[0];
                                    } else {
                                        id_v = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_834 = ceu_acc;
                    if (ceu_closure_834.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_950, "prelude.ceu : (lin 121, col 16)", err);
                    }
                    CEU_Frame ceu_frame_834 = { &ceu_closure_834.Dyn->Closure, ceu_block_950 };
                    
                    CEU_Value ceu_args_834[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_828 = ceu_acc;
                    if (ceu_closure_828.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_950, "prelude.ceu : (lin 121, col 8)", err);
                    }
                    CEU_Frame ceu_frame_828 = { &ceu_closure_828.Dyn->Closure, ceu_block_950 };
                    
                    CEU_Value ceu_args_828[1];
                    
                    ceu_acc = id_v;
ceu_args_828[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_828.closure->proto (
                        &ceu_frame_828,
                        1,
                        ceu_args_828
                    );
                    ceu_assert_pre(ceu_block_950, ceu_acc, "prelude.ceu : (lin 121, col 8) : call error");
                } // CALL
                ceu_args_834[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_pointer} });ceu_args_834[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_834.closure->proto (
                        &ceu_frame_834,
                        2,
                        ceu_args_834
                    );
                    ceu_assert_pre(ceu_block_950, ceu_acc, "prelude.ceu : (lin 121, col 16) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_910 = (CEU_Block) { (ceu_block_950->depth + 1), 0, {.block=ceu_block_950}, NULL };
                        CEU_Block* ceu_block_910 = &_ceu_block_910; 
                        
                        
                        
                            CEU_Value id_i = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_n = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_str = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        ceu_assert_pre(
                            ceu_block_910,
                            ceu_hold_chk_set(&ceu_block_910->dyns, ceu_block_910->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 122, col 9)"
                        );
                    
                
                    id_i = ceu_acc;
                    ceu_gc_inc(id_i);
                    ceu_acc = id_i;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( strlen((id_v).Pointer))} });
                        ceu_assert_pre(
                            ceu_block_910,
                            ceu_hold_chk_set(&ceu_block_910->dyns, ceu_block_910->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 123, col 9)"
                        );
                    
                
                    id_n = ceu_acc;
                    ceu_gc_inc(id_n);
                    ceu_acc = id_n;
                    
                
                // DCL | 
                
                { // VECTOR | 
                    CEU_Value ceu_vec_852 = ceu_vector_create(ceu_block_910);
                    
                    ceu_acc = ceu_vec_852;
                }
                
                        ceu_assert_pre(
                            ceu_block_910,
                            ceu_hold_chk_set(&ceu_block_910->dyns, ceu_block_910->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 124, col 9)"
                        );
                    
                
                    id_str = ceu_acc;
                    ceu_gc_inc(id_str);
                    ceu_acc = id_str;
                    
                
                    CEU_Block* ceu_block_901 = ceu_block_910;
                    // >>> block
                    
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_867 = ceu_acc;
                    if (ceu_closure_867.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_901, "prelude.ceu : (lin 126, col 25)", err);
                    }
                    CEU_Frame ceu_frame_867 = { &ceu_closure_867.Dyn->Closure, ceu_block_901 };
                    
                    CEU_Value ceu_args_867[2];
                    
                    ceu_acc = id_i;
ceu_args_867[0] = ceu_acc;
ceu_acc = id_n;
ceu_args_867[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_867.closure->proto (
                        &ceu_frame_867,
                        2,
                        ceu_args_867
                    );
                    ceu_assert_pre(ceu_block_901, ceu_acc, "prelude.ceu : (lin 126, col 25) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                { // SET | 
                    ceu_acc = ((CEU_Value){ CEU_VALUE_CHAR, {.Char=( ((char*)(id_v).Pointer)[(int)(id_i).Number])} });
                    CEU_Value ceu_set_885 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        
                { // CALL | 
                    ceu_acc = op_hash;

                    CEU_Value ceu_closure_879 = ceu_acc;
                    if (ceu_closure_879.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_901, "prelude.ceu : (lin 127, col 21)", err);
                    }
                    CEU_Frame ceu_frame_879 = { &ceu_closure_879.Dyn->Closure, ceu_block_901 };
                    
                    CEU_Value ceu_args_879[1];
                    
                    ceu_acc = id_str;
ceu_args_879[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_879.closure->proto (
                        &ceu_frame_879,
                        1,
                        ceu_args_879
                    );
                    ceu_assert_pre(ceu_block_901, ceu_acc, "prelude.ceu : (lin 127, col 21) : call error");
                } // CALL
                
                        CEU_Value ceu_idx_881 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_str;

                    ceu_assert_pre(ceu_block_901, ceu_col_check(ceu_acc, ceu_idx_881), "prelude.ceu : (lin 127, col 17)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_881.Number, (ceu_set_885));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_881.Number, (ceu_set_885));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_881, (ceu_set_885));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_901, ok, "prelude.ceu : (lin 127, col 17)");
                        
                }
                
                    ceu_acc = ceu_set_885;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_897 = ceu_acc;
                    if (ceu_closure_897.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_901, "prelude.ceu : (lin 128, col 23)", err);
                    }
                    CEU_Frame ceu_frame_897 = { &ceu_closure_897.Dyn->Closure, ceu_block_901 };
                    
                    CEU_Value ceu_args_897[2];
                    
                    ceu_acc = id_i;
ceu_args_897[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_897[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_897.closure->proto (
                        &ceu_frame_897,
                        2,
                        ceu_args_897
                    );
                    ceu_assert_pre(ceu_block_901, ceu_acc, "prelude.ceu : (lin 128, col 23) : call error");
                } // CALL
                
                    CEU_Value ceu_set_898 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_901,
                                ceu_hold_chk_set(&ceu_block_910->dyns, ceu_block_910->depth, CEU_HOLD_MUTAB, (ceu_set_898), 0, "set error"),
                                "prelude.ceu : (lin 128, col 17)"
                            );
                            ceu_gc_inc((ceu_set_898));
                            ceu_gc_dec(id_i, 1);
                            id_i = (ceu_set_898);
                        }
                        
                    ceu_acc = ceu_set_898;
                }
                
                    }
                
                    // <<< block
                    
                        { // ACC - DROP
                            CEU_Value ceu_906 = id_str;
                            CEU_Frame ceu_frame_906 = { NULL, ceu_block_910 };
                            ceu_assert_pre(ceu_block_910, ceu_drop_f(&ceu_frame_906, 1, &ceu_906), "prelude.ceu : (lin 130, col 14)");
                            ceu_gc_dec(ceu_906, 0);
                            id_str = (CEU_Value) { CEU_VALUE_NIL };
                            ceu_acc = ceu_906;
                        }
                        
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_910, 
                                ceu_hold_chk_set(&ceu_block_950->dyns, ceu_block_950->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 121, col 28)"
                            );
                            
                        
                            if (id_i.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_i, (id_i.Dyn->Any.hld_depth == ceu_block_910->depth));
                            }
                        
                            if (id_n.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_n, (id_n.Dyn->Any.hld_depth == ceu_block_910->depth));
                            }
                        
                            if (id_str.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_str, (id_str.Dyn->Any.hld_depth == ceu_block_910->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_910);
                    }
                    
                    } else {
                        
                    CEU_Block* ceu_block_947 = ceu_block_950;
                    // >>> block
                    
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_926 = ceu_acc;
                    if (ceu_closure_926.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_947, "prelude.ceu : (lin 132, col 20)", err);
                    }
                    CEU_Frame ceu_frame_926 = { &ceu_closure_926.Dyn->Closure, ceu_block_947 };
                    
                    CEU_Value ceu_args_926[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_920 = ceu_acc;
                    if (ceu_closure_920.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_947, "prelude.ceu : (lin 132, col 12)", err);
                    }
                    CEU_Frame ceu_frame_920 = { &ceu_closure_920.Dyn->Closure, ceu_block_947 };
                    
                    CEU_Value ceu_args_920[1];
                    
                    ceu_acc = id_v;
ceu_args_920[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_920.closure->proto (
                        &ceu_frame_920,
                        1,
                        ceu_args_920
                    );
                    ceu_assert_pre(ceu_block_947, ceu_acc, "prelude.ceu : (lin 132, col 12) : call error");
                } // CALL
                ceu_args_926[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_926[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_926.closure->proto (
                        &ceu_frame_926,
                        2,
                        ceu_args_926
                    );
                    ceu_assert_pre(ceu_block_947, ceu_acc, "prelude.ceu : (lin 132, col 20) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_938 = ceu_block_947;
                    // >>> block
                    
            static char str[255];
            snprintf(str, 255, "%g", (id_v).Number);
            
ceu_acc = ((CEU_Value){ CEU_VALUE_NIL });
                { // CALL | 
                    ceu_acc = id_to_dash_string;

                    CEU_Value ceu_closure_936 = ceu_acc;
                    if (ceu_closure_936.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_938, "prelude.ceu : (lin 137, col 13)", err);
                    }
                    CEU_Frame ceu_frame_936 = { &ceu_closure_936.Dyn->Closure, ceu_block_938 };
                    
                    CEU_Value ceu_args_936[1];
                    
                    ceu_acc = ((CEU_Value){ CEU_VALUE_POINTER, {.Pointer=( str)} });ceu_args_936[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_936.closure->proto (
                        &ceu_frame_936,
                        1,
                        ceu_args_936
                    );
                    ceu_assert_pre(ceu_block_938, ceu_acc, "prelude.ceu : (lin 137, col 13) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_944 = ceu_block_947;
                    // >>> block
                    ceu_acc = id_v;

                    // <<< block
                    
                    }
                }
                
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_950, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 120, col 26)"
                            );
                            
                        
                        
                            
                                if (id_v.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_950);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_951 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_951,
                    0
                );
                ceu_acc = ceu_ret_951;
                
                // UPVALS
                
                
                    CEU_Value ceu_set_952 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_2005,
                                ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, (ceu_set_952), 0, "set error"),
                                "prelude.ceu : (lin 120, col 5)"
                            );
                            ceu_gc_inc((ceu_set_952));
                            ceu_gc_dec(id_to_dash_string, 1);
                            id_to_dash_string = (ceu_set_952);
                        }
                        
                    ceu_acc = ceu_set_952;
                }
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_1065 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v;
                            CEU_Block* _id_v_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_1064 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_1064 = &_ceu_block_1064; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1064,
                                            ceu_hold_chk_set(&ceu_block_1064->dyns, ceu_block_1064->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 144, col 26)"
                                        );
                                        id_v = ceu_args[0];
                                    } else {
                                        id_v = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_974 = ceu_acc;
                    if (ceu_closure_974.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1064, "prelude.ceu : (lin 145, col 16)", err);
                    }
                    CEU_Frame ceu_frame_974 = { &ceu_closure_974.Dyn->Closure, ceu_block_1064 };
                    
                    CEU_Value ceu_args_974[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_968 = ceu_acc;
                    if (ceu_closure_968.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1064, "prelude.ceu : (lin 145, col 8)", err);
                    }
                    CEU_Frame ceu_frame_968 = { &ceu_closure_968.Dyn->Closure, ceu_block_1064 };
                    
                    CEU_Value ceu_args_968[1];
                    
                    ceu_acc = id_v;
ceu_args_968[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_968.closure->proto (
                        &ceu_frame_968,
                        1,
                        ceu_args_968
                    );
                    ceu_assert_pre(ceu_block_1064, ceu_acc, "prelude.ceu : (lin 145, col 8) : call error");
                } // CALL
                ceu_args_974[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_tag} });ceu_args_974[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_974.closure->proto (
                        &ceu_frame_974,
                        2,
                        ceu_args_974
                    );
                    ceu_assert_pre(ceu_block_1064, ceu_acc, "prelude.ceu : (lin 145, col 16) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_979 = ceu_block_1064;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( (id_v).Tag)} });
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_1061 = ceu_block_1064;
                    // >>> block
                    
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_995 = ceu_acc;
                    if (ceu_closure_995.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1061, "prelude.ceu : (lin 148, col 20)", err);
                    }
                    CEU_Frame ceu_frame_995 = { &ceu_closure_995.Dyn->Closure, ceu_block_1061 };
                    
                    CEU_Value ceu_args_995[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_989 = ceu_acc;
                    if (ceu_closure_989.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1061, "prelude.ceu : (lin 148, col 12)", err);
                    }
                    CEU_Frame ceu_frame_989 = { &ceu_closure_989.Dyn->Closure, ceu_block_1061 };
                    
                    CEU_Value ceu_args_989[1];
                    
                    ceu_acc = id_v;
ceu_args_989[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_989.closure->proto (
                        &ceu_frame_989,
                        1,
                        ceu_args_989
                    );
                    ceu_assert_pre(ceu_block_1061, ceu_acc, "prelude.ceu : (lin 148, col 12) : call error");
                } // CALL
                ceu_args_995[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vector} });ceu_args_995[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_995.closure->proto (
                        &ceu_frame_995,
                        2,
                        ceu_args_995
                    );
                    ceu_assert_pre(ceu_block_1061, ceu_acc, "prelude.ceu : (lin 148, col 20) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_1052 = ceu_block_1061;
                    // >>> block
                    
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_greater;

                    CEU_Value ceu_closure_1009 = ceu_acc;
                    if (ceu_closure_1009.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1052, "prelude.ceu : (lin 149, col 19)", err);
                    }
                    CEU_Frame ceu_frame_1009 = { &ceu_closure_1009.Dyn->Closure, ceu_block_1052 };
                    
                    CEU_Value ceu_args_1009[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_hash;

                    CEU_Value ceu_closure_1003 = ceu_acc;
                    if (ceu_closure_1003.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1052, "prelude.ceu : (lin 149, col 16)", err);
                    }
                    CEU_Frame ceu_frame_1003 = { &ceu_closure_1003.Dyn->Closure, ceu_block_1052 };
                    
                    CEU_Value ceu_args_1003[1];
                    
                    ceu_acc = id_v;
ceu_args_1003[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1003.closure->proto (
                        &ceu_frame_1003,
                        1,
                        ceu_args_1003
                    );
                    ceu_assert_pre(ceu_block_1052, ceu_acc, "prelude.ceu : (lin 149, col 16) : call error");
                } // CALL
                ceu_args_1009[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });ceu_args_1009[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1009.closure->proto (
                        &ceu_frame_1009,
                        2,
                        ceu_args_1009
                    );
                    ceu_assert_pre(ceu_block_1052, ceu_acc, "prelude.ceu : (lin 149, col 19) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_1043 = ceu_block_1052;
                    // >>> block
                    
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_1029 = ceu_acc;
                    if (ceu_closure_1029.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1043, "prelude.ceu : (lin 150, col 31)", err);
                    }
                    CEU_Frame ceu_frame_1029 = { &ceu_closure_1029.Dyn->Closure, ceu_block_1043 };
                    
                    CEU_Value ceu_args_1029[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_1023 = ceu_acc;
                    if (ceu_closure_1023.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1043, "prelude.ceu : (lin 150, col 20)", err);
                    }
                    CEU_Frame ceu_frame_1023 = { &ceu_closure_1023.Dyn->Closure, ceu_block_1043 };
                    
                    CEU_Value ceu_args_1023[1];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        CEU_Value ceu_idx_1021 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_v;

                    ceu_assert_pre(ceu_block_1043, ceu_col_check(ceu_acc, ceu_idx_1021), "prelude.ceu : (lin 150, col 25)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1021.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1043, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1021.Number), "prelude.ceu : (lin 150, col 25)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1021);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1023[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1023.closure->proto (
                        &ceu_frame_1023,
                        1,
                        ceu_args_1023
                    );
                    ceu_assert_pre(ceu_block_1043, ceu_acc, "prelude.ceu : (lin 150, col 20) : call error");
                } // CALL
                ceu_args_1029[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_char} });ceu_args_1029[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1029.closure->proto (
                        &ceu_frame_1029,
                        2,
                        ceu_args_1029
                    );
                    ceu_assert_pre(ceu_block_1043, ceu_acc, "prelude.ceu : (lin 150, col 31) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_1034 = ceu_block_1043;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( atoi((id_v).Dyn->Vector.buf))} });
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_1040 = ceu_block_1043;
                    // >>> block
                    ceu_acc = id_v;

                    // <<< block
                    
                    }
                }
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_1049 = ceu_block_1052;
                    // >>> block
                    ceu_acc = id_v;

                    // <<< block
                    
                    }
                }
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_1058 = ceu_block_1061;
                    // >>> block
                    ceu_acc = id_v;

                    // <<< block
                    
                    }
                }
                
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1064, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 144, col 26)"
                            );
                            
                        
                        
                            
                                if (id_v.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_1064);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_1065 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_1065,
                    0
                );
                ceu_acc = ceu_ret_1065;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 144, col 1)"
                        );
                    
                
                    id_to_dash_number = ceu_acc;
                    ceu_gc_inc(id_to_dash_number);
                    ceu_acc = id_to_dash_number;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_1108 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v;
                            CEU_Block* _id_v_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_1107 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_1107 = &_ceu_block_1107; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1107,
                                            ceu_hold_chk_set(&ceu_block_1107->dyns, ceu_block_1107->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 164, col 23)"
                                        );
                                        id_v = ceu_args[0];
                                    } else {
                                        id_v = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_1088 = ceu_acc;
                    if (ceu_closure_1088.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1107, "prelude.ceu : (lin 165, col 16)", err);
                    }
                    CEU_Frame ceu_frame_1088 = { &ceu_closure_1088.Dyn->Closure, ceu_block_1107 };
                    
                    CEU_Value ceu_args_1088[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_1082 = ceu_acc;
                    if (ceu_closure_1082.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1107, "prelude.ceu : (lin 165, col 8)", err);
                    }
                    CEU_Frame ceu_frame_1082 = { &ceu_closure_1082.Dyn->Closure, ceu_block_1107 };
                    
                    CEU_Value ceu_args_1082[1];
                    
                    ceu_acc = id_v;
ceu_args_1082[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1082.closure->proto (
                        &ceu_frame_1082,
                        1,
                        ceu_args_1082
                    );
                    ceu_assert_pre(ceu_block_1107, ceu_acc, "prelude.ceu : (lin 165, col 8) : call error");
                } // CALL
                ceu_args_1088[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_string} });ceu_args_1088[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1088.closure->proto (
                        &ceu_frame_1088,
                        2,
                        ceu_args_1088
                    );
                    ceu_assert_pre(ceu_block_1107, ceu_acc, "prelude.ceu : (lin 165, col 16) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_1098 = ceu_block_1107;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_string_dash_to_dash_tag;

                    CEU_Value ceu_closure_1096 = ceu_acc;
                    if (ceu_closure_1096.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1098, "prelude.ceu : (lin 166, col 9)", err);
                    }
                    CEU_Frame ceu_frame_1096 = { &ceu_closure_1096.Dyn->Closure, ceu_block_1098 };
                    
                    CEU_Value ceu_args_1096[1];
                    
                    ceu_acc = id_v;
ceu_args_1096[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1096.closure->proto (
                        &ceu_frame_1096,
                        1,
                        ceu_args_1096
                    );
                    ceu_assert_pre(ceu_block_1098, ceu_acc, "prelude.ceu : (lin 166, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_1104 = ceu_block_1107;
                    // >>> block
                    ceu_acc = id_v;

                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1107, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 164, col 23)"
                            );
                            
                        
                        
                            
                                if (id_v.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_1107);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_1108 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_1108,
                    0
                );
                ceu_acc = ceu_ret_1108;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 164, col 1)"
                        );
                    
                
                    id_to_dash_tag = ceu_acc;
                    ceu_gc_inc(id_to_dash_tag);
                    ceu_acc = id_to_dash_tag;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=12} });
                        ceu_assert_pre(
                            ceu_block_2005,
                            ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 1, col 1)"
                        );
                    
                
                    id_n = ceu_acc;
                    ceu_gc_inc(id_n);
                    ceu_acc = id_n;
                    
                
                // DCL | 
                
                
                    
                    ceu_gc_inc(id_fannkuch);
                    ceu_acc = id_fannkuch;
                    
                
                { // SET | 
                     // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_1875 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_N;
                            CEU_Block* _id_N_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_1874 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_1874 = &_ceu_block_1874; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1874,
                                            ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "fannkuch.ceu : (lin 4, col 25)"
                                        );
                                        id_N = ceu_args[0];
                                    } else {
                                        id_N = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                            CEU_Value id_initial_perm = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_i = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_perm = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_count = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_r = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_perm_count = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_max_flips = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_checksum = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                
                { // VECTOR | 
                    CEU_Value ceu_vec_1132 = ceu_vector_create(ceu_block_1874);
                    
                    ceu_acc = ceu_vec_1132;
                }
                
                        ceu_assert_pre(
                            ceu_block_1874,
                            ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 5, col 5)"
                        );
                    
                
                    id_initial_perm = ceu_acc;
                    ceu_gc_inc(id_initial_perm);
                    ceu_acc = id_initial_perm;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });
                        ceu_assert_pre(
                            ceu_block_1874,
                            ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 6, col 5)"
                        );
                    
                
                    id_i = ceu_acc;
                    ceu_gc_inc(id_i);
                    ceu_acc = id_i;
                    
                
                    CEU_Block* ceu_block_1187 = ceu_block_1874;
                    // >>> block
                    
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_greater;

                    CEU_Value ceu_closure_1153 = ceu_acc;
                    if (ceu_closure_1153.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1187, "fannkuch.ceu : (lin 8, col 21)", err);
                    }
                    CEU_Frame ceu_frame_1153 = { &ceu_closure_1153.Dyn->Closure, ceu_block_1187 };
                    
                    CEU_Value ceu_args_1153[2];
                    
                    ceu_acc = id_i;
ceu_args_1153[0] = ceu_acc;
ceu_acc = id_N;
ceu_args_1153[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1153.closure->proto (
                        &ceu_frame_1153,
                        2,
                        ceu_args_1153
                    );
                    ceu_assert_pre(ceu_block_1187, ceu_acc, "fannkuch.ceu : (lin 8, col 21) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                { // SET | 
                    ceu_acc = id_i;

                    CEU_Value ceu_set_1171 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        
                { // CALL | 
                    ceu_acc = op_hash;

                    CEU_Value ceu_closure_1165 = ceu_acc;
                    if (ceu_closure_1165.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1187, "fannkuch.ceu : (lin 9, col 26)", err);
                    }
                    CEU_Frame ceu_frame_1165 = { &ceu_closure_1165.Dyn->Closure, ceu_block_1187 };
                    
                    CEU_Value ceu_args_1165[1];
                    
                    ceu_acc = id_initial_perm;
ceu_args_1165[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1165.closure->proto (
                        &ceu_frame_1165,
                        1,
                        ceu_args_1165
                    );
                    ceu_assert_pre(ceu_block_1187, ceu_acc, "fannkuch.ceu : (lin 9, col 26) : call error");
                } // CALL
                
                        CEU_Value ceu_idx_1167 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_initial_perm;

                    ceu_assert_pre(ceu_block_1187, ceu_col_check(ceu_acc, ceu_idx_1167), "fannkuch.ceu : (lin 9, col 13)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1167.Number, (ceu_set_1171));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1167.Number, (ceu_set_1171));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1167, (ceu_set_1171));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1187, ok, "fannkuch.ceu : (lin 9, col 13)");
                        
                }
                
                    ceu_acc = ceu_set_1171;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1183 = ceu_acc;
                    if (ceu_closure_1183.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1187, "fannkuch.ceu : (lin 10, col 19)", err);
                    }
                    CEU_Frame ceu_frame_1183 = { &ceu_closure_1183.Dyn->Closure, ceu_block_1187 };
                    
                    CEU_Value ceu_args_1183[2];
                    
                    ceu_acc = id_i;
ceu_args_1183[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1183[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1183.closure->proto (
                        &ceu_frame_1183,
                        2,
                        ceu_args_1183
                    );
                    ceu_assert_pre(ceu_block_1187, ceu_acc, "fannkuch.ceu : (lin 10, col 19) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1184 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1187,
                                ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, (ceu_set_1184), 0, "set error"),
                                "fannkuch.ceu : (lin 10, col 13)"
                            );
                            ceu_gc_inc((ceu_set_1184));
                            ceu_gc_dec(id_i, 1);
                            id_i = (ceu_set_1184);
                        }
                        
                    ceu_acc = ceu_set_1184;
                }
                
                    }
                
                    // <<< block
                    
                { // CALL | 
                    ceu_acc = id_println;

                    CEU_Value ceu_closure_1195 = ceu_acc;
                    if (ceu_closure_1195.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1874, "fannkuch.ceu : (lin 12, col 5)", err);
                    }
                    CEU_Frame ceu_frame_1195 = { &ceu_closure_1195.Dyn->Closure, ceu_block_1874 };
                    
                    CEU_Value ceu_args_1195[1];
                    
                    ceu_acc = id_initial_perm;
ceu_args_1195[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1195.closure->proto (
                        &ceu_frame_1195,
                        1,
                        ceu_args_1195
                    );
                    ceu_assert_pre(ceu_block_1874, ceu_acc, "fannkuch.ceu : (lin 12, col 5) : call error");
                } // CALL
                
                // DCL | 
                
                { // VECTOR | 
                    CEU_Value ceu_vec_1200 = ceu_vector_create(ceu_block_1874);
                    
                    ceu_acc = ceu_vec_1200;
                }
                
                        ceu_assert_pre(
                            ceu_block_1874,
                            ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 14, col 5)"
                        );
                    
                
                    id_perm = ceu_acc;
                    ceu_gc_inc(id_perm);
                    ceu_acc = id_perm;
                    
                
                // DCL | 
                
                { // VECTOR | 
                    CEU_Value ceu_vec_1207 = ceu_vector_create(ceu_block_1874);
                    
                    ceu_acc = ceu_vec_1207;
                }
                
                        ceu_assert_pre(
                            ceu_block_1874,
                            ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 16, col 5)"
                        );
                    
                
                    id_count = ceu_acc;
                    ceu_gc_inc(id_count);
                    ceu_acc = id_count;
                    
                
                    CEU_Block* ceu_block_1245 = ceu_block_1874;
                    // >>> block
                    
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_1225 = ceu_acc;
                    if (ceu_closure_1225.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1245, "fannkuch.ceu : (lin 18, col 25)", err);
                    }
                    CEU_Frame ceu_frame_1225 = { &ceu_closure_1225.Dyn->Closure, ceu_block_1245 };
                    
                    CEU_Value ceu_args_1225[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_hash;

                    CEU_Value ceu_closure_1219 = ceu_acc;
                    if (ceu_closure_1219.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1245, "fannkuch.ceu : (lin 18, col 18)", err);
                    }
                    CEU_Frame ceu_frame_1219 = { &ceu_closure_1219.Dyn->Closure, ceu_block_1245 };
                    
                    CEU_Value ceu_args_1219[1];
                    
                    ceu_acc = id_count;
ceu_args_1219[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1219.closure->proto (
                        &ceu_frame_1219,
                        1,
                        ceu_args_1219
                    );
                    ceu_assert_pre(ceu_block_1245, ceu_acc, "fannkuch.ceu : (lin 18, col 18) : call error");
                } // CALL
                ceu_args_1225[0] = ceu_acc;
ceu_acc = id_N;
ceu_args_1225[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1225.closure->proto (
                        &ceu_frame_1225,
                        2,
                        ceu_args_1225
                    );
                    ceu_assert_pre(ceu_block_1245, ceu_acc, "fannkuch.ceu : (lin 18, col 25) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                { // SET | 
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                    CEU_Value ceu_set_1242 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        
                { // CALL | 
                    ceu_acc = op_hash;

                    CEU_Value ceu_closure_1236 = ceu_acc;
                    if (ceu_closure_1236.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1245, "fannkuch.ceu : (lin 19, col 19)", err);
                    }
                    CEU_Frame ceu_frame_1236 = { &ceu_closure_1236.Dyn->Closure, ceu_block_1245 };
                    
                    CEU_Value ceu_args_1236[1];
                    
                    ceu_acc = id_count;
ceu_args_1236[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1236.closure->proto (
                        &ceu_frame_1236,
                        1,
                        ceu_args_1236
                    );
                    ceu_assert_pre(ceu_block_1245, ceu_acc, "fannkuch.ceu : (lin 19, col 19) : call error");
                } // CALL
                
                        CEU_Value ceu_idx_1238 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_count;

                    ceu_assert_pre(ceu_block_1245, ceu_col_check(ceu_acc, ceu_idx_1238), "fannkuch.ceu : (lin 19, col 13)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1238.Number, (ceu_set_1242));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1238.Number, (ceu_set_1242));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1238, (ceu_set_1242));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1245, ok, "fannkuch.ceu : (lin 19, col 13)");
                        
                }
                
                    ceu_acc = ceu_set_1242;
                }
                
                    }
                
                    // <<< block
                    
                { // CALL | 
                    ceu_acc = id_println;

                    CEU_Value ceu_closure_1253 = ceu_acc;
                    if (ceu_closure_1253.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1874, "fannkuch.ceu : (lin 21, col 5)", err);
                    }
                    CEU_Frame ceu_frame_1253 = { &ceu_closure_1253.Dyn->Closure, ceu_block_1874 };
                    
                    CEU_Value ceu_args_1253[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='a'} });ceu_args_1253[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1253.closure->proto (
                        &ceu_frame_1253,
                        1,
                        ceu_args_1253
                    );
                    ceu_assert_pre(ceu_block_1874, ceu_acc, "fannkuch.ceu : (lin 21, col 5) : call error");
                } // CALL
                
                // DCL | 
                ceu_acc = id_N;

                        ceu_assert_pre(
                            ceu_block_1874,
                            ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 23, col 5)"
                        );
                    
                
                    id_r = ceu_acc;
                    ceu_gc_inc(id_r);
                    ceu_acc = id_r;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        ceu_assert_pre(
                            ceu_block_1874,
                            ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 25, col 5)"
                        );
                    
                
                    id_perm_count = ceu_acc;
                    ceu_gc_inc(id_perm_count);
                    ceu_acc = id_perm_count;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        ceu_assert_pre(
                            ceu_block_1874,
                            ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 26, col 5)"
                        );
                    
                
                    id_max_flips = ceu_acc;
                    ceu_gc_inc(id_max_flips);
                    ceu_acc = id_max_flips;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        ceu_assert_pre(
                            ceu_block_1874,
                            ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 27, col 5)"
                        );
                    
                
                    id_checksum = ceu_acc;
                    ceu_gc_inc(id_checksum);
                    ceu_acc = id_checksum;
                    
                
                { // CALL | 
                    ceu_acc = id_println;

                    CEU_Value ceu_closure_1284 = ceu_acc;
                    if (ceu_closure_1284.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1874, "fannkuch.ceu : (lin 29, col 5)", err);
                    }
                    CEU_Frame ceu_frame_1284 = { &ceu_closure_1284.Dyn->Closure, ceu_block_1874 };
                    
                    CEU_Value ceu_args_1284[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='b'} });ceu_args_1284[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1284.closure->proto (
                        &ceu_frame_1284,
                        1,
                        ceu_args_1284
                    );
                    ceu_assert_pre(ceu_block_1874, ceu_acc, "fannkuch.ceu : (lin 29, col 5) : call error");
                } // CALL
                
                    { // BLOCK | 
                        CEU_Block _ceu_block_1871 = (CEU_Block) { (ceu_block_1874->depth + 1), 0, {.block=ceu_block_1874}, NULL };
                        CEU_Block* ceu_block_1871 = &_ceu_block_1871; 
                        
                        
                        
                            CEU_Value id_shouldBreak = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                    while (1) { // LOOP | 
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_1594 = (CEU_Block) { (ceu_block_1871->depth + 1), 0, {.block=ceu_block_1871}, NULL };
                        CEU_Block* ceu_block_1594 = &_ceu_block_1594; 
                        
                        
                        
                            CEU_Value id_flips_count = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_h = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                { // SET | 
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                    CEU_Value ceu_set_1295 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1594,
                                ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, (ceu_set_1295), 0, "set error"),
                                "fannkuch.ceu : (lin 32, col 17)"
                            );
                            ceu_gc_inc((ceu_set_1295));
                            ceu_gc_dec(id_i, 1);
                            id_i = (ceu_set_1295);
                        }
                        
                    ceu_acc = ceu_set_1295;
                }
                
                    CEU_Block* ceu_block_1355 = ceu_block_1594;
                    // >>> block
                    
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_greater;

                    CEU_Value ceu_closure_1316 = ceu_acc;
                    if (ceu_closure_1316.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1355, "fannkuch.ceu : (lin 34, col 29)", err);
                    }
                    CEU_Frame ceu_frame_1316 = { &ceu_closure_1316.Dyn->Closure, ceu_block_1355 };
                    
                    CEU_Value ceu_args_1316[2];
                    
                    ceu_acc = id_i;
ceu_args_1316[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1312 = ceu_acc;
                    if (ceu_closure_1312.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1355, "fannkuch.ceu : (lin 34, col 34)", err);
                    }
                    CEU_Frame ceu_frame_1312 = { &ceu_closure_1312.Dyn->Closure, ceu_block_1355 };
                    
                    CEU_Value ceu_args_1312[2];
                    
                    ceu_acc = id_N;
ceu_args_1312[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1312[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1312.closure->proto (
                        &ceu_frame_1312,
                        2,
                        ceu_args_1312
                    );
                    ceu_assert_pre(ceu_block_1355, ceu_acc, "fannkuch.ceu : (lin 34, col 34) : call error");
                } // CALL
                ceu_args_1316[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1316.closure->proto (
                        &ceu_frame_1316,
                        2,
                        ceu_args_1316
                    );
                    ceu_assert_pre(ceu_block_1355, ceu_acc, "fannkuch.ceu : (lin 34, col 29) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                { // SET | 
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_i;

                        CEU_Value ceu_idx_1338 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_initial_perm;

                    ceu_assert_pre(ceu_block_1355, ceu_col_check(ceu_acc, ceu_idx_1338), "fannkuch.ceu : (lin 35, col 35)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1338.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1355, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1338.Number), "fannkuch.ceu : (lin 35, col 35)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1338);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                    CEU_Value ceu_set_1339 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        
                { // CALL | 
                    ceu_acc = op_hash;

                    CEU_Value ceu_closure_1328 = ceu_acc;
                    if (ceu_closure_1328.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1355, "fannkuch.ceu : (lin 35, col 26)", err);
                    }
                    CEU_Frame ceu_frame_1328 = { &ceu_closure_1328.Dyn->Closure, ceu_block_1355 };
                    
                    CEU_Value ceu_args_1328[1];
                    
                    ceu_acc = id_perm;
ceu_args_1328[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1328.closure->proto (
                        &ceu_frame_1328,
                        1,
                        ceu_args_1328
                    );
                    ceu_assert_pre(ceu_block_1355, ceu_acc, "fannkuch.ceu : (lin 35, col 26) : call error");
                } // CALL
                
                        CEU_Value ceu_idx_1330 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_perm;

                    ceu_assert_pre(ceu_block_1355, ceu_col_check(ceu_acc, ceu_idx_1330), "fannkuch.ceu : (lin 35, col 21)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1330.Number, (ceu_set_1339));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1330.Number, (ceu_set_1339));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1330, (ceu_set_1339));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1355, ok, "fannkuch.ceu : (lin 35, col 21)");
                        
                }
                
                    ceu_acc = ceu_set_1339;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1351 = ceu_acc;
                    if (ceu_closure_1351.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1355, "fannkuch.ceu : (lin 36, col 27)", err);
                    }
                    CEU_Frame ceu_frame_1351 = { &ceu_closure_1351.Dyn->Closure, ceu_block_1355 };
                    
                    CEU_Value ceu_args_1351[2];
                    
                    ceu_acc = id_i;
ceu_args_1351[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1351[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1351.closure->proto (
                        &ceu_frame_1351,
                        2,
                        ceu_args_1351
                    );
                    ceu_assert_pre(ceu_block_1355, ceu_acc, "fannkuch.ceu : (lin 36, col 27) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1352 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1355,
                                ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, (ceu_set_1352), 0, "set error"),
                                "fannkuch.ceu : (lin 36, col 21)"
                            );
                            ceu_gc_inc((ceu_set_1352));
                            ceu_gc_dec(id_i, 1);
                            id_i = (ceu_set_1352);
                        }
                        
                    ceu_acc = ceu_set_1352;
                }
                
                    }
                
                    // <<< block
                    
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        ceu_assert_pre(
                            ceu_block_1594,
                            ceu_hold_chk_set(&ceu_block_1594->dyns, ceu_block_1594->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 39, col 13)"
                        );
                    
                
                    id_flips_count = ceu_acc;
                    ceu_gc_inc(id_flips_count);
                    ceu_acc = id_flips_count;
                    
                
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        CEU_Value ceu_idx_1372 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_perm;

                    ceu_assert_pre(ceu_block_1594, ceu_col_check(ceu_acc, ceu_idx_1372), "fannkuch.ceu : (lin 40, col 21)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1372.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1594, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1372.Number), "fannkuch.ceu : (lin 40, col 21)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1372);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_1594,
                            ceu_hold_chk_set(&ceu_block_1594->dyns, ceu_block_1594->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 40, col 13)"
                        );
                    
                
                    id_h = ceu_acc;
                    ceu_gc_inc(id_h);
                    ceu_acc = id_h;
                    
                
                    { // BLOCK | 
                        CEU_Block _ceu_block_1516 = (CEU_Block) { (ceu_block_1594->depth + 1), 0, {.block=ceu_block_1594}, NULL };
                        CEU_Block* ceu_block_1516 = &_ceu_block_1516; 
                        
                        
                        
                            CEU_Value id_j = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_less_equals;

                    CEU_Value ceu_closure_1385 = ceu_acc;
                    if (ceu_closure_1385.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1516, "fannkuch.ceu : (lin 42, col 28)", err);
                    }
                    CEU_Frame ceu_frame_1385 = { &ceu_closure_1385.Dyn->Closure, ceu_block_1516 };
                    
                    CEU_Value ceu_args_1385[2];
                    
                    ceu_acc = id_h;
ceu_args_1385[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1385[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1385.closure->proto (
                        &ceu_frame_1385,
                        2,
                        ceu_args_1385
                    );
                    ceu_assert_pre(ceu_block_1516, ceu_acc, "fannkuch.ceu : (lin 42, col 28) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                { // SET | 
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                    CEU_Value ceu_set_1393 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1516,
                                ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, (ceu_set_1393), 0, "set error"),
                                "fannkuch.ceu : (lin 44, col 21)"
                            );
                            ceu_gc_inc((ceu_set_1393));
                            ceu_gc_dec(id_i, 1);
                            id_i = (ceu_set_1393);
                        }
                        
                    ceu_acc = ceu_set_1393;
                }
                
                // DCL | 
                ceu_acc = id_i;

                        ceu_assert_pre(
                            ceu_block_1516,
                            ceu_hold_chk_set(&ceu_block_1516->dyns, ceu_block_1516->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 45, col 17)"
                        );
                    
                
                    id_j = ceu_acc;
                    ceu_gc_inc(id_j);
                    ceu_acc = id_j;
                    
                
                    { // BLOCK | 
                        CEU_Block _ceu_block_1487 = (CEU_Block) { (ceu_block_1516->depth + 1), 0, {.block=ceu_block_1516}, NULL };
                        CEU_Block* ceu_block_1487 = &_ceu_block_1487; 
                        
                        
                        
                            CEU_Value id_a = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_b = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_greater_equals;

                    CEU_Value ceu_closure_1411 = ceu_acc;
                    if (ceu_closure_1411.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1487, "fannkuch.ceu : (lin 47, col 32)", err);
                    }
                    CEU_Frame ceu_frame_1411 = { &ceu_closure_1411.Dyn->Closure, ceu_block_1487 };
                    
                    CEU_Value ceu_args_1411[2];
                    
                    ceu_acc = id_i;
ceu_args_1411[0] = ceu_acc;
ceu_acc = id_j;
ceu_args_1411[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1411.closure->proto (
                        &ceu_frame_1411,
                        2,
                        ceu_args_1411
                    );
                    ceu_assert_pre(ceu_block_1487, ceu_acc, "fannkuch.ceu : (lin 47, col 32) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_i;

                        CEU_Value ceu_idx_1422 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_perm;

                    ceu_assert_pre(ceu_block_1487, ceu_col_check(ceu_acc, ceu_idx_1422), "fannkuch.ceu : (lin 49, col 29)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1422.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1487, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1422.Number), "fannkuch.ceu : (lin 49, col 29)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1422);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_1487,
                            ceu_hold_chk_set(&ceu_block_1487->dyns, ceu_block_1487->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 49, col 21)"
                        );
                    
                
                    id_a = ceu_acc;
                    ceu_gc_inc(id_a);
                    ceu_acc = id_a;
                    
                
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_j;

                        CEU_Value ceu_idx_1433 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_perm;

                    ceu_assert_pre(ceu_block_1487, ceu_col_check(ceu_acc, ceu_idx_1433), "fannkuch.ceu : (lin 50, col 29)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1433.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1487, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1433.Number), "fannkuch.ceu : (lin 50, col 29)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1433);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_1487,
                            ceu_hold_chk_set(&ceu_block_1487->dyns, ceu_block_1487->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 50, col 21)"
                        );
                    
                
                    id_b = ceu_acc;
                    ceu_gc_inc(id_b);
                    ceu_acc = id_b;
                    
                
                { // SET | 
                    ceu_acc = id_b;

                    CEU_Value ceu_set_1446 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_i;

                        CEU_Value ceu_idx_1442 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_perm;

                    ceu_assert_pre(ceu_block_1487, ceu_col_check(ceu_acc, ceu_idx_1442), "fannkuch.ceu : (lin 52, col 25)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1442.Number, (ceu_set_1446));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1442.Number, (ceu_set_1446));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1442, (ceu_set_1446));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1487, ok, "fannkuch.ceu : (lin 52, col 25)");
                        
                }
                
                    ceu_acc = ceu_set_1446;
                }
                
                { // SET | 
                    ceu_acc = id_a;

                    CEU_Value ceu_set_1458 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_j;

                        CEU_Value ceu_idx_1454 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_perm;

                    ceu_assert_pre(ceu_block_1487, ceu_col_check(ceu_acc, ceu_idx_1454), "fannkuch.ceu : (lin 53, col 25)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1454.Number, (ceu_set_1458));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1454.Number, (ceu_set_1458));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1454, (ceu_set_1458));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1487, ok, "fannkuch.ceu : (lin 53, col 25)");
                        
                }
                
                    ceu_acc = ceu_set_1458;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1470 = ceu_acc;
                    if (ceu_closure_1470.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1487, "fannkuch.ceu : (lin 55, col 31)", err);
                    }
                    CEU_Frame ceu_frame_1470 = { &ceu_closure_1470.Dyn->Closure, ceu_block_1487 };
                    
                    CEU_Value ceu_args_1470[2];
                    
                    ceu_acc = id_i;
ceu_args_1470[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1470[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1470.closure->proto (
                        &ceu_frame_1470,
                        2,
                        ceu_args_1470
                    );
                    ceu_assert_pre(ceu_block_1487, ceu_acc, "fannkuch.ceu : (lin 55, col 31) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1471 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1487,
                                ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, (ceu_set_1471), 0, "set error"),
                                "fannkuch.ceu : (lin 55, col 25)"
                            );
                            ceu_gc_inc((ceu_set_1471));
                            ceu_gc_dec(id_i, 1);
                            id_i = (ceu_set_1471);
                        }
                        
                    ceu_acc = ceu_set_1471;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1483 = ceu_acc;
                    if (ceu_closure_1483.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1487, "fannkuch.ceu : (lin 56, col 31)", err);
                    }
                    CEU_Frame ceu_frame_1483 = { &ceu_closure_1483.Dyn->Closure, ceu_block_1487 };
                    
                    CEU_Value ceu_args_1483[2];
                    
                    ceu_acc = id_j;
ceu_args_1483[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1483[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1483.closure->proto (
                        &ceu_frame_1483,
                        2,
                        ceu_args_1483
                    );
                    ceu_assert_pre(ceu_block_1487, ceu_acc, "fannkuch.ceu : (lin 56, col 31) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1484 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1487,
                                ceu_hold_chk_set(&ceu_block_1516->dyns, ceu_block_1516->depth, CEU_HOLD_MUTAB, (ceu_set_1484), 0, "set error"),
                                "fannkuch.ceu : (lin 56, col 25)"
                            );
                            ceu_gc_inc((ceu_set_1484));
                            ceu_gc_dec(id_j, 1);
                            id_j = (ceu_set_1484);
                        }
                        
                    ceu_acc = ceu_set_1484;
                }
                
                    }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1487, 
                                ceu_hold_chk_set(&ceu_block_1516->dyns, ceu_block_1516->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "fannkuch.ceu : (lin 46, col 17)"
                            );
                            
                        
                            if (id_a.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_a, (id_a.Dyn->Any.hld_depth == ceu_block_1487->depth));
                            }
                        
                            if (id_b.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_b, (id_b.Dyn->Any.hld_depth == ceu_block_1487->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_1487);
                    }
                    
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1500 = ceu_acc;
                    if (ceu_closure_1500.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1516, "fannkuch.ceu : (lin 59, col 47)", err);
                    }
                    CEU_Frame ceu_frame_1500 = { &ceu_closure_1500.Dyn->Closure, ceu_block_1516 };
                    
                    CEU_Value ceu_args_1500[2];
                    
                    ceu_acc = id_flips_count;
ceu_args_1500[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1500[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1500.closure->proto (
                        &ceu_frame_1500,
                        2,
                        ceu_args_1500
                    );
                    ceu_assert_pre(ceu_block_1516, ceu_acc, "fannkuch.ceu : (lin 59, col 47) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1501 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1516,
                                ceu_hold_chk_set(&ceu_block_1594->dyns, ceu_block_1594->depth, CEU_HOLD_MUTAB, (ceu_set_1501), 0, "set error"),
                                "fannkuch.ceu : (lin 59, col 21)"
                            );
                            ceu_gc_inc((ceu_set_1501));
                            ceu_gc_dec(id_flips_count, 1);
                            id_flips_count = (ceu_set_1501);
                        }
                        
                    ceu_acc = ceu_set_1501;
                }
                
                { // SET | 
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        CEU_Value ceu_idx_1512 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_initial_perm;

                    ceu_assert_pre(ceu_block_1516, ceu_col_check(ceu_acc, ceu_idx_1512), "fannkuch.ceu : (lin 60, col 25)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1512.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1516, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1512.Number), "fannkuch.ceu : (lin 60, col 25)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1512);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                    CEU_Value ceu_set_1513 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1516,
                                ceu_hold_chk_set(&ceu_block_1594->dyns, ceu_block_1594->depth, CEU_HOLD_MUTAB, (ceu_set_1513), 0, "set error"),
                                "fannkuch.ceu : (lin 60, col 21)"
                            );
                            ceu_gc_inc((ceu_set_1513));
                            ceu_gc_dec(id_h, 1);
                            id_h = (ceu_set_1513);
                        }
                        
                    ceu_acc = ceu_set_1513;
                }
                
                    }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1516, 
                                ceu_hold_chk_set(&ceu_block_1594->dyns, ceu_block_1594->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "fannkuch.ceu : (lin 41, col 13)"
                            );
                            
                        
                            if (id_j.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_j, (id_j.Dyn->Any.hld_depth == ceu_block_1516->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_1516);
                    }
                    
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_greater;

                    CEU_Value ceu_closure_1526 = ceu_acc;
                    if (ceu_closure_1526.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1594, "fannkuch.ceu : (lin 63, col 28)", err);
                    }
                    CEU_Frame ceu_frame_1526 = { &ceu_closure_1526.Dyn->Closure, ceu_block_1594 };
                    
                    CEU_Value ceu_args_1526[2];
                    
                    ceu_acc = id_flips_count;
ceu_args_1526[0] = ceu_acc;
ceu_acc = id_max_flips;
ceu_args_1526[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1526.closure->proto (
                        &ceu_frame_1526,
                        2,
                        ceu_args_1526
                    );
                    ceu_assert_pre(ceu_block_1594, ceu_acc, "fannkuch.ceu : (lin 63, col 28) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_1536 = ceu_block_1594;
                    // >>> block
                    
                { // SET | 
                    ceu_acc = id_flips_count;

                    CEU_Value ceu_set_1534 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1536,
                                ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, (ceu_set_1534), 0, "set error"),
                                "fannkuch.ceu : (lin 64, col 21)"
                            );
                            ceu_gc_inc((ceu_set_1534));
                            ceu_gc_dec(id_max_flips, 1);
                            id_max_flips = (ceu_set_1534);
                        }
                        
                    ceu_acc = ceu_set_1534;
                }
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_1539 = ceu_block_1594;
                    // >>> block
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NIL });
                    // <<< block
                    
                    }
                }
                
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_1557 = ceu_acc;
                    if (ceu_closure_1557.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1594, "fannkuch.ceu : (lin 67, col 33)", err);
                    }
                    CEU_Frame ceu_frame_1557 = { &ceu_closure_1557.Dyn->Closure, ceu_block_1594 };
                    
                    CEU_Value ceu_args_1557[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_null;

                    CEU_Value ceu_closure_1550 = ceu_acc;
                    if (ceu_closure_1550.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1594, "fannkuch.ceu : (lin 67, col 28)", err);
                    }
                    CEU_Frame ceu_frame_1550 = { &ceu_closure_1550.Dyn->Closure, ceu_block_1594 };
                    
                    CEU_Value ceu_args_1550[2];
                    
                    ceu_acc = id_perm_count;
ceu_args_1550[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=2} });ceu_args_1550[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1550.closure->proto (
                        &ceu_frame_1550,
                        2,
                        ceu_args_1550
                    );
                    ceu_assert_pre(ceu_block_1594, ceu_acc, "fannkuch.ceu : (lin 67, col 28) : call error");
                } // CALL
                ceu_args_1557[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });ceu_args_1557[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1557.closure->proto (
                        &ceu_frame_1557,
                        2,
                        ceu_args_1557
                    );
                    ceu_assert_pre(ceu_block_1594, ceu_acc, "fannkuch.ceu : (lin 67, col 33) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_1573 = ceu_block_1594;
                    // >>> block
                    
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1570 = ceu_acc;
                    if (ceu_closure_1570.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1573, "fannkuch.ceu : (lin 68, col 41)", err);
                    }
                    CEU_Frame ceu_frame_1570 = { &ceu_closure_1570.Dyn->Closure, ceu_block_1573 };
                    
                    CEU_Value ceu_args_1570[2];
                    
                    ceu_acc = id_checksum;
ceu_args_1570[0] = ceu_acc;
ceu_acc = id_flips_count;
ceu_args_1570[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1570.closure->proto (
                        &ceu_frame_1570,
                        2,
                        ceu_args_1570
                    );
                    ceu_assert_pre(ceu_block_1573, ceu_acc, "fannkuch.ceu : (lin 68, col 41) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1571 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1573,
                                ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, (ceu_set_1571), 0, "set error"),
                                "fannkuch.ceu : (lin 68, col 21)"
                            );
                            ceu_gc_inc((ceu_set_1571));
                            ceu_gc_dec(id_checksum, 1);
                            id_checksum = (ceu_set_1571);
                        }
                        
                    ceu_acc = ceu_set_1571;
                }
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_1590 = ceu_block_1594;
                    // >>> block
                    
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1587 = ceu_acc;
                    if (ceu_closure_1587.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1590, "fannkuch.ceu : (lin 70, col 41)", err);
                    }
                    CEU_Frame ceu_frame_1587 = { &ceu_closure_1587.Dyn->Closure, ceu_block_1590 };
                    
                    CEU_Value ceu_args_1587[2];
                    
                    ceu_acc = id_checksum;
ceu_args_1587[0] = ceu_acc;
ceu_acc = id_flips_count;
ceu_args_1587[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1587.closure->proto (
                        &ceu_frame_1587,
                        2,
                        ceu_args_1587
                    );
                    ceu_assert_pre(ceu_block_1590, ceu_acc, "fannkuch.ceu : (lin 70, col 41) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1588 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1590,
                                ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, (ceu_set_1588), 0, "set error"),
                                "fannkuch.ceu : (lin 70, col 21)"
                            );
                            ceu_gc_inc((ceu_set_1588));
                            ceu_gc_dec(id_checksum, 1);
                            id_checksum = (ceu_set_1588);
                        }
                        
                    ceu_acc = ceu_set_1588;
                }
                
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1594, 
                                ceu_hold_chk_set(&ceu_block_1871->dyns, ceu_block_1871->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "fannkuch.ceu : (lin 31, col 9)"
                            );
                            
                        
                            if (id_flips_count.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_flips_count, (id_flips_count.Dyn->Any.hld_depth == ceu_block_1594->depth));
                            }
                        
                            if (id_h.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_h, (id_h.Dyn->Any.hld_depth == ceu_block_1594->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_1594);
                    }
                    
                { // CALL | 
                    ceu_acc = id_print;

                    CEU_Value ceu_closure_1601 = ceu_acc;
                    if (ceu_closure_1601.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1871, "fannkuch.ceu : (lin 74, col 9)", err);
                    }
                    CEU_Frame ceu_frame_1601 = { &ceu_closure_1601.Dyn->Closure, ceu_block_1871 };
                    
                    CEU_Value ceu_args_1601[1];
                    
                    ceu_acc = id_count;
ceu_args_1601[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1601.closure->proto (
                        &ceu_frame_1601,
                        1,
                        ceu_args_1601
                    );
                    ceu_assert_pre(ceu_block_1871, ceu_acc, "fannkuch.ceu : (lin 74, col 9) : call error");
                } // CALL
                
                    CEU_Block* ceu_block_1648 = ceu_block_1871;
                    // >>> block
                    
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_less_equals;

                    CEU_Value ceu_closure_1613 = ceu_acc;
                    if (ceu_closure_1613.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1648, "fannkuch.ceu : (lin 76, col 24)", err);
                    }
                    CEU_Frame ceu_frame_1613 = { &ceu_closure_1613.Dyn->Closure, ceu_block_1648 };
                    
                    CEU_Value ceu_args_1613[2];
                    
                    ceu_acc = id_r;
ceu_args_1613[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1613[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1613.closure->proto (
                        &ceu_frame_1613,
                        2,
                        ceu_args_1613
                    );
                    ceu_assert_pre(ceu_block_1648, ceu_acc, "fannkuch.ceu : (lin 76, col 24) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                { // SET | 
                    ceu_acc = id_r;

                    CEU_Value ceu_set_1632 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1626 = ceu_acc;
                    if (ceu_closure_1626.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1648, "fannkuch.ceu : (lin 77, col 24)", err);
                    }
                    CEU_Frame ceu_frame_1626 = { &ceu_closure_1626.Dyn->Closure, ceu_block_1648 };
                    
                    CEU_Value ceu_args_1626[2];
                    
                    ceu_acc = id_r;
ceu_args_1626[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1626[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1626.closure->proto (
                        &ceu_frame_1626,
                        2,
                        ceu_args_1626
                    );
                    ceu_assert_pre(ceu_block_1648, ceu_acc, "fannkuch.ceu : (lin 77, col 24) : call error");
                } // CALL
                
                        CEU_Value ceu_idx_1628 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_count;

                    ceu_assert_pre(ceu_block_1648, ceu_col_check(ceu_acc, ceu_idx_1628), "fannkuch.ceu : (lin 77, col 17)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1628.Number, (ceu_set_1632));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1628.Number, (ceu_set_1632));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1628, (ceu_set_1632));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1648, ok, "fannkuch.ceu : (lin 77, col 17)");
                        
                }
                
                    ceu_acc = ceu_set_1632;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1644 = ceu_acc;
                    if (ceu_closure_1644.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1648, "fannkuch.ceu : (lin 78, col 23)", err);
                    }
                    CEU_Frame ceu_frame_1644 = { &ceu_closure_1644.Dyn->Closure, ceu_block_1648 };
                    
                    CEU_Value ceu_args_1644[2];
                    
                    ceu_acc = id_r;
ceu_args_1644[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1644[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1644.closure->proto (
                        &ceu_frame_1644,
                        2,
                        ceu_args_1644
                    );
                    ceu_assert_pre(ceu_block_1648, ceu_acc, "fannkuch.ceu : (lin 78, col 23) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1645 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1648,
                                ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, (ceu_set_1645), 0, "set error"),
                                "fannkuch.ceu : (lin 78, col 17)"
                            );
                            ceu_gc_inc((ceu_set_1645));
                            ceu_gc_dec(id_r, 1);
                            id_r = (ceu_set_1645);
                        }
                        
                    ceu_acc = ceu_set_1645;
                }
                
                    }
                
                    // <<< block
                    
                { // CALL | 
                    ceu_acc = id_print;

                    CEU_Value ceu_closure_1656 = ceu_acc;
                    if (ceu_closure_1656.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1871, "fannkuch.ceu : (lin 80, col 9)", err);
                    }
                    CEU_Frame ceu_frame_1656 = { &ceu_closure_1656.Dyn->Closure, ceu_block_1871 };
                    
                    CEU_Value ceu_args_1656[1];
                    
                    ceu_acc = id_count;
ceu_args_1656[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1656.closure->proto (
                        &ceu_frame_1656,
                        1,
                        ceu_args_1656
                    );
                    ceu_assert_pre(ceu_block_1871, ceu_acc, "fannkuch.ceu : (lin 80, col 9) : call error");
                } // CALL
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_BOOL, {.Bool=0} });
                        ceu_assert_pre(
                            ceu_block_1871,
                            ceu_hold_chk_set(&ceu_block_1871->dyns, ceu_block_1871->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 82, col 9)"
                        );
                    
                
                    id_shouldBreak = ceu_acc;
                    ceu_gc_inc(id_shouldBreak);
                    ceu_acc = id_shouldBreak;
                    
                
                    CEU_Block* ceu_block_1830 = ceu_block_1871;
                    // >>> block
                    
                    while (1) { // LOOP | 
                        
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_1673 = ceu_acc;
                    if (ceu_closure_1673.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1830, "fannkuch.ceu : (lin 85, col 18)", err);
                    }
                    CEU_Frame ceu_frame_1673 = { &ceu_closure_1673.Dyn->Closure, ceu_block_1830 };
                    
                    CEU_Value ceu_args_1673[2];
                    
                    ceu_acc = id_r;
ceu_args_1673[0] = ceu_acc;
ceu_acc = id_N;
ceu_args_1673[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1673.closure->proto (
                        &ceu_frame_1673,
                        2,
                        ceu_args_1673
                    );
                    ceu_assert_pre(ceu_block_1830, ceu_acc, "fannkuch.ceu : (lin 85, col 18) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_1818 = (CEU_Block) { (ceu_block_1830->depth + 1), 0, {.block=ceu_block_1830}, NULL };
                        CEU_Block* ceu_block_1818 = &_ceu_block_1818; 
                        
                        
                        
                            CEU_Value id_tmp = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_r1 = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        CEU_Value ceu_idx_1684 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_initial_perm;

                    ceu_assert_pre(ceu_block_1818, ceu_col_check(ceu_acc, ceu_idx_1684), "fannkuch.ceu : (lin 86, col 27)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1684.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1818, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1684.Number), "fannkuch.ceu : (lin 86, col 27)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1684);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_1818,
                            ceu_hold_chk_set(&ceu_block_1818->dyns, ceu_block_1818->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 86, col 17)"
                        );
                    
                
                    id_tmp = ceu_acc;
                    ceu_gc_inc(id_tmp);
                    ceu_acc = id_tmp;
                    
                
                { // SET | 
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                    CEU_Value ceu_set_1692 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1818,
                                ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, (ceu_set_1692), 0, "set error"),
                                "fannkuch.ceu : (lin 88, col 21)"
                            );
                            ceu_gc_inc((ceu_set_1692));
                            ceu_gc_dec(id_i, 1);
                            id_i = (ceu_set_1692);
                        }
                        
                    ceu_acc = ceu_set_1692;
                }
                
                    CEU_Block* ceu_block_1744 = ceu_block_1818;
                    // >>> block
                    
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_greater_equals;

                    CEU_Value ceu_closure_1704 = ceu_acc;
                    if (ceu_closure_1704.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1744, "fannkuch.ceu : (lin 90, col 32)", err);
                    }
                    CEU_Frame ceu_frame_1704 = { &ceu_closure_1704.Dyn->Closure, ceu_block_1744 };
                    
                    CEU_Value ceu_args_1704[2];
                    
                    ceu_acc = id_i;
ceu_args_1704[0] = ceu_acc;
ceu_acc = id_r;
ceu_args_1704[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1704.closure->proto (
                        &ceu_frame_1704,
                        2,
                        ceu_args_1704
                    );
                    ceu_assert_pre(ceu_block_1744, ceu_acc, "fannkuch.ceu : (lin 90, col 32) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                { // SET | 
                    
                { // INDEX | 
                    // IDX
                    
                        
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1725 = ceu_acc;
                    if (ceu_closure_1725.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1744, "fannkuch.ceu : (lin 91, col 58)", err);
                    }
                    CEU_Frame ceu_frame_1725 = { &ceu_closure_1725.Dyn->Closure, ceu_block_1744 };
                    
                    CEU_Value ceu_args_1725[2];
                    
                    ceu_acc = id_i;
ceu_args_1725[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1725[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1725.closure->proto (
                        &ceu_frame_1725,
                        2,
                        ceu_args_1725
                    );
                    ceu_assert_pre(ceu_block_1744, ceu_acc, "fannkuch.ceu : (lin 91, col 58) : call error");
                } // CALL
                
                        CEU_Value ceu_idx_1727 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_initial_perm;

                    ceu_assert_pre(ceu_block_1744, ceu_col_check(ceu_acc, ceu_idx_1727), "fannkuch.ceu : (lin 91, col 43)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1727.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1744, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1727.Number), "fannkuch.ceu : (lin 91, col 43)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1727);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                    CEU_Value ceu_set_1728 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_i;

                        CEU_Value ceu_idx_1713 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_initial_perm;

                    ceu_assert_pre(ceu_block_1744, ceu_col_check(ceu_acc, ceu_idx_1713), "fannkuch.ceu : (lin 91, col 25)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1713.Number, (ceu_set_1728));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1713.Number, (ceu_set_1728));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1713, (ceu_set_1728));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1744, ok, "fannkuch.ceu : (lin 91, col 25)");
                        
                }
                
                    ceu_acc = ceu_set_1728;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1740 = ceu_acc;
                    if (ceu_closure_1740.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1744, "fannkuch.ceu : (lin 92, col 31)", err);
                    }
                    CEU_Frame ceu_frame_1740 = { &ceu_closure_1740.Dyn->Closure, ceu_block_1744 };
                    
                    CEU_Value ceu_args_1740[2];
                    
                    ceu_acc = id_i;
ceu_args_1740[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1740[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1740.closure->proto (
                        &ceu_frame_1740,
                        2,
                        ceu_args_1740
                    );
                    ceu_assert_pre(ceu_block_1744, ceu_acc, "fannkuch.ceu : (lin 92, col 31) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1741 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1744,
                                ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, (ceu_set_1741), 0, "set error"),
                                "fannkuch.ceu : (lin 92, col 25)"
                            );
                            ceu_gc_inc((ceu_set_1741));
                            ceu_gc_dec(id_i, 1);
                            id_i = (ceu_set_1741);
                        }
                        
                    ceu_acc = ceu_set_1741;
                }
                
                    }
                
                    // <<< block
                    
                { // SET | 
                    ceu_acc = id_tmp;

                    CEU_Value ceu_set_1763 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1757 = ceu_acc;
                    if (ceu_closure_1757.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1818, "fannkuch.ceu : (lin 94, col 36)", err);
                    }
                    CEU_Frame ceu_frame_1757 = { &ceu_closure_1757.Dyn->Closure, ceu_block_1818 };
                    
                    CEU_Value ceu_args_1757[2];
                    
                    ceu_acc = id_r;
ceu_args_1757[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1757[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1757.closure->proto (
                        &ceu_frame_1757,
                        2,
                        ceu_args_1757
                    );
                    ceu_assert_pre(ceu_block_1818, ceu_acc, "fannkuch.ceu : (lin 94, col 36) : call error");
                } // CALL
                
                        CEU_Value ceu_idx_1759 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_initial_perm;

                    ceu_assert_pre(ceu_block_1818, ceu_col_check(ceu_acc, ceu_idx_1759), "fannkuch.ceu : (lin 94, col 21)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1759.Number, (ceu_set_1763));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1759.Number, (ceu_set_1763));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1759, (ceu_set_1763));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1818, ok, "fannkuch.ceu : (lin 94, col 21)");
                        
                }
                
                    ceu_acc = ceu_set_1763;
                }
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1774 = ceu_acc;
                    if (ceu_closure_1774.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1818, "fannkuch.ceu : (lin 96, col 28)", err);
                    }
                    CEU_Frame ceu_frame_1774 = { &ceu_closure_1774.Dyn->Closure, ceu_block_1818 };
                    
                    CEU_Value ceu_args_1774[2];
                    
                    ceu_acc = id_r;
ceu_args_1774[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1774[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1774.closure->proto (
                        &ceu_frame_1774,
                        2,
                        ceu_args_1774
                    );
                    ceu_assert_pre(ceu_block_1818, ceu_acc, "fannkuch.ceu : (lin 96, col 28) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_1818,
                            ceu_hold_chk_set(&ceu_block_1818->dyns, ceu_block_1818->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 96, col 17)"
                        );
                    
                
                    id_r1 = ceu_acc;
                    ceu_gc_inc(id_r1);
                    ceu_acc = id_r1;
                    
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1797 = ceu_acc;
                    if (ceu_closure_1797.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1818, "fannkuch.ceu : (lin 97, col 43)", err);
                    }
                    CEU_Frame ceu_frame_1797 = { &ceu_closure_1797.Dyn->Closure, ceu_block_1818 };
                    
                    CEU_Value ceu_args_1797[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_r1;

                        CEU_Value ceu_idx_1791 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_count;

                    ceu_assert_pre(ceu_block_1818, ceu_col_check(ceu_acc, ceu_idx_1791), "fannkuch.ceu : (lin 97, col 33)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1791.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1818, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1791.Number), "fannkuch.ceu : (lin 97, col 33)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1791);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1797[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1797[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1797.closure->proto (
                        &ceu_frame_1797,
                        2,
                        ceu_args_1797
                    );
                    ceu_assert_pre(ceu_block_1818, ceu_acc, "fannkuch.ceu : (lin 97, col 43) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1798 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_r1;

                        CEU_Value ceu_idx_1783 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_count;

                    ceu_assert_pre(ceu_block_1818, ceu_col_check(ceu_acc, ceu_idx_1783), "fannkuch.ceu : (lin 97, col 21)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1783.Number, (ceu_set_1798));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1783.Number, (ceu_set_1798));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1783, (ceu_set_1798));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1818, ok, "fannkuch.ceu : (lin 97, col 21)");
                        
                }
                
                    ceu_acc = ceu_set_1798;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_greater;

                    CEU_Value ceu_closure_1815 = ceu_acc;
                    if (ceu_closure_1815.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1818, "fannkuch.ceu : (lin 98, col 45)", err);
                    }
                    CEU_Frame ceu_frame_1815 = { &ceu_closure_1815.Dyn->Closure, ceu_block_1818 };
                    
                    CEU_Value ceu_args_1815[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_r1;

                        CEU_Value ceu_idx_1809 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_count;

                    ceu_assert_pre(ceu_block_1818, ceu_col_check(ceu_acc, ceu_idx_1809), "fannkuch.ceu : (lin 98, col 35)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1809.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1818, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1809.Number), "fannkuch.ceu : (lin 98, col 35)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1809);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1815[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });ceu_args_1815[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1815.closure->proto (
                        &ceu_frame_1815,
                        2,
                        ceu_args_1815
                    );
                    ceu_assert_pre(ceu_block_1818, ceu_acc, "fannkuch.ceu : (lin 98, col 45) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1816 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1818,
                                ceu_hold_chk_set(&ceu_block_1871->dyns, ceu_block_1871->depth, CEU_HOLD_MUTAB, (ceu_set_1816), 0, "set error"),
                                "fannkuch.ceu : (lin 98, col 21)"
                            );
                            ceu_gc_inc((ceu_set_1816));
                            ceu_gc_dec(id_shouldBreak, 1);
                            id_shouldBreak = (ceu_set_1816);
                        }
                        
                    ceu_acc = ceu_set_1816;
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1818, 
                                ceu_hold_chk_set(&ceu_block_1830->dyns, ceu_block_1830->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "fannkuch.ceu : (lin 85, col 23)"
                            );
                            
                        
                            if (id_tmp.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_tmp, (id_tmp.Dyn->Any.hld_depth == ceu_block_1818->depth));
                            }
                        
                            if (id_r1.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_r1, (id_r1.Dyn->Any.hld_depth == ceu_block_1818->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_1818);
                    }
                    
                    } else {
                        
                    CEU_Block* ceu_block_1821 = ceu_block_1830;
                    // >>> block
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NIL });
                    // <<< block
                    
                    }
                }
                
                ceu_acc = id_shouldBreak;

                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                    }
                
                    // <<< block
                    
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_1840 = ceu_acc;
                    if (ceu_closure_1840.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1871, "fannkuch.ceu : (lin 103, col 14)", err);
                    }
                    CEU_Frame ceu_frame_1840 = { &ceu_closure_1840.Dyn->Closure, ceu_block_1871 };
                    
                    CEU_Value ceu_args_1840[2];
                    
                    ceu_acc = id_r;
ceu_args_1840[0] = ceu_acc;
ceu_acc = id_N;
ceu_args_1840[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1840.closure->proto (
                        &ceu_frame_1840,
                        2,
                        ceu_args_1840
                    );
                    ceu_assert_pre(ceu_block_1871, ceu_acc, "fannkuch.ceu : (lin 103, col 14) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_1851 = (CEU_Block) { (ceu_block_1871->depth + 1), 0, {.block=ceu_block_1871}, NULL };
                        CEU_Block* ceu_block_1851 = &_ceu_block_1851; 
                        
                        
                        
                        
                        
                        // >>> block
                        
                { // VECTOR | 
                    CEU_Value ceu_vec_1848 = ceu_vector_create(ceu_block_1851);
                    ceu_acc = id_checksum;

                        ceu_assert_pre(
                            ceu_block_1851,
                            ceu_vector_set(&ceu_vec_1848.Dyn->Vector, 0, ceu_acc),
                            "fannkuch.ceu : (lin 104, col 13)"
                        );
                        ceu_acc = id_max_flips;

                        ceu_assert_pre(
                            ceu_block_1851,
                            ceu_vector_set(&ceu_vec_1848.Dyn->Vector, 1, ceu_acc),
                            "fannkuch.ceu : (lin 104, col 13)"
                        );
                        
                    ceu_acc = ceu_vec_1848;
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1851, 
                                ceu_hold_chk_set(&ceu_block_1871->dyns, ceu_block_1871->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "fannkuch.ceu : (lin 103, col 19)"
                            );
                            
                        
                        
                        ceu_block_free(ceu_block_1851);
                    }
                    
                    } else {
                        
                    CEU_Block* ceu_block_1854 = ceu_block_1871;
                    // >>> block
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NIL });
                    // <<< block
                    
                    }
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1867 = ceu_acc;
                    if (ceu_closure_1867.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1871, "fannkuch.ceu : (lin 109, col 37)", err);
                    }
                    CEU_Frame ceu_frame_1867 = { &ceu_closure_1867.Dyn->Closure, ceu_block_1871 };
                    
                    CEU_Value ceu_args_1867[2];
                    
                    ceu_acc = id_perm_count;
ceu_args_1867[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1867[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1867.closure->proto (
                        &ceu_frame_1867,
                        2,
                        ceu_args_1867
                    );
                    ceu_assert_pre(ceu_block_1871, ceu_acc, "fannkuch.ceu : (lin 109, col 37) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1868 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1871,
                                ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_MUTAB, (ceu_set_1868), 0, "set error"),
                                "fannkuch.ceu : (lin 109, col 13)"
                            );
                            ceu_gc_inc((ceu_set_1868));
                            ceu_gc_dec(id_perm_count, 1);
                            id_perm_count = (ceu_set_1868);
                        }
                        
                    ceu_acc = ceu_set_1868;
                }
                
                    }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1871, 
                                ceu_hold_chk_set(&ceu_block_1874->dyns, ceu_block_1874->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "fannkuch.ceu : (lin 30, col 5)"
                            );
                            
                        
                            if (id_shouldBreak.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_shouldBreak, (id_shouldBreak.Dyn->Any.hld_depth == ceu_block_1871->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_1871);
                    }
                    
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1874, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "fannkuch.ceu : (lin 4, col 25)"
                            );
                            
                        
                            if (id_initial_perm.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_initial_perm, (id_initial_perm.Dyn->Any.hld_depth == ceu_block_1874->depth));
                            }
                        
                            if (id_i.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_i, (id_i.Dyn->Any.hld_depth == ceu_block_1874->depth));
                            }
                        
                            if (id_perm.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_perm, (id_perm.Dyn->Any.hld_depth == ceu_block_1874->depth));
                            }
                        
                            if (id_count.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_count, (id_count.Dyn->Any.hld_depth == ceu_block_1874->depth));
                            }
                        
                            if (id_r.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_r, (id_r.Dyn->Any.hld_depth == ceu_block_1874->depth));
                            }
                        
                            if (id_perm_count.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_perm_count, (id_perm_count.Dyn->Any.hld_depth == ceu_block_1874->depth));
                            }
                        
                            if (id_max_flips.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_max_flips, (id_max_flips.Dyn->Any.hld_depth == ceu_block_1874->depth));
                            }
                        
                            if (id_checksum.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_checksum, (id_checksum.Dyn->Any.hld_depth == ceu_block_1874->depth));
                            }
                        
                        
                            
                                if (id_N.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_N, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_N.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_1874);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_1875 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_1875,
                    0
                );
                ceu_acc = ceu_ret_1875;
                
                // UPVALS
                
                
                    CEU_Value ceu_set_1876 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_2005,
                                ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, (ceu_set_1876), 0, "set error"),
                                "fannkuch.ceu : (lin 4, col 5)"
                            );
                            ceu_gc_inc((ceu_set_1876));
                            ceu_gc_dec(id_fannkuch, 1);
                            id_fannkuch = (ceu_set_1876);
                        }
                        
                    ceu_acc = ceu_set_1876;
                }
                
                // DCL | 
                
                
                    
                    ceu_gc_inc(id_main);
                    ceu_acc = id_main;
                    
                
                { // SET | 
                     // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_1995 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_N;
                            CEU_Block* _id_N_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_1994 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_1994 = &_ceu_block_1994; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1994,
                                            ceu_hold_chk_set(&ceu_block_1994->dyns, ceu_block_1994->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "fannkuch.ceu : (lin 114, col 21)"
                                        );
                                        id_N = ceu_args[0];
                                    } else {
                                        id_N = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                            CEU_Value id_ret = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_checksum = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_flips = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                
                { // CALL | 
                    ceu_acc = id_fannkuch;

                    CEU_Value ceu_closure_1898 = ceu_acc;
                    if (ceu_closure_1898.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1994, "fannkuch.ceu : (lin 115, col 15)", err);
                    }
                    CEU_Frame ceu_frame_1898 = { &ceu_closure_1898.Dyn->Closure, ceu_block_1994 };
                    
                    CEU_Value ceu_args_1898[1];
                    
                    ceu_acc = id_N;
ceu_args_1898[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1898.closure->proto (
                        &ceu_frame_1898,
                        1,
                        ceu_args_1898
                    );
                    ceu_assert_pre(ceu_block_1994, ceu_acc, "fannkuch.ceu : (lin 115, col 15) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_hold_chk_set(&ceu_block_1994->dyns, ceu_block_1994->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 115, col 5)"
                        );
                    
                
                    id_ret = ceu_acc;
                    ceu_gc_inc(id_ret);
                    ceu_acc = id_ret;
                    
                
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        CEU_Value ceu_idx_1909 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_ret;

                    ceu_assert_pre(ceu_block_1994, ceu_col_check(ceu_acc, ceu_idx_1909), "fannkuch.ceu : (lin 116, col 20)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1909.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1994, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1909.Number), "fannkuch.ceu : (lin 116, col 20)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1909);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_hold_chk_set(&ceu_block_1994->dyns, ceu_block_1994->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 116, col 5)"
                        );
                    
                
                    id_checksum = ceu_acc;
                    ceu_gc_inc(id_checksum);
                    ceu_acc = id_checksum;
                    
                
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });
                        CEU_Value ceu_idx_1920 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_ret;

                    ceu_assert_pre(ceu_block_1994, ceu_col_check(ceu_acc, ceu_idx_1920), "fannkuch.ceu : (lin 117, col 17)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1920.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1994, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1920.Number), "fannkuch.ceu : (lin 117, col 17)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1920);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_hold_chk_set(&ceu_block_1994->dyns, ceu_block_1994->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "fannkuch.ceu : (lin 117, col 5)"
                        );
                    
                
                    id_flips = ceu_acc;
                    ceu_gc_inc(id_flips);
                    ceu_acc = id_flips;
                    
                
                { // CALL | 
                    ceu_acc = id_println;

                    CEU_Value ceu_closure_1928 = ceu_acc;
                    if (ceu_closure_1928.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1994, "fannkuch.ceu : (lin 119, col 5)", err);
                    }
                    CEU_Frame ceu_frame_1928 = { &ceu_closure_1928.Dyn->Closure, ceu_block_1994 };
                    
                    CEU_Value ceu_args_1928[1];
                    
                    ceu_acc = id_checksum;
ceu_args_1928[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1928.closure->proto (
                        &ceu_frame_1928,
                        1,
                        ceu_args_1928
                    );
                    ceu_assert_pre(ceu_block_1994, ceu_acc, "fannkuch.ceu : (lin 119, col 5) : call error");
                } // CALL
                
                { // CALL | 
                    ceu_acc = id_println;

                    CEU_Value ceu_closure_1992 = ceu_acc;
                    if (ceu_closure_1992.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1994, "fannkuch.ceu : (lin 120, col 5)", err);
                    }
                    CEU_Frame ceu_frame_1992 = { &ceu_closure_1992.Dyn->Closure, ceu_block_1994 };
                    
                    CEU_Value ceu_args_1992[4];
                    
                    
                { // VECTOR | 
                    CEU_Value ceu_vec_1968 = ceu_vector_create(ceu_block_1994);
                    ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='P'} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1968.Dyn->Vector, 0, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 13)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='f'} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1968.Dyn->Vector, 1, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 13)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='a'} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1968.Dyn->Vector, 2, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 13)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='n'} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1968.Dyn->Vector, 3, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 13)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='n'} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1968.Dyn->Vector, 4, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 13)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='k'} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1968.Dyn->Vector, 5, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 13)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='u'} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1968.Dyn->Vector, 6, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 13)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='c'} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1968.Dyn->Vector, 7, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 13)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='h'} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1968.Dyn->Vector, 8, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 13)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='e'} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1968.Dyn->Vector, 9, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 13)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='n'} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1968.Dyn->Vector, 10, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 13)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='('} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1968.Dyn->Vector, 11, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 13)"
                        );
                        
                    ceu_acc = ceu_vec_1968;
                }
                ceu_args_1992[0] = ceu_acc;
ceu_acc = id_N;
ceu_args_1992[1] = ceu_acc;

                { // VECTOR | 
                    CEU_Value ceu_vec_1986 = ceu_vector_create(ceu_block_1994);
                    ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char=')'} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1986.Dyn->Vector, 0, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 32)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char=' '} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1986.Dyn->Vector, 1, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 32)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char='='} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1986.Dyn->Vector, 2, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 32)"
                        );
                        ceu_acc = ((CEU_Value) { CEU_VALUE_CHAR, {.Char=' '} });
                        ceu_assert_pre(
                            ceu_block_1994,
                            ceu_vector_set(&ceu_vec_1986.Dyn->Vector, 3, ceu_acc),
                            "fannkuch.ceu : (lin 120, col 32)"
                        );
                        
                    ceu_acc = ceu_vec_1986;
                }
                ceu_args_1992[2] = ceu_acc;
ceu_acc = id_flips;
ceu_args_1992[3] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1992.closure->proto (
                        &ceu_frame_1992,
                        4,
                        ceu_args_1992
                    );
                    ceu_assert_pre(ceu_block_1994, ceu_acc, "fannkuch.ceu : (lin 120, col 5) : call error");
                } // CALL
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1994, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "fannkuch.ceu : (lin 114, col 21)"
                            );
                            
                        
                            if (id_ret.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_ret, (id_ret.Dyn->Any.hld_depth == ceu_block_1994->depth));
                            }
                        
                            if (id_checksum.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_checksum, (id_checksum.Dyn->Any.hld_depth == ceu_block_1994->depth));
                            }
                        
                            if (id_flips.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_flips, (id_flips.Dyn->Any.hld_depth == ceu_block_1994->depth));
                            }
                        
                        
                            
                                if (id_N.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_N, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_N.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_1994);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_1995 = ceu_closure_create (
                    ceu_block_2005,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_1995,
                    0
                );
                ceu_acc = ceu_ret_1995;
                
                // UPVALS
                
                
                    CEU_Value ceu_set_1996 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_2005,
                                ceu_hold_chk_set(&ceu_block_2005->dyns, ceu_block_2005->depth, CEU_HOLD_MUTAB, (ceu_set_1996), 0, "set error"),
                                "fannkuch.ceu : (lin 114, col 5)"
                            );
                            ceu_gc_inc((ceu_set_1996));
                            ceu_gc_dec(id_main, 1);
                            id_main = (ceu_set_1996);
                        }
                        
                    ceu_acc = ceu_set_1996;
                }
                
                { // CALL | 
                    ceu_acc = id_fannkuch;

                    CEU_Value ceu_closure_2003 = ceu_acc;
                    if (ceu_closure_2003.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2005, "fannkuch.ceu : (lin 123, col 1)", err);
                    }
                    CEU_Frame ceu_frame_2003 = { &ceu_closure_2003.Dyn->Closure, ceu_block_2005 };
                    
                    CEU_Value ceu_args_2003[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=3} });ceu_args_2003[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2003.closure->proto (
                        &ceu_frame_2003,
                        1,
                        ceu_args_2003
                    );
                    ceu_assert_pre(ceu_block_2005, ceu_acc, "fannkuch.ceu : (lin 123, col 1) : call error");
                } // CALL
                
                        // <<< block
                        
                        
                        
                            if (op_ampersand_ampersand.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_ampersand_ampersand, (op_ampersand_ampersand.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (op_bar_bar.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_bar_bar, (op_bar_bar.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (op_plus.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_plus, (op_plus.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (op_minus.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_minus, (op_minus.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (op_asterisk.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_asterisk, (op_asterisk.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (op_asterisk_asterisk.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_asterisk_asterisk, (op_asterisk_asterisk.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (op_slash.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_slash, (op_slash.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (op_slash_slash.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_slash_slash, (op_slash_slash.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (op_null.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_null, (op_null.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (op_greater_equals.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_greater_equals, (op_greater_equals.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (op_greater.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_greater, (op_greater.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (op_less_equals.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_less_equals, (op_less_equals.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (op_less.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_less, (op_less.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (id_to_dash_string.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_to_dash_string, (id_to_dash_string.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (id_to_dash_number.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_to_dash_number, (id_to_dash_number.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (id_to_dash_tag.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_to_dash_tag, (id_to_dash_tag.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (id_n.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_n, (id_n.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (id_fannkuch.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_fannkuch, (id_fannkuch.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                            if (id_main.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_main, (id_main.Dyn->Any.hld_depth == ceu_block_2005->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_2005);
                    }
                    
            return 0;
        }
    