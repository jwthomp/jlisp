#include "value.h"
#include "value_helpers.h"
#include "vm.h"
#include "vm_exec.h"
#include "assert.h"
#include "err.h"
#include "reader.h"
#include "compile.h"
#include "gc.h"
#include "symbols.h"
#include "lambda.h"

#include "lib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/timeb.h>


typedef struct {
	char const*m_name;
	vm_func_t m_func;
	int m_arg_count;
	bool m_is_macro;
	bool m_dynamic;
} internal_func_def_t;

value_t *print_vm(vm_t *p_vm);
value_t *print_env(vm_t *p_vm);
value_t *print_stack(vm_t *p_vm);
value_t *print_symbols(vm_t *p_vm);
value_t *print_val(vm_t *p_vm);
value_t *print(vm_t *p_vm);
value_t *atom(vm_t *p_vm);
value_t *cons(vm_t *p_vm);
value_t *car(vm_t *p_vm);
value_t *cdr(vm_t *p_vm);
value_t *call(vm_t *p_vm);
value_t *minus(vm_t *p_vm);
value_t *plus(vm_t *p_vm);
value_t *times(vm_t *p_vm);
value_t *less_then(vm_t *p_vm);
value_t *status(vm_t *p_vm);
value_t *eq(vm_t *p_vm);
value_t *load(vm_t *p_vm);
value_t *progn(vm_t *p_vm);
value_t *boundp(vm_t *p_vm);
value_t *fboundp(vm_t *p_vm);
value_t *debug(vm_t *p_vm);
value_t *let(vm_t *p_vm);
value_t *read(vm_t *p_vm);
value_t *eval_lib(vm_t *p_vm);
value_t *gc_lib(vm_t *p_vm);
value_t *spawn_lib(vm_t *p_vm);
value_t *type_of(vm_t *p_vm);
value_t *symbol_name(vm_t *p_vm);
value_t *seq(vm_t *p_vm);
value_t *dbg_time(vm_t *p_vm);

static struct timeb g_time;

static internal_func_def_t g_ifuncs[31] = {
	{"PRINT-VAL", print_val, 0, false, true},
	{"PRINT-VM", print_vm, 0, false, true},
	{"PRINT-ENV", print_env, 0, false, true},
	{"PRINT-SP", print_stack, 0, false, true},
	{"PRINT-SYMBOLS", print_symbols, 0, false, true},
	{"PRINT", print, 1, false, true},
	{"ATOM", atom, 1, false, true},
	{"CONS", cons, 2, false, true},
	{"CAR", car, 1, false, true},
	{"CDR", cdr, 1, false, true},
	{"CALL", call, 2, false, true}, 				// ?
	{"*", times, 2, false, true},
	{"+", plus, 2, false, true},
	{"-", minus, 2, false, true},
	{"<", less_then, 2, false, true},
	{"STATUS", status, 0, false, true},
	{"EQ", eq, 2, false, true},
	{"LOAD", load, 1, false, true},
	{"BOUNDP", boundp, 1, false, true},
	{"FBOUNDP", fboundp, 1, false, true},
	{"DEBUG", debug, 1, false, true},
	{"PROGN", progn, -1, true, true},				// Broken (progn 'a 'b)
	{"LET", let, -1, true, true},					// Broken
	{"READ", read, 0, false, true},
	{"EVAL", eval_lib, 1, false, true},
	{"GC", gc_lib, 0, false, true},
	{"SPAWN", spawn_lib, 0, false, true},
	{"TYPE-OF", type_of, 1, false, true},
	{"SYMBOL-NAME", symbol_name, 1, false, true},
	{"SEQ", seq, 1, false, true},
	{"CLOCK", dbg_time, 0, false, true},
};

#define NUM_IFUNCS 31

value_t *gc_lib(vm_t *p_vm)
{
	gc(p_vm, 1);

	p_vm->m_ip++;
	p_vm->m_stack[p_vm->m_sp++] = nil;

	return t;
}

value_t *spawn_lib(vm_t *p_vm)
{
	value_t * form = p_vm->m_stack[p_vm->m_bp + 1];

	// Create a vm thread
	value_t *vm_val = value_create_process(p_vm, vm_current_vm());
	vm_t *vm = *(vm_t **)vm_val->m_data;

	vm_exec_add_vm(vm_val);
	vm_push_exec_state(vm, form);

	p_vm->m_ip++;
	p_vm->m_stack[p_vm->m_sp++] = t;
	
	return t;
}

value_t *eval_lib(vm_t *p_vm)
{
	// Save VM
	int vm_bp = p_vm->m_bp;
	int vm_sp = p_vm->m_sp;
	int vm_ev = p_vm->m_ev;
	int vm_ip = p_vm->m_ip;
	value_t *vm_current_env = p_vm->m_current_env[p_vm->m_ev - 1];

	if (g_debug_display == true) {
		printf("EVAL Enter: sp: %lu bp: %lu \n", p_vm->m_sp, p_vm->m_bp);
		vm_print_stack(p_vm);
	}

	// Set up a trap
    int i = setjmp(*push_handler_stack());
	value_t *ret = nil;
    if (i == 0) {
		p_vm->m_ip++;
		value_t *form = p_vm->m_stack[p_vm->m_bp + 1];
		ret = eval(p_vm, form);
	} else {
		// Restore stack do to trapped error
		p_vm->m_bp = vm_bp;
		p_vm->m_sp = vm_sp;
		p_vm->m_ev = vm_ev;
		p_vm->m_ip = vm_ip;
		p_vm->m_current_env[p_vm->m_ev - 1] = vm_current_env;

		printf("\t%s\n", g_err);
	}

	pop_handler_stack();

	if (g_debug_display == true) {
		printf("EVAL Exit: bp: %lu\n", p_vm->m_bp);
		vm_print_stack(p_vm);
	}

	return ret;
}

value_t *read(vm_t *p_vm)
{
    char input[256];
    printf("R> ");
    gets(input);
    stream_t *strm = stream_create(input);
    reader(p_vm, strm, false);
    value_t *rd = p_vm->m_stack[p_vm->m_bp + 1];

	p_vm->m_ip++;
	p_vm->m_stack[p_vm->m_sp++] = rd;

    return t;
}


value_t *let(vm_t *p_vm)
{
	// Transform from (let ((var val) (var val)) body) to
	// ((lambda (var var) body) val val)

	value_t *largs = car(p_vm, p_vm->m_stack[p_vm->m_bp + 1]);
	value_t *body = cdr(p_vm, p_vm->m_stack[p_vm->m_bp + 1]);

//printf("let: args "); value_print(p_vm, largs); printf("\n");
//printf("let: body "); value_print(p_vm, body); printf("\n");

	// Break ((var val) (var val)) down into a list of args and vals
	value_t *arg_list = nil;
	value_t *val_list = nil;

	while(car(p_vm, largs) != nil) {
		value_t *arg = car(p_vm, car(p_vm, largs));
		value_t *val = car(p_vm, cdr(p_vm, car(p_vm, largs)));

		arg_list = value_create_cons(p_vm, arg, arg_list);
		val_list = value_create_cons(p_vm, val, val_list);

		largs = cdr(p_vm, largs);
	}
		
	vm_push(p_vm, value_create_symbol(p_vm, "LAMBDA"));
	vm_push(p_vm, arg_list);
	vm_push(p_vm, body);
	vm_cons(p_vm);
	vm_cons(p_vm);
	vm_push(p_vm, val_list);
	vm_cons(p_vm);
	
printf("let end: "); value_print(p_vm, p_vm->m_stack[p_vm->m_sp - 1]); printf("\n");

	p_vm->m_ip++;

	//p_vm->m_stack[p_vm->m_sp++] = p_vm->m_stack[p_vm->m_sp - 1];
	return t;
}


value_t *dbg_time(vm_t *p_vm)
{
	timespec ts;

	struct timeb tp;
	ftime(&tp);

	p_vm->m_ip++;
	p_vm->m_stack[p_vm->m_sp++] = make_fixnum(1000 * (tp.time - g_time.time) + (tp.millitm - g_time.millitm));
	return t;
}

value_t *progn(vm_t *p_vm)
{
//printf("prognC: "); value_print(car(p_vm->m_stack[p_vm->m_bp + 1])); printf("\n");
	vm_push(p_vm, value_create_symbol(p_vm, "FUNCALL"));
	vm_push(p_vm, value_create_symbol(p_vm, "LAMBDA"));
	vm_push(p_vm, nil);
	vm_push(p_vm, car(p_vm, p_vm->m_stack[p_vm->m_bp + 1]));
	vm_cons(p_vm);
	vm_cons(p_vm);
	vm_list(p_vm, 1);
	vm_cons(p_vm);
//printf("end prognC: "); value_print(p_vm->m_stack[p_vm->m_sp - 1]); printf("\n");

	p_vm->m_ip++;

	return t;
}

value_t *symbol_name(vm_t *p_vm)
{
	value_t *p_val = p_vm->m_stack[p_vm->m_bp + 1];
	verify(is_symbol(p_val) == true, "The value %s is not of type SYMBOL", value_sprint(p_vm, p_val));

	p_vm->m_ip++;
	p_vm->m_stack[p_vm->m_sp++] = p_val->m_cons[0];

	return t;
}

value_t *type_of(vm_t *p_vm)
{
	value_t *p_val = p_vm->m_stack[p_vm->m_bp + 1];

	if (is_fixnum(p_val) == true) {
		p_vm->m_stack[p_vm->m_sp++] = value_create_string(p_vm, "FIXNUM");
		return value_create_string(p_vm, "FIXNUM");
	}

	p_vm->m_ip++;
	p_vm->m_stack[p_vm->m_sp++] = value_create_string(p_vm, valuetype_print(p_val->m_type));

	return t;
}

value_t *seq(vm_t *p_vm)
{
	value_t *arg = p_vm->m_stack[p_vm->m_bp + 1];
	verify(is_fixnum(arg), "Argument not a fixnum");

	int i = to_fixnum(arg);

	value_t *ret = value_create_cons(p_vm, make_fixnum(i--), nil);
	value_t *ret2 = ret;
	
	while(i) {
		ret2->m_cons[1] = value_create_cons(p_vm, make_fixnum(i--), nil);
		ret2 = ret2->m_cons[1];
	}

	p_vm->m_stack[p_vm->m_sp++] = ret;
	p_vm->m_ip++;

	return t;
}



value_t *print_vm(vm_t *p_vm)
{
	printf("sp: %lu ev: %lu\n", p_vm->m_sp, p_vm->m_ev);
	p_vm->m_stack[p_vm->m_sp++] = t; 
	p_vm->m_ip++;
	return t;
}

value_t *print_env(vm_t *p_vm)
{
	vm_print_env(p_vm);
	p_vm->m_stack[p_vm->m_sp++] = t;
	p_vm->m_ip++;
	return t;
}

value_t *print_stack(vm_t *p_vm)
{
	vm_print_stack(p_vm);
	p_vm->m_stack[p_vm->m_sp++] = t;
	p_vm->m_ip++;
	return t;
}

value_t *print_symbols(vm_t *p_vm)
{
	vm_print_symbols(p_vm);
	p_vm->m_stack[p_vm->m_sp++] = t;
	p_vm->m_ip++;
	return t;
}

value_t *symbol_value(vm_t *p_vm)
{
	value_t *arg = p_vm->m_stack[p_vm->m_bp + 1];

	if (arg->m_cons[1] != voidobj) {
		return car(p_vm, arg->m_cons[1]);
	}

	// Find and return the value of this symbol
	value_t * ret = environment_binding_find(p_vm, arg, false);
	p_vm->m_ip++;
	if (ret == nil) {
		p_vm->m_stack[p_vm->m_sp++] = ret;
		return ret;
	} else {
		p_vm->m_stack[p_vm->m_sp++] = binding_get_value(ret);
		return t;
	}
}

value_t *boundp(vm_t *p_vm)
{
	value_t *arg = p_vm->m_stack[p_vm->m_bp + 1];

	if (arg->m_cons[1] != voidobj) {
		return car(p_vm, arg->m_cons[1]);
	}

	// Find and return the value of this symbol
	value_t *ret =  environment_binding_find(p_vm, arg, false);
	p_vm->m_ip++;
	if (ret == nil) {
		p_vm->m_stack[p_vm->m_sp++] = ret;
		return ret;
	} else {
		p_vm->m_stack[p_vm->m_sp++] = binding_get_value(ret);
		return t;
	}
}

value_t *fboundp(vm_t *p_vm)
{
	value_t *arg = p_vm->m_stack[p_vm->m_bp + 1];

	if (arg->m_cons[2] != voidobj) {
		return car(p_vm, arg->m_cons[2]);
	}

	// Find and return the value of this symbol
	value_t *ret =  environment_binding_find(p_vm, arg, true);
	p_vm->m_ip++;
	if (ret == nil) {
		p_vm->m_stack[p_vm->m_sp++] = ret;
		return ret;
	} else {
		p_vm->m_stack[p_vm->m_sp++] = binding_get_value(ret);
		return t;
	}
}

value_t *atom(vm_t *p_vm)
{
	value_t *arg = p_vm->m_stack[p_vm->m_bp + 1];
	if (is_cons(arg) == true) {
		p_vm->m_stack[p_vm->m_sp++] = nil;
		p_vm->m_ip++;
		return nil;
	} else {
		//p_vm->m_stack[p_vm->m_sp++] = t;
		p_vm->m_ip++;
		return t;
	}
}


value_t *debug(vm_t *p_vm)
{
	value_t *args = p_vm->m_stack[p_vm->m_bp + 1];

	value_print(p_vm, p_vm->m_stack[p_vm->m_bp + 1]);

	if (is_fixnum(args)) {
		g_step_debug = true;
		p_vm->m_ip++;
		return t;
	} else if (args != nil) {
		g_debug_display = true;
		p_vm->m_stack[p_vm->m_sp++] = t;
		p_vm->m_ip++;
		return t;
	} else {
		g_debug_display = false;
		p_vm->m_stack[p_vm->m_sp++] = nil;
		p_vm->m_ip++;
		return nil;
	}
}




value_t *status(vm_t *p_vm)
{
	// Display stack
	printf("sp: %lu\n", p_vm->m_sp);

	// Display memory
printf("\n-------static Heap--------\n");
	value_t *heap = g_static_heap;
	int count = 0;
	unsigned long mem = 0;
	while(heap) {
		count++;
		if (is_fixnum(heap)) {
			mem += sizeof(heap);
			printf("%d] %ud ", count, sizeof(heap));
		} else {
		mem += sizeof(value_t) + heap->m_size;
		printf("%d - %p] %lu ", count, heap, sizeof(value_t) + heap->m_size);
		}
		value_print(p_vm, heap);
		printf("\n");
		heap = heap->m_heapptr;
	}

printf("\n-------g0 Heap--------\n");
	heap = p_vm->m_heap_g0;
	count = 0;
	mem = 0;
	while(heap) {
		count++;
		if (is_string(heap)) {
			heap = heap->m_heapptr;
			continue;
		}

		if (is_fixnum(heap)) {
			mem += sizeof(heap);
			printf("%d] %ud ", count, sizeof(heap));
		} else {
		mem += sizeof(value_t) + heap->m_size;
		printf("%d - %p] %lu ", count, heap, sizeof(value_t) + heap->m_size);
		}
		value_print(p_vm, heap);
		printf("\n");
		heap = heap->m_heapptr;
	}

printf("\n-------g1 Heap--------\n");
	heap = p_vm->m_heap_g1;
	count = 0;
	mem = 0;
	while(heap) {
		count++;
		if (is_fixnum(heap)) {
			mem += sizeof(heap);
			printf("%d] %ud ", count, sizeof(heap));
		} else {
		mem += sizeof(value_t) + heap->m_size;
		printf("%d - %p] %lu ", count, heap, sizeof(value_t) + heap->m_size);
		}
		value_print(p_vm, heap);
		printf("\n");
		heap = heap->m_heapptr;
	}

#if 0
	printf("\n\n-----FREE HEAP----\n\n");

	heap = p_vm->m_free_heap;
	while(heap) {
		printf("%d] %lu ", count, sizeof(value_t) + heap->m_size);
		value_print(heap);
		printf("\n");
		heap = heap->m_heapptr;
	}
#endif


	printf("Live Objects: %d\n", count);
	printf("Memory used: %lu\n", mem);

	p_vm->m_stack[p_vm->m_sp++] = t;
	p_vm->m_ip++;
	
	return t;
}


value_t *load(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	assert(is_string(first));

	struct stat st;
	if (stat(first->m_data, &st) != 0) {
		return nil;
	}

	FILE *fp = fopen(first->m_data, "r");
	char *input = (char *)malloc(st.st_size + 1);
	memset(input, 0, st.st_size + 1);

	unsigned long file_size = (unsigned long)st.st_size;

	fread(input, file_size, 1, fp);
	fclose(fp);

	int start_sp = p_vm->m_sp;

	stream_t *strm = stream_create(input);
	int args = reader(p_vm, strm, false);

//printf("start sp: %d sp: %lu args: %d\n", start_sp, p_vm->m_sp, args);
//vm_print_stack(p_vm);

	p_vm->m_ip++;

	int i = 0;
    while (i < args) {
//	for (int i = 1; i <= args; i++) {
//		p_vm->m_sp--;
        value_t *form = p_vm->m_stack[start_sp + i];

printf("form: "); value_print(p_vm, form); printf("\n");

//        value_t *lambda = compile(p_vm, nil, list(p_vm, form));
 //       value_t *closure = make_closure(p_vm, lambda);
  //      vm_push_exec_state(p_vm, closure);
			eval(p_vm, form);
			vm_exec(p_vm, p_vm->m_exp - 1, false);
		i++;
    }

	

	free(input);

	return t;
}


value_t *call(vm_t *p_vm)
{
	value_t *closure = p_vm->m_stack[p_vm->m_bp + 1];
	int nargs = p_vm->m_sp - p_vm->m_bp - 2;

printf("exec: args: %d", nargs); value_print(p_vm, closure); printf("\n");


	p_vm->m_ip++;
	vm_push_exec_state(p_vm, closure);
	p_vm->m_ip = 0;
	p_vm->m_bp = p_vm->m_bp + 1;


	return t;
}


value_t *cons(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	p_vm->m_ip++;
	p_vm->m_stack[p_vm->m_sp++] = value_create_cons(p_vm, first, second);
	return t;
}

value_t *car(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];

	p_vm->m_ip++;
	if(is_null(first)) {
		p_vm->m_stack[p_vm->m_sp++] = first;
		return t;
	}

verify(is_cons(first) == true, "car'ing something not a cons %d", first->m_type);

//printf("f: "); value_print(p_vm, first->m_cons[0]); printf("\n");

	p_vm->m_stack[p_vm->m_sp++] = first->m_cons[0];
	return t;
}

value_t *cdr(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];

	p_vm->m_ip++;
	if(is_null(first)) {
		p_vm->m_stack[p_vm->m_sp++] = first;
		return first;
	}
	assert(is_cons(first));

	p_vm->m_stack[p_vm->m_sp++] = first->m_cons[1];
	return t;
}

value_t *eq(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	bool first_num = is_fixnum(first);
	bool second_num = is_fixnum(second);

	p_vm->m_ip++;

	if (first_num != second_num) {
		p_vm->m_stack[p_vm->m_sp++] = nil;
		return nil;
	}

	if (first_num == true && second_num == true) {
		p_vm->m_stack[p_vm->m_sp++] = to_fixnum(first) == to_fixnum(second) ? t : nil;
		return t;
	}


	if (first->m_type != second->m_type) {
		p_vm->m_stack[p_vm->m_sp++] = nil;
		return nil;
	}

	verify(first->m_type == second->m_type, "eq: Types are not the same %d != %d\n",
		first->m_type, second->m_type);

	switch (first->m_type) {
		case VT_NUMBER:
		case VT_SYMBOL:
		case VT_STRING:
			if (value_equal(first, second) == true) {
				p_vm->m_stack[p_vm->m_sp++] = t;
				return t;
			} else {
				p_vm->m_stack[p_vm->m_sp++] = nil;
				return nil;
			}

		default:
			assert(!"Trying to test unsupported type with eq");
			break;
	}

	p_vm->m_stack[p_vm->m_sp++] = nil;
	return nil;
}

value_t *less_then(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	verify(is_fixnum(first), "argument X is not a number: %s", value_sprint(p_vm, first));
	verify(is_fixnum(second), "argument Y is not a number: %s", value_sprint(p_vm, second));

	p_vm->m_ip++;

	if (to_fixnum(first) < to_fixnum(second)) {
		p_vm->m_stack[p_vm->m_sp++] = t;
		return t;
	} else {
		p_vm->m_stack[p_vm->m_sp++] = nil;
		return nil;
	}
}

value_t *minus(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	verify(is_fixnum(first), "argument X is not a number: %s", value_sprint(p_vm, first));
	verify(is_fixnum(second), "argument Y is not a number: %s", value_sprint(p_vm, second));

	p_vm->m_ip++;
	p_vm->m_stack[p_vm->m_sp++] = make_fixnum(to_fixnum(first) - to_fixnum(second));
	return t;
}

value_t *times(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	verify(is_fixnum(first), "argument X is not a number: %s", value_sprint(p_vm, first));
	verify(is_fixnum(second), "argument Y is not a number: %s", value_sprint(p_vm, second));

	p_vm->m_stack[p_vm->m_sp++] = make_fixnum(to_fixnum(first) * to_fixnum(second));
	p_vm->m_ip++;

	return t;
}
value_t *plus(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	verify(is_fixnum(first), "argument X is not a number: %s", value_sprint(p_vm, first));
	verify(is_fixnum(second), "argument Y is not a number: %s", value_sprint(p_vm, second));

	p_vm->m_stack[p_vm->m_sp++] = make_fixnum(to_fixnum(first) + to_fixnum(second));

	p_vm->m_ip++;

	return t;
}

value_t *print_val(vm_t *p_vm)
{
	value_t *arg = p_vm->m_stack[p_vm->m_bp + 1];

	value_t *p = NULL;
	if (is_symbol_function_dynamic(arg)) {
		p  = car(p_vm, arg->m_cons[2]);
	} else {
		p = environment_binding_find(p_vm, arg, true); 
	}

 	if (is_closure(p)) {
		lambda_t *l = (lambda_t *)p->m_cons[1]->m_data;
		value_t *pool = l->m_pool;

		printf("---- Closure ----\n");
		printf("Pool: \n");
		int pool_size = pool->m_size / sizeof(value_t *);
		for (int i = 0; i < pool_size; i++) {
			printf("%d] ", i);
			value_print(p_vm, ((value_t **)pool->m_data)[i]);
			printf("\n");
		}

		printf("\nBytecode: \n");
		bytecode_t *code = (bytecode_t *)l->m_bytecode->m_data;
		for (unsigned long i = 0; i < (l->m_bytecode->m_size / sizeof(bytecode_t)); i++) {
			printf("op: %s code: %lu\n", g_opcode_print[code[i].m_opcode], code[i].m_value);
		}
		printf("-----------------\n");
	} else {
		value_print(p_vm, p); printf("\n");
	}

	p_vm->m_stack[p_vm->m_sp++] = p;
//	p_vm->m_stack[p_vm->m_sp++] = last->m_cons[0];
	p_vm->m_ip++;
	return t;
}

value_t *print(vm_t *p_vm)
{
//	printf("PRINT!!!!!! sp: %lu bp: %lu>> \n", p_vm->m_sp, p_vm->m_bp);
//	vm_print_stack(p_vm);
//	printf("<<\n");

	value_t *arg = p_vm->m_stack[p_vm->m_bp + 1];

	printf("PRINT: ");value_print(p_vm, arg); printf("\n");

	p_vm->m_stack[p_vm->m_sp++] = arg;

//	p_vm->m_stack[p_vm->m_sp++] = last->m_cons[0];
	p_vm->m_ip++;

	return t;
}




void lib_init(vm_t *p_vm)
{
	int i;

	for (i = 0; i < NUM_IFUNCS; i++) {
		vm_bindf(p_vm, g_ifuncs[i].m_name, g_ifuncs[i].m_func, g_ifuncs[i].m_arg_count, g_ifuncs[i].m_is_macro, g_ifuncs[i].m_dynamic);
	}

	ftime(&g_time);
	
	if (nil == NULL) {
		nil = value_create_symbol(p_vm, "NIL");
	}
	bind_internal(p_vm, nil, nil, false, true);

	if (t == NULL) {
		t = value_create_symbol(p_vm, "T");
	}
	vm_bind(p_vm, "T", t, true);

}
