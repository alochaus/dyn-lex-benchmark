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
                        CEU_Block _ceu_block_1238 = (CEU_Block) { 1, 1, {.frame=&_ceu_frame_}, NULL };
                        CEU_Block* ceu_block_1238 = &_ceu_block_1238; 
                        
                            // main block varargs (...)
                            CEU_Value id__dot__dot__dot_ = ceu_tuple_create(ceu_block_1238, ceu_argc);
                            for (int i=0; i<ceu_argc; i++) {
                                CEU_Value vec = ceu_vector_from_c_string(ceu_block_1238, ceu_argv[i]);
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
                        
                            CEU_Value id_fib = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
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
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_31,
                    0
                );
                ceu_acc = ceu_ret_31;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_60,
                    0
                );
                ceu_acc = ceu_ret_60;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 11, col 1)"
                        );
                    
                
                    op_bar_bar = ceu_acc;
                    ceu_gc_inc(op_bar_bar);
                    ceu_acc = op_bar_bar;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_126 (
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
                        CEU_Block _ceu_block_125 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_125 = &_ceu_block_125; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_125,
                                            ceu_hold_chk_set(&ceu_block_125->dyns, ceu_block_125->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 21, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_125,
                                            ceu_hold_chk_set(&ceu_block_125->dyns, ceu_block_125->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 21, col 27)"
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

                    CEU_Value ceu_closure_106 = ceu_acc;
                    if (ceu_closure_106.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_125, "prelude.ceu : (lin 22, col 30)", err);
                    }
                    CEU_Frame ceu_frame_106 = { &ceu_closure_106.Dyn->Closure, ceu_block_125 };
                    
                    CEU_Value ceu_args_106[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_86 = ceu_acc;
                    if (ceu_closure_86.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_125, "prelude.ceu : (lin 22, col 18)", err);
                    }
                    CEU_Frame ceu_frame_86 = { &ceu_closure_86.Dyn->Closure, ceu_block_125 };
                    
                    CEU_Value ceu_args_86[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_80 = ceu_acc;
                    if (ceu_closure_80.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_125, "prelude.ceu : (lin 22, col 9)", err);
                    }
                    CEU_Frame ceu_frame_80 = { &ceu_closure_80.Dyn->Closure, ceu_block_125 };
                    
                    CEU_Value ceu_args_80[1];
                    
                    ceu_acc = id_v1;
ceu_args_80[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_80.closure->proto (
                        &ceu_frame_80,
                        1,
                        ceu_args_80
                    );
                    ceu_assert_pre(ceu_block_125, ceu_acc, "prelude.ceu : (lin 22, col 9) : call error");
                } // CALL
                ceu_args_86[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_86[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_86.closure->proto (
                        &ceu_frame_86,
                        2,
                        ceu_args_86
                    );
                    ceu_assert_pre(ceu_block_125, ceu_acc, "prelude.ceu : (lin 22, col 18) : call error");
                } // CALL
                ceu_args_106[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_102 = ceu_acc;
                    if (ceu_closure_102.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_125, "prelude.ceu : (lin 22, col 43)", err);
                    }
                    CEU_Frame ceu_frame_102 = { &ceu_closure_102.Dyn->Closure, ceu_block_125 };
                    
                    CEU_Value ceu_args_102[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_96 = ceu_acc;
                    if (ceu_closure_96.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_125, "prelude.ceu : (lin 22, col 34)", err);
                    }
                    CEU_Frame ceu_frame_96 = { &ceu_closure_96.Dyn->Closure, ceu_block_125 };
                    
                    CEU_Value ceu_args_96[1];
                    
                    ceu_acc = id_v2;
ceu_args_96[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_96.closure->proto (
                        &ceu_frame_96,
                        1,
                        ceu_args_96
                    );
                    ceu_assert_pre(ceu_block_125, ceu_acc, "prelude.ceu : (lin 22, col 34) : call error");
                } // CALL
                ceu_args_102[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_102[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_102.closure->proto (
                        &ceu_frame_102,
                        2,
                        ceu_args_102
                    );
                    ceu_assert_pre(ceu_block_125, ceu_acc, "prelude.ceu : (lin 22, col 43) : call error");
                } // CALL
                ceu_args_106[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_106.closure->proto (
                        &ceu_frame_106,
                        2,
                        ceu_args_106
                    );
                    ceu_assert_pre(ceu_block_125, ceu_acc, "prelude.ceu : (lin 22, col 30) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_116 = ceu_block_125;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_114 = ceu_acc;
                    if (ceu_closure_114.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_116, "prelude.ceu : (lin 23, col 9)", err);
                    }
                    CEU_Frame ceu_frame_114 = { &ceu_closure_114.Dyn->Closure, ceu_block_116 };
                    
                    CEU_Value ceu_args_114[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_114[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_114.closure->proto (
                        &ceu_frame_114,
                        1,
                        ceu_args_114
                    );
                    ceu_assert_pre(ceu_block_116, ceu_acc, "prelude.ceu : (lin 23, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_122 = ceu_block_125;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( ((id_v1).Number + (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_125, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 21, col 27)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_125);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_126 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_126,
                    0
                );
                ceu_acc = ceu_ret_126;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 21, col 1)"
                        );
                    
                
                    op_plus = ceu_acc;
                    ceu_gc_inc(op_plus);
                    ceu_acc = op_plus;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_239 (
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
                        CEU_Block _ceu_block_238 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_238 = &_ceu_block_238; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_238,
                                            ceu_hold_chk_set(&ceu_block_238->dyns, ceu_block_238->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 29, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_238,
                                            ceu_hold_chk_set(&ceu_block_238->dyns, ceu_block_238->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 29, col 27)"
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

                    CEU_Value ceu_closure_147 = ceu_acc;
                    if (ceu_closure_147.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_238, "prelude.ceu : (lin 30, col 14)", err);
                    }
                    CEU_Frame ceu_frame_147 = { &ceu_closure_147.Dyn->Closure, ceu_block_238 };
                    
                    CEU_Value ceu_args_147[1];
                    
                    ceu_acc = id_v1;
ceu_args_147[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_147.closure->proto (
                        &ceu_frame_147,
                        1,
                        ceu_args_147
                    );
                    ceu_assert_pre(ceu_block_238, ceu_acc, "prelude.ceu : (lin 30, col 14) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_238,
                            ceu_hold_chk_set(&ceu_block_238->dyns, ceu_block_238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 30, col 5)"
                        );
                    
                
                    id_t1 = ceu_acc;
                    ceu_gc_inc(id_t1);
                    ceu_acc = id_t1;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_158 = ceu_acc;
                    if (ceu_closure_158.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_238, "prelude.ceu : (lin 31, col 14)", err);
                    }
                    CEU_Frame ceu_frame_158 = { &ceu_closure_158.Dyn->Closure, ceu_block_238 };
                    
                    CEU_Value ceu_args_158[1];
                    
                    ceu_acc = id_v2;
ceu_args_158[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_158.closure->proto (
                        &ceu_frame_158,
                        1,
                        ceu_args_158
                    );
                    ceu_assert_pre(ceu_block_238, ceu_acc, "prelude.ceu : (lin 31, col 14) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_238,
                            ceu_hold_chk_set(&ceu_block_238->dyns, ceu_block_238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 31, col 5)"
                        );
                    
                
                    id_t2 = ceu_acc;
                    ceu_gc_inc(id_t2);
                    ceu_acc = id_t2;
                    
                
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_ampersand_ampersand;

                    CEU_Value ceu_closure_184 = ceu_acc;
                    if (ceu_closure_184.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_238, "prelude.ceu : (lin 32, col 24)", err);
                    }
                    CEU_Frame ceu_frame_184 = { &ceu_closure_184.Dyn->Closure, ceu_block_238 };
                    
                    CEU_Value ceu_args_184[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_169 = ceu_acc;
                    if (ceu_closure_169.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_238, "prelude.ceu : (lin 32, col 12)", err);
                    }
                    CEU_Frame ceu_frame_169 = { &ceu_closure_169.Dyn->Closure, ceu_block_238 };
                    
                    CEU_Value ceu_args_169[2];
                    
                    ceu_acc = id_t1;
ceu_args_169[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_169[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_169.closure->proto (
                        &ceu_frame_169,
                        2,
                        ceu_args_169
                    );
                    ceu_assert_pre(ceu_block_238, ceu_acc, "prelude.ceu : (lin 32, col 12) : call error");
                } // CALL
                ceu_args_184[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_180 = ceu_acc;
                    if (ceu_closure_180.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_238, "prelude.ceu : (lin 32, col 31)", err);
                    }
                    CEU_Frame ceu_frame_180 = { &ceu_closure_180.Dyn->Closure, ceu_block_238 };
                    
                    CEU_Value ceu_args_180[2];
                    
                    ceu_acc = id_t2;
ceu_args_180[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_nil} });ceu_args_180[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_180.closure->proto (
                        &ceu_frame_180,
                        2,
                        ceu_args_180
                    );
                    ceu_assert_pre(ceu_block_238, ceu_acc, "prelude.ceu : (lin 32, col 31) : call error");
                } // CALL
                ceu_args_184[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_184.closure->proto (
                        &ceu_frame_184,
                        2,
                        ceu_args_184
                    );
                    ceu_assert_pre(ceu_block_238, ceu_acc, "prelude.ceu : (lin 32, col 24) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_189 = ceu_block_238;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( (- (id_v1).Number))} });
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_235 = ceu_block_238;
                    // >>> block
                    
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_216 = ceu_acc;
                    if (ceu_closure_216.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_235, "prelude.ceu : (lin 35, col 28)", err);
                    }
                    CEU_Frame ceu_frame_216 = { &ceu_closure_216.Dyn->Closure, ceu_block_235 };
                    
                    CEU_Value ceu_args_216[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_201 = ceu_acc;
                    if (ceu_closure_201.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_235, "prelude.ceu : (lin 35, col 16)", err);
                    }
                    CEU_Frame ceu_frame_201 = { &ceu_closure_201.Dyn->Closure, ceu_block_235 };
                    
                    CEU_Value ceu_args_201[2];
                    
                    ceu_acc = id_t1;
ceu_args_201[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_201[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_201.closure->proto (
                        &ceu_frame_201,
                        2,
                        ceu_args_201
                    );
                    ceu_assert_pre(ceu_block_235, ceu_acc, "prelude.ceu : (lin 35, col 16) : call error");
                } // CALL
                ceu_args_216[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_212 = ceu_acc;
                    if (ceu_closure_212.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_235, "prelude.ceu : (lin 35, col 35)", err);
                    }
                    CEU_Frame ceu_frame_212 = { &ceu_closure_212.Dyn->Closure, ceu_block_235 };
                    
                    CEU_Value ceu_args_212[2];
                    
                    ceu_acc = id_t2;
ceu_args_212[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_212[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_212.closure->proto (
                        &ceu_frame_212,
                        2,
                        ceu_args_212
                    );
                    ceu_assert_pre(ceu_block_235, ceu_acc, "prelude.ceu : (lin 35, col 35) : call error");
                } // CALL
                ceu_args_216[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_216.closure->proto (
                        &ceu_frame_216,
                        2,
                        ceu_args_216
                    );
                    ceu_assert_pre(ceu_block_235, ceu_acc, "prelude.ceu : (lin 35, col 28) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_226 = ceu_block_235;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_224 = ceu_acc;
                    if (ceu_closure_224.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_226, "prelude.ceu : (lin 36, col 13)", err);
                    }
                    CEU_Frame ceu_frame_224 = { &ceu_closure_224.Dyn->Closure, ceu_block_226 };
                    
                    CEU_Value ceu_args_224[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_224[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_224.closure->proto (
                        &ceu_frame_224,
                        1,
                        ceu_args_224
                    );
                    ceu_assert_pre(ceu_block_226, ceu_acc, "prelude.ceu : (lin 36, col 13) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_232 = ceu_block_235;
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
                                ceu_block_238, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 29, col 27)"
                            );
                            
                        
                            if (id_t1.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_t1, (id_t1.Dyn->Any.hld_depth == ceu_block_238->depth));
                            }
                        
                            if (id_t2.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_t2, (id_t2.Dyn->Any.hld_depth == ceu_block_238->depth));
                            }
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_238);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_239 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_239,
                    0
                );
                ceu_acc = ceu_ret_239;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 29, col 1)"
                        );
                    
                
                    op_minus = ceu_acc;
                    ceu_gc_inc(op_minus);
                    ceu_acc = op_minus;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_305 (
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
                        CEU_Block _ceu_block_304 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_304 = &_ceu_block_304; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_304,
                                            ceu_hold_chk_set(&ceu_block_304->dyns, ceu_block_304->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 43, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_304,
                                            ceu_hold_chk_set(&ceu_block_304->dyns, ceu_block_304->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 43, col 27)"
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

                    CEU_Value ceu_closure_285 = ceu_acc;
                    if (ceu_closure_285.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_304, "prelude.ceu : (lin 44, col 30)", err);
                    }
                    CEU_Frame ceu_frame_285 = { &ceu_closure_285.Dyn->Closure, ceu_block_304 };
                    
                    CEU_Value ceu_args_285[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_265 = ceu_acc;
                    if (ceu_closure_265.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_304, "prelude.ceu : (lin 44, col 18)", err);
                    }
                    CEU_Frame ceu_frame_265 = { &ceu_closure_265.Dyn->Closure, ceu_block_304 };
                    
                    CEU_Value ceu_args_265[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_259 = ceu_acc;
                    if (ceu_closure_259.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_304, "prelude.ceu : (lin 44, col 9)", err);
                    }
                    CEU_Frame ceu_frame_259 = { &ceu_closure_259.Dyn->Closure, ceu_block_304 };
                    
                    CEU_Value ceu_args_259[1];
                    
                    ceu_acc = id_v1;
ceu_args_259[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_259.closure->proto (
                        &ceu_frame_259,
                        1,
                        ceu_args_259
                    );
                    ceu_assert_pre(ceu_block_304, ceu_acc, "prelude.ceu : (lin 44, col 9) : call error");
                } // CALL
                ceu_args_265[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_265[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_265.closure->proto (
                        &ceu_frame_265,
                        2,
                        ceu_args_265
                    );
                    ceu_assert_pre(ceu_block_304, ceu_acc, "prelude.ceu : (lin 44, col 18) : call error");
                } // CALL
                ceu_args_285[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_281 = ceu_acc;
                    if (ceu_closure_281.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_304, "prelude.ceu : (lin 44, col 43)", err);
                    }
                    CEU_Frame ceu_frame_281 = { &ceu_closure_281.Dyn->Closure, ceu_block_304 };
                    
                    CEU_Value ceu_args_281[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_275 = ceu_acc;
                    if (ceu_closure_275.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_304, "prelude.ceu : (lin 44, col 34)", err);
                    }
                    CEU_Frame ceu_frame_275 = { &ceu_closure_275.Dyn->Closure, ceu_block_304 };
                    
                    CEU_Value ceu_args_275[1];
                    
                    ceu_acc = id_v2;
ceu_args_275[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_275.closure->proto (
                        &ceu_frame_275,
                        1,
                        ceu_args_275
                    );
                    ceu_assert_pre(ceu_block_304, ceu_acc, "prelude.ceu : (lin 44, col 34) : call error");
                } // CALL
                ceu_args_281[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_281[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_281.closure->proto (
                        &ceu_frame_281,
                        2,
                        ceu_args_281
                    );
                    ceu_assert_pre(ceu_block_304, ceu_acc, "prelude.ceu : (lin 44, col 43) : call error");
                } // CALL
                ceu_args_285[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_285.closure->proto (
                        &ceu_frame_285,
                        2,
                        ceu_args_285
                    );
                    ceu_assert_pre(ceu_block_304, ceu_acc, "prelude.ceu : (lin 44, col 30) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_295 = ceu_block_304;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_293 = ceu_acc;
                    if (ceu_closure_293.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_295, "prelude.ceu : (lin 45, col 9)", err);
                    }
                    CEU_Frame ceu_frame_293 = { &ceu_closure_293.Dyn->Closure, ceu_block_295 };
                    
                    CEU_Value ceu_args_293[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_293[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_293.closure->proto (
                        &ceu_frame_293,
                        1,
                        ceu_args_293
                    );
                    ceu_assert_pre(ceu_block_295, ceu_acc, "prelude.ceu : (lin 45, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_301 = ceu_block_304;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( ((id_v1).Number * (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_304, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 43, col 27)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_304);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_305 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_305,
                    0
                );
                ceu_acc = ceu_ret_305;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 43, col 1)"
                        );
                    
                
                    op_asterisk = ceu_acc;
                    ceu_gc_inc(op_asterisk);
                    ceu_acc = op_asterisk;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_371 (
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
                        CEU_Block _ceu_block_370 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_370 = &_ceu_block_370; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_370,
                                            ceu_hold_chk_set(&ceu_block_370->dyns, ceu_block_370->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 51, col 28)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_370,
                                            ceu_hold_chk_set(&ceu_block_370->dyns, ceu_block_370->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 51, col 28)"
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

                    CEU_Value ceu_closure_351 = ceu_acc;
                    if (ceu_closure_351.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_370, "prelude.ceu : (lin 52, col 30)", err);
                    }
                    CEU_Frame ceu_frame_351 = { &ceu_closure_351.Dyn->Closure, ceu_block_370 };
                    
                    CEU_Value ceu_args_351[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_331 = ceu_acc;
                    if (ceu_closure_331.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_370, "prelude.ceu : (lin 52, col 18)", err);
                    }
                    CEU_Frame ceu_frame_331 = { &ceu_closure_331.Dyn->Closure, ceu_block_370 };
                    
                    CEU_Value ceu_args_331[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_325 = ceu_acc;
                    if (ceu_closure_325.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_370, "prelude.ceu : (lin 52, col 9)", err);
                    }
                    CEU_Frame ceu_frame_325 = { &ceu_closure_325.Dyn->Closure, ceu_block_370 };
                    
                    CEU_Value ceu_args_325[1];
                    
                    ceu_acc = id_v1;
ceu_args_325[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_325.closure->proto (
                        &ceu_frame_325,
                        1,
                        ceu_args_325
                    );
                    ceu_assert_pre(ceu_block_370, ceu_acc, "prelude.ceu : (lin 52, col 9) : call error");
                } // CALL
                ceu_args_331[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_331[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_331.closure->proto (
                        &ceu_frame_331,
                        2,
                        ceu_args_331
                    );
                    ceu_assert_pre(ceu_block_370, ceu_acc, "prelude.ceu : (lin 52, col 18) : call error");
                } // CALL
                ceu_args_351[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_347 = ceu_acc;
                    if (ceu_closure_347.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_370, "prelude.ceu : (lin 52, col 43)", err);
                    }
                    CEU_Frame ceu_frame_347 = { &ceu_closure_347.Dyn->Closure, ceu_block_370 };
                    
                    CEU_Value ceu_args_347[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_341 = ceu_acc;
                    if (ceu_closure_341.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_370, "prelude.ceu : (lin 52, col 34)", err);
                    }
                    CEU_Frame ceu_frame_341 = { &ceu_closure_341.Dyn->Closure, ceu_block_370 };
                    
                    CEU_Value ceu_args_341[1];
                    
                    ceu_acc = id_v2;
ceu_args_341[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_341.closure->proto (
                        &ceu_frame_341,
                        1,
                        ceu_args_341
                    );
                    ceu_assert_pre(ceu_block_370, ceu_acc, "prelude.ceu : (lin 52, col 34) : call error");
                } // CALL
                ceu_args_347[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_347[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_347.closure->proto (
                        &ceu_frame_347,
                        2,
                        ceu_args_347
                    );
                    ceu_assert_pre(ceu_block_370, ceu_acc, "prelude.ceu : (lin 52, col 43) : call error");
                } // CALL
                ceu_args_351[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_351.closure->proto (
                        &ceu_frame_351,
                        2,
                        ceu_args_351
                    );
                    ceu_assert_pre(ceu_block_370, ceu_acc, "prelude.ceu : (lin 52, col 30) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_361 = ceu_block_370;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_359 = ceu_acc;
                    if (ceu_closure_359.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_361, "prelude.ceu : (lin 53, col 9)", err);
                    }
                    CEU_Frame ceu_frame_359 = { &ceu_closure_359.Dyn->Closure, ceu_block_361 };
                    
                    CEU_Value ceu_args_359[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_359[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_359.closure->proto (
                        &ceu_frame_359,
                        1,
                        ceu_args_359
                    );
                    ceu_assert_pre(ceu_block_361, ceu_acc, "prelude.ceu : (lin 53, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_367 = ceu_block_370;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( powf((id_v1).Number, (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_370, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 51, col 28)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_370);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_371 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_371,
                    0
                );
                ceu_acc = ceu_ret_371;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 51, col 1)"
                        );
                    
                
                    op_asterisk_asterisk = ceu_acc;
                    ceu_gc_inc(op_asterisk_asterisk);
                    ceu_acc = op_asterisk_asterisk;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_437 (
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
                        CEU_Block _ceu_block_436 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_436 = &_ceu_block_436; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_436,
                                            ceu_hold_chk_set(&ceu_block_436->dyns, ceu_block_436->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 59, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_436,
                                            ceu_hold_chk_set(&ceu_block_436->dyns, ceu_block_436->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 59, col 27)"
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

                    CEU_Value ceu_closure_417 = ceu_acc;
                    if (ceu_closure_417.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_436, "prelude.ceu : (lin 60, col 30)", err);
                    }
                    CEU_Frame ceu_frame_417 = { &ceu_closure_417.Dyn->Closure, ceu_block_436 };
                    
                    CEU_Value ceu_args_417[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_397 = ceu_acc;
                    if (ceu_closure_397.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_436, "prelude.ceu : (lin 60, col 18)", err);
                    }
                    CEU_Frame ceu_frame_397 = { &ceu_closure_397.Dyn->Closure, ceu_block_436 };
                    
                    CEU_Value ceu_args_397[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_391 = ceu_acc;
                    if (ceu_closure_391.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_436, "prelude.ceu : (lin 60, col 9)", err);
                    }
                    CEU_Frame ceu_frame_391 = { &ceu_closure_391.Dyn->Closure, ceu_block_436 };
                    
                    CEU_Value ceu_args_391[1];
                    
                    ceu_acc = id_v1;
ceu_args_391[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_391.closure->proto (
                        &ceu_frame_391,
                        1,
                        ceu_args_391
                    );
                    ceu_assert_pre(ceu_block_436, ceu_acc, "prelude.ceu : (lin 60, col 9) : call error");
                } // CALL
                ceu_args_397[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_397[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_397.closure->proto (
                        &ceu_frame_397,
                        2,
                        ceu_args_397
                    );
                    ceu_assert_pre(ceu_block_436, ceu_acc, "prelude.ceu : (lin 60, col 18) : call error");
                } // CALL
                ceu_args_417[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_413 = ceu_acc;
                    if (ceu_closure_413.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_436, "prelude.ceu : (lin 60, col 43)", err);
                    }
                    CEU_Frame ceu_frame_413 = { &ceu_closure_413.Dyn->Closure, ceu_block_436 };
                    
                    CEU_Value ceu_args_413[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_407 = ceu_acc;
                    if (ceu_closure_407.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_436, "prelude.ceu : (lin 60, col 34)", err);
                    }
                    CEU_Frame ceu_frame_407 = { &ceu_closure_407.Dyn->Closure, ceu_block_436 };
                    
                    CEU_Value ceu_args_407[1];
                    
                    ceu_acc = id_v2;
ceu_args_407[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_407.closure->proto (
                        &ceu_frame_407,
                        1,
                        ceu_args_407
                    );
                    ceu_assert_pre(ceu_block_436, ceu_acc, "prelude.ceu : (lin 60, col 34) : call error");
                } // CALL
                ceu_args_413[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_413[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_413.closure->proto (
                        &ceu_frame_413,
                        2,
                        ceu_args_413
                    );
                    ceu_assert_pre(ceu_block_436, ceu_acc, "prelude.ceu : (lin 60, col 43) : call error");
                } // CALL
                ceu_args_417[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_417.closure->proto (
                        &ceu_frame_417,
                        2,
                        ceu_args_417
                    );
                    ceu_assert_pre(ceu_block_436, ceu_acc, "prelude.ceu : (lin 60, col 30) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_427 = ceu_block_436;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_425 = ceu_acc;
                    if (ceu_closure_425.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_427, "prelude.ceu : (lin 61, col 9)", err);
                    }
                    CEU_Frame ceu_frame_425 = { &ceu_closure_425.Dyn->Closure, ceu_block_427 };
                    
                    CEU_Value ceu_args_425[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_425[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_425.closure->proto (
                        &ceu_frame_425,
                        1,
                        ceu_args_425
                    );
                    ceu_assert_pre(ceu_block_427, ceu_acc, "prelude.ceu : (lin 61, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_433 = ceu_block_436;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( ((id_v1).Number / (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_436, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 59, col 27)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_436);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_437 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_437,
                    0
                );
                ceu_acc = ceu_ret_437;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 59, col 1)"
                        );
                    
                
                    op_slash = ceu_acc;
                    ceu_gc_inc(op_slash);
                    ceu_acc = op_slash;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_503 (
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
                        CEU_Block _ceu_block_502 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_502 = &_ceu_block_502; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_502,
                                            ceu_hold_chk_set(&ceu_block_502->dyns, ceu_block_502->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 67, col 28)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_502,
                                            ceu_hold_chk_set(&ceu_block_502->dyns, ceu_block_502->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 67, col 28)"
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

                    CEU_Value ceu_closure_483 = ceu_acc;
                    if (ceu_closure_483.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_502, "prelude.ceu : (lin 68, col 30)", err);
                    }
                    CEU_Frame ceu_frame_483 = { &ceu_closure_483.Dyn->Closure, ceu_block_502 };
                    
                    CEU_Value ceu_args_483[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_463 = ceu_acc;
                    if (ceu_closure_463.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_502, "prelude.ceu : (lin 68, col 18)", err);
                    }
                    CEU_Frame ceu_frame_463 = { &ceu_closure_463.Dyn->Closure, ceu_block_502 };
                    
                    CEU_Value ceu_args_463[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_457 = ceu_acc;
                    if (ceu_closure_457.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_502, "prelude.ceu : (lin 68, col 9)", err);
                    }
                    CEU_Frame ceu_frame_457 = { &ceu_closure_457.Dyn->Closure, ceu_block_502 };
                    
                    CEU_Value ceu_args_457[1];
                    
                    ceu_acc = id_v1;
ceu_args_457[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_457.closure->proto (
                        &ceu_frame_457,
                        1,
                        ceu_args_457
                    );
                    ceu_assert_pre(ceu_block_502, ceu_acc, "prelude.ceu : (lin 68, col 9) : call error");
                } // CALL
                ceu_args_463[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_463[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_463.closure->proto (
                        &ceu_frame_463,
                        2,
                        ceu_args_463
                    );
                    ceu_assert_pre(ceu_block_502, ceu_acc, "prelude.ceu : (lin 68, col 18) : call error");
                } // CALL
                ceu_args_483[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_479 = ceu_acc;
                    if (ceu_closure_479.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_502, "prelude.ceu : (lin 68, col 43)", err);
                    }
                    CEU_Frame ceu_frame_479 = { &ceu_closure_479.Dyn->Closure, ceu_block_502 };
                    
                    CEU_Value ceu_args_479[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_473 = ceu_acc;
                    if (ceu_closure_473.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_502, "prelude.ceu : (lin 68, col 34)", err);
                    }
                    CEU_Frame ceu_frame_473 = { &ceu_closure_473.Dyn->Closure, ceu_block_502 };
                    
                    CEU_Value ceu_args_473[1];
                    
                    ceu_acc = id_v2;
ceu_args_473[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_473.closure->proto (
                        &ceu_frame_473,
                        1,
                        ceu_args_473
                    );
                    ceu_assert_pre(ceu_block_502, ceu_acc, "prelude.ceu : (lin 68, col 34) : call error");
                } // CALL
                ceu_args_479[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_479[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_479.closure->proto (
                        &ceu_frame_479,
                        2,
                        ceu_args_479
                    );
                    ceu_assert_pre(ceu_block_502, ceu_acc, "prelude.ceu : (lin 68, col 43) : call error");
                } // CALL
                ceu_args_483[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_483.closure->proto (
                        &ceu_frame_483,
                        2,
                        ceu_args_483
                    );
                    ceu_assert_pre(ceu_block_502, ceu_acc, "prelude.ceu : (lin 68, col 30) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_493 = ceu_block_502;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_491 = ceu_acc;
                    if (ceu_closure_491.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_493, "prelude.ceu : (lin 69, col 9)", err);
                    }
                    CEU_Frame ceu_frame_491 = { &ceu_closure_491.Dyn->Closure, ceu_block_493 };
                    
                    CEU_Value ceu_args_491[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_491[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_491.closure->proto (
                        &ceu_frame_491,
                        1,
                        ceu_args_491
                    );
                    ceu_assert_pre(ceu_block_493, ceu_acc, "prelude.ceu : (lin 69, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_499 = ceu_block_502;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( powf((id_v1).Number, 1/(id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_502, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 67, col 28)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_502);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_503 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_503,
                    0
                );
                ceu_acc = ceu_ret_503;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 67, col 1)"
                        );
                    
                
                    op_slash_slash = ceu_acc;
                    ceu_gc_inc(op_slash_slash);
                    ceu_acc = op_slash_slash;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_569 (
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
                        CEU_Block _ceu_block_568 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_568 = &_ceu_block_568; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_568,
                                            ceu_hold_chk_set(&ceu_block_568->dyns, ceu_block_568->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 75, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_568,
                                            ceu_hold_chk_set(&ceu_block_568->dyns, ceu_block_568->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 75, col 27)"
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

                    CEU_Value ceu_closure_549 = ceu_acc;
                    if (ceu_closure_549.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_568, "prelude.ceu : (lin 76, col 30)", err);
                    }
                    CEU_Frame ceu_frame_549 = { &ceu_closure_549.Dyn->Closure, ceu_block_568 };
                    
                    CEU_Value ceu_args_549[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_529 = ceu_acc;
                    if (ceu_closure_529.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_568, "prelude.ceu : (lin 76, col 18)", err);
                    }
                    CEU_Frame ceu_frame_529 = { &ceu_closure_529.Dyn->Closure, ceu_block_568 };
                    
                    CEU_Value ceu_args_529[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_523 = ceu_acc;
                    if (ceu_closure_523.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_568, "prelude.ceu : (lin 76, col 9)", err);
                    }
                    CEU_Frame ceu_frame_523 = { &ceu_closure_523.Dyn->Closure, ceu_block_568 };
                    
                    CEU_Value ceu_args_523[1];
                    
                    ceu_acc = id_v1;
ceu_args_523[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_523.closure->proto (
                        &ceu_frame_523,
                        1,
                        ceu_args_523
                    );
                    ceu_assert_pre(ceu_block_568, ceu_acc, "prelude.ceu : (lin 76, col 9) : call error");
                } // CALL
                ceu_args_529[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_529[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_529.closure->proto (
                        &ceu_frame_529,
                        2,
                        ceu_args_529
                    );
                    ceu_assert_pre(ceu_block_568, ceu_acc, "prelude.ceu : (lin 76, col 18) : call error");
                } // CALL
                ceu_args_549[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_545 = ceu_acc;
                    if (ceu_closure_545.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_568, "prelude.ceu : (lin 76, col 43)", err);
                    }
                    CEU_Frame ceu_frame_545 = { &ceu_closure_545.Dyn->Closure, ceu_block_568 };
                    
                    CEU_Value ceu_args_545[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_539 = ceu_acc;
                    if (ceu_closure_539.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_568, "prelude.ceu : (lin 76, col 34)", err);
                    }
                    CEU_Frame ceu_frame_539 = { &ceu_closure_539.Dyn->Closure, ceu_block_568 };
                    
                    CEU_Value ceu_args_539[1];
                    
                    ceu_acc = id_v2;
ceu_args_539[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_539.closure->proto (
                        &ceu_frame_539,
                        1,
                        ceu_args_539
                    );
                    ceu_assert_pre(ceu_block_568, ceu_acc, "prelude.ceu : (lin 76, col 34) : call error");
                } // CALL
                ceu_args_545[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_545[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_545.closure->proto (
                        &ceu_frame_545,
                        2,
                        ceu_args_545
                    );
                    ceu_assert_pre(ceu_block_568, ceu_acc, "prelude.ceu : (lin 76, col 43) : call error");
                } // CALL
                ceu_args_549[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_549.closure->proto (
                        &ceu_frame_549,
                        2,
                        ceu_args_549
                    );
                    ceu_assert_pre(ceu_block_568, ceu_acc, "prelude.ceu : (lin 76, col 30) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_559 = ceu_block_568;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_557 = ceu_acc;
                    if (ceu_closure_557.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_559, "prelude.ceu : (lin 77, col 9)", err);
                    }
                    CEU_Frame ceu_frame_557 = { &ceu_closure_557.Dyn->Closure, ceu_block_559 };
                    
                    CEU_Value ceu_args_557[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_557[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_557.closure->proto (
                        &ceu_frame_557,
                        1,
                        ceu_args_557
                    );
                    ceu_assert_pre(ceu_block_559, ceu_acc, "prelude.ceu : (lin 77, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_565 = ceu_block_568;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( fmod((id_v1).Number, (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_568, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 75, col 27)"
                            );
                            
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_568);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_569 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_569,
                    0
                );
                ceu_acc = ceu_ret_569;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 75, col 1)"
                        );
                    
                
                    op_null = ceu_acc;
                    ceu_gc_inc(op_null);
                    ceu_acc = op_null;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_641 (
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
                        CEU_Block _ceu_block_640 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_640 = &_ceu_block_640; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_640,
                                            ceu_hold_chk_set(&ceu_block_640->dyns, ceu_block_640->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 85, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_640,
                                            ceu_hold_chk_set(&ceu_block_640->dyns, ceu_block_640->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 85, col 27)"
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

                    CEU_Value ceu_closure_590 = ceu_acc;
                    if (ceu_closure_590.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_640, "prelude.ceu : (lin 86, col 14)", err);
                    }
                    CEU_Frame ceu_frame_590 = { &ceu_closure_590.Dyn->Closure, ceu_block_640 };
                    
                    CEU_Value ceu_args_590[1];
                    
                    ceu_acc = id_v1;
ceu_args_590[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_590.closure->proto (
                        &ceu_frame_590,
                        1,
                        ceu_args_590
                    );
                    ceu_assert_pre(ceu_block_640, ceu_acc, "prelude.ceu : (lin 86, col 14) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_640,
                            ceu_hold_chk_set(&ceu_block_640->dyns, ceu_block_640->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 86, col 5)"
                        );
                    
                
                    id_t1 = ceu_acc;
                    ceu_gc_inc(id_t1);
                    ceu_acc = id_t1;
                    
                
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_621 = ceu_acc;
                    if (ceu_closure_621.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_640, "prelude.ceu : (lin 87, col 25)", err);
                    }
                    CEU_Frame ceu_frame_621 = { &ceu_closure_621.Dyn->Closure, ceu_block_640 };
                    
                    CEU_Value ceu_args_621[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_606 = ceu_acc;
                    if (ceu_closure_606.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_640, "prelude.ceu : (lin 87, col 12)", err);
                    }
                    CEU_Frame ceu_frame_606 = { &ceu_closure_606.Dyn->Closure, ceu_block_640 };
                    
                    CEU_Value ceu_args_606[2];
                    
                    ceu_acc = id_t1;
ceu_args_606[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_603 = ceu_acc;
                    if (ceu_closure_603.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_640, "prelude.ceu : (lin 87, col 15)", err);
                    }
                    CEU_Frame ceu_frame_603 = { &ceu_closure_603.Dyn->Closure, ceu_block_640 };
                    
                    CEU_Value ceu_args_603[1];
                    
                    ceu_acc = id_v2;
ceu_args_603[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_603.closure->proto (
                        &ceu_frame_603,
                        1,
                        ceu_args_603
                    );
                    ceu_assert_pre(ceu_block_640, ceu_acc, "prelude.ceu : (lin 87, col 15) : call error");
                } // CALL
                ceu_args_606[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_606.closure->proto (
                        &ceu_frame_606,
                        2,
                        ceu_args_606
                    );
                    ceu_assert_pre(ceu_block_640, ceu_acc, "prelude.ceu : (lin 87, col 12) : call error");
                } // CALL
                ceu_args_621[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_617 = ceu_acc;
                    if (ceu_closure_617.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_640, "prelude.ceu : (lin 87, col 32)", err);
                    }
                    CEU_Frame ceu_frame_617 = { &ceu_closure_617.Dyn->Closure, ceu_block_640 };
                    
                    CEU_Value ceu_args_617[2];
                    
                    ceu_acc = id_t1;
ceu_args_617[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_617[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_617.closure->proto (
                        &ceu_frame_617,
                        2,
                        ceu_args_617
                    );
                    ceu_assert_pre(ceu_block_640, ceu_acc, "prelude.ceu : (lin 87, col 32) : call error");
                } // CALL
                ceu_args_621[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_621.closure->proto (
                        &ceu_frame_621,
                        2,
                        ceu_args_621
                    );
                    ceu_assert_pre(ceu_block_640, ceu_acc, "prelude.ceu : (lin 87, col 25) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_631 = ceu_block_640;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_629 = ceu_acc;
                    if (ceu_closure_629.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_631, "prelude.ceu : (lin 88, col 9)", err);
                    }
                    CEU_Frame ceu_frame_629 = { &ceu_closure_629.Dyn->Closure, ceu_block_631 };
                    
                    CEU_Value ceu_args_629[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_629[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_629.closure->proto (
                        &ceu_frame_629,
                        1,
                        ceu_args_629
                    );
                    ceu_assert_pre(ceu_block_631, ceu_acc, "prelude.ceu : (lin 88, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_637 = ceu_block_640;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_BOOL, {.Bool=( ((id_v1).Number >= (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_640, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 85, col 27)"
                            );
                            
                        
                            if (id_t1.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_t1, (id_t1.Dyn->Any.hld_depth == ceu_block_640->depth));
                            }
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_640);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_641 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_641,
                    0
                );
                ceu_acc = ceu_ret_641;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 85, col 1)"
                        );
                    
                
                    op_greater_equals = ceu_acc;
                    ceu_gc_inc(op_greater_equals);
                    ceu_acc = op_greater_equals;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_713 (
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
                        CEU_Block _ceu_block_712 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_712 = &_ceu_block_712; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_712,
                                            ceu_hold_chk_set(&ceu_block_712->dyns, ceu_block_712->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 94, col 26)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_712,
                                            ceu_hold_chk_set(&ceu_block_712->dyns, ceu_block_712->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 94, col 26)"
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

                    CEU_Value ceu_closure_662 = ceu_acc;
                    if (ceu_closure_662.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_712, "prelude.ceu : (lin 95, col 14)", err);
                    }
                    CEU_Frame ceu_frame_662 = { &ceu_closure_662.Dyn->Closure, ceu_block_712 };
                    
                    CEU_Value ceu_args_662[1];
                    
                    ceu_acc = id_v1;
ceu_args_662[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_662.closure->proto (
                        &ceu_frame_662,
                        1,
                        ceu_args_662
                    );
                    ceu_assert_pre(ceu_block_712, ceu_acc, "prelude.ceu : (lin 95, col 14) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_712,
                            ceu_hold_chk_set(&ceu_block_712->dyns, ceu_block_712->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 95, col 5)"
                        );
                    
                
                    id_t1 = ceu_acc;
                    ceu_gc_inc(id_t1);
                    ceu_acc = id_t1;
                    
                
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_693 = ceu_acc;
                    if (ceu_closure_693.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_712, "prelude.ceu : (lin 96, col 25)", err);
                    }
                    CEU_Frame ceu_frame_693 = { &ceu_closure_693.Dyn->Closure, ceu_block_712 };
                    
                    CEU_Value ceu_args_693[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_678 = ceu_acc;
                    if (ceu_closure_678.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_712, "prelude.ceu : (lin 96, col 12)", err);
                    }
                    CEU_Frame ceu_frame_678 = { &ceu_closure_678.Dyn->Closure, ceu_block_712 };
                    
                    CEU_Value ceu_args_678[2];
                    
                    ceu_acc = id_t1;
ceu_args_678[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_675 = ceu_acc;
                    if (ceu_closure_675.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_712, "prelude.ceu : (lin 96, col 15)", err);
                    }
                    CEU_Frame ceu_frame_675 = { &ceu_closure_675.Dyn->Closure, ceu_block_712 };
                    
                    CEU_Value ceu_args_675[1];
                    
                    ceu_acc = id_v2;
ceu_args_675[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_675.closure->proto (
                        &ceu_frame_675,
                        1,
                        ceu_args_675
                    );
                    ceu_assert_pre(ceu_block_712, ceu_acc, "prelude.ceu : (lin 96, col 15) : call error");
                } // CALL
                ceu_args_678[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_678.closure->proto (
                        &ceu_frame_678,
                        2,
                        ceu_args_678
                    );
                    ceu_assert_pre(ceu_block_712, ceu_acc, "prelude.ceu : (lin 96, col 12) : call error");
                } // CALL
                ceu_args_693[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_689 = ceu_acc;
                    if (ceu_closure_689.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_712, "prelude.ceu : (lin 96, col 32)", err);
                    }
                    CEU_Frame ceu_frame_689 = { &ceu_closure_689.Dyn->Closure, ceu_block_712 };
                    
                    CEU_Value ceu_args_689[2];
                    
                    ceu_acc = id_t1;
ceu_args_689[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_689[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_689.closure->proto (
                        &ceu_frame_689,
                        2,
                        ceu_args_689
                    );
                    ceu_assert_pre(ceu_block_712, ceu_acc, "prelude.ceu : (lin 96, col 32) : call error");
                } // CALL
                ceu_args_693[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_693.closure->proto (
                        &ceu_frame_693,
                        2,
                        ceu_args_693
                    );
                    ceu_assert_pre(ceu_block_712, ceu_acc, "prelude.ceu : (lin 96, col 25) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_703 = ceu_block_712;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_701 = ceu_acc;
                    if (ceu_closure_701.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_703, "prelude.ceu : (lin 97, col 9)", err);
                    }
                    CEU_Frame ceu_frame_701 = { &ceu_closure_701.Dyn->Closure, ceu_block_703 };
                    
                    CEU_Value ceu_args_701[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_701[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_701.closure->proto (
                        &ceu_frame_701,
                        1,
                        ceu_args_701
                    );
                    ceu_assert_pre(ceu_block_703, ceu_acc, "prelude.ceu : (lin 97, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_709 = ceu_block_712;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_BOOL, {.Bool=( ((id_v1).Number > (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_712, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 94, col 26)"
                            );
                            
                        
                            if (id_t1.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_t1, (id_t1.Dyn->Any.hld_depth == ceu_block_712->depth));
                            }
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_712);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_713 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_713,
                    0
                );
                ceu_acc = ceu_ret_713;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 94, col 1)"
                        );
                    
                
                    op_greater = ceu_acc;
                    ceu_gc_inc(op_greater);
                    ceu_acc = op_greater;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_785 (
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
                        CEU_Block _ceu_block_784 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_784 = &_ceu_block_784; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_784,
                                            ceu_hold_chk_set(&ceu_block_784->dyns, ceu_block_784->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 103, col 27)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_784,
                                            ceu_hold_chk_set(&ceu_block_784->dyns, ceu_block_784->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 103, col 27)"
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

                    CEU_Value ceu_closure_734 = ceu_acc;
                    if (ceu_closure_734.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_784, "prelude.ceu : (lin 104, col 14)", err);
                    }
                    CEU_Frame ceu_frame_734 = { &ceu_closure_734.Dyn->Closure, ceu_block_784 };
                    
                    CEU_Value ceu_args_734[1];
                    
                    ceu_acc = id_v1;
ceu_args_734[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_734.closure->proto (
                        &ceu_frame_734,
                        1,
                        ceu_args_734
                    );
                    ceu_assert_pre(ceu_block_784, ceu_acc, "prelude.ceu : (lin 104, col 14) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_784,
                            ceu_hold_chk_set(&ceu_block_784->dyns, ceu_block_784->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 104, col 5)"
                        );
                    
                
                    id_t1 = ceu_acc;
                    ceu_gc_inc(id_t1);
                    ceu_acc = id_t1;
                    
                
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_765 = ceu_acc;
                    if (ceu_closure_765.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_784, "prelude.ceu : (lin 105, col 25)", err);
                    }
                    CEU_Frame ceu_frame_765 = { &ceu_closure_765.Dyn->Closure, ceu_block_784 };
                    
                    CEU_Value ceu_args_765[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_750 = ceu_acc;
                    if (ceu_closure_750.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_784, "prelude.ceu : (lin 105, col 12)", err);
                    }
                    CEU_Frame ceu_frame_750 = { &ceu_closure_750.Dyn->Closure, ceu_block_784 };
                    
                    CEU_Value ceu_args_750[2];
                    
                    ceu_acc = id_t1;
ceu_args_750[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_747 = ceu_acc;
                    if (ceu_closure_747.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_784, "prelude.ceu : (lin 105, col 15)", err);
                    }
                    CEU_Frame ceu_frame_747 = { &ceu_closure_747.Dyn->Closure, ceu_block_784 };
                    
                    CEU_Value ceu_args_747[1];
                    
                    ceu_acc = id_v2;
ceu_args_747[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_747.closure->proto (
                        &ceu_frame_747,
                        1,
                        ceu_args_747
                    );
                    ceu_assert_pre(ceu_block_784, ceu_acc, "prelude.ceu : (lin 105, col 15) : call error");
                } // CALL
                ceu_args_750[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_750.closure->proto (
                        &ceu_frame_750,
                        2,
                        ceu_args_750
                    );
                    ceu_assert_pre(ceu_block_784, ceu_acc, "prelude.ceu : (lin 105, col 12) : call error");
                } // CALL
                ceu_args_765[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_761 = ceu_acc;
                    if (ceu_closure_761.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_784, "prelude.ceu : (lin 105, col 32)", err);
                    }
                    CEU_Frame ceu_frame_761 = { &ceu_closure_761.Dyn->Closure, ceu_block_784 };
                    
                    CEU_Value ceu_args_761[2];
                    
                    ceu_acc = id_t1;
ceu_args_761[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_761[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_761.closure->proto (
                        &ceu_frame_761,
                        2,
                        ceu_args_761
                    );
                    ceu_assert_pre(ceu_block_784, ceu_acc, "prelude.ceu : (lin 105, col 32) : call error");
                } // CALL
                ceu_args_765[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_765.closure->proto (
                        &ceu_frame_765,
                        2,
                        ceu_args_765
                    );
                    ceu_assert_pre(ceu_block_784, ceu_acc, "prelude.ceu : (lin 105, col 25) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_775 = ceu_block_784;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_773 = ceu_acc;
                    if (ceu_closure_773.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_775, "prelude.ceu : (lin 106, col 9)", err);
                    }
                    CEU_Frame ceu_frame_773 = { &ceu_closure_773.Dyn->Closure, ceu_block_775 };
                    
                    CEU_Value ceu_args_773[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_773[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_773.closure->proto (
                        &ceu_frame_773,
                        1,
                        ceu_args_773
                    );
                    ceu_assert_pre(ceu_block_775, ceu_acc, "prelude.ceu : (lin 106, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_781 = ceu_block_784;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_BOOL, {.Bool=( ((id_v1).Number <= (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_784, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 103, col 27)"
                            );
                            
                        
                            if (id_t1.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_t1, (id_t1.Dyn->Any.hld_depth == ceu_block_784->depth));
                            }
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_784);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_785 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_785,
                    0
                );
                ceu_acc = ceu_ret_785;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 103, col 1)"
                        );
                    
                
                    op_less_equals = ceu_acc;
                    ceu_gc_inc(op_less_equals);
                    ceu_acc = op_less_equals;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_857 (
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
                        CEU_Block _ceu_block_856 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_856 = &_ceu_block_856; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_856,
                                            ceu_hold_chk_set(&ceu_block_856->dyns, ceu_block_856->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 112, col 26)"
                                        );
                                        id_v1 = ceu_args[0];
                                    } else {
                                        id_v1 = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_856,
                                            ceu_hold_chk_set(&ceu_block_856->dyns, ceu_block_856->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "prelude.ceu : (lin 112, col 26)"
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

                    CEU_Value ceu_closure_806 = ceu_acc;
                    if (ceu_closure_806.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_856, "prelude.ceu : (lin 113, col 14)", err);
                    }
                    CEU_Frame ceu_frame_806 = { &ceu_closure_806.Dyn->Closure, ceu_block_856 };
                    
                    CEU_Value ceu_args_806[1];
                    
                    ceu_acc = id_v1;
ceu_args_806[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_806.closure->proto (
                        &ceu_frame_806,
                        1,
                        ceu_args_806
                    );
                    ceu_assert_pre(ceu_block_856, ceu_acc, "prelude.ceu : (lin 113, col 14) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_856,
                            ceu_hold_chk_set(&ceu_block_856->dyns, ceu_block_856->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 113, col 5)"
                        );
                    
                
                    id_t1 = ceu_acc;
                    ceu_gc_inc(id_t1);
                    ceu_acc = id_t1;
                    
                
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_bar_bar;

                    CEU_Value ceu_closure_837 = ceu_acc;
                    if (ceu_closure_837.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_856, "prelude.ceu : (lin 114, col 25)", err);
                    }
                    CEU_Frame ceu_frame_837 = { &ceu_closure_837.Dyn->Closure, ceu_block_856 };
                    
                    CEU_Value ceu_args_837[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_822 = ceu_acc;
                    if (ceu_closure_822.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_856, "prelude.ceu : (lin 114, col 12)", err);
                    }
                    CEU_Frame ceu_frame_822 = { &ceu_closure_822.Dyn->Closure, ceu_block_856 };
                    
                    CEU_Value ceu_args_822[2];
                    
                    ceu_acc = id_t1;
ceu_args_822[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_819 = ceu_acc;
                    if (ceu_closure_819.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_856, "prelude.ceu : (lin 114, col 15)", err);
                    }
                    CEU_Frame ceu_frame_819 = { &ceu_closure_819.Dyn->Closure, ceu_block_856 };
                    
                    CEU_Value ceu_args_819[1];
                    
                    ceu_acc = id_v2;
ceu_args_819[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_819.closure->proto (
                        &ceu_frame_819,
                        1,
                        ceu_args_819
                    );
                    ceu_assert_pre(ceu_block_856, ceu_acc, "prelude.ceu : (lin 114, col 15) : call error");
                } // CALL
                ceu_args_822[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_822.closure->proto (
                        &ceu_frame_822,
                        2,
                        ceu_args_822
                    );
                    ceu_assert_pre(ceu_block_856, ceu_acc, "prelude.ceu : (lin 114, col 12) : call error");
                } // CALL
                ceu_args_837[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash_equals;

                    CEU_Value ceu_closure_833 = ceu_acc;
                    if (ceu_closure_833.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_856, "prelude.ceu : (lin 114, col 32)", err);
                    }
                    CEU_Frame ceu_frame_833 = { &ceu_closure_833.Dyn->Closure, ceu_block_856 };
                    
                    CEU_Value ceu_args_833[2];
                    
                    ceu_acc = id_t1;
ceu_args_833[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_833[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_833.closure->proto (
                        &ceu_frame_833,
                        2,
                        ceu_args_833
                    );
                    ceu_assert_pre(ceu_block_856, ceu_acc, "prelude.ceu : (lin 114, col 32) : call error");
                } // CALL
                ceu_args_837[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_837.closure->proto (
                        &ceu_frame_837,
                        2,
                        ceu_args_837
                    );
                    ceu_assert_pre(ceu_block_856, ceu_acc, "prelude.ceu : (lin 114, col 25) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_847 = ceu_block_856;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_error;

                    CEU_Value ceu_closure_845 = ceu_acc;
                    if (ceu_closure_845.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_847, "prelude.ceu : (lin 115, col 9)", err);
                    }
                    CEU_Frame ceu_frame_845 = { &ceu_closure_845.Dyn->Closure, ceu_block_847 };
                    
                    CEU_Value ceu_args_845[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_error} });ceu_args_845[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_845.closure->proto (
                        &ceu_frame_845,
                        1,
                        ceu_args_845
                    );
                    ceu_assert_pre(ceu_block_847, ceu_acc, "prelude.ceu : (lin 115, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_853 = ceu_block_856;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_BOOL, {.Bool=( ((id_v1).Number < (id_v2).Number))} });
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_856, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 112, col 26)"
                            );
                            
                        
                            if (id_t1.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_t1, (id_t1.Dyn->Any.hld_depth == ceu_block_856->depth));
                            }
                        
                        
                            
                                if (id_v1.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v1, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v1.Dyn));
                                }
                                
                                if (id_v2.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v2, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v2.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_856);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_857 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_857,
                    0
                );
                ceu_acc = ceu_ret_857;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 112, col 1)"
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
                    CEU_Value ceu_proto_1001 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v;
                            CEU_Block* _id_v_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_1000 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_1000 = &_ceu_block_1000; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1000,
                                            ceu_hold_chk_set(&ceu_block_1000->dyns, ceu_block_1000->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 124, col 26)"
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

                    CEU_Value ceu_closure_884 = ceu_acc;
                    if (ceu_closure_884.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1000, "prelude.ceu : (lin 125, col 16)", err);
                    }
                    CEU_Frame ceu_frame_884 = { &ceu_closure_884.Dyn->Closure, ceu_block_1000 };
                    
                    CEU_Value ceu_args_884[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_878 = ceu_acc;
                    if (ceu_closure_878.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1000, "prelude.ceu : (lin 125, col 8)", err);
                    }
                    CEU_Frame ceu_frame_878 = { &ceu_closure_878.Dyn->Closure, ceu_block_1000 };
                    
                    CEU_Value ceu_args_878[1];
                    
                    ceu_acc = id_v;
ceu_args_878[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_878.closure->proto (
                        &ceu_frame_878,
                        1,
                        ceu_args_878
                    );
                    ceu_assert_pre(ceu_block_1000, ceu_acc, "prelude.ceu : (lin 125, col 8) : call error");
                } // CALL
                ceu_args_884[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_pointer} });ceu_args_884[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_884.closure->proto (
                        &ceu_frame_884,
                        2,
                        ceu_args_884
                    );
                    ceu_assert_pre(ceu_block_1000, ceu_acc, "prelude.ceu : (lin 125, col 16) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_960 = (CEU_Block) { (ceu_block_1000->depth + 1), 0, {.block=ceu_block_1000}, NULL };
                        CEU_Block* ceu_block_960 = &_ceu_block_960; 
                        
                        
                        
                            CEU_Value id_i = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_n = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_str = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        ceu_assert_pre(
                            ceu_block_960,
                            ceu_hold_chk_set(&ceu_block_960->dyns, ceu_block_960->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 126, col 9)"
                        );
                    
                
                    id_i = ceu_acc;
                    ceu_gc_inc(id_i);
                    ceu_acc = id_i;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( strlen((id_v).Pointer))} });
                        ceu_assert_pre(
                            ceu_block_960,
                            ceu_hold_chk_set(&ceu_block_960->dyns, ceu_block_960->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 127, col 9)"
                        );
                    
                
                    id_n = ceu_acc;
                    ceu_gc_inc(id_n);
                    ceu_acc = id_n;
                    
                
                // DCL | 
                
                { // VECTOR | 
                    CEU_Value ceu_vec_902 = ceu_vector_create(ceu_block_960);
                    
                    ceu_acc = ceu_vec_902;
                }
                
                        ceu_assert_pre(
                            ceu_block_960,
                            ceu_hold_chk_set(&ceu_block_960->dyns, ceu_block_960->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 128, col 9)"
                        );
                    
                
                    id_str = ceu_acc;
                    ceu_gc_inc(id_str);
                    ceu_acc = id_str;
                    
                
                    CEU_Block* ceu_block_951 = ceu_block_960;
                    // >>> block
                    
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_917 = ceu_acc;
                    if (ceu_closure_917.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_951, "prelude.ceu : (lin 130, col 25)", err);
                    }
                    CEU_Frame ceu_frame_917 = { &ceu_closure_917.Dyn->Closure, ceu_block_951 };
                    
                    CEU_Value ceu_args_917[2];
                    
                    ceu_acc = id_i;
ceu_args_917[0] = ceu_acc;
ceu_acc = id_n;
ceu_args_917[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_917.closure->proto (
                        &ceu_frame_917,
                        2,
                        ceu_args_917
                    );
                    ceu_assert_pre(ceu_block_951, ceu_acc, "prelude.ceu : (lin 130, col 25) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                { // SET | 
                    ceu_acc = ((CEU_Value){ CEU_VALUE_CHAR, {.Char=( ((char*)(id_v).Pointer)[(int)(id_i).Number])} });
                    CEU_Value ceu_set_935 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        
                { // CALL | 
                    ceu_acc = op_hash;

                    CEU_Value ceu_closure_929 = ceu_acc;
                    if (ceu_closure_929.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_951, "prelude.ceu : (lin 131, col 21)", err);
                    }
                    CEU_Frame ceu_frame_929 = { &ceu_closure_929.Dyn->Closure, ceu_block_951 };
                    
                    CEU_Value ceu_args_929[1];
                    
                    ceu_acc = id_str;
ceu_args_929[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_929.closure->proto (
                        &ceu_frame_929,
                        1,
                        ceu_args_929
                    );
                    ceu_assert_pre(ceu_block_951, ceu_acc, "prelude.ceu : (lin 131, col 21) : call error");
                } // CALL
                
                        CEU_Value ceu_idx_931 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_str;

                    ceu_assert_pre(ceu_block_951, ceu_col_check(ceu_acc, ceu_idx_931), "prelude.ceu : (lin 131, col 17)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_931.Number, (ceu_set_935));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_931.Number, (ceu_set_935));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_931, (ceu_set_935));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_951, ok, "prelude.ceu : (lin 131, col 17)");
                        
                }
                
                    ceu_acc = ceu_set_935;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_947 = ceu_acc;
                    if (ceu_closure_947.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_951, "prelude.ceu : (lin 132, col 23)", err);
                    }
                    CEU_Frame ceu_frame_947 = { &ceu_closure_947.Dyn->Closure, ceu_block_951 };
                    
                    CEU_Value ceu_args_947[2];
                    
                    ceu_acc = id_i;
ceu_args_947[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_947[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_947.closure->proto (
                        &ceu_frame_947,
                        2,
                        ceu_args_947
                    );
                    ceu_assert_pre(ceu_block_951, ceu_acc, "prelude.ceu : (lin 132, col 23) : call error");
                } // CALL
                
                    CEU_Value ceu_set_948 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_951,
                                ceu_hold_chk_set(&ceu_block_960->dyns, ceu_block_960->depth, CEU_HOLD_MUTAB, (ceu_set_948), 0, "set error"),
                                "prelude.ceu : (lin 132, col 17)"
                            );
                            ceu_gc_inc((ceu_set_948));
                            ceu_gc_dec(id_i, 1);
                            id_i = (ceu_set_948);
                        }
                        
                    ceu_acc = ceu_set_948;
                }
                
                    }
                
                    // <<< block
                    
                        { // ACC - DROP
                            CEU_Value ceu_956 = id_str;
                            CEU_Frame ceu_frame_956 = { NULL, ceu_block_960 };
                            ceu_assert_pre(ceu_block_960, ceu_drop_f(&ceu_frame_956, 1, &ceu_956), "prelude.ceu : (lin 134, col 14)");
                            ceu_gc_dec(ceu_956, 0);
                            id_str = (CEU_Value) { CEU_VALUE_NIL };
                            ceu_acc = ceu_956;
                        }
                        
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_960, 
                                ceu_hold_chk_set(&ceu_block_1000->dyns, ceu_block_1000->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 125, col 28)"
                            );
                            
                        
                            if (id_i.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_i, (id_i.Dyn->Any.hld_depth == ceu_block_960->depth));
                            }
                        
                            if (id_n.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_n, (id_n.Dyn->Any.hld_depth == ceu_block_960->depth));
                            }
                        
                            if (id_str.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_str, (id_str.Dyn->Any.hld_depth == ceu_block_960->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_960);
                    }
                    
                    } else {
                        
                    CEU_Block* ceu_block_997 = ceu_block_1000;
                    // >>> block
                    
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_976 = ceu_acc;
                    if (ceu_closure_976.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_997, "prelude.ceu : (lin 136, col 20)", err);
                    }
                    CEU_Frame ceu_frame_976 = { &ceu_closure_976.Dyn->Closure, ceu_block_997 };
                    
                    CEU_Value ceu_args_976[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_970 = ceu_acc;
                    if (ceu_closure_970.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_997, "prelude.ceu : (lin 136, col 12)", err);
                    }
                    CEU_Frame ceu_frame_970 = { &ceu_closure_970.Dyn->Closure, ceu_block_997 };
                    
                    CEU_Value ceu_args_970[1];
                    
                    ceu_acc = id_v;
ceu_args_970[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_970.closure->proto (
                        &ceu_frame_970,
                        1,
                        ceu_args_970
                    );
                    ceu_assert_pre(ceu_block_997, ceu_acc, "prelude.ceu : (lin 136, col 12) : call error");
                } // CALL
                ceu_args_976[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_number} });ceu_args_976[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_976.closure->proto (
                        &ceu_frame_976,
                        2,
                        ceu_args_976
                    );
                    ceu_assert_pre(ceu_block_997, ceu_acc, "prelude.ceu : (lin 136, col 20) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_988 = ceu_block_997;
                    // >>> block
                    
            static char str[255];
            snprintf(str, 255, "%g", (id_v).Number);
            
ceu_acc = ((CEU_Value){ CEU_VALUE_NIL });
                { // CALL | 
                    ceu_acc = id_to_dash_string;

                    CEU_Value ceu_closure_986 = ceu_acc;
                    if (ceu_closure_986.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_988, "prelude.ceu : (lin 141, col 13)", err);
                    }
                    CEU_Frame ceu_frame_986 = { &ceu_closure_986.Dyn->Closure, ceu_block_988 };
                    
                    CEU_Value ceu_args_986[1];
                    
                    ceu_acc = ((CEU_Value){ CEU_VALUE_POINTER, {.Pointer=( str)} });ceu_args_986[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_986.closure->proto (
                        &ceu_frame_986,
                        1,
                        ceu_args_986
                    );
                    ceu_assert_pre(ceu_block_988, ceu_acc, "prelude.ceu : (lin 141, col 13) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_994 = ceu_block_997;
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
                                ceu_block_1000, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 124, col 26)"
                            );
                            
                        
                        
                            
                                if (id_v.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_1000);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_1001 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_1001,
                    0
                );
                ceu_acc = ceu_ret_1001;
                
                // UPVALS
                
                
                    CEU_Value ceu_set_1002 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1238,
                                ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, (ceu_set_1002), 0, "set error"),
                                "prelude.ceu : (lin 124, col 5)"
                            );
                            ceu_gc_inc((ceu_set_1002));
                            ceu_gc_dec(id_to_dash_string, 1);
                            id_to_dash_string = (ceu_set_1002);
                        }
                        
                    ceu_acc = ceu_set_1002;
                }
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_1115 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v;
                            CEU_Block* _id_v_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_1114 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_1114 = &_ceu_block_1114; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1114,
                                            ceu_hold_chk_set(&ceu_block_1114->dyns, ceu_block_1114->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 148, col 26)"
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

                    CEU_Value ceu_closure_1024 = ceu_acc;
                    if (ceu_closure_1024.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1114, "prelude.ceu : (lin 149, col 16)", err);
                    }
                    CEU_Frame ceu_frame_1024 = { &ceu_closure_1024.Dyn->Closure, ceu_block_1114 };
                    
                    CEU_Value ceu_args_1024[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_1018 = ceu_acc;
                    if (ceu_closure_1018.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1114, "prelude.ceu : (lin 149, col 8)", err);
                    }
                    CEU_Frame ceu_frame_1018 = { &ceu_closure_1018.Dyn->Closure, ceu_block_1114 };
                    
                    CEU_Value ceu_args_1018[1];
                    
                    ceu_acc = id_v;
ceu_args_1018[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1018.closure->proto (
                        &ceu_frame_1018,
                        1,
                        ceu_args_1018
                    );
                    ceu_assert_pre(ceu_block_1114, ceu_acc, "prelude.ceu : (lin 149, col 8) : call error");
                } // CALL
                ceu_args_1024[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_tag} });ceu_args_1024[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1024.closure->proto (
                        &ceu_frame_1024,
                        2,
                        ceu_args_1024
                    );
                    ceu_assert_pre(ceu_block_1114, ceu_acc, "prelude.ceu : (lin 149, col 16) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_1029 = ceu_block_1114;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( (id_v).Tag)} });
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_1111 = ceu_block_1114;
                    // >>> block
                    
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_1045 = ceu_acc;
                    if (ceu_closure_1045.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1111, "prelude.ceu : (lin 152, col 20)", err);
                    }
                    CEU_Frame ceu_frame_1045 = { &ceu_closure_1045.Dyn->Closure, ceu_block_1111 };
                    
                    CEU_Value ceu_args_1045[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_1039 = ceu_acc;
                    if (ceu_closure_1039.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1111, "prelude.ceu : (lin 152, col 12)", err);
                    }
                    CEU_Frame ceu_frame_1039 = { &ceu_closure_1039.Dyn->Closure, ceu_block_1111 };
                    
                    CEU_Value ceu_args_1039[1];
                    
                    ceu_acc = id_v;
ceu_args_1039[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1039.closure->proto (
                        &ceu_frame_1039,
                        1,
                        ceu_args_1039
                    );
                    ceu_assert_pre(ceu_block_1111, ceu_acc, "prelude.ceu : (lin 152, col 12) : call error");
                } // CALL
                ceu_args_1045[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vector} });ceu_args_1045[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1045.closure->proto (
                        &ceu_frame_1045,
                        2,
                        ceu_args_1045
                    );
                    ceu_assert_pre(ceu_block_1111, ceu_acc, "prelude.ceu : (lin 152, col 20) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_1102 = ceu_block_1111;
                    // >>> block
                    
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_greater;

                    CEU_Value ceu_closure_1059 = ceu_acc;
                    if (ceu_closure_1059.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1102, "prelude.ceu : (lin 153, col 19)", err);
                    }
                    CEU_Frame ceu_frame_1059 = { &ceu_closure_1059.Dyn->Closure, ceu_block_1102 };
                    
                    CEU_Value ceu_args_1059[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_hash;

                    CEU_Value ceu_closure_1053 = ceu_acc;
                    if (ceu_closure_1053.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1102, "prelude.ceu : (lin 153, col 16)", err);
                    }
                    CEU_Frame ceu_frame_1053 = { &ceu_closure_1053.Dyn->Closure, ceu_block_1102 };
                    
                    CEU_Value ceu_args_1053[1];
                    
                    ceu_acc = id_v;
ceu_args_1053[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1053.closure->proto (
                        &ceu_frame_1053,
                        1,
                        ceu_args_1053
                    );
                    ceu_assert_pre(ceu_block_1102, ceu_acc, "prelude.ceu : (lin 153, col 16) : call error");
                } // CALL
                ceu_args_1059[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });ceu_args_1059[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1059.closure->proto (
                        &ceu_frame_1059,
                        2,
                        ceu_args_1059
                    );
                    ceu_assert_pre(ceu_block_1102, ceu_acc, "prelude.ceu : (lin 153, col 19) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_1093 = ceu_block_1102;
                    // >>> block
                    
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_1079 = ceu_acc;
                    if (ceu_closure_1079.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1093, "prelude.ceu : (lin 154, col 31)", err);
                    }
                    CEU_Frame ceu_frame_1079 = { &ceu_closure_1079.Dyn->Closure, ceu_block_1093 };
                    
                    CEU_Value ceu_args_1079[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_1073 = ceu_acc;
                    if (ceu_closure_1073.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1093, "prelude.ceu : (lin 154, col 20)", err);
                    }
                    CEU_Frame ceu_frame_1073 = { &ceu_closure_1073.Dyn->Closure, ceu_block_1093 };
                    
                    CEU_Value ceu_args_1073[1];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        CEU_Value ceu_idx_1071 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_v;

                    ceu_assert_pre(ceu_block_1093, ceu_col_check(ceu_acc, ceu_idx_1071), "prelude.ceu : (lin 154, col 25)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1071.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1093, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1071.Number), "prelude.ceu : (lin 154, col 25)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1071);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1073[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1073.closure->proto (
                        &ceu_frame_1073,
                        1,
                        ceu_args_1073
                    );
                    ceu_assert_pre(ceu_block_1093, ceu_acc, "prelude.ceu : (lin 154, col 20) : call error");
                } // CALL
                ceu_args_1079[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_char} });ceu_args_1079[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1079.closure->proto (
                        &ceu_frame_1079,
                        2,
                        ceu_args_1079
                    );
                    ceu_assert_pre(ceu_block_1093, ceu_acc, "prelude.ceu : (lin 154, col 31) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_1084 = ceu_block_1093;
                    // >>> block
                    ceu_acc = ((CEU_Value){ CEU_VALUE_NUMBER, {.Number=( atoi((id_v).Dyn->Vector.buf))} });
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_1090 = ceu_block_1093;
                    // >>> block
                    ceu_acc = id_v;

                    // <<< block
                    
                    }
                }
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_1099 = ceu_block_1102;
                    // >>> block
                    ceu_acc = id_v;

                    // <<< block
                    
                    }
                }
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_1108 = ceu_block_1111;
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
                                ceu_block_1114, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 148, col 26)"
                            );
                            
                        
                        
                            
                                if (id_v.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_1114);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_1115 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_1115,
                    0
                );
                ceu_acc = ceu_ret_1115;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 148, col 1)"
                        );
                    
                
                    id_to_dash_number = ceu_acc;
                    ceu_gc_inc(id_to_dash_number);
                    ceu_acc = id_to_dash_number;
                    
                
                // DCL | 
                 // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_1158 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_v;
                            CEU_Block* _id_v_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_1157 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_1157 = &_ceu_block_1157; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1157,
                                            ceu_hold_chk_set(&ceu_block_1157->dyns, ceu_block_1157->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "prelude.ceu : (lin 168, col 23)"
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

                    CEU_Value ceu_closure_1138 = ceu_acc;
                    if (ceu_closure_1138.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1157, "prelude.ceu : (lin 169, col 16)", err);
                    }
                    CEU_Frame ceu_frame_1138 = { &ceu_closure_1138.Dyn->Closure, ceu_block_1157 };
                    
                    CEU_Value ceu_args_1138[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_type;

                    CEU_Value ceu_closure_1132 = ceu_acc;
                    if (ceu_closure_1132.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1157, "prelude.ceu : (lin 169, col 8)", err);
                    }
                    CEU_Frame ceu_frame_1132 = { &ceu_closure_1132.Dyn->Closure, ceu_block_1157 };
                    
                    CEU_Value ceu_args_1132[1];
                    
                    ceu_acc = id_v;
ceu_args_1132[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1132.closure->proto (
                        &ceu_frame_1132,
                        1,
                        ceu_args_1132
                    );
                    ceu_assert_pre(ceu_block_1157, ceu_acc, "prelude.ceu : (lin 169, col 8) : call error");
                } // CALL
                ceu_args_1138[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_string} });ceu_args_1138[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1138.closure->proto (
                        &ceu_frame_1138,
                        2,
                        ceu_args_1138
                    );
                    ceu_assert_pre(ceu_block_1157, ceu_acc, "prelude.ceu : (lin 169, col 16) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_1148 = ceu_block_1157;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = id_string_dash_to_dash_tag;

                    CEU_Value ceu_closure_1146 = ceu_acc;
                    if (ceu_closure_1146.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1148, "prelude.ceu : (lin 170, col 9)", err);
                    }
                    CEU_Frame ceu_frame_1146 = { &ceu_closure_1146.Dyn->Closure, ceu_block_1148 };
                    
                    CEU_Value ceu_args_1146[1];
                    
                    ceu_acc = id_v;
ceu_args_1146[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1146.closure->proto (
                        &ceu_frame_1146,
                        1,
                        ceu_args_1146
                    );
                    ceu_assert_pre(ceu_block_1148, ceu_acc, "prelude.ceu : (lin 170, col 9) : call error");
                } // CALL
                
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_1154 = ceu_block_1157;
                    // >>> block
                    ceu_acc = id_v;

                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1157, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "prelude.ceu : (lin 168, col 23)"
                            );
                            
                        
                        
                            
                                if (id_v.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_v, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_v.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_1157);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_1158 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_1158,
                    0
                );
                ceu_acc = ceu_ret_1158;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_1238,
                            ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 168, col 1)"
                        );
                    
                
                    id_to_dash_tag = ceu_acc;
                    ceu_gc_inc(id_to_dash_tag);
                    ceu_acc = id_to_dash_tag;
                    
                
                // DCL | 
                
                
                    
                    ceu_gc_inc(id_fib);
                    ceu_acc = id_fib;
                    
                
                { // SET | 
                     // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_1223 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_n;
                            CEU_Block* _id_n_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_1222 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_1222 = &_ceu_block_1222; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1222,
                                            ceu_hold_chk_set(&ceu_block_1222->dyns, ceu_block_1222->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "fib.ceu : (lin 2, col 20)"
                                        );
                                        id_n = ceu_args[0];
                                    } else {
                                        id_n = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        
                { // IF | 
                    
                { // CALL | 
                    ceu_acc = op_less_equals;

                    CEU_Value ceu_closure_1180 = ceu_acc;
                    if (ceu_closure_1180.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1222, "fib.ceu : (lin 3, col 10)", err);
                    }
                    CEU_Frame ceu_frame_1180 = { &ceu_closure_1180.Dyn->Closure, ceu_block_1222 };
                    
                    CEU_Value ceu_args_1180[2];
                    
                    ceu_acc = id_n;
ceu_args_1180[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1180[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1180.closure->proto (
                        &ceu_frame_1180,
                        2,
                        ceu_args_1180
                    );
                    ceu_assert_pre(ceu_block_1222, ceu_acc, "fib.ceu : (lin 3, col 10) : call error");
                } // CALL
                
                    if (ceu_as_bool(ceu_acc)) {
                        
                    CEU_Block* ceu_block_1185 = ceu_block_1222;
                    // >>> block
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });
                    // <<< block
                    
                    } else {
                        
                    CEU_Block* ceu_block_1219 = ceu_block_1222;
                    // >>> block
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1217 = ceu_acc;
                    if (ceu_closure_1217.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1219, "fib.ceu : (lin 6, col 18)", err);
                    }
                    CEU_Frame ceu_frame_1217 = { &ceu_closure_1217.Dyn->Closure, ceu_block_1219 };
                    
                    CEU_Value ceu_args_1217[2];
                    
                    
                { // CALL | 
                    ceu_acc = id_fib;

                    CEU_Value ceu_closure_1200 = ceu_acc;
                    if (ceu_closure_1200.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1219, "fib.ceu : (lin 6, col 9)", err);
                    }
                    CEU_Frame ceu_frame_1200 = { &ceu_closure_1200.Dyn->Closure, ceu_block_1219 };
                    
                    CEU_Value ceu_args_1200[1];
                    
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1198 = ceu_acc;
                    if (ceu_closure_1198.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1219, "fib.ceu : (lin 6, col 14)", err);
                    }
                    CEU_Frame ceu_frame_1198 = { &ceu_closure_1198.Dyn->Closure, ceu_block_1219 };
                    
                    CEU_Value ceu_args_1198[2];
                    
                    ceu_acc = id_n;
ceu_args_1198[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1198[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1198.closure->proto (
                        &ceu_frame_1198,
                        2,
                        ceu_args_1198
                    );
                    ceu_assert_pre(ceu_block_1219, ceu_acc, "fib.ceu : (lin 6, col 14) : call error");
                } // CALL
                ceu_args_1200[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1200.closure->proto (
                        &ceu_frame_1200,
                        1,
                        ceu_args_1200
                    );
                    ceu_assert_pre(ceu_block_1219, ceu_acc, "fib.ceu : (lin 6, col 9) : call error");
                } // CALL
                ceu_args_1217[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = id_fib;

                    CEU_Value ceu_closure_1214 = ceu_acc;
                    if (ceu_closure_1214.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1219, "fib.ceu : (lin 6, col 20)", err);
                    }
                    CEU_Frame ceu_frame_1214 = { &ceu_closure_1214.Dyn->Closure, ceu_block_1219 };
                    
                    CEU_Value ceu_args_1214[1];
                    
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1212 = ceu_acc;
                    if (ceu_closure_1212.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1219, "fib.ceu : (lin 6, col 25)", err);
                    }
                    CEU_Frame ceu_frame_1212 = { &ceu_closure_1212.Dyn->Closure, ceu_block_1219 };
                    
                    CEU_Value ceu_args_1212[2];
                    
                    ceu_acc = id_n;
ceu_args_1212[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=2} });ceu_args_1212[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1212.closure->proto (
                        &ceu_frame_1212,
                        2,
                        ceu_args_1212
                    );
                    ceu_assert_pre(ceu_block_1219, ceu_acc, "fib.ceu : (lin 6, col 25) : call error");
                } // CALL
                ceu_args_1214[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1214.closure->proto (
                        &ceu_frame_1214,
                        1,
                        ceu_args_1214
                    );
                    ceu_assert_pre(ceu_block_1219, ceu_acc, "fib.ceu : (lin 6, col 20) : call error");
                } // CALL
                ceu_args_1217[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1217.closure->proto (
                        &ceu_frame_1217,
                        2,
                        ceu_args_1217
                    );
                    ceu_assert_pre(ceu_block_1219, ceu_acc, "fib.ceu : (lin 6, col 18) : call error");
                } // CALL
                
                    // <<< block
                    
                    }
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1222, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "fib.ceu : (lin 2, col 20)"
                            );
                            
                        
                        
                            
                                if (id_n.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_n, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_n.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_1222);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_1223 = ceu_closure_create (
                    ceu_block_1238,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_1223,
                    0
                );
                ceu_acc = ceu_ret_1223;
                
                // UPVALS
                
                
                    CEU_Value ceu_set_1224 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1238,
                                ceu_hold_chk_set(&ceu_block_1238->dyns, ceu_block_1238->depth, CEU_HOLD_MUTAB, (ceu_set_1224), 0, "set error"),
                                "fib.ceu : (lin 2, col 5)"
                            );
                            ceu_gc_inc((ceu_set_1224));
                            ceu_gc_dec(id_fib, 1);
                            id_fib = (ceu_set_1224);
                        }
                        
                    ceu_acc = ceu_set_1224;
                }
                
                { // CALL | 
                    ceu_acc = id_println;

                    CEU_Value ceu_closure_1236 = ceu_acc;
                    if (ceu_closure_1236.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1238, "fib.ceu : (lin 10, col 1)", err);
                    }
                    CEU_Frame ceu_frame_1236 = { &ceu_closure_1236.Dyn->Closure, ceu_block_1238 };
                    
                    CEU_Value ceu_args_1236[1];
                    
                    
                { // CALL | 
                    ceu_acc = id_fib;

                    CEU_Value ceu_closure_1234 = ceu_acc;
                    if (ceu_closure_1234.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1238, "fib.ceu : (lin 10, col 9)", err);
                    }
                    CEU_Frame ceu_frame_1234 = { &ceu_closure_1234.Dyn->Closure, ceu_block_1238 };
                    
                    CEU_Value ceu_args_1234[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=35} });ceu_args_1234[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1234.closure->proto (
                        &ceu_frame_1234,
                        1,
                        ceu_args_1234
                    );
                    ceu_assert_pre(ceu_block_1238, ceu_acc, "fib.ceu : (lin 10, col 9) : call error");
                } // CALL
                ceu_args_1236[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1236.closure->proto (
                        &ceu_frame_1236,
                        1,
                        ceu_args_1236
                    );
                    ceu_assert_pre(ceu_block_1238, ceu_acc, "fib.ceu : (lin 10, col 1) : call error");
                } // CALL
                
                        // <<< block
                        
                        
                        
                            if (op_ampersand_ampersand.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_ampersand_ampersand, (op_ampersand_ampersand.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (op_bar_bar.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_bar_bar, (op_bar_bar.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (op_plus.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_plus, (op_plus.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (op_minus.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_minus, (op_minus.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (op_asterisk.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_asterisk, (op_asterisk.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (op_asterisk_asterisk.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_asterisk_asterisk, (op_asterisk_asterisk.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (op_slash.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_slash, (op_slash.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (op_slash_slash.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_slash_slash, (op_slash_slash.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (op_null.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_null, (op_null.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (op_greater_equals.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_greater_equals, (op_greater_equals.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (op_greater.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_greater, (op_greater.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (op_less_equals.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_less_equals, (op_less_equals.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (op_less.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_less, (op_less.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (id_to_dash_string.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_to_dash_string, (id_to_dash_string.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (id_to_dash_number.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_to_dash_number, (id_to_dash_number.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (id_to_dash_tag.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_to_dash_tag, (id_to_dash_tag.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                            if (id_fib.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_fib, (id_fib.Dyn->Any.hld_depth == ceu_block_1238->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_1238);
                    }
                    
            return 0;
        }
    