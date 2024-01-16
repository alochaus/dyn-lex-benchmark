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
                
                #define CEU_TAG_x (15)
                CEU_Tags_Names ceu_tag_x = { CEU_TAG_x, ":x", &ceu_tag_string };
                
                #define CEU_TAG_y (16)
                CEU_Tags_Names ceu_tag_y = { CEU_TAG_y, ":y", &ceu_tag_x };
                
                #define CEU_TAG_z (17)
                CEU_Tags_Names ceu_tag_z = { CEU_TAG_z, ":z", &ceu_tag_y };
                
                #define CEU_TAG_vx (18)
                CEU_Tags_Names ceu_tag_vx = { CEU_TAG_vx, ":vx", &ceu_tag_z };
                
                #define CEU_TAG_vy (19)
                CEU_Tags_Names ceu_tag_vy = { CEU_TAG_vy, ":vy", &ceu_tag_vx };
                
                #define CEU_TAG_vz (20)
                CEU_Tags_Names ceu_tag_vz = { CEU_TAG_vz, ":vz", &ceu_tag_vy };
                
                #define CEU_TAG_mass (21)
                CEU_Tags_Names ceu_tag_mass = { CEU_TAG_mass, ":mass", &ceu_tag_vz };
                
                CEU_Tags_Names* CEU_TAGS = &ceu_tag_mass;
            
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
                        CEU_Block _ceu_block_3004 = (CEU_Block) { 1, 1, {.frame=&_ceu_frame_}, NULL };
                        CEU_Block* ceu_block_3004 = &_ceu_block_3004; 
                        
                            // main block varargs (...)
                            CEU_Value id__dot__dot__dot_ = ceu_tuple_create(ceu_block_3004, ceu_argc);
                            for (int i=0; i<ceu_argc; i++) {
                                CEU_Value vec = ceu_vector_from_c_string(ceu_block_3004, ceu_argv[i]);
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
                        
                            CEU_Value id_new_body = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_offset_momentum = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_advance = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_advance_multiple_steps = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_energy = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_PI = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_SOLAR_MASS = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_DAYS_PER_YEAR = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_bodies = (CEU_Value) { CEU_VALUE_NIL };
                        
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_31,
                    0
                );
                ceu_acc = ceu_ret_31;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_60,
                    0
                );
                ceu_acc = ceu_ret_60;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_76,
                    0
                );
                ceu_acc = ceu_ret_76;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_189,
                    0
                );
                ceu_acc = ceu_ret_189;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_255,
                    0
                );
                ceu_acc = ceu_ret_255;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_321,
                    0
                );
                ceu_acc = ceu_ret_321;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_387,
                    0
                );
                ceu_acc = ceu_ret_387;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_453,
                    0
                );
                ceu_acc = ceu_ret_453;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_519,
                    0
                );
                ceu_acc = ceu_ret_519;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_591,
                    0
                );
                ceu_acc = ceu_ret_591;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_663,
                    0
                );
                ceu_acc = ceu_ret_663;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_735,
                    0
                );
                ceu_acc = ceu_ret_735;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_807,
                    0
                );
                ceu_acc = ceu_ret_807;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
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
                                ceu_block_3004,
                                ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, (ceu_set_952), 0, "set error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_1065,
                    0
                );
                ceu_acc = ceu_ret_1065;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
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
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_1108,
                    0
                );
                ceu_acc = ceu_ret_1108;
                
                // UPVALS
                
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "prelude.ceu : (lin 164, col 1)"
                        );
                    
                
                    id_to_dash_tag = ceu_acc;
                    ceu_gc_inc(id_to_dash_tag);
                    ceu_acc = id_to_dash_tag;
                    
                
                // DCL | 
                
                
                    
                    ceu_gc_inc(id_new_body);
                    ceu_acc = id_new_body;
                    
                
                { // SET | 
                     // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_1195 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_x;
                            CEU_Block* _id_x_;
                            
                            CEU_Value id_y;
                            CEU_Block* _id_y_;
                            
                            CEU_Value id_z;
                            CEU_Block* _id_z_;
                            
                            CEU_Value id_vx;
                            CEU_Block* _id_vx_;
                            
                            CEU_Value id_vy;
                            CEU_Block* _id_vy_;
                            
                            CEU_Value id_vz;
                            CEU_Block* _id_vz_;
                            
                            CEU_Value id_mass;
                            CEU_Block* _id_mass_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_1194 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_1194 = &_ceu_block_1194; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1194,
                                            ceu_hold_chk_set(&ceu_block_1194->dyns, ceu_block_1194->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "nbody.ceu : (lin 2, col 49)"
                                        );
                                        id_x = ceu_args[0];
                                    } else {
                                        id_x = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1194,
                                            ceu_hold_chk_set(&ceu_block_1194->dyns, ceu_block_1194->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "nbody.ceu : (lin 2, col 49)"
                                        );
                                        id_y = ceu_args[1];
                                    } else {
                                        id_y = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (2 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1194,
                                            ceu_hold_chk_set(&ceu_block_1194->dyns, ceu_block_1194->depth, CEU_HOLD_FLEET, ceu_args[2], 1, "argument error"),
                                            "nbody.ceu : (lin 2, col 49)"
                                        );
                                        id_z = ceu_args[2];
                                    } else {
                                        id_z = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (3 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1194,
                                            ceu_hold_chk_set(&ceu_block_1194->dyns, ceu_block_1194->depth, CEU_HOLD_FLEET, ceu_args[3], 1, "argument error"),
                                            "nbody.ceu : (lin 2, col 49)"
                                        );
                                        id_vx = ceu_args[3];
                                    } else {
                                        id_vx = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (4 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1194,
                                            ceu_hold_chk_set(&ceu_block_1194->dyns, ceu_block_1194->depth, CEU_HOLD_FLEET, ceu_args[4], 1, "argument error"),
                                            "nbody.ceu : (lin 2, col 49)"
                                        );
                                        id_vy = ceu_args[4];
                                    } else {
                                        id_vy = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (5 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1194,
                                            ceu_hold_chk_set(&ceu_block_1194->dyns, ceu_block_1194->depth, CEU_HOLD_FLEET, ceu_args[5], 1, "argument error"),
                                            "nbody.ceu : (lin 2, col 49)"
                                        );
                                        id_vz = ceu_args[5];
                                    } else {
                                        id_vz = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (6 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1194,
                                            ceu_hold_chk_set(&ceu_block_1194->dyns, ceu_block_1194->depth, CEU_HOLD_FLEET, ceu_args[6], 1, "argument error"),
                                            "nbody.ceu : (lin 2, col 49)"
                                        );
                                        id_mass = ceu_args[6];
                                    } else {
                                        id_mass = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        
                { // DICT | 
                    CEU_Value ceu_dict_1191 = ceu_dict_create(ceu_block_1194);
                    
                        {
                            ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_x} });
                            CEU_Value ceu_key_1191 = ceu_acc;
                            ceu_acc = id_x;

                            CEU_Value ceu_val_1191 = ceu_acc;
                            ceu_assert_pre(
                                ceu_block_1194,
                                ceu_dict_set(&ceu_dict_1191.Dyn->Dict, ceu_key_1191, ceu_val_1191),
                                "nbody.ceu : (lin 3, col 5)"
                            );
                        }
                    
                        {
                            ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_y} });
                            CEU_Value ceu_key_1191 = ceu_acc;
                            ceu_acc = id_y;

                            CEU_Value ceu_val_1191 = ceu_acc;
                            ceu_assert_pre(
                                ceu_block_1194,
                                ceu_dict_set(&ceu_dict_1191.Dyn->Dict, ceu_key_1191, ceu_val_1191),
                                "nbody.ceu : (lin 3, col 5)"
                            );
                        }
                    
                        {
                            ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_z} });
                            CEU_Value ceu_key_1191 = ceu_acc;
                            ceu_acc = id_z;

                            CEU_Value ceu_val_1191 = ceu_acc;
                            ceu_assert_pre(
                                ceu_block_1194,
                                ceu_dict_set(&ceu_dict_1191.Dyn->Dict, ceu_key_1191, ceu_val_1191),
                                "nbody.ceu : (lin 3, col 5)"
                            );
                        }
                    
                        {
                            ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vx} });
                            CEU_Value ceu_key_1191 = ceu_acc;
                            ceu_acc = id_vx;

                            CEU_Value ceu_val_1191 = ceu_acc;
                            ceu_assert_pre(
                                ceu_block_1194,
                                ceu_dict_set(&ceu_dict_1191.Dyn->Dict, ceu_key_1191, ceu_val_1191),
                                "nbody.ceu : (lin 3, col 5)"
                            );
                        }
                    
                        {
                            ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vy} });
                            CEU_Value ceu_key_1191 = ceu_acc;
                            ceu_acc = id_vy;

                            CEU_Value ceu_val_1191 = ceu_acc;
                            ceu_assert_pre(
                                ceu_block_1194,
                                ceu_dict_set(&ceu_dict_1191.Dyn->Dict, ceu_key_1191, ceu_val_1191),
                                "nbody.ceu : (lin 3, col 5)"
                            );
                        }
                    
                        {
                            ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vz} });
                            CEU_Value ceu_key_1191 = ceu_acc;
                            ceu_acc = id_vz;

                            CEU_Value ceu_val_1191 = ceu_acc;
                            ceu_assert_pre(
                                ceu_block_1194,
                                ceu_dict_set(&ceu_dict_1191.Dyn->Dict, ceu_key_1191, ceu_val_1191),
                                "nbody.ceu : (lin 3, col 5)"
                            );
                        }
                    
                        {
                            ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_mass} });
                            CEU_Value ceu_key_1191 = ceu_acc;
                            ceu_acc = id_mass;

                            CEU_Value ceu_val_1191 = ceu_acc;
                            ceu_assert_pre(
                                ceu_block_1194,
                                ceu_dict_set(&ceu_dict_1191.Dyn->Dict, ceu_key_1191, ceu_val_1191),
                                "nbody.ceu : (lin 3, col 5)"
                            );
                        }
                    
                    ceu_acc = ceu_dict_1191;
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1194, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "nbody.ceu : (lin 2, col 49)"
                            );
                            
                        
                        
                            
                                if (id_x.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_x, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_x.Dyn));
                                }
                                
                                if (id_y.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_y, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_y.Dyn));
                                }
                                
                                if (id_z.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_z, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_z.Dyn));
                                }
                                
                                if (id_vx.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_vx, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_vx.Dyn));
                                }
                                
                                if (id_vy.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_vy, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_vy.Dyn));
                                }
                                
                                if (id_vz.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_vz, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_vz.Dyn));
                                }
                                
                                if (id_mass.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_mass, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_mass.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_1194);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_1195 = ceu_closure_create (
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_1195,
                    0
                );
                ceu_acc = ceu_ret_1195;
                
                // UPVALS
                
                
                    CEU_Value ceu_set_1196 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_3004,
                                ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, (ceu_set_1196), 0, "set error"),
                                "nbody.ceu : (lin 2, col 5)"
                            );
                            ceu_gc_inc((ceu_set_1196));
                            ceu_gc_dec(id_new_body, 1);
                            id_new_body = (ceu_set_1196);
                        }
                        
                    ceu_acc = ceu_set_1196;
                }
                
                // DCL | 
                
                
                    
                    ceu_gc_inc(id_offset_momentum);
                    ceu_acc = id_offset_momentum;
                    
                
                { // SET | 
                     // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_1492 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_bodies;
                            CEU_Block* _id_bodies_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_1491 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_1491 = &_ceu_block_1491; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_1491,
                                            ceu_hold_chk_set(&ceu_block_1491->dyns, ceu_block_1491->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "nbody.ceu : (lin 15, col 37)"
                                        );
                                        id_bodies = ceu_args[0];
                                    } else {
                                        id_bodies = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                            CEU_Value id_n = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_px = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_py = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_pz = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_i = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_sun = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_solar_mass = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_hash;

                    CEU_Value ceu_closure_1217 = ceu_acc;
                    if (ceu_closure_1217.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1491, "nbody.ceu : (lin 16, col 13)", err);
                    }
                    CEU_Frame ceu_frame_1217 = { &ceu_closure_1217.Dyn->Closure, ceu_block_1491 };
                    
                    CEU_Value ceu_args_1217[1];
                    
                    ceu_acc = id_bodies;
ceu_args_1217[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1217.closure->proto (
                        &ceu_frame_1217,
                        1,
                        ceu_args_1217
                    );
                    ceu_assert_pre(ceu_block_1491, ceu_acc, "nbody.ceu : (lin 16, col 13) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_1491,
                            ceu_hold_chk_set(&ceu_block_1491->dyns, ceu_block_1491->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 16, col 5)"
                        );
                    
                
                    id_n = ceu_acc;
                    ceu_gc_inc(id_n);
                    ceu_acc = id_n;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0} });
                        ceu_assert_pre(
                            ceu_block_1491,
                            ceu_hold_chk_set(&ceu_block_1491->dyns, ceu_block_1491->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 17, col 5)"
                        );
                    
                
                    id_px = ceu_acc;
                    ceu_gc_inc(id_px);
                    ceu_acc = id_px;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0} });
                        ceu_assert_pre(
                            ceu_block_1491,
                            ceu_hold_chk_set(&ceu_block_1491->dyns, ceu_block_1491->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 18, col 5)"
                        );
                    
                
                    id_py = ceu_acc;
                    ceu_gc_inc(id_py);
                    ceu_acc = id_py;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0} });
                        ceu_assert_pre(
                            ceu_block_1491,
                            ceu_hold_chk_set(&ceu_block_1491->dyns, ceu_block_1491->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 19, col 5)"
                        );
                    
                
                    id_pz = ceu_acc;
                    ceu_gc_inc(id_pz);
                    ceu_acc = id_pz;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        ceu_assert_pre(
                            ceu_block_1491,
                            ceu_hold_chk_set(&ceu_block_1491->dyns, ceu_block_1491->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 21, col 5)"
                        );
                    
                
                    id_i = ceu_acc;
                    ceu_gc_inc(id_i);
                    ceu_acc = id_i;
                    
                
                    { // BLOCK | 
                        CEU_Block _ceu_block_1373 = (CEU_Block) { (ceu_block_1491->depth + 1), 0, {.block=ceu_block_1491}, NULL };
                        CEU_Block* ceu_block_1373 = &_ceu_block_1373; 
                        
                        
                        
                            CEU_Value id_bi = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_bim = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_1255 = ceu_acc;
                    if (ceu_closure_1255.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1373, "nbody.ceu : (lin 23, col 21)", err);
                    }
                    CEU_Frame ceu_frame_1255 = { &ceu_closure_1255.Dyn->Closure, ceu_block_1373 };
                    
                    CEU_Value ceu_args_1255[2];
                    
                    ceu_acc = id_i;
ceu_args_1255[0] = ceu_acc;
ceu_acc = id_n;
ceu_args_1255[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1255.closure->proto (
                        &ceu_frame_1255,
                        2,
                        ceu_args_1255
                    );
                    ceu_assert_pre(ceu_block_1373, ceu_acc, "nbody.ceu : (lin 23, col 21) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_i;

                        CEU_Value ceu_idx_1267 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bodies;

                    ceu_assert_pre(ceu_block_1373, ceu_col_check(ceu_acc, ceu_idx_1267), "nbody.ceu : (lin 24, col 18)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1267.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1373, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1267.Number), "nbody.ceu : (lin 24, col 18)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1267);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_1373,
                            ceu_hold_chk_set(&ceu_block_1373->dyns, ceu_block_1373->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 24, col 9)"
                        );
                    
                
                    id_bi = ceu_acc;
                    ceu_gc_inc(id_bi);
                    ceu_acc = id_bi;
                    
                
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_mass} });
                        CEU_Value ceu_idx_1278 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1373, ceu_col_check(ceu_acc, ceu_idx_1278), "nbody.ceu : (lin 25, col 19)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1278.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1373, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1278.Number), "nbody.ceu : (lin 25, col 19)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1278);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_1373,
                            ceu_hold_chk_set(&ceu_block_1373->dyns, ceu_block_1373->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 25, col 9)"
                        );
                    
                
                    id_bim = ceu_acc;
                    ceu_gc_inc(id_bim);
                    ceu_acc = id_bim;
                    
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1304 = ceu_acc;
                    if (ceu_closure_1304.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1373, "nbody.ceu : (lin 27, col 21)", err);
                    }
                    CEU_Frame ceu_frame_1304 = { &ceu_closure_1304.Dyn->Closure, ceu_block_1373 };
                    
                    CEU_Value ceu_args_1304[2];
                    
                    ceu_acc = id_px;
ceu_args_1304[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1300 = ceu_acc;
                    if (ceu_closure_1300.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1373, "nbody.ceu : (lin 27, col 32)", err);
                    }
                    CEU_Frame ceu_frame_1300 = { &ceu_closure_1300.Dyn->Closure, ceu_block_1373 };
                    
                    CEU_Value ceu_args_1300[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vx} });
                        CEU_Value ceu_idx_1294 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1373, ceu_col_check(ceu_acc, ceu_idx_1294), "nbody.ceu : (lin 27, col 24)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1294.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1373, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1294.Number), "nbody.ceu : (lin 27, col 24)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1294);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1300[0] = ceu_acc;
ceu_acc = id_bim;
ceu_args_1300[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1300.closure->proto (
                        &ceu_frame_1300,
                        2,
                        ceu_args_1300
                    );
                    ceu_assert_pre(ceu_block_1373, ceu_acc, "nbody.ceu : (lin 27, col 32) : call error");
                } // CALL
                ceu_args_1304[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1304.closure->proto (
                        &ceu_frame_1304,
                        2,
                        ceu_args_1304
                    );
                    ceu_assert_pre(ceu_block_1373, ceu_acc, "nbody.ceu : (lin 27, col 21) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1305 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1373,
                                ceu_hold_chk_set(&ceu_block_1491->dyns, ceu_block_1491->depth, CEU_HOLD_MUTAB, (ceu_set_1305), 0, "set error"),
                                "nbody.ceu : (lin 27, col 13)"
                            );
                            ceu_gc_inc((ceu_set_1305));
                            ceu_gc_dec(id_px, 1);
                            id_px = (ceu_set_1305);
                        }
                        
                    ceu_acc = ceu_set_1305;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1330 = ceu_acc;
                    if (ceu_closure_1330.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1373, "nbody.ceu : (lin 28, col 21)", err);
                    }
                    CEU_Frame ceu_frame_1330 = { &ceu_closure_1330.Dyn->Closure, ceu_block_1373 };
                    
                    CEU_Value ceu_args_1330[2];
                    
                    ceu_acc = id_py;
ceu_args_1330[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1326 = ceu_acc;
                    if (ceu_closure_1326.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1373, "nbody.ceu : (lin 28, col 32)", err);
                    }
                    CEU_Frame ceu_frame_1326 = { &ceu_closure_1326.Dyn->Closure, ceu_block_1373 };
                    
                    CEU_Value ceu_args_1326[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vy} });
                        CEU_Value ceu_idx_1320 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1373, ceu_col_check(ceu_acc, ceu_idx_1320), "nbody.ceu : (lin 28, col 24)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1320.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1373, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1320.Number), "nbody.ceu : (lin 28, col 24)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1320);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1326[0] = ceu_acc;
ceu_acc = id_bim;
ceu_args_1326[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1326.closure->proto (
                        &ceu_frame_1326,
                        2,
                        ceu_args_1326
                    );
                    ceu_assert_pre(ceu_block_1373, ceu_acc, "nbody.ceu : (lin 28, col 32) : call error");
                } // CALL
                ceu_args_1330[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1330.closure->proto (
                        &ceu_frame_1330,
                        2,
                        ceu_args_1330
                    );
                    ceu_assert_pre(ceu_block_1373, ceu_acc, "nbody.ceu : (lin 28, col 21) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1331 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1373,
                                ceu_hold_chk_set(&ceu_block_1491->dyns, ceu_block_1491->depth, CEU_HOLD_MUTAB, (ceu_set_1331), 0, "set error"),
                                "nbody.ceu : (lin 28, col 13)"
                            );
                            ceu_gc_inc((ceu_set_1331));
                            ceu_gc_dec(id_py, 1);
                            id_py = (ceu_set_1331);
                        }
                        
                    ceu_acc = ceu_set_1331;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1356 = ceu_acc;
                    if (ceu_closure_1356.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1373, "nbody.ceu : (lin 29, col 21)", err);
                    }
                    CEU_Frame ceu_frame_1356 = { &ceu_closure_1356.Dyn->Closure, ceu_block_1373 };
                    
                    CEU_Value ceu_args_1356[2];
                    
                    ceu_acc = id_pz;
ceu_args_1356[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1352 = ceu_acc;
                    if (ceu_closure_1352.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1373, "nbody.ceu : (lin 29, col 32)", err);
                    }
                    CEU_Frame ceu_frame_1352 = { &ceu_closure_1352.Dyn->Closure, ceu_block_1373 };
                    
                    CEU_Value ceu_args_1352[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vz} });
                        CEU_Value ceu_idx_1346 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1373, ceu_col_check(ceu_acc, ceu_idx_1346), "nbody.ceu : (lin 29, col 24)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1346.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1373, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1346.Number), "nbody.ceu : (lin 29, col 24)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1346);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1352[0] = ceu_acc;
ceu_acc = id_bim;
ceu_args_1352[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1352.closure->proto (
                        &ceu_frame_1352,
                        2,
                        ceu_args_1352
                    );
                    ceu_assert_pre(ceu_block_1373, ceu_acc, "nbody.ceu : (lin 29, col 32) : call error");
                } // CALL
                ceu_args_1356[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1356.closure->proto (
                        &ceu_frame_1356,
                        2,
                        ceu_args_1356
                    );
                    ceu_assert_pre(ceu_block_1373, ceu_acc, "nbody.ceu : (lin 29, col 21) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1357 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1373,
                                ceu_hold_chk_set(&ceu_block_1491->dyns, ceu_block_1491->depth, CEU_HOLD_MUTAB, (ceu_set_1357), 0, "set error"),
                                "nbody.ceu : (lin 29, col 13)"
                            );
                            ceu_gc_inc((ceu_set_1357));
                            ceu_gc_dec(id_pz, 1);
                            id_pz = (ceu_set_1357);
                        }
                        
                    ceu_acc = ceu_set_1357;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1369 = ceu_acc;
                    if (ceu_closure_1369.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1373, "nbody.ceu : (lin 30, col 19)", err);
                    }
                    CEU_Frame ceu_frame_1369 = { &ceu_closure_1369.Dyn->Closure, ceu_block_1373 };
                    
                    CEU_Value ceu_args_1369[2];
                    
                    ceu_acc = id_i;
ceu_args_1369[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1369[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1369.closure->proto (
                        &ceu_frame_1369,
                        2,
                        ceu_args_1369
                    );
                    ceu_assert_pre(ceu_block_1373, ceu_acc, "nbody.ceu : (lin 30, col 19) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1370 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1373,
                                ceu_hold_chk_set(&ceu_block_1491->dyns, ceu_block_1491->depth, CEU_HOLD_MUTAB, (ceu_set_1370), 0, "set error"),
                                "nbody.ceu : (lin 30, col 13)"
                            );
                            ceu_gc_inc((ceu_set_1370));
                            ceu_gc_dec(id_i, 1);
                            id_i = (ceu_set_1370);
                        }
                        
                    ceu_acc = ceu_set_1370;
                }
                
                    }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1373, 
                                ceu_hold_chk_set(&ceu_block_1491->dyns, ceu_block_1491->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "nbody.ceu : (lin 22, col 5)"
                            );
                            
                        
                            if (id_bi.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_bi, (id_bi.Dyn->Any.hld_depth == ceu_block_1373->depth));
                            }
                        
                            if (id_bim.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_bim, (id_bim.Dyn->Any.hld_depth == ceu_block_1373->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_1373);
                    }
                    
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        CEU_Value ceu_idx_1384 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bodies;

                    ceu_assert_pre(ceu_block_1491, ceu_col_check(ceu_acc, ceu_idx_1384), "nbody.ceu : (lin 33, col 15)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1384.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1491, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1384.Number), "nbody.ceu : (lin 33, col 15)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1384);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_1491,
                            ceu_hold_chk_set(&ceu_block_1491->dyns, ceu_block_1491->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 33, col 5)"
                        );
                    
                
                    id_sun = ceu_acc;
                    ceu_gc_inc(id_sun);
                    ceu_acc = id_sun;
                    
                
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_mass} });
                        CEU_Value ceu_idx_1395 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_sun;

                    ceu_assert_pre(ceu_block_1491, ceu_col_check(ceu_acc, ceu_idx_1395), "nbody.ceu : (lin 34, col 22)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1395.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1491, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1395.Number), "nbody.ceu : (lin 34, col 22)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1395);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_1491,
                            ceu_hold_chk_set(&ceu_block_1491->dyns, ceu_block_1491->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 34, col 5)"
                        );
                    
                
                    id_solar_mass = ceu_acc;
                    ceu_gc_inc(id_solar_mass);
                    ceu_acc = id_solar_mass;
                    
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1426 = ceu_acc;
                    if (ceu_closure_1426.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1491, "nbody.ceu : (lin 35, col 29)", err);
                    }
                    CEU_Frame ceu_frame_1426 = { &ceu_closure_1426.Dyn->Closure, ceu_block_1491 };
                    
                    CEU_Value ceu_args_1426[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vx} });
                        CEU_Value ceu_idx_1412 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_sun;

                    ceu_assert_pre(ceu_block_1491, ceu_col_check(ceu_acc, ceu_idx_1412), "nbody.ceu : (lin 35, col 20)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1412.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1491, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1412.Number), "nbody.ceu : (lin 35, col 20)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1412);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1426[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash;

                    CEU_Value ceu_closure_1422 = ceu_acc;
                    if (ceu_closure_1422.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1491, "nbody.ceu : (lin 35, col 35)", err);
                    }
                    CEU_Frame ceu_frame_1422 = { &ceu_closure_1422.Dyn->Closure, ceu_block_1491 };
                    
                    CEU_Value ceu_args_1422[2];
                    
                    ceu_acc = id_px;
ceu_args_1422[0] = ceu_acc;
ceu_acc = id_solar_mass;
ceu_args_1422[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1422.closure->proto (
                        &ceu_frame_1422,
                        2,
                        ceu_args_1422
                    );
                    ceu_assert_pre(ceu_block_1491, ceu_acc, "nbody.ceu : (lin 35, col 35) : call error");
                } // CALL
                ceu_args_1426[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1426.closure->proto (
                        &ceu_frame_1426,
                        2,
                        ceu_args_1426
                    );
                    ceu_assert_pre(ceu_block_1491, ceu_acc, "nbody.ceu : (lin 35, col 29) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1427 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vx} });
                        CEU_Value ceu_idx_1404 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_sun;

                    ceu_assert_pre(ceu_block_1491, ceu_col_check(ceu_acc, ceu_idx_1404), "nbody.ceu : (lin 35, col 9)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1404.Number, (ceu_set_1427));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1404.Number, (ceu_set_1427));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1404, (ceu_set_1427));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1491, ok, "nbody.ceu : (lin 35, col 9)");
                        
                }
                
                    ceu_acc = ceu_set_1427;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1457 = ceu_acc;
                    if (ceu_closure_1457.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1491, "nbody.ceu : (lin 36, col 29)", err);
                    }
                    CEU_Frame ceu_frame_1457 = { &ceu_closure_1457.Dyn->Closure, ceu_block_1491 };
                    
                    CEU_Value ceu_args_1457[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vy} });
                        CEU_Value ceu_idx_1443 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_sun;

                    ceu_assert_pre(ceu_block_1491, ceu_col_check(ceu_acc, ceu_idx_1443), "nbody.ceu : (lin 36, col 20)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1443.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1491, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1443.Number), "nbody.ceu : (lin 36, col 20)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1443);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1457[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash;

                    CEU_Value ceu_closure_1453 = ceu_acc;
                    if (ceu_closure_1453.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1491, "nbody.ceu : (lin 36, col 35)", err);
                    }
                    CEU_Frame ceu_frame_1453 = { &ceu_closure_1453.Dyn->Closure, ceu_block_1491 };
                    
                    CEU_Value ceu_args_1453[2];
                    
                    ceu_acc = id_py;
ceu_args_1453[0] = ceu_acc;
ceu_acc = id_solar_mass;
ceu_args_1453[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1453.closure->proto (
                        &ceu_frame_1453,
                        2,
                        ceu_args_1453
                    );
                    ceu_assert_pre(ceu_block_1491, ceu_acc, "nbody.ceu : (lin 36, col 35) : call error");
                } // CALL
                ceu_args_1457[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1457.closure->proto (
                        &ceu_frame_1457,
                        2,
                        ceu_args_1457
                    );
                    ceu_assert_pre(ceu_block_1491, ceu_acc, "nbody.ceu : (lin 36, col 29) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1458 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vy} });
                        CEU_Value ceu_idx_1435 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_sun;

                    ceu_assert_pre(ceu_block_1491, ceu_col_check(ceu_acc, ceu_idx_1435), "nbody.ceu : (lin 36, col 9)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1435.Number, (ceu_set_1458));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1435.Number, (ceu_set_1458));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1435, (ceu_set_1458));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1491, ok, "nbody.ceu : (lin 36, col 9)");
                        
                }
                
                    ceu_acc = ceu_set_1458;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1488 = ceu_acc;
                    if (ceu_closure_1488.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1491, "nbody.ceu : (lin 37, col 29)", err);
                    }
                    CEU_Frame ceu_frame_1488 = { &ceu_closure_1488.Dyn->Closure, ceu_block_1491 };
                    
                    CEU_Value ceu_args_1488[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vz} });
                        CEU_Value ceu_idx_1474 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_sun;

                    ceu_assert_pre(ceu_block_1491, ceu_col_check(ceu_acc, ceu_idx_1474), "nbody.ceu : (lin 37, col 20)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1474.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1491, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1474.Number), "nbody.ceu : (lin 37, col 20)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1474);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1488[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash;

                    CEU_Value ceu_closure_1484 = ceu_acc;
                    if (ceu_closure_1484.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1491, "nbody.ceu : (lin 37, col 35)", err);
                    }
                    CEU_Frame ceu_frame_1484 = { &ceu_closure_1484.Dyn->Closure, ceu_block_1491 };
                    
                    CEU_Value ceu_args_1484[2];
                    
                    ceu_acc = id_pz;
ceu_args_1484[0] = ceu_acc;
ceu_acc = id_solar_mass;
ceu_args_1484[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1484.closure->proto (
                        &ceu_frame_1484,
                        2,
                        ceu_args_1484
                    );
                    ceu_assert_pre(ceu_block_1491, ceu_acc, "nbody.ceu : (lin 37, col 35) : call error");
                } // CALL
                ceu_args_1488[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1488.closure->proto (
                        &ceu_frame_1488,
                        2,
                        ceu_args_1488
                    );
                    ceu_assert_pre(ceu_block_1491, ceu_acc, "nbody.ceu : (lin 37, col 29) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1489 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vz} });
                        CEU_Value ceu_idx_1466 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_sun;

                    ceu_assert_pre(ceu_block_1491, ceu_col_check(ceu_acc, ceu_idx_1466), "nbody.ceu : (lin 37, col 9)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1466.Number, (ceu_set_1489));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1466.Number, (ceu_set_1489));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1466, (ceu_set_1489));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1491, ok, "nbody.ceu : (lin 37, col 9)");
                        
                }
                
                    ceu_acc = ceu_set_1489;
                }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1491, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "nbody.ceu : (lin 15, col 37)"
                            );
                            
                        
                            if (id_n.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_n, (id_n.Dyn->Any.hld_depth == ceu_block_1491->depth));
                            }
                        
                            if (id_px.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_px, (id_px.Dyn->Any.hld_depth == ceu_block_1491->depth));
                            }
                        
                            if (id_py.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_py, (id_py.Dyn->Any.hld_depth == ceu_block_1491->depth));
                            }
                        
                            if (id_pz.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_pz, (id_pz.Dyn->Any.hld_depth == ceu_block_1491->depth));
                            }
                        
                            if (id_i.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_i, (id_i.Dyn->Any.hld_depth == ceu_block_1491->depth));
                            }
                        
                            if (id_sun.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_sun, (id_sun.Dyn->Any.hld_depth == ceu_block_1491->depth));
                            }
                        
                            if (id_solar_mass.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_solar_mass, (id_solar_mass.Dyn->Any.hld_depth == ceu_block_1491->depth));
                            }
                        
                        
                            
                                if (id_bodies.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_bodies, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_bodies.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_1491);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_1492 = ceu_closure_create (
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_1492,
                    0
                );
                ceu_acc = ceu_ret_1492;
                
                // UPVALS
                
                
                    CEU_Value ceu_set_1493 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_3004,
                                ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, (ceu_set_1493), 0, "set error"),
                                "nbody.ceu : (lin 15, col 5)"
                            );
                            ceu_gc_inc((ceu_set_1493));
                            ceu_gc_dec(id_offset_momentum, 1);
                            id_offset_momentum = (ceu_set_1493);
                        }
                        
                    ceu_acc = ceu_set_1493;
                }
                
                // DCL | 
                
                
                    
                    ceu_gc_inc(id_advance);
                    ceu_acc = id_advance;
                    
                
                { // SET | 
                     // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_2153 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_bodies;
                            CEU_Block* _id_bodies_;
                            
                            CEU_Value id_dt;
                            CEU_Block* _id_dt_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_2152 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_2152 = &_ceu_block_2152; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_2152,
                                            ceu_hold_chk_set(&ceu_block_2152->dyns, ceu_block_2152->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "nbody.ceu : (lin 41, col 33)"
                                        );
                                        id_bodies = ceu_args[0];
                                    } else {
                                        id_bodies = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_2152,
                                            ceu_hold_chk_set(&ceu_block_2152->dyns, ceu_block_2152->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "nbody.ceu : (lin 41, col 33)"
                                        );
                                        id_dt = ceu_args[1];
                                    } else {
                                        id_dt = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                            CEU_Value id_n = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_i = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_k = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_hash;

                    CEU_Value ceu_closure_1516 = ceu_acc;
                    if (ceu_closure_1516.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2152, "nbody.ceu : (lin 42, col 13)", err);
                    }
                    CEU_Frame ceu_frame_1516 = { &ceu_closure_1516.Dyn->Closure, ceu_block_2152 };
                    
                    CEU_Value ceu_args_1516[1];
                    
                    ceu_acc = id_bodies;
ceu_args_1516[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1516.closure->proto (
                        &ceu_frame_1516,
                        1,
                        ceu_args_1516
                    );
                    ceu_assert_pre(ceu_block_2152, ceu_acc, "nbody.ceu : (lin 42, col 13) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_2152,
                            ceu_hold_chk_set(&ceu_block_2152->dyns, ceu_block_2152->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 42, col 5)"
                        );
                    
                
                    id_n = ceu_acc;
                    ceu_gc_inc(id_n);
                    ceu_acc = id_n;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        ceu_assert_pre(
                            ceu_block_2152,
                            ceu_hold_chk_set(&ceu_block_2152->dyns, ceu_block_2152->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 43, col 5)"
                        );
                    
                
                    id_i = ceu_acc;
                    ceu_gc_inc(id_i);
                    ceu_acc = id_i;
                    
                
                    { // BLOCK | 
                        CEU_Block _ceu_block_1992 = (CEU_Block) { (ceu_block_2152->depth + 1), 0, {.block=ceu_block_2152}, NULL };
                        CEU_Block* ceu_block_1992 = &_ceu_block_1992; 
                        
                        
                        
                            CEU_Value id_bi = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_j = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_1536 = ceu_acc;
                    if (ceu_closure_1536.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1992, "nbody.ceu : (lin 45, col 21)", err);
                    }
                    CEU_Frame ceu_frame_1536 = { &ceu_closure_1536.Dyn->Closure, ceu_block_1992 };
                    
                    CEU_Value ceu_args_1536[2];
                    
                    ceu_acc = id_i;
ceu_args_1536[0] = ceu_acc;
ceu_acc = id_n;
ceu_args_1536[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1536.closure->proto (
                        &ceu_frame_1536,
                        2,
                        ceu_args_1536
                    );
                    ceu_assert_pre(ceu_block_1992, ceu_acc, "nbody.ceu : (lin 45, col 21) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_i;

                        CEU_Value ceu_idx_1548 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bodies;

                    ceu_assert_pre(ceu_block_1992, ceu_col_check(ceu_acc, ceu_idx_1548), "nbody.ceu : (lin 46, col 18)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1548.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1992, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1548.Number), "nbody.ceu : (lin 46, col 18)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1548);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_1992,
                            ceu_hold_chk_set(&ceu_block_1992->dyns, ceu_block_1992->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 46, col 9)"
                        );
                    
                
                    id_bi = ceu_acc;
                    ceu_gc_inc(id_bi);
                    ceu_acc = id_bi;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1560 = ceu_acc;
                    if (ceu_closure_1560.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1992, "nbody.ceu : (lin 48, col 19)", err);
                    }
                    CEU_Frame ceu_frame_1560 = { &ceu_closure_1560.Dyn->Closure, ceu_block_1992 };
                    
                    CEU_Value ceu_args_1560[2];
                    
                    ceu_acc = id_i;
ceu_args_1560[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1560[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1560.closure->proto (
                        &ceu_frame_1560,
                        2,
                        ceu_args_1560
                    );
                    ceu_assert_pre(ceu_block_1992, ceu_acc, "nbody.ceu : (lin 48, col 19) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_1992,
                            ceu_hold_chk_set(&ceu_block_1992->dyns, ceu_block_1992->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 48, col 9)"
                        );
                    
                
                    id_j = ceu_acc;
                    ceu_gc_inc(id_j);
                    ceu_acc = id_j;
                    
                
                    { // BLOCK | 
                        CEU_Block _ceu_block_1975 = (CEU_Block) { (ceu_block_1992->depth + 1), 0, {.block=ceu_block_1992}, NULL };
                        CEU_Block* ceu_block_1975 = &_ceu_block_1975; 
                        
                        
                        
                            CEU_Value id_bj = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_dx = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_dy = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_dz = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_dist = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_mag = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_bjm = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_bim = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_1574 = ceu_acc;
                    if (ceu_closure_1574.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 50, col 25)", err);
                    }
                    CEU_Frame ceu_frame_1574 = { &ceu_closure_1574.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1574[2];
                    
                    ceu_acc = id_j;
ceu_args_1574[0] = ceu_acc;
ceu_acc = id_n;
ceu_args_1574[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1574.closure->proto (
                        &ceu_frame_1574,
                        2,
                        ceu_args_1574
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 50, col 25) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_j;

                        CEU_Value ceu_idx_1586 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bodies;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1586), "nbody.ceu : (lin 51, col 22)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1586.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1586.Number), "nbody.ceu : (lin 51, col 22)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1586);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_1975,
                            ceu_hold_chk_set(&ceu_block_1975->dyns, ceu_block_1975->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 51, col 13)"
                        );
                    
                
                    id_bj = ceu_acc;
                    ceu_gc_inc(id_bj);
                    ceu_acc = id_bj;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1608 = ceu_acc;
                    if (ceu_closure_1608.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 52, col 29)", err);
                    }
                    CEU_Frame ceu_frame_1608 = { &ceu_closure_1608.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1608[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_x} });
                        CEU_Value ceu_idx_1597 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1597), "nbody.ceu : (lin 52, col 22)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1597.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1597.Number), "nbody.ceu : (lin 52, col 22)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1597);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1608[0] = ceu_acc;

                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_x} });
                        CEU_Value ceu_idx_1605 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1605), "nbody.ceu : (lin 52, col 31)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1605.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1605.Number), "nbody.ceu : (lin 52, col 31)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1605);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1608[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1608.closure->proto (
                        &ceu_frame_1608,
                        2,
                        ceu_args_1608
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 52, col 29) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_1975,
                            ceu_hold_chk_set(&ceu_block_1975->dyns, ceu_block_1975->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 52, col 13)"
                        );
                    
                
                    id_dx = ceu_acc;
                    ceu_gc_inc(id_dx);
                    ceu_acc = id_dx;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1630 = ceu_acc;
                    if (ceu_closure_1630.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 53, col 29)", err);
                    }
                    CEU_Frame ceu_frame_1630 = { &ceu_closure_1630.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1630[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_y} });
                        CEU_Value ceu_idx_1619 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1619), "nbody.ceu : (lin 53, col 22)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1619.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1619.Number), "nbody.ceu : (lin 53, col 22)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1619);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1630[0] = ceu_acc;

                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_y} });
                        CEU_Value ceu_idx_1627 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1627), "nbody.ceu : (lin 53, col 31)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1627.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1627.Number), "nbody.ceu : (lin 53, col 31)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1627);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1630[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1630.closure->proto (
                        &ceu_frame_1630,
                        2,
                        ceu_args_1630
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 53, col 29) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_1975,
                            ceu_hold_chk_set(&ceu_block_1975->dyns, ceu_block_1975->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 53, col 13)"
                        );
                    
                
                    id_dy = ceu_acc;
                    ceu_gc_inc(id_dy);
                    ceu_acc = id_dy;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1652 = ceu_acc;
                    if (ceu_closure_1652.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 54, col 29)", err);
                    }
                    CEU_Frame ceu_frame_1652 = { &ceu_closure_1652.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1652[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_z} });
                        CEU_Value ceu_idx_1641 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1641), "nbody.ceu : (lin 54, col 22)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1641.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1641.Number), "nbody.ceu : (lin 54, col 22)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1641);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1652[0] = ceu_acc;

                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_z} });
                        CEU_Value ceu_idx_1649 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1649), "nbody.ceu : (lin 54, col 31)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1649.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1649.Number), "nbody.ceu : (lin 54, col 31)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1649);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1652[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1652.closure->proto (
                        &ceu_frame_1652,
                        2,
                        ceu_args_1652
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 54, col 29) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_1975,
                            ceu_hold_chk_set(&ceu_block_1975->dyns, ceu_block_1975->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 54, col 13)"
                        );
                    
                
                    id_dz = ceu_acc;
                    ceu_gc_inc(id_dz);
                    ceu_acc = id_dz;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1712 = ceu_acc;
                    if (ceu_closure_1712.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 56, col 55)", err);
                    }
                    CEU_Frame ceu_frame_1712 = { &ceu_closure_1712.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1712[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1697 = ceu_acc;
                    if (ceu_closure_1697.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 56, col 33)", err);
                    }
                    CEU_Frame ceu_frame_1697 = { &ceu_closure_1697.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1697[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1666 = ceu_acc;
                    if (ceu_closure_1666.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 56, col 28)", err);
                    }
                    CEU_Frame ceu_frame_1666 = { &ceu_closure_1666.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1666[2];
                    
                    ceu_acc = id_dx;
ceu_args_1666[0] = ceu_acc;
ceu_acc = id_dx;
ceu_args_1666[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1666.closure->proto (
                        &ceu_frame_1666,
                        2,
                        ceu_args_1666
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 56, col 28) : call error");
                } // CALL
                ceu_args_1697[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1693 = ceu_acc;
                    if (ceu_closure_1693.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 56, col 44)", err);
                    }
                    CEU_Frame ceu_frame_1693 = { &ceu_closure_1693.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1693[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1678 = ceu_acc;
                    if (ceu_closure_1678.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 56, col 39)", err);
                    }
                    CEU_Frame ceu_frame_1678 = { &ceu_closure_1678.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1678[2];
                    
                    ceu_acc = id_dy;
ceu_args_1678[0] = ceu_acc;
ceu_acc = id_dy;
ceu_args_1678[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1678.closure->proto (
                        &ceu_frame_1678,
                        2,
                        ceu_args_1678
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 56, col 39) : call error");
                } // CALL
                ceu_args_1693[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1689 = ceu_acc;
                    if (ceu_closure_1689.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 56, col 49)", err);
                    }
                    CEU_Frame ceu_frame_1689 = { &ceu_closure_1689.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1689[2];
                    
                    ceu_acc = id_dz;
ceu_args_1689[0] = ceu_acc;
ceu_acc = id_dz;
ceu_args_1689[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1689.closure->proto (
                        &ceu_frame_1689,
                        2,
                        ceu_args_1689
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 56, col 49) : call error");
                } // CALL
                ceu_args_1693[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1693.closure->proto (
                        &ceu_frame_1693,
                        2,
                        ceu_args_1693
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 56, col 44) : call error");
                } // CALL
                ceu_args_1697[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1697.closure->proto (
                        &ceu_frame_1697,
                        2,
                        ceu_args_1697
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 56, col 33) : call error");
                } // CALL
                ceu_args_1712[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash;

                    CEU_Value ceu_closure_1708 = ceu_acc;
                    if (ceu_closure_1708.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 56, col 58)", err);
                    }
                    CEU_Frame ceu_frame_1708 = { &ceu_closure_1708.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1708[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1708[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=2} });ceu_args_1708[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1708.closure->proto (
                        &ceu_frame_1708,
                        2,
                        ceu_args_1708
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 56, col 58) : call error");
                } // CALL
                ceu_args_1712[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1712.closure->proto (
                        &ceu_frame_1712,
                        2,
                        ceu_args_1712
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 56, col 55) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_1975,
                            ceu_hold_chk_set(&ceu_block_1975->dyns, ceu_block_1975->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 56, col 13)"
                        );
                    
                
                    id_dist = ceu_acc;
                    ceu_gc_inc(id_dist);
                    ceu_acc = id_dist;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_slash;

                    CEU_Value ceu_closure_1738 = ceu_acc;
                    if (ceu_closure_1738.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 57, col 26)", err);
                    }
                    CEU_Frame ceu_frame_1738 = { &ceu_closure_1738.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1738[2];
                    
                    ceu_acc = id_dt;
ceu_args_1738[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1734 = ceu_acc;
                    if (ceu_closure_1734.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 57, col 41)", err);
                    }
                    CEU_Frame ceu_frame_1734 = { &ceu_closure_1734.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1734[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1728 = ceu_acc;
                    if (ceu_closure_1728.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 57, col 34)", err);
                    }
                    CEU_Frame ceu_frame_1728 = { &ceu_closure_1728.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1728[2];
                    
                    ceu_acc = id_dist;
ceu_args_1728[0] = ceu_acc;
ceu_acc = id_dist;
ceu_args_1728[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1728.closure->proto (
                        &ceu_frame_1728,
                        2,
                        ceu_args_1728
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 57, col 34) : call error");
                } // CALL
                ceu_args_1734[0] = ceu_acc;
ceu_acc = id_dist;
ceu_args_1734[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1734.closure->proto (
                        &ceu_frame_1734,
                        2,
                        ceu_args_1734
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 57, col 41) : call error");
                } // CALL
                ceu_args_1738[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1738.closure->proto (
                        &ceu_frame_1738,
                        2,
                        ceu_args_1738
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 57, col 26) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_1975,
                            ceu_hold_chk_set(&ceu_block_1975->dyns, ceu_block_1975->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 57, col 13)"
                        );
                    
                
                    id_mag = ceu_acc;
                    ceu_gc_inc(id_mag);
                    ceu_acc = id_mag;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1755 = ceu_acc;
                    if (ceu_closure_1755.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 59, col 33)", err);
                    }
                    CEU_Frame ceu_frame_1755 = { &ceu_closure_1755.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1755[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_mass} });
                        CEU_Value ceu_idx_1749 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1749), "nbody.ceu : (lin 59, col 23)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1749.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1749.Number), "nbody.ceu : (lin 59, col 23)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1749);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1755[0] = ceu_acc;
ceu_acc = id_mag;
ceu_args_1755[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1755.closure->proto (
                        &ceu_frame_1755,
                        2,
                        ceu_args_1755
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 59, col 33) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_1975,
                            ceu_hold_chk_set(&ceu_block_1975->dyns, ceu_block_1975->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 59, col 13)"
                        );
                    
                
                    id_bjm = ceu_acc;
                    ceu_gc_inc(id_bjm);
                    ceu_acc = id_bjm;
                    
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1786 = ceu_acc;
                    if (ceu_closure_1786.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 60, col 35)", err);
                    }
                    CEU_Frame ceu_frame_1786 = { &ceu_closure_1786.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1786[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vx} });
                        CEU_Value ceu_idx_1772 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1772), "nbody.ceu : (lin 60, col 27)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1772.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1772.Number), "nbody.ceu : (lin 60, col 27)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1772);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1786[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1782 = ceu_acc;
                    if (ceu_closure_1782.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 60, col 41)", err);
                    }
                    CEU_Frame ceu_frame_1782 = { &ceu_closure_1782.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1782[2];
                    
                    ceu_acc = id_dx;
ceu_args_1782[0] = ceu_acc;
ceu_acc = id_bjm;
ceu_args_1782[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1782.closure->proto (
                        &ceu_frame_1782,
                        2,
                        ceu_args_1782
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 60, col 41) : call error");
                } // CALL
                ceu_args_1786[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1786.closure->proto (
                        &ceu_frame_1786,
                        2,
                        ceu_args_1786
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 60, col 35) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1787 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vx} });
                        CEU_Value ceu_idx_1764 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1764), "nbody.ceu : (lin 60, col 17)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1764.Number, (ceu_set_1787));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1764.Number, (ceu_set_1787));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1764, (ceu_set_1787));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1975, ok, "nbody.ceu : (lin 60, col 17)");
                        
                }
                
                    ceu_acc = ceu_set_1787;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1817 = ceu_acc;
                    if (ceu_closure_1817.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 61, col 35)", err);
                    }
                    CEU_Frame ceu_frame_1817 = { &ceu_closure_1817.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1817[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vy} });
                        CEU_Value ceu_idx_1803 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1803), "nbody.ceu : (lin 61, col 27)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1803.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1803.Number), "nbody.ceu : (lin 61, col 27)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1803);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1817[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1813 = ceu_acc;
                    if (ceu_closure_1813.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 61, col 41)", err);
                    }
                    CEU_Frame ceu_frame_1813 = { &ceu_closure_1813.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1813[2];
                    
                    ceu_acc = id_dy;
ceu_args_1813[0] = ceu_acc;
ceu_acc = id_bjm;
ceu_args_1813[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1813.closure->proto (
                        &ceu_frame_1813,
                        2,
                        ceu_args_1813
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 61, col 41) : call error");
                } // CALL
                ceu_args_1817[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1817.closure->proto (
                        &ceu_frame_1817,
                        2,
                        ceu_args_1817
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 61, col 35) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1818 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vy} });
                        CEU_Value ceu_idx_1795 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1795), "nbody.ceu : (lin 61, col 17)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1795.Number, (ceu_set_1818));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1795.Number, (ceu_set_1818));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1795, (ceu_set_1818));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1975, ok, "nbody.ceu : (lin 61, col 17)");
                        
                }
                
                    ceu_acc = ceu_set_1818;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_1848 = ceu_acc;
                    if (ceu_closure_1848.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 62, col 35)", err);
                    }
                    CEU_Frame ceu_frame_1848 = { &ceu_closure_1848.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1848[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vz} });
                        CEU_Value ceu_idx_1834 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1834), "nbody.ceu : (lin 62, col 27)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1834.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1834.Number), "nbody.ceu : (lin 62, col 27)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1834);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1848[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1844 = ceu_acc;
                    if (ceu_closure_1844.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 62, col 41)", err);
                    }
                    CEU_Frame ceu_frame_1844 = { &ceu_closure_1844.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1844[2];
                    
                    ceu_acc = id_dz;
ceu_args_1844[0] = ceu_acc;
ceu_acc = id_bjm;
ceu_args_1844[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1844.closure->proto (
                        &ceu_frame_1844,
                        2,
                        ceu_args_1844
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 62, col 41) : call error");
                } // CALL
                ceu_args_1848[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1848.closure->proto (
                        &ceu_frame_1848,
                        2,
                        ceu_args_1848
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 62, col 35) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1849 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vz} });
                        CEU_Value ceu_idx_1826 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1826), "nbody.ceu : (lin 62, col 17)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1826.Number, (ceu_set_1849));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1826.Number, (ceu_set_1849));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1826, (ceu_set_1849));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1975, ok, "nbody.ceu : (lin 62, col 17)");
                        
                }
                
                    ceu_acc = ceu_set_1849;
                }
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1865 = ceu_acc;
                    if (ceu_closure_1865.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 64, col 33)", err);
                    }
                    CEU_Frame ceu_frame_1865 = { &ceu_closure_1865.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1865[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_mass} });
                        CEU_Value ceu_idx_1859 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1859), "nbody.ceu : (lin 64, col 23)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1859.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1859.Number), "nbody.ceu : (lin 64, col 23)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1859);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1865[0] = ceu_acc;
ceu_acc = id_mag;
ceu_args_1865[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1865.closure->proto (
                        &ceu_frame_1865,
                        2,
                        ceu_args_1865
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 64, col 33) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_1975,
                            ceu_hold_chk_set(&ceu_block_1975->dyns, ceu_block_1975->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 64, col 13)"
                        );
                    
                
                    id_bim = ceu_acc;
                    ceu_gc_inc(id_bim);
                    ceu_acc = id_bim;
                    
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1896 = ceu_acc;
                    if (ceu_closure_1896.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 65, col 35)", err);
                    }
                    CEU_Frame ceu_frame_1896 = { &ceu_closure_1896.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1896[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vx} });
                        CEU_Value ceu_idx_1882 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1882), "nbody.ceu : (lin 65, col 27)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1882.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1882.Number), "nbody.ceu : (lin 65, col 27)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1882);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1896[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1892 = ceu_acc;
                    if (ceu_closure_1892.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 65, col 41)", err);
                    }
                    CEU_Frame ceu_frame_1892 = { &ceu_closure_1892.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1892[2];
                    
                    ceu_acc = id_dx;
ceu_args_1892[0] = ceu_acc;
ceu_acc = id_bim;
ceu_args_1892[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1892.closure->proto (
                        &ceu_frame_1892,
                        2,
                        ceu_args_1892
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 65, col 41) : call error");
                } // CALL
                ceu_args_1896[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1896.closure->proto (
                        &ceu_frame_1896,
                        2,
                        ceu_args_1896
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 65, col 35) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1897 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vx} });
                        CEU_Value ceu_idx_1874 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1874), "nbody.ceu : (lin 65, col 17)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1874.Number, (ceu_set_1897));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1874.Number, (ceu_set_1897));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1874, (ceu_set_1897));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1975, ok, "nbody.ceu : (lin 65, col 17)");
                        
                }
                
                    ceu_acc = ceu_set_1897;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1927 = ceu_acc;
                    if (ceu_closure_1927.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 66, col 35)", err);
                    }
                    CEU_Frame ceu_frame_1927 = { &ceu_closure_1927.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1927[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vy} });
                        CEU_Value ceu_idx_1913 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1913), "nbody.ceu : (lin 66, col 27)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1913.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1913.Number), "nbody.ceu : (lin 66, col 27)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1913);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1927[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1923 = ceu_acc;
                    if (ceu_closure_1923.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 66, col 41)", err);
                    }
                    CEU_Frame ceu_frame_1923 = { &ceu_closure_1923.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1923[2];
                    
                    ceu_acc = id_dy;
ceu_args_1923[0] = ceu_acc;
ceu_acc = id_bim;
ceu_args_1923[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1923.closure->proto (
                        &ceu_frame_1923,
                        2,
                        ceu_args_1923
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 66, col 41) : call error");
                } // CALL
                ceu_args_1927[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1927.closure->proto (
                        &ceu_frame_1927,
                        2,
                        ceu_args_1927
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 66, col 35) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1928 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vy} });
                        CEU_Value ceu_idx_1905 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1905), "nbody.ceu : (lin 66, col 17)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1905.Number, (ceu_set_1928));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1905.Number, (ceu_set_1928));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1905, (ceu_set_1928));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1975, ok, "nbody.ceu : (lin 66, col 17)");
                        
                }
                
                    ceu_acc = ceu_set_1928;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1958 = ceu_acc;
                    if (ceu_closure_1958.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 67, col 35)", err);
                    }
                    CEU_Frame ceu_frame_1958 = { &ceu_closure_1958.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1958[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vz} });
                        CEU_Value ceu_idx_1944 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1944), "nbody.ceu : (lin 67, col 27)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_1944.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_1975, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_1944.Number), "nbody.ceu : (lin 67, col 27)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_1944);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_1958[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_1954 = ceu_acc;
                    if (ceu_closure_1954.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 67, col 41)", err);
                    }
                    CEU_Frame ceu_frame_1954 = { &ceu_closure_1954.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1954[2];
                    
                    ceu_acc = id_dz;
ceu_args_1954[0] = ceu_acc;
ceu_acc = id_bim;
ceu_args_1954[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1954.closure->proto (
                        &ceu_frame_1954,
                        2,
                        ceu_args_1954
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 67, col 41) : call error");
                } // CALL
                ceu_args_1958[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1958.closure->proto (
                        &ceu_frame_1958,
                        2,
                        ceu_args_1958
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 67, col 35) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1959 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vz} });
                        CEU_Value ceu_idx_1936 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_1975, ceu_col_check(ceu_acc, ceu_idx_1936), "nbody.ceu : (lin 67, col 17)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_1936.Number, (ceu_set_1959));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_1936.Number, (ceu_set_1959));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_1936, (ceu_set_1959));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_1975, ok, "nbody.ceu : (lin 67, col 17)");
                        
                }
                
                    ceu_acc = ceu_set_1959;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1971 = ceu_acc;
                    if (ceu_closure_1971.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1975, "nbody.ceu : (lin 69, col 23)", err);
                    }
                    CEU_Frame ceu_frame_1971 = { &ceu_closure_1971.Dyn->Closure, ceu_block_1975 };
                    
                    CEU_Value ceu_args_1971[2];
                    
                    ceu_acc = id_j;
ceu_args_1971[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1971[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1971.closure->proto (
                        &ceu_frame_1971,
                        2,
                        ceu_args_1971
                    );
                    ceu_assert_pre(ceu_block_1975, ceu_acc, "nbody.ceu : (lin 69, col 23) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1972 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1975,
                                ceu_hold_chk_set(&ceu_block_1992->dyns, ceu_block_1992->depth, CEU_HOLD_MUTAB, (ceu_set_1972), 0, "set error"),
                                "nbody.ceu : (lin 69, col 17)"
                            );
                            ceu_gc_inc((ceu_set_1972));
                            ceu_gc_dec(id_j, 1);
                            id_j = (ceu_set_1972);
                        }
                        
                    ceu_acc = ceu_set_1972;
                }
                
                    }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1975, 
                                ceu_hold_chk_set(&ceu_block_1992->dyns, ceu_block_1992->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "nbody.ceu : (lin 49, col 9)"
                            );
                            
                        
                            if (id_bj.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_bj, (id_bj.Dyn->Any.hld_depth == ceu_block_1975->depth));
                            }
                        
                            if (id_dx.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_dx, (id_dx.Dyn->Any.hld_depth == ceu_block_1975->depth));
                            }
                        
                            if (id_dy.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_dy, (id_dy.Dyn->Any.hld_depth == ceu_block_1975->depth));
                            }
                        
                            if (id_dz.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_dz, (id_dz.Dyn->Any.hld_depth == ceu_block_1975->depth));
                            }
                        
                            if (id_dist.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_dist, (id_dist.Dyn->Any.hld_depth == ceu_block_1975->depth));
                            }
                        
                            if (id_mag.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_mag, (id_mag.Dyn->Any.hld_depth == ceu_block_1975->depth));
                            }
                        
                            if (id_bjm.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_bjm, (id_bjm.Dyn->Any.hld_depth == ceu_block_1975->depth));
                            }
                        
                            if (id_bim.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_bim, (id_bim.Dyn->Any.hld_depth == ceu_block_1975->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_1975);
                    }
                    
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_1988 = ceu_acc;
                    if (ceu_closure_1988.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_1992, "nbody.ceu : (lin 71, col 19)", err);
                    }
                    CEU_Frame ceu_frame_1988 = { &ceu_closure_1988.Dyn->Closure, ceu_block_1992 };
                    
                    CEU_Value ceu_args_1988[2];
                    
                    ceu_acc = id_i;
ceu_args_1988[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_1988[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_1988.closure->proto (
                        &ceu_frame_1988,
                        2,
                        ceu_args_1988
                    );
                    ceu_assert_pre(ceu_block_1992, ceu_acc, "nbody.ceu : (lin 71, col 19) : call error");
                } // CALL
                
                    CEU_Value ceu_set_1989 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_1992,
                                ceu_hold_chk_set(&ceu_block_2152->dyns, ceu_block_2152->depth, CEU_HOLD_MUTAB, (ceu_set_1989), 0, "set error"),
                                "nbody.ceu : (lin 71, col 13)"
                            );
                            ceu_gc_inc((ceu_set_1989));
                            ceu_gc_dec(id_i, 1);
                            id_i = (ceu_set_1989);
                        }
                        
                    ceu_acc = ceu_set_1989;
                }
                
                    }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_1992, 
                                ceu_hold_chk_set(&ceu_block_2152->dyns, ceu_block_2152->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "nbody.ceu : (lin 44, col 5)"
                            );
                            
                        
                            if (id_bi.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_bi, (id_bi.Dyn->Any.hld_depth == ceu_block_1992->depth));
                            }
                        
                            if (id_j.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_j, (id_j.Dyn->Any.hld_depth == ceu_block_1992->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_1992);
                    }
                    
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        ceu_assert_pre(
                            ceu_block_2152,
                            ceu_hold_chk_set(&ceu_block_2152->dyns, ceu_block_2152->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 74, col 5)"
                        );
                    
                
                    id_k = ceu_acc;
                    ceu_gc_inc(id_k);
                    ceu_acc = id_k;
                    
                
                    { // BLOCK | 
                        CEU_Block _ceu_block_2149 = (CEU_Block) { (ceu_block_2152->depth + 1), 0, {.block=ceu_block_2152}, NULL };
                        CEU_Block* ceu_block_2149 = &_ceu_block_2149; 
                        
                        
                        
                            CEU_Value id_bi = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_2012 = ceu_acc;
                    if (ceu_closure_2012.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2149, "nbody.ceu : (lin 76, col 21)", err);
                    }
                    CEU_Frame ceu_frame_2012 = { &ceu_closure_2012.Dyn->Closure, ceu_block_2149 };
                    
                    CEU_Value ceu_args_2012[2];
                    
                    ceu_acc = id_k;
ceu_args_2012[0] = ceu_acc;
ceu_acc = id_n;
ceu_args_2012[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2012.closure->proto (
                        &ceu_frame_2012,
                        2,
                        ceu_args_2012
                    );
                    ceu_assert_pre(ceu_block_2149, ceu_acc, "nbody.ceu : (lin 76, col 21) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_k;

                        CEU_Value ceu_idx_2024 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bodies;

                    ceu_assert_pre(ceu_block_2149, ceu_col_check(ceu_acc, ceu_idx_2024), "nbody.ceu : (lin 77, col 18)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2024.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2149, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2024.Number), "nbody.ceu : (lin 77, col 18)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2024);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_2149,
                            ceu_hold_chk_set(&ceu_block_2149->dyns, ceu_block_2149->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 77, col 9)"
                        );
                    
                
                    id_bi = ceu_acc;
                    ceu_gc_inc(id_bi);
                    ceu_acc = id_bi;
                    
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_2060 = ceu_acc;
                    if (ceu_closure_2060.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2149, "nbody.ceu : (lin 79, col 29)", err);
                    }
                    CEU_Frame ceu_frame_2060 = { &ceu_closure_2060.Dyn->Closure, ceu_block_2149 };
                    
                    CEU_Value ceu_args_2060[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_x} });
                        CEU_Value ceu_idx_2041 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2149, ceu_col_check(ceu_acc, ceu_idx_2041), "nbody.ceu : (lin 79, col 22)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2041.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2149, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2041.Number), "nbody.ceu : (lin 79, col 22)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2041);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2060[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2056 = ceu_acc;
                    if (ceu_closure_2056.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2149, "nbody.ceu : (lin 79, col 35)", err);
                    }
                    CEU_Frame ceu_frame_2056 = { &ceu_closure_2056.Dyn->Closure, ceu_block_2149 };
                    
                    CEU_Value ceu_args_2056[2];
                    
                    ceu_acc = id_dt;
ceu_args_2056[0] = ceu_acc;

                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vx} });
                        CEU_Value ceu_idx_2053 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2149, ceu_col_check(ceu_acc, ceu_idx_2053), "nbody.ceu : (lin 79, col 37)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2053.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2149, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2053.Number), "nbody.ceu : (lin 79, col 37)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2053);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2056[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2056.closure->proto (
                        &ceu_frame_2056,
                        2,
                        ceu_args_2056
                    );
                    ceu_assert_pre(ceu_block_2149, ceu_acc, "nbody.ceu : (lin 79, col 35) : call error");
                } // CALL
                ceu_args_2060[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2060.closure->proto (
                        &ceu_frame_2060,
                        2,
                        ceu_args_2060
                    );
                    ceu_assert_pre(ceu_block_2149, ceu_acc, "nbody.ceu : (lin 79, col 29) : call error");
                } // CALL
                
                    CEU_Value ceu_set_2061 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_x} });
                        CEU_Value ceu_idx_2033 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2149, ceu_col_check(ceu_acc, ceu_idx_2033), "nbody.ceu : (lin 79, col 13)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_2033.Number, (ceu_set_2061));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_2033.Number, (ceu_set_2061));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_2033, (ceu_set_2061));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_2149, ok, "nbody.ceu : (lin 79, col 13)");
                        
                }
                
                    ceu_acc = ceu_set_2061;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_2096 = ceu_acc;
                    if (ceu_closure_2096.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2149, "nbody.ceu : (lin 80, col 29)", err);
                    }
                    CEU_Frame ceu_frame_2096 = { &ceu_closure_2096.Dyn->Closure, ceu_block_2149 };
                    
                    CEU_Value ceu_args_2096[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_y} });
                        CEU_Value ceu_idx_2077 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2149, ceu_col_check(ceu_acc, ceu_idx_2077), "nbody.ceu : (lin 80, col 22)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2077.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2149, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2077.Number), "nbody.ceu : (lin 80, col 22)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2077);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2096[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2092 = ceu_acc;
                    if (ceu_closure_2092.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2149, "nbody.ceu : (lin 80, col 35)", err);
                    }
                    CEU_Frame ceu_frame_2092 = { &ceu_closure_2092.Dyn->Closure, ceu_block_2149 };
                    
                    CEU_Value ceu_args_2092[2];
                    
                    ceu_acc = id_dt;
ceu_args_2092[0] = ceu_acc;

                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vy} });
                        CEU_Value ceu_idx_2089 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2149, ceu_col_check(ceu_acc, ceu_idx_2089), "nbody.ceu : (lin 80, col 37)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2089.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2149, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2089.Number), "nbody.ceu : (lin 80, col 37)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2089);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2092[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2092.closure->proto (
                        &ceu_frame_2092,
                        2,
                        ceu_args_2092
                    );
                    ceu_assert_pre(ceu_block_2149, ceu_acc, "nbody.ceu : (lin 80, col 35) : call error");
                } // CALL
                ceu_args_2096[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2096.closure->proto (
                        &ceu_frame_2096,
                        2,
                        ceu_args_2096
                    );
                    ceu_assert_pre(ceu_block_2149, ceu_acc, "nbody.ceu : (lin 80, col 29) : call error");
                } // CALL
                
                    CEU_Value ceu_set_2097 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_y} });
                        CEU_Value ceu_idx_2069 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2149, ceu_col_check(ceu_acc, ceu_idx_2069), "nbody.ceu : (lin 80, col 13)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_2069.Number, (ceu_set_2097));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_2069.Number, (ceu_set_2097));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_2069, (ceu_set_2097));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_2149, ok, "nbody.ceu : (lin 80, col 13)");
                        
                }
                
                    ceu_acc = ceu_set_2097;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_2132 = ceu_acc;
                    if (ceu_closure_2132.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2149, "nbody.ceu : (lin 81, col 29)", err);
                    }
                    CEU_Frame ceu_frame_2132 = { &ceu_closure_2132.Dyn->Closure, ceu_block_2149 };
                    
                    CEU_Value ceu_args_2132[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_z} });
                        CEU_Value ceu_idx_2113 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2149, ceu_col_check(ceu_acc, ceu_idx_2113), "nbody.ceu : (lin 81, col 22)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2113.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2149, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2113.Number), "nbody.ceu : (lin 81, col 22)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2113);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2132[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2128 = ceu_acc;
                    if (ceu_closure_2128.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2149, "nbody.ceu : (lin 81, col 35)", err);
                    }
                    CEU_Frame ceu_frame_2128 = { &ceu_closure_2128.Dyn->Closure, ceu_block_2149 };
                    
                    CEU_Value ceu_args_2128[2];
                    
                    ceu_acc = id_dt;
ceu_args_2128[0] = ceu_acc;

                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vz} });
                        CEU_Value ceu_idx_2125 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2149, ceu_col_check(ceu_acc, ceu_idx_2125), "nbody.ceu : (lin 81, col 37)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2125.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2149, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2125.Number), "nbody.ceu : (lin 81, col 37)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2125);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2128[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2128.closure->proto (
                        &ceu_frame_2128,
                        2,
                        ceu_args_2128
                    );
                    ceu_assert_pre(ceu_block_2149, ceu_acc, "nbody.ceu : (lin 81, col 35) : call error");
                } // CALL
                ceu_args_2132[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2132.closure->proto (
                        &ceu_frame_2132,
                        2,
                        ceu_args_2132
                    );
                    ceu_assert_pre(ceu_block_2149, ceu_acc, "nbody.ceu : (lin 81, col 29) : call error");
                } // CALL
                
                    CEU_Value ceu_set_2133 = ceu_acc;
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_z} });
                        CEU_Value ceu_idx_2105 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2149, ceu_col_check(ceu_acc, ceu_idx_2105), "nbody.ceu : (lin 81, col 13)");
                
                        CEU_Value ok = { CEU_VALUE_NIL };
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ok = ceu_tuple_set(&ceu_acc.Dyn->Tuple, ceu_idx_2105.Number, (ceu_set_2133));
                                break;
                            case CEU_VALUE_VECTOR:
                                ok = ceu_vector_set(&ceu_acc.Dyn->Vector, ceu_idx_2105.Number, (ceu_set_2133));
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ok = ceu_dict_set(&ceu_dict.Dyn->Dict, ceu_idx_2105, (ceu_set_2133));
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                        ceu_assert_pre(ceu_block_2149, ok, "nbody.ceu : (lin 81, col 13)");
                        
                }
                
                    ceu_acc = ceu_set_2133;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_2145 = ceu_acc;
                    if (ceu_closure_2145.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2149, "nbody.ceu : (lin 83, col 19)", err);
                    }
                    CEU_Frame ceu_frame_2145 = { &ceu_closure_2145.Dyn->Closure, ceu_block_2149 };
                    
                    CEU_Value ceu_args_2145[2];
                    
                    ceu_acc = id_k;
ceu_args_2145[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_2145[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2145.closure->proto (
                        &ceu_frame_2145,
                        2,
                        ceu_args_2145
                    );
                    ceu_assert_pre(ceu_block_2149, ceu_acc, "nbody.ceu : (lin 83, col 19) : call error");
                } // CALL
                
                    CEU_Value ceu_set_2146 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_2149,
                                ceu_hold_chk_set(&ceu_block_2152->dyns, ceu_block_2152->depth, CEU_HOLD_MUTAB, (ceu_set_2146), 0, "set error"),
                                "nbody.ceu : (lin 83, col 13)"
                            );
                            ceu_gc_inc((ceu_set_2146));
                            ceu_gc_dec(id_k, 1);
                            id_k = (ceu_set_2146);
                        }
                        
                    ceu_acc = ceu_set_2146;
                }
                
                    }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_2149, 
                                ceu_hold_chk_set(&ceu_block_2152->dyns, ceu_block_2152->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "nbody.ceu : (lin 75, col 5)"
                            );
                            
                        
                            if (id_bi.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_bi, (id_bi.Dyn->Any.hld_depth == ceu_block_2149->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_2149);
                    }
                    
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_2152, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "nbody.ceu : (lin 41, col 33)"
                            );
                            
                        
                            if (id_n.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_n, (id_n.Dyn->Any.hld_depth == ceu_block_2152->depth));
                            }
                        
                            if (id_i.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_i, (id_i.Dyn->Any.hld_depth == ceu_block_2152->depth));
                            }
                        
                            if (id_k.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_k, (id_k.Dyn->Any.hld_depth == ceu_block_2152->depth));
                            }
                        
                        
                            
                                if (id_bodies.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_bodies, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_bodies.Dyn));
                                }
                                
                                if (id_dt.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_dt, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_dt.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_2152);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_2153 = ceu_closure_create (
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_2153,
                    0
                );
                ceu_acc = ceu_ret_2153;
                
                // UPVALS
                
                
                    CEU_Value ceu_set_2154 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_3004,
                                ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, (ceu_set_2154), 0, "set error"),
                                "nbody.ceu : (lin 41, col 5)"
                            );
                            ceu_gc_inc((ceu_set_2154));
                            ceu_gc_dec(id_advance, 1);
                            id_advance = (ceu_set_2154);
                        }
                        
                    ceu_acc = ceu_set_2154;
                }
                
                // DCL | 
                
                
                    
                    ceu_gc_inc(id_advance_multiple_steps);
                    ceu_acc = id_advance_multiple_steps;
                    
                
                { // SET | 
                     // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_2221 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_nsteps;
                            CEU_Block* _id_nsteps_;
                            
                            CEU_Value id_bodies;
                            CEU_Block* _id_bodies_;
                            
                            CEU_Value id_dt;
                            CEU_Block* _id_dt_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_2220 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_2220 = &_ceu_block_2220; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_2220,
                                            ceu_hold_chk_set(&ceu_block_2220->dyns, ceu_block_2220->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "nbody.ceu : (lin 88, col 56)"
                                        );
                                        id_nsteps = ceu_args[0];
                                    } else {
                                        id_nsteps = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (1 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_2220,
                                            ceu_hold_chk_set(&ceu_block_2220->dyns, ceu_block_2220->depth, CEU_HOLD_FLEET, ceu_args[1], 1, "argument error"),
                                            "nbody.ceu : (lin 88, col 56)"
                                        );
                                        id_bodies = ceu_args[1];
                                    } else {
                                        id_bodies = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                    if (2 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_2220,
                                            ceu_hold_chk_set(&ceu_block_2220->dyns, ceu_block_2220->depth, CEU_HOLD_FLEET, ceu_args[2], 1, "argument error"),
                                            "nbody.ceu : (lin 88, col 56)"
                                        );
                                        id_dt = ceu_args[2];
                                    } else {
                                        id_dt = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                            CEU_Value id_i = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        ceu_assert_pre(
                            ceu_block_2220,
                            ceu_hold_chk_set(&ceu_block_2220->dyns, ceu_block_2220->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 89, col 5)"
                        );
                    
                
                    id_i = ceu_acc;
                    ceu_gc_inc(id_i);
                    ceu_acc = id_i;
                    
                
                    CEU_Block* ceu_block_2217 = ceu_block_2220;
                    // >>> block
                    
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_2189 = ceu_acc;
                    if (ceu_closure_2189.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2217, "nbody.ceu : (lin 91, col 21)", err);
                    }
                    CEU_Frame ceu_frame_2189 = { &ceu_closure_2189.Dyn->Closure, ceu_block_2217 };
                    
                    CEU_Value ceu_args_2189[2];
                    
                    ceu_acc = id_i;
ceu_args_2189[0] = ceu_acc;
ceu_acc = id_nsteps;
ceu_args_2189[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2189.closure->proto (
                        &ceu_frame_2189,
                        2,
                        ceu_args_2189
                    );
                    ceu_assert_pre(ceu_block_2217, ceu_acc, "nbody.ceu : (lin 91, col 21) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                { // CALL | 
                    ceu_acc = id_advance;

                    CEU_Value ceu_closure_2201 = ceu_acc;
                    if (ceu_closure_2201.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2217, "nbody.ceu : (lin 92, col 9)", err);
                    }
                    CEU_Frame ceu_frame_2201 = { &ceu_closure_2201.Dyn->Closure, ceu_block_2217 };
                    
                    CEU_Value ceu_args_2201[2];
                    
                    ceu_acc = id_bodies;
ceu_args_2201[0] = ceu_acc;
ceu_acc = id_dt;
ceu_args_2201[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2201.closure->proto (
                        &ceu_frame_2201,
                        2,
                        ceu_args_2201
                    );
                    ceu_assert_pre(ceu_block_2217, ceu_acc, "nbody.ceu : (lin 92, col 9) : call error");
                } // CALL
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_2213 = ceu_acc;
                    if (ceu_closure_2213.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2217, "nbody.ceu : (lin 93, col 19)", err);
                    }
                    CEU_Frame ceu_frame_2213 = { &ceu_closure_2213.Dyn->Closure, ceu_block_2217 };
                    
                    CEU_Value ceu_args_2213[2];
                    
                    ceu_acc = id_i;
ceu_args_2213[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_2213[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2213.closure->proto (
                        &ceu_frame_2213,
                        2,
                        ceu_args_2213
                    );
                    ceu_assert_pre(ceu_block_2217, ceu_acc, "nbody.ceu : (lin 93, col 19) : call error");
                } // CALL
                
                    CEU_Value ceu_set_2214 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_2217,
                                ceu_hold_chk_set(&ceu_block_2220->dyns, ceu_block_2220->depth, CEU_HOLD_MUTAB, (ceu_set_2214), 0, "set error"),
                                "nbody.ceu : (lin 93, col 13)"
                            );
                            ceu_gc_inc((ceu_set_2214));
                            ceu_gc_dec(id_i, 1);
                            id_i = (ceu_set_2214);
                        }
                        
                    ceu_acc = ceu_set_2214;
                }
                
                    }
                
                    // <<< block
                    
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_2220, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "nbody.ceu : (lin 88, col 56)"
                            );
                            
                        
                            if (id_i.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_i, (id_i.Dyn->Any.hld_depth == ceu_block_2220->depth));
                            }
                        
                        
                            
                                if (id_nsteps.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_nsteps, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_nsteps.Dyn));
                                }
                                
                                if (id_bodies.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_bodies, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_bodies.Dyn));
                                }
                                
                                if (id_dt.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_dt, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_dt.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_2220);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_2221 = ceu_closure_create (
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_2221,
                    0
                );
                ceu_acc = ceu_ret_2221;
                
                // UPVALS
                
                
                    CEU_Value ceu_set_2222 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_3004,
                                ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, (ceu_set_2222), 0, "set error"),
                                "nbody.ceu : (lin 88, col 5)"
                            );
                            ceu_gc_inc((ceu_set_2222));
                            ceu_gc_dec(id_advance_multiple_steps, 1);
                            id_advance_multiple_steps = (ceu_set_2222);
                        }
                        
                    ceu_acc = ceu_set_2222;
                }
                
                // DCL | 
                
                
                    
                    ceu_gc_inc(id_energy);
                    ceu_acc = id_energy;
                    
                
                { // SET | 
                     // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_2631 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_bodies;
                            CEU_Block* _id_bodies_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_2630 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_2630 = &_ceu_block_2630; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_2630,
                                            ceu_hold_chk_set(&ceu_block_2630->dyns, ceu_block_2630->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "nbody.ceu : (lin 98, col 28)"
                                        );
                                        id_bodies = ceu_args[0];
                                    } else {
                                        id_bodies = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                            CEU_Value id_n = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_e = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_i = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_hash;

                    CEU_Value ceu_closure_2243 = ceu_acc;
                    if (ceu_closure_2243.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2630, "nbody.ceu : (lin 99, col 13)", err);
                    }
                    CEU_Frame ceu_frame_2243 = { &ceu_closure_2243.Dyn->Closure, ceu_block_2630 };
                    
                    CEU_Value ceu_args_2243[1];
                    
                    ceu_acc = id_bodies;
ceu_args_2243[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2243.closure->proto (
                        &ceu_frame_2243,
                        1,
                        ceu_args_2243
                    );
                    ceu_assert_pre(ceu_block_2630, ceu_acc, "nbody.ceu : (lin 99, col 13) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_2630,
                            ceu_hold_chk_set(&ceu_block_2630->dyns, ceu_block_2630->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 99, col 5)"
                        );
                    
                
                    id_n = ceu_acc;
                    ceu_gc_inc(id_n);
                    ceu_acc = id_n;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0} });
                        ceu_assert_pre(
                            ceu_block_2630,
                            ceu_hold_chk_set(&ceu_block_2630->dyns, ceu_block_2630->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 100, col 5)"
                        );
                    
                
                    id_e = ceu_acc;
                    ceu_gc_inc(id_e);
                    ceu_acc = id_e;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0} });
                        ceu_assert_pre(
                            ceu_block_2630,
                            ceu_hold_chk_set(&ceu_block_2630->dyns, ceu_block_2630->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 101, col 5)"
                        );
                    
                
                    id_i = ceu_acc;
                    ceu_gc_inc(id_i);
                    ceu_acc = id_i;
                    
                
                    { // BLOCK | 
                        CEU_Block _ceu_block_2625 = (CEU_Block) { (ceu_block_2630->depth + 1), 0, {.block=ceu_block_2630}, NULL };
                        CEU_Block* ceu_block_2625 = &_ceu_block_2625; 
                        
                        
                        
                            CEU_Value id_bi = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_vx = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_vy = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_vz = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_j = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_2269 = ceu_acc;
                    if (ceu_closure_2269.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2625, "nbody.ceu : (lin 103, col 21)", err);
                    }
                    CEU_Frame ceu_frame_2269 = { &ceu_closure_2269.Dyn->Closure, ceu_block_2625 };
                    
                    CEU_Value ceu_args_2269[2];
                    
                    ceu_acc = id_i;
ceu_args_2269[0] = ceu_acc;
ceu_acc = id_n;
ceu_args_2269[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2269.closure->proto (
                        &ceu_frame_2269,
                        2,
                        ceu_args_2269
                    );
                    ceu_assert_pre(ceu_block_2625, ceu_acc, "nbody.ceu : (lin 103, col 21) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_i;

                        CEU_Value ceu_idx_2281 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bodies;

                    ceu_assert_pre(ceu_block_2625, ceu_col_check(ceu_acc, ceu_idx_2281), "nbody.ceu : (lin 104, col 18)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2281.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2625, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2281.Number), "nbody.ceu : (lin 104, col 18)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2281);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_2625,
                            ceu_hold_chk_set(&ceu_block_2625->dyns, ceu_block_2625->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 104, col 9)"
                        );
                    
                
                    id_bi = ceu_acc;
                    ceu_gc_inc(id_bi);
                    ceu_acc = id_bi;
                    
                
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vx} });
                        CEU_Value ceu_idx_2292 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2625, ceu_col_check(ceu_acc, ceu_idx_2292), "nbody.ceu : (lin 105, col 18)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2292.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2625, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2292.Number), "nbody.ceu : (lin 105, col 18)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2292);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_2625,
                            ceu_hold_chk_set(&ceu_block_2625->dyns, ceu_block_2625->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 105, col 9)"
                        );
                    
                
                    id_vx = ceu_acc;
                    ceu_gc_inc(id_vx);
                    ceu_acc = id_vx;
                    
                
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vy} });
                        CEU_Value ceu_idx_2303 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2625, ceu_col_check(ceu_acc, ceu_idx_2303), "nbody.ceu : (lin 106, col 18)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2303.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2625, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2303.Number), "nbody.ceu : (lin 106, col 18)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2303);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_2625,
                            ceu_hold_chk_set(&ceu_block_2625->dyns, ceu_block_2625->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 106, col 9)"
                        );
                    
                
                    id_vy = ceu_acc;
                    ceu_gc_inc(id_vy);
                    ceu_acc = id_vy;
                    
                
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_vz} });
                        CEU_Value ceu_idx_2314 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2625, ceu_col_check(ceu_acc, ceu_idx_2314), "nbody.ceu : (lin 107, col 18)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2314.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2625, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2314.Number), "nbody.ceu : (lin 107, col 18)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2314);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_2625,
                            ceu_hold_chk_set(&ceu_block_2625->dyns, ceu_block_2625->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 107, col 9)"
                        );
                    
                
                    id_vz = ceu_acc;
                    ceu_gc_inc(id_vz);
                    ceu_acc = id_vz;
                    
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_2390 = ceu_acc;
                    if (ceu_closure_2390.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2625, "nbody.ceu : (lin 108, col 21)", err);
                    }
                    CEU_Frame ceu_frame_2390 = { &ceu_closure_2390.Dyn->Closure, ceu_block_2625 };
                    
                    CEU_Value ceu_args_2390[2];
                    
                    ceu_acc = id_e;
ceu_args_2390[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2386 = ceu_acc;
                    if (ceu_closure_2386.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2625, "nbody.ceu : (lin 108, col 28)", err);
                    }
                    CEU_Frame ceu_frame_2386 = { &ceu_closure_2386.Dyn->Closure, ceu_block_2625 };
                    
                    CEU_Value ceu_args_2386[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.5} });ceu_args_2386[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2382 = ceu_acc;
                    if (ceu_closure_2382.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2625, "nbody.ceu : (lin 108, col 41)", err);
                    }
                    CEU_Frame ceu_frame_2382 = { &ceu_closure_2382.Dyn->Closure, ceu_block_2625 };
                    
                    CEU_Value ceu_args_2382[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_mass} });
                        CEU_Value ceu_idx_2336 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2625, ceu_col_check(ceu_acc, ceu_idx_2336), "nbody.ceu : (lin 108, col 31)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2336.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2625, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2336.Number), "nbody.ceu : (lin 108, col 31)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2336);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2382[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_2378 = ceu_acc;
                    if (ceu_closure_2378.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2625, "nbody.ceu : (lin 108, col 52)", err);
                    }
                    CEU_Frame ceu_frame_2378 = { &ceu_closure_2378.Dyn->Closure, ceu_block_2625 };
                    
                    CEU_Value ceu_args_2378[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2347 = ceu_acc;
                    if (ceu_closure_2347.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2625, "nbody.ceu : (lin 108, col 47)", err);
                    }
                    CEU_Frame ceu_frame_2347 = { &ceu_closure_2347.Dyn->Closure, ceu_block_2625 };
                    
                    CEU_Value ceu_args_2347[2];
                    
                    ceu_acc = id_vx;
ceu_args_2347[0] = ceu_acc;
ceu_acc = id_vx;
ceu_args_2347[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2347.closure->proto (
                        &ceu_frame_2347,
                        2,
                        ceu_args_2347
                    );
                    ceu_assert_pre(ceu_block_2625, ceu_acc, "nbody.ceu : (lin 108, col 47) : call error");
                } // CALL
                ceu_args_2378[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_2374 = ceu_acc;
                    if (ceu_closure_2374.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2625, "nbody.ceu : (lin 108, col 63)", err);
                    }
                    CEU_Frame ceu_frame_2374 = { &ceu_closure_2374.Dyn->Closure, ceu_block_2625 };
                    
                    CEU_Value ceu_args_2374[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2359 = ceu_acc;
                    if (ceu_closure_2359.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2625, "nbody.ceu : (lin 108, col 58)", err);
                    }
                    CEU_Frame ceu_frame_2359 = { &ceu_closure_2359.Dyn->Closure, ceu_block_2625 };
                    
                    CEU_Value ceu_args_2359[2];
                    
                    ceu_acc = id_vy;
ceu_args_2359[0] = ceu_acc;
ceu_acc = id_vy;
ceu_args_2359[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2359.closure->proto (
                        &ceu_frame_2359,
                        2,
                        ceu_args_2359
                    );
                    ceu_assert_pre(ceu_block_2625, ceu_acc, "nbody.ceu : (lin 108, col 58) : call error");
                } // CALL
                ceu_args_2374[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2370 = ceu_acc;
                    if (ceu_closure_2370.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2625, "nbody.ceu : (lin 108, col 68)", err);
                    }
                    CEU_Frame ceu_frame_2370 = { &ceu_closure_2370.Dyn->Closure, ceu_block_2625 };
                    
                    CEU_Value ceu_args_2370[2];
                    
                    ceu_acc = id_vz;
ceu_args_2370[0] = ceu_acc;
ceu_acc = id_vz;
ceu_args_2370[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2370.closure->proto (
                        &ceu_frame_2370,
                        2,
                        ceu_args_2370
                    );
                    ceu_assert_pre(ceu_block_2625, ceu_acc, "nbody.ceu : (lin 108, col 68) : call error");
                } // CALL
                ceu_args_2374[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2374.closure->proto (
                        &ceu_frame_2374,
                        2,
                        ceu_args_2374
                    );
                    ceu_assert_pre(ceu_block_2625, ceu_acc, "nbody.ceu : (lin 108, col 63) : call error");
                } // CALL
                ceu_args_2378[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2378.closure->proto (
                        &ceu_frame_2378,
                        2,
                        ceu_args_2378
                    );
                    ceu_assert_pre(ceu_block_2625, ceu_acc, "nbody.ceu : (lin 108, col 52) : call error");
                } // CALL
                ceu_args_2382[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2382.closure->proto (
                        &ceu_frame_2382,
                        2,
                        ceu_args_2382
                    );
                    ceu_assert_pre(ceu_block_2625, ceu_acc, "nbody.ceu : (lin 108, col 41) : call error");
                } // CALL
                ceu_args_2386[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2386.closure->proto (
                        &ceu_frame_2386,
                        2,
                        ceu_args_2386
                    );
                    ceu_assert_pre(ceu_block_2625, ceu_acc, "nbody.ceu : (lin 108, col 28) : call error");
                } // CALL
                ceu_args_2390[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2390.closure->proto (
                        &ceu_frame_2390,
                        2,
                        ceu_args_2390
                    );
                    ceu_assert_pre(ceu_block_2625, ceu_acc, "nbody.ceu : (lin 108, col 21) : call error");
                } // CALL
                
                    CEU_Value ceu_set_2391 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_2625,
                                ceu_hold_chk_set(&ceu_block_2630->dyns, ceu_block_2630->depth, CEU_HOLD_MUTAB, (ceu_set_2391), 0, "set error"),
                                "nbody.ceu : (lin 108, col 13)"
                            );
                            ceu_gc_inc((ceu_set_2391));
                            ceu_gc_dec(id_e, 1);
                            id_e = (ceu_set_2391);
                        }
                        
                    ceu_acc = ceu_set_2391;
                }
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_2402 = ceu_acc;
                    if (ceu_closure_2402.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2625, "nbody.ceu : (lin 110, col 19)", err);
                    }
                    CEU_Frame ceu_frame_2402 = { &ceu_closure_2402.Dyn->Closure, ceu_block_2625 };
                    
                    CEU_Value ceu_args_2402[2];
                    
                    ceu_acc = id_i;
ceu_args_2402[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_2402[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2402.closure->proto (
                        &ceu_frame_2402,
                        2,
                        ceu_args_2402
                    );
                    ceu_assert_pre(ceu_block_2625, ceu_acc, "nbody.ceu : (lin 110, col 19) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_2625,
                            ceu_hold_chk_set(&ceu_block_2625->dyns, ceu_block_2625->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 110, col 9)"
                        );
                    
                
                    id_j = ceu_acc;
                    ceu_gc_inc(id_j);
                    ceu_acc = id_j;
                    
                
                    { // BLOCK | 
                        CEU_Block _ceu_block_2608 = (CEU_Block) { (ceu_block_2625->depth + 1), 0, {.block=ceu_block_2625}, NULL };
                        CEU_Block* ceu_block_2608 = &_ceu_block_2608; 
                        
                        
                        
                            CEU_Value id_bj = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_dx = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_dy = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_dz = (CEU_Value) { CEU_VALUE_NIL };
                        
                            CEU_Value id_distance = (CEU_Value) { CEU_VALUE_NIL };
                        
                        
                        
                        // >>> block
                        
                    while (1) { // LOOP | 
                        
                
                { // CALL | 
                    ceu_acc = op_equals_equals;

                    CEU_Value ceu_closure_2416 = ceu_acc;
                    if (ceu_closure_2416.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 112, col 25)", err);
                    }
                    CEU_Frame ceu_frame_2416 = { &ceu_closure_2416.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2416[2];
                    
                    ceu_acc = id_j;
ceu_args_2416[0] = ceu_acc;
ceu_acc = id_n;
ceu_args_2416[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2416.closure->proto (
                        &ceu_frame_2416,
                        2,
                        ceu_args_2416
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 112, col 25) : call error");
                } // CALL
                
                if (ceu_as_bool(ceu_acc)) {
                    
                    break;
                }
            
                // DCL | 
                
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = id_j;

                        CEU_Value ceu_idx_2428 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bodies;

                    ceu_assert_pre(ceu_block_2608, ceu_col_check(ceu_acc, ceu_idx_2428), "nbody.ceu : (lin 113, col 22)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2428.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2608, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2428.Number), "nbody.ceu : (lin 113, col 22)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2428);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                
                        ceu_assert_pre(
                            ceu_block_2608,
                            ceu_hold_chk_set(&ceu_block_2608->dyns, ceu_block_2608->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 113, col 13)"
                        );
                    
                
                    id_bj = ceu_acc;
                    ceu_gc_inc(id_bj);
                    ceu_acc = id_bj;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2450 = ceu_acc;
                    if (ceu_closure_2450.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 114, col 29)", err);
                    }
                    CEU_Frame ceu_frame_2450 = { &ceu_closure_2450.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2450[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_x} });
                        CEU_Value ceu_idx_2439 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2608, ceu_col_check(ceu_acc, ceu_idx_2439), "nbody.ceu : (lin 114, col 22)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2439.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2608, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2439.Number), "nbody.ceu : (lin 114, col 22)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2439);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2450[0] = ceu_acc;

                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_x} });
                        CEU_Value ceu_idx_2447 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_2608, ceu_col_check(ceu_acc, ceu_idx_2447), "nbody.ceu : (lin 114, col 31)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2447.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2608, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2447.Number), "nbody.ceu : (lin 114, col 31)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2447);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2450[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2450.closure->proto (
                        &ceu_frame_2450,
                        2,
                        ceu_args_2450
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 114, col 29) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_2608,
                            ceu_hold_chk_set(&ceu_block_2608->dyns, ceu_block_2608->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 114, col 13)"
                        );
                    
                
                    id_dx = ceu_acc;
                    ceu_gc_inc(id_dx);
                    ceu_acc = id_dx;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2472 = ceu_acc;
                    if (ceu_closure_2472.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 115, col 29)", err);
                    }
                    CEU_Frame ceu_frame_2472 = { &ceu_closure_2472.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2472[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_y} });
                        CEU_Value ceu_idx_2461 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2608, ceu_col_check(ceu_acc, ceu_idx_2461), "nbody.ceu : (lin 115, col 22)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2461.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2608, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2461.Number), "nbody.ceu : (lin 115, col 22)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2461);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2472[0] = ceu_acc;

                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_y} });
                        CEU_Value ceu_idx_2469 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_2608, ceu_col_check(ceu_acc, ceu_idx_2469), "nbody.ceu : (lin 115, col 31)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2469.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2608, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2469.Number), "nbody.ceu : (lin 115, col 31)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2469);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2472[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2472.closure->proto (
                        &ceu_frame_2472,
                        2,
                        ceu_args_2472
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 115, col 29) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_2608,
                            ceu_hold_chk_set(&ceu_block_2608->dyns, ceu_block_2608->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 115, col 13)"
                        );
                    
                
                    id_dy = ceu_acc;
                    ceu_gc_inc(id_dy);
                    ceu_acc = id_dy;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2494 = ceu_acc;
                    if (ceu_closure_2494.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 116, col 29)", err);
                    }
                    CEU_Frame ceu_frame_2494 = { &ceu_closure_2494.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2494[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_z} });
                        CEU_Value ceu_idx_2483 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2608, ceu_col_check(ceu_acc, ceu_idx_2483), "nbody.ceu : (lin 116, col 22)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2483.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2608, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2483.Number), "nbody.ceu : (lin 116, col 22)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2483);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2494[0] = ceu_acc;

                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_z} });
                        CEU_Value ceu_idx_2491 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_2608, ceu_col_check(ceu_acc, ceu_idx_2491), "nbody.ceu : (lin 116, col 31)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2491.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2608, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2491.Number), "nbody.ceu : (lin 116, col 31)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2491);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2494[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2494.closure->proto (
                        &ceu_frame_2494,
                        2,
                        ceu_args_2494
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 116, col 29) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_2608,
                            ceu_hold_chk_set(&ceu_block_2608->dyns, ceu_block_2608->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 116, col 13)"
                        );
                    
                
                    id_dz = ceu_acc;
                    ceu_gc_inc(id_dz);
                    ceu_acc = id_dz;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2552 = ceu_acc;
                    if (ceu_closure_2552.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 117, col 57)", err);
                    }
                    CEU_Frame ceu_frame_2552 = { &ceu_closure_2552.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2552[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_2537 = ceu_acc;
                    if (ceu_closure_2537.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 117, col 47)", err);
                    }
                    CEU_Frame ceu_frame_2537 = { &ceu_closure_2537.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2537[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_2523 = ceu_acc;
                    if (ceu_closure_2523.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 117, col 37)", err);
                    }
                    CEU_Frame ceu_frame_2523 = { &ceu_closure_2523.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2523[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2508 = ceu_acc;
                    if (ceu_closure_2508.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 117, col 32)", err);
                    }
                    CEU_Frame ceu_frame_2508 = { &ceu_closure_2508.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2508[2];
                    
                    ceu_acc = id_dx;
ceu_args_2508[0] = ceu_acc;
ceu_acc = id_dx;
ceu_args_2508[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2508.closure->proto (
                        &ceu_frame_2508,
                        2,
                        ceu_args_2508
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 117, col 32) : call error");
                } // CALL
                ceu_args_2523[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2519 = ceu_acc;
                    if (ceu_closure_2519.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 117, col 42)", err);
                    }
                    CEU_Frame ceu_frame_2519 = { &ceu_closure_2519.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2519[2];
                    
                    ceu_acc = id_dy;
ceu_args_2519[0] = ceu_acc;
ceu_acc = id_dy;
ceu_args_2519[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2519.closure->proto (
                        &ceu_frame_2519,
                        2,
                        ceu_args_2519
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 117, col 42) : call error");
                } // CALL
                ceu_args_2523[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2523.closure->proto (
                        &ceu_frame_2523,
                        2,
                        ceu_args_2523
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 117, col 37) : call error");
                } // CALL
                ceu_args_2537[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2533 = ceu_acc;
                    if (ceu_closure_2533.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 117, col 52)", err);
                    }
                    CEU_Frame ceu_frame_2533 = { &ceu_closure_2533.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2533[2];
                    
                    ceu_acc = id_dz;
ceu_args_2533[0] = ceu_acc;
ceu_acc = id_dz;
ceu_args_2533[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2533.closure->proto (
                        &ceu_frame_2533,
                        2,
                        ceu_args_2533
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 117, col 52) : call error");
                } // CALL
                ceu_args_2537[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2537.closure->proto (
                        &ceu_frame_2537,
                        2,
                        ceu_args_2537
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 117, col 47) : call error");
                } // CALL
                ceu_args_2552[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash;

                    CEU_Value ceu_closure_2548 = ceu_acc;
                    if (ceu_closure_2548.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 117, col 60)", err);
                    }
                    CEU_Frame ceu_frame_2548 = { &ceu_closure_2548.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2548[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_2548[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=2} });ceu_args_2548[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2548.closure->proto (
                        &ceu_frame_2548,
                        2,
                        ceu_args_2548
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 117, col 60) : call error");
                } // CALL
                ceu_args_2552[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2552.closure->proto (
                        &ceu_frame_2552,
                        2,
                        ceu_args_2552
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 117, col 57) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_2608,
                            ceu_hold_chk_set(&ceu_block_2608->dyns, ceu_block_2608->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 117, col 13)"
                        );
                    
                
                    id_distance = ceu_acc;
                    ceu_gc_inc(id_distance);
                    ceu_acc = id_distance;
                    
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2591 = ceu_acc;
                    if (ceu_closure_2591.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 118, col 23)", err);
                    }
                    CEU_Frame ceu_frame_2591 = { &ceu_closure_2591.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2591[2];
                    
                    ceu_acc = id_e;
ceu_args_2591[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_slash;

                    CEU_Value ceu_closure_2587 = ceu_acc;
                    if (ceu_closure_2587.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 118, col 50)", err);
                    }
                    CEU_Frame ceu_frame_2587 = { &ceu_closure_2587.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2587[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2580 = ceu_acc;
                    if (ceu_closure_2580.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 118, col 37)", err);
                    }
                    CEU_Frame ceu_frame_2580 = { &ceu_closure_2580.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2580[2];
                    
                    
                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_mass} });
                        CEU_Value ceu_idx_2569 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bi;

                    ceu_assert_pre(ceu_block_2608, ceu_col_check(ceu_acc, ceu_idx_2569), "nbody.ceu : (lin 118, col 27)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2569.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2608, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2569.Number), "nbody.ceu : (lin 118, col 27)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2569);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2580[0] = ceu_acc;

                { // INDEX | 
                    // IDX
                    
                        ceu_acc = ((CEU_Value) { CEU_VALUE_TAG, {.Tag=CEU_TAG_mass} });
                        CEU_Value ceu_idx_2577 = ceu_acc;
                        
                    // COL
                    ceu_acc = id_bj;

                    ceu_assert_pre(ceu_block_2608, ceu_col_check(ceu_acc, ceu_idx_2577), "nbody.ceu : (lin 118, col 39)");
                
                        switch (ceu_acc.type) {
                            case CEU_VALUE_TUPLE:
                                ceu_acc = ceu_acc.Dyn->Tuple.buf[(int) ceu_idx_2577.Number];
                                break;
                            case CEU_VALUE_VECTOR:
                                ceu_acc = ceu_assert_pre(ceu_block_2608, ceu_vector_get(&ceu_acc.Dyn->Vector, ceu_idx_2577.Number), "nbody.ceu : (lin 118, col 39)");
                                break;
                            case CEU_VALUE_DICT: {
                                CEU_Value ceu_dict = ceu_acc;
                                ceu_acc = ceu_dict_get(&ceu_dict.Dyn->Dict, ceu_idx_2577);
                                break;
                            }
                            default:
                                assert(0 && "bug found");
                        }
                    
                }
                ceu_args_2580[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2580.closure->proto (
                        &ceu_frame_2580,
                        2,
                        ceu_args_2580
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 118, col 37) : call error");
                } // CALL
                ceu_args_2587[0] = ceu_acc;
ceu_acc = id_distance;
ceu_args_2587[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2587.closure->proto (
                        &ceu_frame_2587,
                        2,
                        ceu_args_2587
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 118, col 50) : call error");
                } // CALL
                ceu_args_2591[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2591.closure->proto (
                        &ceu_frame_2591,
                        2,
                        ceu_args_2591
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 118, col 23) : call error");
                } // CALL
                
                    CEU_Value ceu_set_2592 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_2608,
                                ceu_hold_chk_set(&ceu_block_2630->dyns, ceu_block_2630->depth, CEU_HOLD_MUTAB, (ceu_set_2592), 0, "set error"),
                                "nbody.ceu : (lin 118, col 17)"
                            );
                            ceu_gc_inc((ceu_set_2592));
                            ceu_gc_dec(id_e, 1);
                            id_e = (ceu_set_2592);
                        }
                        
                    ceu_acc = ceu_set_2592;
                }
                
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_2604 = ceu_acc;
                    if (ceu_closure_2604.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2608, "nbody.ceu : (lin 120, col 23)", err);
                    }
                    CEU_Frame ceu_frame_2604 = { &ceu_closure_2604.Dyn->Closure, ceu_block_2608 };
                    
                    CEU_Value ceu_args_2604[2];
                    
                    ceu_acc = id_j;
ceu_args_2604[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_2604[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2604.closure->proto (
                        &ceu_frame_2604,
                        2,
                        ceu_args_2604
                    );
                    ceu_assert_pre(ceu_block_2608, ceu_acc, "nbody.ceu : (lin 120, col 23) : call error");
                } // CALL
                
                    CEU_Value ceu_set_2605 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_2608,
                                ceu_hold_chk_set(&ceu_block_2625->dyns, ceu_block_2625->depth, CEU_HOLD_MUTAB, (ceu_set_2605), 0, "set error"),
                                "nbody.ceu : (lin 120, col 17)"
                            );
                            ceu_gc_inc((ceu_set_2605));
                            ceu_gc_dec(id_j, 1);
                            id_j = (ceu_set_2605);
                        }
                        
                    ceu_acc = ceu_set_2605;
                }
                
                    }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_2608, 
                                ceu_hold_chk_set(&ceu_block_2625->dyns, ceu_block_2625->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "nbody.ceu : (lin 111, col 9)"
                            );
                            
                        
                            if (id_bj.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_bj, (id_bj.Dyn->Any.hld_depth == ceu_block_2608->depth));
                            }
                        
                            if (id_dx.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_dx, (id_dx.Dyn->Any.hld_depth == ceu_block_2608->depth));
                            }
                        
                            if (id_dy.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_dy, (id_dy.Dyn->Any.hld_depth == ceu_block_2608->depth));
                            }
                        
                            if (id_dz.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_dz, (id_dz.Dyn->Any.hld_depth == ceu_block_2608->depth));
                            }
                        
                            if (id_distance.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_distance, (id_distance.Dyn->Any.hld_depth == ceu_block_2608->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_2608);
                    }
                    
                { // SET | 
                    
                { // CALL | 
                    ceu_acc = op_plus;

                    CEU_Value ceu_closure_2621 = ceu_acc;
                    if (ceu_closure_2621.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2625, "nbody.ceu : (lin 122, col 19)", err);
                    }
                    CEU_Frame ceu_frame_2621 = { &ceu_closure_2621.Dyn->Closure, ceu_block_2625 };
                    
                    CEU_Value ceu_args_2621[2];
                    
                    ceu_acc = id_i;
ceu_args_2621[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1} });ceu_args_2621[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2621.closure->proto (
                        &ceu_frame_2621,
                        2,
                        ceu_args_2621
                    );
                    ceu_assert_pre(ceu_block_2625, ceu_acc, "nbody.ceu : (lin 122, col 19) : call error");
                } // CALL
                
                    CEU_Value ceu_set_2622 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_2625,
                                ceu_hold_chk_set(&ceu_block_2630->dyns, ceu_block_2630->depth, CEU_HOLD_MUTAB, (ceu_set_2622), 0, "set error"),
                                "nbody.ceu : (lin 122, col 13)"
                            );
                            ceu_gc_inc((ceu_set_2622));
                            ceu_gc_dec(id_i, 1);
                            id_i = (ceu_set_2622);
                        }
                        
                    ceu_acc = ceu_set_2622;
                }
                
                    }
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_2625, 
                                ceu_hold_chk_set(&ceu_block_2630->dyns, ceu_block_2630->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "nbody.ceu : (lin 102, col 5)"
                            );
                            
                        
                            if (id_bi.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_bi, (id_bi.Dyn->Any.hld_depth == ceu_block_2625->depth));
                            }
                        
                            if (id_vx.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_vx, (id_vx.Dyn->Any.hld_depth == ceu_block_2625->depth));
                            }
                        
                            if (id_vy.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_vy, (id_vy.Dyn->Any.hld_depth == ceu_block_2625->depth));
                            }
                        
                            if (id_vz.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_vz, (id_vz.Dyn->Any.hld_depth == ceu_block_2625->depth));
                            }
                        
                            if (id_j.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_j, (id_j.Dyn->Any.hld_depth == ceu_block_2625->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_2625);
                    }
                    ceu_acc = id_e;

                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_2630, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "nbody.ceu : (lin 98, col 28)"
                            );
                            
                        
                            if (id_n.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_n, (id_n.Dyn->Any.hld_depth == ceu_block_2630->depth));
                            }
                        
                            if (id_e.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_e, (id_e.Dyn->Any.hld_depth == ceu_block_2630->depth));
                            }
                        
                            if (id_i.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_i, (id_i.Dyn->Any.hld_depth == ceu_block_2630->depth));
                            }
                        
                        
                            
                                if (id_bodies.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_bodies, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_bodies.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_2630);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_2631 = ceu_closure_create (
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_2631,
                    0
                );
                ceu_acc = ceu_ret_2631;
                
                // UPVALS
                
                
                    CEU_Value ceu_set_2632 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_3004,
                                ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, (ceu_set_2632), 0, "set error"),
                                "nbody.ceu : (lin 98, col 5)"
                            );
                            ceu_gc_inc((ceu_set_2632));
                            ceu_gc_dec(id_energy, 1);
                            id_energy = (ceu_set_2632);
                        }
                        
                    ceu_acc = ceu_set_2632;
                }
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=3.141592653589793} });
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 127, col 1)"
                        );
                    
                
                    id_PI = ceu_acc;
                    ceu_gc_inc(id_PI);
                    ceu_acc = id_PI;
                    
                
                // DCL | 
                
                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2655 = ceu_acc;
                    if (ceu_closure_2655.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 128, col 25)", err);
                    }
                    CEU_Frame ceu_frame_2655 = { &ceu_closure_2655.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2655[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2649 = ceu_acc;
                    if (ceu_closure_2649.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 128, col 20)", err);
                    }
                    CEU_Frame ceu_frame_2649 = { &ceu_closure_2649.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2649[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=4} });ceu_args_2649[0] = ceu_acc;
ceu_acc = id_PI;
ceu_args_2649[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2649.closure->proto (
                        &ceu_frame_2649,
                        2,
                        ceu_args_2649
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 128, col 20) : call error");
                } // CALL
                ceu_args_2655[0] = ceu_acc;
ceu_acc = id_PI;
ceu_args_2655[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2655.closure->proto (
                        &ceu_frame_2655,
                        2,
                        ceu_args_2655
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 128, col 25) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 128, col 1)"
                        );
                    
                
                    id_SOLAR_MASS = ceu_acc;
                    ceu_gc_inc(id_SOLAR_MASS);
                    ceu_acc = id_SOLAR_MASS;
                    
                
                // DCL | 
                ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=365.24} });
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 129, col 1)"
                        );
                    
                
                    id_DAYS_PER_YEAR = ceu_acc;
                    ceu_gc_inc(id_DAYS_PER_YEAR);
                    ceu_acc = id_DAYS_PER_YEAR;
                    
                
                // DCL | 
                
                { // VECTOR | 
                    CEU_Value ceu_vec_2933 = ceu_vector_create(ceu_block_3004);
                    
                { // CALL | 
                    ceu_acc = id_new_body;

                    CEU_Value ceu_closure_2691 = ceu_acc;
                    if (ceu_closure_2691.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 131, col 5)", err);
                    }
                    CEU_Frame ceu_frame_2691 = { &ceu_closure_2691.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2691[7];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0} });ceu_args_2691[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0} });ceu_args_2691[1] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0} });ceu_args_2691[2] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0} });ceu_args_2691[3] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0} });ceu_args_2691[4] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0} });ceu_args_2691[5] = ceu_acc;
ceu_acc = id_SOLAR_MASS;
ceu_args_2691[6] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2691.closure->proto (
                        &ceu_frame_2691,
                        7,
                        ceu_args_2691
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 131, col 5) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_vector_set(&ceu_vec_2933.Dyn->Vector, 0, ceu_acc),
                            "nbody.ceu : (lin 130, col 14)"
                        );
                        
                { // CALL | 
                    ceu_acc = id_new_body;

                    CEU_Value ceu_closure_2753 = ceu_acc;
                    if (ceu_closure_2753.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 139, col 5)", err);
                    }
                    CEU_Frame ceu_frame_2753 = { &ceu_closure_2753.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2753[7];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=4.84143144246472090} });ceu_args_2753[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2704 = ceu_acc;
                    if (ceu_closure_2704.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 141, col 4)", err);
                    }
                    CEU_Frame ceu_frame_2704 = { &ceu_closure_2704.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2704[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1.16032004402742839} });ceu_args_2704[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2704.closure->proto (
                        &ceu_frame_2704,
                        1,
                        ceu_args_2704
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 141, col 4) : call error");
                } // CALL
                ceu_args_2753[1] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2711 = ceu_acc;
                    if (ceu_closure_2711.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 142, col 4)", err);
                    }
                    CEU_Frame ceu_frame_2711 = { &ceu_closure_2711.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2711[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.103622044471123109} });ceu_args_2711[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2711.closure->proto (
                        &ceu_frame_2711,
                        1,
                        ceu_args_2711
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 142, col 4) : call error");
                } // CALL
                ceu_args_2753[2] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2720 = ceu_acc;
                    if (ceu_closure_2720.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 143, col 28)", err);
                    }
                    CEU_Frame ceu_frame_2720 = { &ceu_closure_2720.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2720[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.00166007664274403694} });ceu_args_2720[0] = ceu_acc;
ceu_acc = id_DAYS_PER_YEAR;
ceu_args_2720[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2720.closure->proto (
                        &ceu_frame_2720,
                        2,
                        ceu_args_2720
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 143, col 28) : call error");
                } // CALL
                ceu_args_2753[3] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2729 = ceu_acc;
                    if (ceu_closure_2729.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 144, col 28)", err);
                    }
                    CEU_Frame ceu_frame_2729 = { &ceu_closure_2729.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2729[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.00769901118419740425} });ceu_args_2729[0] = ceu_acc;
ceu_acc = id_DAYS_PER_YEAR;
ceu_args_2729[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2729.closure->proto (
                        &ceu_frame_2729,
                        2,
                        ceu_args_2729
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 144, col 28) : call error");
                } // CALL
                ceu_args_2753[4] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2742 = ceu_acc;
                    if (ceu_closure_2742.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 145, col 30)", err);
                    }
                    CEU_Frame ceu_frame_2742 = { &ceu_closure_2742.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2742[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2736 = ceu_acc;
                    if (ceu_closure_2736.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 145, col 4)", err);
                    }
                    CEU_Frame ceu_frame_2736 = { &ceu_closure_2736.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2736[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0000690460016972063023} });ceu_args_2736[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2736.closure->proto (
                        &ceu_frame_2736,
                        1,
                        ceu_args_2736
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 145, col 4) : call error");
                } // CALL
                ceu_args_2742[0] = ceu_acc;
ceu_acc = id_DAYS_PER_YEAR;
ceu_args_2742[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2742.closure->proto (
                        &ceu_frame_2742,
                        2,
                        ceu_args_2742
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 145, col 30) : call error");
                } // CALL
                ceu_args_2753[5] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2751 = ceu_acc;
                    if (ceu_closure_2751.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 146, col 29)", err);
                    }
                    CEU_Frame ceu_frame_2751 = { &ceu_closure_2751.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2751[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.000954791938424326609} });ceu_args_2751[0] = ceu_acc;
ceu_acc = id_SOLAR_MASS;
ceu_args_2751[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2751.closure->proto (
                        &ceu_frame_2751,
                        2,
                        ceu_args_2751
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 146, col 29) : call error");
                } // CALL
                ceu_args_2753[6] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2753.closure->proto (
                        &ceu_frame_2753,
                        7,
                        ceu_args_2753
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 139, col 5) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_vector_set(&ceu_vec_2933.Dyn->Vector, 1, ceu_acc),
                            "nbody.ceu : (lin 130, col 14)"
                        );
                        
                { // CALL | 
                    ceu_acc = id_new_body;

                    CEU_Value ceu_closure_2811 = ceu_acc;
                    if (ceu_closure_2811.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 147, col 5)", err);
                    }
                    CEU_Frame ceu_frame_2811 = { &ceu_closure_2811.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2811[7];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=8.34336671824457987} });ceu_args_2811[0] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=4.12479856412430479} });ceu_args_2811[1] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2769 = ceu_acc;
                    if (ceu_closure_2769.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 150, col 4)", err);
                    }
                    CEU_Frame ceu_frame_2769 = { &ceu_closure_2769.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2769[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.403523417114321381} });ceu_args_2769[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2769.closure->proto (
                        &ceu_frame_2769,
                        1,
                        ceu_args_2769
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 150, col 4) : call error");
                } // CALL
                ceu_args_2811[2] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2782 = ceu_acc;
                    if (ceu_closure_2782.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 151, col 28)", err);
                    }
                    CEU_Frame ceu_frame_2782 = { &ceu_closure_2782.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2782[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2776 = ceu_acc;
                    if (ceu_closure_2776.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 151, col 4)", err);
                    }
                    CEU_Frame ceu_frame_2776 = { &ceu_closure_2776.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2776[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.00276742510726862411} });ceu_args_2776[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2776.closure->proto (
                        &ceu_frame_2776,
                        1,
                        ceu_args_2776
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 151, col 4) : call error");
                } // CALL
                ceu_args_2782[0] = ceu_acc;
ceu_acc = id_DAYS_PER_YEAR;
ceu_args_2782[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2782.closure->proto (
                        &ceu_frame_2782,
                        2,
                        ceu_args_2782
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 151, col 28) : call error");
                } // CALL
                ceu_args_2811[3] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2791 = ceu_acc;
                    if (ceu_closure_2791.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 152, col 28)", err);
                    }
                    CEU_Frame ceu_frame_2791 = { &ceu_closure_2791.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2791[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.00499852801234917238} });ceu_args_2791[0] = ceu_acc;
ceu_acc = id_DAYS_PER_YEAR;
ceu_args_2791[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2791.closure->proto (
                        &ceu_frame_2791,
                        2,
                        ceu_args_2791
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 152, col 28) : call error");
                } // CALL
                ceu_args_2811[4] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2800 = ceu_acc;
                    if (ceu_closure_2800.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 153, col 30)", err);
                    }
                    CEU_Frame ceu_frame_2800 = { &ceu_closure_2800.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2800[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0000230417297573763929} });ceu_args_2800[0] = ceu_acc;
ceu_acc = id_DAYS_PER_YEAR;
ceu_args_2800[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2800.closure->proto (
                        &ceu_frame_2800,
                        2,
                        ceu_args_2800
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 153, col 30) : call error");
                } // CALL
                ceu_args_2811[5] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2809 = ceu_acc;
                    if (ceu_closure_2809.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 154, col 29)", err);
                    }
                    CEU_Frame ceu_frame_2809 = { &ceu_closure_2809.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2809[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.000285885980666130812} });ceu_args_2809[0] = ceu_acc;
ceu_acc = id_SOLAR_MASS;
ceu_args_2809[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2809.closure->proto (
                        &ceu_frame_2809,
                        2,
                        ceu_args_2809
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 154, col 29) : call error");
                } // CALL
                ceu_args_2811[6] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2811.closure->proto (
                        &ceu_frame_2811,
                        7,
                        ceu_args_2811
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 147, col 5) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_vector_set(&ceu_vec_2933.Dyn->Vector, 2, ceu_acc),
                            "nbody.ceu : (lin 130, col 14)"
                        );
                        
                { // CALL | 
                    ceu_acc = id_new_body;

                    CEU_Value ceu_closure_2873 = ceu_acc;
                    if (ceu_closure_2873.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 155, col 5)", err);
                    }
                    CEU_Frame ceu_frame_2873 = { &ceu_closure_2873.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2873[7];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=12.8943695621391310} });ceu_args_2873[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2824 = ceu_acc;
                    if (ceu_closure_2824.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 157, col 4)", err);
                    }
                    CEU_Frame ceu_frame_2824 = { &ceu_closure_2824.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2824[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=15.1111514016986312} });ceu_args_2824[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2824.closure->proto (
                        &ceu_frame_2824,
                        1,
                        ceu_args_2824
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 157, col 4) : call error");
                } // CALL
                ceu_args_2873[1] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2831 = ceu_acc;
                    if (ceu_closure_2831.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 158, col 4)", err);
                    }
                    CEU_Frame ceu_frame_2831 = { &ceu_closure_2831.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2831[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.223307578892655734} });ceu_args_2831[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2831.closure->proto (
                        &ceu_frame_2831,
                        1,
                        ceu_args_2831
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 158, col 4) : call error");
                } // CALL
                ceu_args_2873[2] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2840 = ceu_acc;
                    if (ceu_closure_2840.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 159, col 28)", err);
                    }
                    CEU_Frame ceu_frame_2840 = { &ceu_closure_2840.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2840[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.00296460137564761618} });ceu_args_2840[0] = ceu_acc;
ceu_acc = id_DAYS_PER_YEAR;
ceu_args_2840[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2840.closure->proto (
                        &ceu_frame_2840,
                        2,
                        ceu_args_2840
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 159, col 28) : call error");
                } // CALL
                ceu_args_2873[3] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2849 = ceu_acc;
                    if (ceu_closure_2849.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 160, col 28)", err);
                    }
                    CEU_Frame ceu_frame_2849 = { &ceu_closure_2849.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2849[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.00237847173959480950} });ceu_args_2849[0] = ceu_acc;
ceu_acc = id_DAYS_PER_YEAR;
ceu_args_2849[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2849.closure->proto (
                        &ceu_frame_2849,
                        2,
                        ceu_args_2849
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 160, col 28) : call error");
                } // CALL
                ceu_args_2873[4] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2862 = ceu_acc;
                    if (ceu_closure_2862.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 161, col 30)", err);
                    }
                    CEU_Frame ceu_frame_2862 = { &ceu_closure_2862.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2862[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2856 = ceu_acc;
                    if (ceu_closure_2856.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 161, col 4)", err);
                    }
                    CEU_Frame ceu_frame_2856 = { &ceu_closure_2856.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2856[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0000296589568540237556} });ceu_args_2856[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2856.closure->proto (
                        &ceu_frame_2856,
                        1,
                        ceu_args_2856
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 161, col 4) : call error");
                } // CALL
                ceu_args_2862[0] = ceu_acc;
ceu_acc = id_DAYS_PER_YEAR;
ceu_args_2862[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2862.closure->proto (
                        &ceu_frame_2862,
                        2,
                        ceu_args_2862
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 161, col 30) : call error");
                } // CALL
                ceu_args_2873[5] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2871 = ceu_acc;
                    if (ceu_closure_2871.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 162, col 30)", err);
                    }
                    CEU_Frame ceu_frame_2871 = { &ceu_closure_2871.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2871[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0000436624404335156298} });ceu_args_2871[0] = ceu_acc;
ceu_acc = id_SOLAR_MASS;
ceu_args_2871[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2871.closure->proto (
                        &ceu_frame_2871,
                        2,
                        ceu_args_2871
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 162, col 30) : call error");
                } // CALL
                ceu_args_2873[6] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2873.closure->proto (
                        &ceu_frame_2873,
                        7,
                        ceu_args_2873
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 155, col 5) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_vector_set(&ceu_vec_2933.Dyn->Vector, 3, ceu_acc),
                            "nbody.ceu : (lin 130, col 14)"
                        );
                        
                { // CALL | 
                    ceu_acc = id_new_body;

                    CEU_Value ceu_closure_2931 = ceu_acc;
                    if (ceu_closure_2931.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 166, col 5)", err);
                    }
                    CEU_Frame ceu_frame_2931 = { &ceu_closure_2931.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2931[7];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=1.537969711485091650} });ceu_args_2931[0] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2886 = ceu_acc;
                    if (ceu_closure_2886.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 168, col 8)", err);
                    }
                    CEU_Frame ceu_frame_2886 = { &ceu_closure_2886.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2886[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=2.591931460998796410} });ceu_args_2886[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2886.closure->proto (
                        &ceu_frame_2886,
                        1,
                        ceu_args_2886
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 168, col 8) : call error");
                } // CALL
                ceu_args_2931[1] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.179258772950371181} });ceu_args_2931[2] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2898 = ceu_acc;
                    if (ceu_closure_2898.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 170, col 32)", err);
                    }
                    CEU_Frame ceu_frame_2898 = { &ceu_closure_2898.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2898[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.00268067772490389322} });ceu_args_2898[0] = ceu_acc;
ceu_acc = id_DAYS_PER_YEAR;
ceu_args_2898[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2898.closure->proto (
                        &ceu_frame_2898,
                        2,
                        ceu_args_2898
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 170, col 32) : call error");
                } // CALL
                ceu_args_2931[3] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2907 = ceu_acc;
                    if (ceu_closure_2907.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 171, col 32)", err);
                    }
                    CEU_Frame ceu_frame_2907 = { &ceu_closure_2907.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2907[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.00162824170038242295} });ceu_args_2907[0] = ceu_acc;
ceu_acc = id_DAYS_PER_YEAR;
ceu_args_2907[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2907.closure->proto (
                        &ceu_frame_2907,
                        2,
                        ceu_args_2907
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 171, col 32) : call error");
                } // CALL
                ceu_args_2931[4] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2920 = ceu_acc;
                    if (ceu_closure_2920.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 172, col 34)", err);
                    }
                    CEU_Frame ceu_frame_2920 = { &ceu_closure_2920.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2920[2];
                    
                    
                { // CALL | 
                    ceu_acc = op_minus;

                    CEU_Value ceu_closure_2914 = ceu_acc;
                    if (ceu_closure_2914.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 172, col 8)", err);
                    }
                    CEU_Frame ceu_frame_2914 = { &ceu_closure_2914.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2914[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0000951592254519715870} });ceu_args_2914[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2914.closure->proto (
                        &ceu_frame_2914,
                        1,
                        ceu_args_2914
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 172, col 8) : call error");
                } // CALL
                ceu_args_2920[0] = ceu_acc;
ceu_acc = id_DAYS_PER_YEAR;
ceu_args_2920[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2920.closure->proto (
                        &ceu_frame_2920,
                        2,
                        ceu_args_2920
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 172, col 34) : call error");
                } // CALL
                ceu_args_2931[5] = ceu_acc;

                { // CALL | 
                    ceu_acc = op_asterisk;

                    CEU_Value ceu_closure_2929 = ceu_acc;
                    if (ceu_closure_2929.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 173, col 34)", err);
                    }
                    CEU_Frame ceu_frame_2929 = { &ceu_closure_2929.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_2929[2];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.0000515138902046611451} });ceu_args_2929[0] = ceu_acc;
ceu_acc = id_SOLAR_MASS;
ceu_args_2929[1] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2929.closure->proto (
                        &ceu_frame_2929,
                        2,
                        ceu_args_2929
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 173, col 34) : call error");
                } // CALL
                ceu_args_2931[6] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2931.closure->proto (
                        &ceu_frame_2931,
                        7,
                        ceu_args_2931
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 166, col 5) : call error");
                } // CALL
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_vector_set(&ceu_vec_2933.Dyn->Vector, 4, ceu_acc),
                            "nbody.ceu : (lin 130, col 14)"
                        );
                        
                    ceu_acc = ceu_vec_2933;
                }
                
                        ceu_assert_pre(
                            ceu_block_3004,
                            ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, ceu_acc, 0, "declaration error"),
                            "nbody.ceu : (lin 130, col 1)"
                        );
                    
                
                    id_bodies = ceu_acc;
                    ceu_gc_inc(id_bodies);
                    ceu_acc = id_bodies;
                    
                
                // DCL | 
                
                
                    
                    ceu_gc_inc(id_main);
                    ceu_acc = id_main;
                    
                
                { // SET | 
                     // TYPE | 
                    
                 // PROTO | 
                    CEU_Value ceu_proto_2994 (
                        CEU_Frame* ceu_frame,
                        int ceu_n,
                        CEU_Value ceu_args[]
                    ) {
                        CEU_Value ceu_acc;        
                        
                        
                            CEU_Value id_N;
                            CEU_Block* _id_N_;
                            
                        
                    { // BLOCK | 
                        CEU_Block _ceu_block_2993 = (CEU_Block) { ceu_frame->up_block->depth + 1, 1, {.frame=ceu_frame}, NULL };
                        CEU_Block* ceu_block_2993 = &_ceu_block_2993; 
                        
                        
                            { // func args
                                ceu_gc_inc_args(ceu_n, ceu_args);
                                
                                    if (0 < ceu_n) {
                                        ceu_assert_pre(
                                            ceu_block_2993,
                                            ceu_hold_chk_set(&ceu_block_2993->dyns, ceu_block_2993->depth, CEU_HOLD_FLEET, ceu_args[0], 1, "argument error"),
                                            "nbody.ceu : (lin 177, col 21)"
                                        );
                                        id_N = ceu_args[0];
                                    } else {
                                        id_N = (CEU_Value) { CEU_VALUE_NIL };
                                    }
                                    
                                
                            }
                            
                        
                        
                        
                        // >>> block
                        
                { // CALL | 
                    ceu_acc = id_offset_momentum;

                    CEU_Value ceu_closure_2954 = ceu_acc;
                    if (ceu_closure_2954.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2993, "nbody.ceu : (lin 178, col 5)", err);
                    }
                    CEU_Frame ceu_frame_2954 = { &ceu_closure_2954.Dyn->Closure, ceu_block_2993 };
                    
                    CEU_Value ceu_args_2954[1];
                    
                    ceu_acc = id_bodies;
ceu_args_2954[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2954.closure->proto (
                        &ceu_frame_2954,
                        1,
                        ceu_args_2954
                    );
                    ceu_assert_pre(ceu_block_2993, ceu_acc, "nbody.ceu : (lin 178, col 5) : call error");
                } // CALL
                
                { // CALL | 
                    ceu_acc = id_println;

                    CEU_Value ceu_closure_2966 = ceu_acc;
                    if (ceu_closure_2966.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2993, "nbody.ceu : (lin 179, col 5)", err);
                    }
                    CEU_Frame ceu_frame_2966 = { &ceu_closure_2966.Dyn->Closure, ceu_block_2993 };
                    
                    CEU_Value ceu_args_2966[1];
                    
                    
                { // CALL | 
                    ceu_acc = id_energy;

                    CEU_Value ceu_closure_2964 = ceu_acc;
                    if (ceu_closure_2964.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2993, "nbody.ceu : (lin 179, col 13)", err);
                    }
                    CEU_Frame ceu_frame_2964 = { &ceu_closure_2964.Dyn->Closure, ceu_block_2993 };
                    
                    CEU_Value ceu_args_2964[1];
                    
                    ceu_acc = id_bodies;
ceu_args_2964[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2964.closure->proto (
                        &ceu_frame_2964,
                        1,
                        ceu_args_2964
                    );
                    ceu_assert_pre(ceu_block_2993, ceu_acc, "nbody.ceu : (lin 179, col 13) : call error");
                } // CALL
                ceu_args_2966[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2966.closure->proto (
                        &ceu_frame_2966,
                        1,
                        ceu_args_2966
                    );
                    ceu_assert_pre(ceu_block_2993, ceu_acc, "nbody.ceu : (lin 179, col 5) : call error");
                } // CALL
                
                { // CALL | 
                    ceu_acc = id_advance_multiple_steps;

                    CEU_Value ceu_closure_2979 = ceu_acc;
                    if (ceu_closure_2979.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2993, "nbody.ceu : (lin 180, col 5)", err);
                    }
                    CEU_Frame ceu_frame_2979 = { &ceu_closure_2979.Dyn->Closure, ceu_block_2993 };
                    
                    CEU_Value ceu_args_2979[3];
                    
                    ceu_acc = id_N;
ceu_args_2979[0] = ceu_acc;
ceu_acc = id_bodies;
ceu_args_2979[1] = ceu_acc;
ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=0.01} });ceu_args_2979[2] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2979.closure->proto (
                        &ceu_frame_2979,
                        3,
                        ceu_args_2979
                    );
                    ceu_assert_pre(ceu_block_2993, ceu_acc, "nbody.ceu : (lin 180, col 5) : call error");
                } // CALL
                
                { // CALL | 
                    ceu_acc = id_println;

                    CEU_Value ceu_closure_2991 = ceu_acc;
                    if (ceu_closure_2991.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2993, "nbody.ceu : (lin 181, col 5)", err);
                    }
                    CEU_Frame ceu_frame_2991 = { &ceu_closure_2991.Dyn->Closure, ceu_block_2993 };
                    
                    CEU_Value ceu_args_2991[1];
                    
                    
                { // CALL | 
                    ceu_acc = id_energy;

                    CEU_Value ceu_closure_2989 = ceu_acc;
                    if (ceu_closure_2989.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_2993, "nbody.ceu : (lin 181, col 13)", err);
                    }
                    CEU_Frame ceu_frame_2989 = { &ceu_closure_2989.Dyn->Closure, ceu_block_2993 };
                    
                    CEU_Value ceu_args_2989[1];
                    
                    ceu_acc = id_bodies;
ceu_args_2989[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2989.closure->proto (
                        &ceu_frame_2989,
                        1,
                        ceu_args_2989
                    );
                    ceu_assert_pre(ceu_block_2993, ceu_acc, "nbody.ceu : (lin 181, col 13) : call error");
                } // CALL
                ceu_args_2991[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_2991.closure->proto (
                        &ceu_frame_2991,
                        1,
                        ceu_args_2991
                    );
                    ceu_assert_pre(ceu_block_2993, ceu_acc, "nbody.ceu : (lin 181, col 5) : call error");
                } // CALL
                
                        // <<< block
                        
                        
                            // move up dynamic ceu_acc (return or error)
                            ceu_assert_pre(
                                ceu_block_2993, 
                                ceu_hold_chk_set(&ceu_frame->up_block->dyns, ceu_frame->up_block->depth, CEU_HOLD_FLEET, ceu_acc, 0, "block escape error"),
                                "nbody.ceu : (lin 177, col 21)"
                            );
                            
                        
                        
                            
                                if (id_N.type > CEU_VALUE_DYNAMIC) {
                                    ceu_gc_dec(id_N, !(ceu_acc.type>CEU_VALUE_DYNAMIC && ceu_acc.Dyn==id_N.Dyn));
                                }
                                
                        
                        ceu_block_free(ceu_block_2993);
                    }
                    
                        return ceu_acc;
                    }
                 // CLOSURE | 
                CEU_Value ceu_ret_2994 = ceu_closure_create (
                    ceu_block_3004,
                    CEU_HOLD_FLEET,
                    NULL,
                    ceu_proto_2994,
                    0
                );
                ceu_acc = ceu_ret_2994;
                
                // UPVALS
                
                
                    CEU_Value ceu_set_2995 = ceu_acc;
                    
                        { // ACC - SET
                            ceu_assert_pre(
                                ceu_block_3004,
                                ceu_hold_chk_set(&ceu_block_3004->dyns, ceu_block_3004->depth, CEU_HOLD_MUTAB, (ceu_set_2995), 0, "set error"),
                                "nbody.ceu : (lin 177, col 5)"
                            );
                            ceu_gc_inc((ceu_set_2995));
                            ceu_gc_dec(id_main, 1);
                            id_main = (ceu_set_2995);
                        }
                        
                    ceu_acc = ceu_set_2995;
                }
                
                { // CALL | 
                    ceu_acc = id_main;

                    CEU_Value ceu_closure_3002 = ceu_acc;
                    if (ceu_closure_3002.type != CEU_VALUE_CLOSURE) {
                        CEU_Value err = { CEU_VALUE_ERROR, {.Error="call error : expected function"} };
                        ceu_ferror_pre(ceu_block_3004, "nbody.ceu : (lin 184, col 1)", err);
                    }
                    CEU_Frame ceu_frame_3002 = { &ceu_closure_3002.Dyn->Closure, ceu_block_3004 };
                    
                    CEU_Value ceu_args_3002[1];
                    
                    ceu_acc = ((CEU_Value) { CEU_VALUE_NUMBER, {.Number=50000000} });ceu_args_3002[0] = ceu_acc;

                    
                    

                    ceu_acc = ceu_frame_3002.closure->proto (
                        &ceu_frame_3002,
                        1,
                        ceu_args_3002
                    );
                    ceu_assert_pre(ceu_block_3004, ceu_acc, "nbody.ceu : (lin 184, col 1) : call error");
                } // CALL
                
                        // <<< block
                        
                        
                        
                            if (op_ampersand_ampersand.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_ampersand_ampersand, (op_ampersand_ampersand.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (op_bar_bar.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_bar_bar, (op_bar_bar.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (op_plus.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_plus, (op_plus.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (op_minus.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_minus, (op_minus.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (op_asterisk.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_asterisk, (op_asterisk.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (op_asterisk_asterisk.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_asterisk_asterisk, (op_asterisk_asterisk.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (op_slash.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_slash, (op_slash.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (op_slash_slash.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_slash_slash, (op_slash_slash.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (op_null.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_null, (op_null.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (op_greater_equals.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_greater_equals, (op_greater_equals.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (op_greater.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_greater, (op_greater.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (op_less_equals.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_less_equals, (op_less_equals.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (op_less.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(op_less, (op_less.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (id_to_dash_string.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_to_dash_string, (id_to_dash_string.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (id_to_dash_number.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_to_dash_number, (id_to_dash_number.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (id_to_dash_tag.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_to_dash_tag, (id_to_dash_tag.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (id_new_body.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_new_body, (id_new_body.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (id_offset_momentum.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_offset_momentum, (id_offset_momentum.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (id_advance.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_advance, (id_advance.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (id_advance_multiple_steps.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_advance_multiple_steps, (id_advance_multiple_steps.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (id_energy.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_energy, (id_energy.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (id_PI.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_PI, (id_PI.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (id_SOLAR_MASS.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_SOLAR_MASS, (id_SOLAR_MASS.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (id_DAYS_PER_YEAR.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_DAYS_PER_YEAR, (id_DAYS_PER_YEAR.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (id_bodies.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_bodies, (id_bodies.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                            if (id_main.type > CEU_VALUE_DYNAMIC) {
                                ceu_gc_dec(id_main, (id_main.Dyn->Any.hld_depth == ceu_block_3004->depth));
                            }
                        
                        
                        ceu_block_free(ceu_block_3004);
                    }
                    
            return 0;
        }
    