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

static internal_func_def_t g_ifuncs[] = {
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

#define NUM_IFUNCS 29

value_t *gc_lib(vm_t *p_vm)
{
	gc(p_vm, 1);

	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_stack[p_vm->m_bp] = p_vm->nil;
	p_vm->m_ip++;

	return p_vm->nil;
}

value_t *spawn_lib(vm_t *p_vm)
{
	// To spawn a process, we have to create a new vm
	// Then copy the closure over to it and call it
	// Return a pid
	return NULL;
}

value_t *eval_lib(vm_t *p_vm)
{
	// Save VM
	int vm_bp = p_vm->m_bp;
	int vm_sp = p_vm->m_sp;
	int vm_ev = p_vm->m_ev;
	int vm_ip = p_vm->m_ip;
	value_t *vm_current_env = p_vm->m_current_env[p_vm->m_ev - 1];

	// Set up a trap
    int i = setjmp(*push_handler_stack());
	value_t *ret = p_vm->nil;
    if (i == 0) {
		ret = eval(p_vm, p_vm->m_stack[p_vm->m_bp + 1]);
		p_vm->m_sp = p_vm->m_bp + 1;
		p_vm->m_stack[p_vm->m_bp] = ret;
		p_vm->m_ip++;
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

	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_stack[p_vm->m_bp] = rd;
	p_vm->m_ip++;

    return rd;
}


value_t *let(vm_t *p_vm)
{
	// Transform from (let ((var val) (var val)) body) to
	// ((lambda (var var) body) val val)

	value_t *largs = car(p_vm, p_vm->m_stack[p_vm->m_bp + 1]);
	value_t *body = cdr(p_vm, p_vm->m_stack[p_vm->m_bp + 1]);

printf("let: args "); value_print(p_vm, largs); printf("\n");
printf("let: body "); value_print(p_vm, body); printf("\n");

	// Break ((var val) (var val)) down into a list of args and vals
	value_t *arg_list = p_vm->nil;
	value_t *val_list = p_vm->nil;

	while(car(p_vm, largs) != p_vm->nil) {
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

	p_vm->m_stack[p_vm->m_bp] = p_vm->m_stack[p_vm->m_sp - 1];
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;

	return p_vm->m_stack[p_vm->m_sp - 1];
}


value_t *dbg_time(vm_t *p_vm)
{
	timespec ts;

	struct timeb tp;
	ftime(&tp);

	p_vm->m_stack[p_vm->m_bp] = make_fixnum(1000 * (tp.time - g_time.time) + (tp.millitm - g_time.millitm));
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;


	return make_fixnum(1000 * (tp.time - g_time.time) + (tp.millitm - g_time.millitm));
}

value_t *progn(vm_t *p_vm)
{
//printf("prognC: "); value_print(car(p_vm->m_stack[p_vm->m_bp + 1])); printf("\n");
	vm_push(p_vm, value_create_symbol(p_vm, "FUNCALL"));
	vm_push(p_vm, value_create_symbol(p_vm, "LAMBDA"));
	vm_push(p_vm, p_vm->nil);
	vm_push(p_vm, car(p_vm, p_vm->m_stack[p_vm->m_bp + 1]));
	vm_cons(p_vm);
	vm_cons(p_vm);
	vm_list(p_vm, 1);
	vm_cons(p_vm);
//printf("end prognC: "); value_print(p_vm->m_stack[p_vm->m_sp - 1]); printf("\n");

	p_vm->m_stack[p_vm->m_bp] = p_vm->m_stack[p_vm->m_sp - 1];
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;

	return p_vm->m_stack[p_vm->m_sp - 1];
}

value_t *symbol_name(vm_t *p_vm)
{
	value_t *p_val = p_vm->m_stack[p_vm->m_bp + 1];
	verify(is_symbol(p_vm, p_val) == true, "The value %s is not of type SYMBOL", value_sprint(p_vm, p_val));

	p_vm->m_stack[p_vm->m_bp] = p_val->m_cons[0];
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;

	return p_val->m_cons[0];
}

value_t *type_of(vm_t *p_vm)
{
	value_t *p_val = p_vm->m_stack[p_vm->m_bp + 1];

	if (is_fixnum(p_val) == true) {
		return value_create_string(p_vm, "FIXNUM");
	}

	p_vm->m_stack[p_vm->m_bp] = value_create_string(p_vm, valuetype_print(p_val->m_type));
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;

	return p_vm->m_stack[p_vm->m_bp];
}

value_t *seq(vm_t *p_vm)
{
	value_t *arg = p_vm->m_stack[p_vm->m_bp + 1];
	verify(is_fixnum(arg), "Argument not a fixnum");

	int i = to_fixnum(arg);

	value_t *ret = value_create_cons(p_vm, make_fixnum(i--), p_vm->nil);
	value_t *ret2 = ret;
	
	while(i) {
		ret2->m_cons[1] = value_create_cons(p_vm, make_fixnum(i--), p_vm->nil);
		ret2 = ret2->m_cons[1];
	}

	p_vm->m_stack[p_vm->m_bp] = ret;
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;

	return ret;
}



value_t *print_vm(vm_t *p_vm)
{
	printf("sp: %lu ev: %lu\n", p_vm->m_sp, p_vm->m_ev);
	p_vm->m_stack[p_vm->m_bp] = p_vm->nil;
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;
	return p_vm->nil;
}

value_t *print_env(vm_t *p_vm)
{
	vm_print_env(p_vm);
	p_vm->m_stack[p_vm->m_bp] = p_vm->nil;
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;
	return p_vm->nil;
}

value_t *print_stack(vm_t *p_vm)
{
	vm_print_stack(p_vm);
	p_vm->m_stack[p_vm->m_bp] = p_vm->nil;
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;
	return p_vm->nil;
}

value_t *print_symbols(vm_t *p_vm)
{
	vm_print_symbols(p_vm);
	p_vm->m_stack[p_vm->m_bp] = p_vm->nil;
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;
	return p_vm->nil;
}

value_t *symbol_value(vm_t *p_vm)
{
	value_t *arg = p_vm->m_stack[p_vm->m_bp + 1];

	if (arg->m_cons[1] != p_vm->voidobj) {
		return car(p_vm, arg->m_cons[1]);
	}

	// Find and return the value of this symbol
	value_t * ret = environment_binding_find(p_vm, arg, false);
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;
	if (ret == p_vm->nil) {
		p_vm->m_stack[p_vm->m_bp] = ret;
		return ret;
	} else {
		p_vm->m_stack[p_vm->m_bp] = binding_get_value(ret);
		return binding_get_value(ret);
	}
}

value_t *boundp(vm_t *p_vm)
{
	value_t *arg = p_vm->m_stack[p_vm->m_bp + 1];

	if (arg->m_cons[1] != p_vm->voidobj) {
		return car(p_vm, arg->m_cons[1]);
	}

	// Find and return the value of this symbol
	value_t *ret =  environment_binding_find(p_vm, arg, false);
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;
	if (ret == p_vm->nil) {
		p_vm->m_stack[p_vm->m_bp] = ret;
		return ret;
	} else {
		p_vm->m_stack[p_vm->m_bp] = binding_get_value(ret);
		return binding_get_value(ret);
	}
}

value_t *fboundp(vm_t *p_vm)
{
	value_t *arg = p_vm->m_stack[p_vm->m_bp + 1];

	if (arg->m_cons[2] != p_vm->voidobj) {
		return car(p_vm, arg->m_cons[2]);
	}

	// Find and return the value of this symbol
	value_t *ret =  environment_binding_find(p_vm, arg, true);
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;
	if (ret == p_vm->nil) {
		p_vm->m_stack[p_vm->m_bp] = ret;
		return ret;
	} else {
		p_vm->m_stack[p_vm->m_bp] = binding_get_value(ret);
		return binding_get_value(ret);
	}
}

value_t *atom(vm_t *p_vm)
{
	value_t *arg = p_vm->m_stack[p_vm->m_bp + 1];
	if (is_cons(p_vm, arg) == true) {
		p_vm->m_sp = p_vm->m_bp + 1;
		p_vm->m_stack[p_vm->m_bp] = p_vm->nil;
		p_vm->m_ip++;
		return p_vm->nil;
	} else {
		p_vm->m_sp = p_vm->m_bp + 1;
		p_vm->m_stack[p_vm->m_bp] = p_vm->t;
		p_vm->m_ip++;
		return p_vm->t;
	}
}


value_t *debug(vm_t *p_vm)
{
	value_t *args = p_vm->m_stack[p_vm->m_bp + 1];

	value_print(p_vm, p_vm->m_stack[p_vm->m_bp + 1]);

	if (args != p_vm->nil) {
		g_debug_display = true;
		p_vm->m_sp = p_vm->m_bp + 1;
		p_vm->m_stack[p_vm->m_bp] = p_vm->t;
		p_vm->m_ip++;
		return p_vm->t;
	} else {
		g_debug_display = false;
		p_vm->m_sp = p_vm->m_bp + 1;
		p_vm->m_stack[p_vm->m_bp] = p_vm->nil;
		p_vm->m_ip++;
		return p_vm->nil;
	}
}




value_t *status(vm_t *p_vm)
{
	// Display stack
	printf("sp: %lu\n", p_vm->m_sp);

	// Display memory
printf("\n-------static Heap--------\n");
	value_t *heap = p_vm->m_static_heap;
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
		if (is_string(p_vm, heap)) {
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

	p_vm->m_stack[p_vm->m_bp] = p_vm->nil;
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;
	
	return p_vm->nil;
}


value_t *load(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	assert(is_string(p_vm, first));

	struct stat st;
	if (stat(first->m_data, &st) != 0) {
		return p_vm->nil;
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

printf("start sp: %d sp: %lu args: %d\n", start_sp, p_vm->m_sp, args);
vm_print_stack(p_vm);

	p_vm->m_ip++;
	//p_vm->m_stack[p_vm->m_bp] = p_vm->t;
	//p_vm->m_sp = p_vm->m_bp + 1;

	int i = 1;
    while (i <= args) {
        value_t *form = p_vm->m_stack[p_vm->m_sp - i];

printf("form: "); value_print(p_vm, form); printf("\n");

        value_t *lambda = compile(p_vm, p_vm->nil, list(p_vm, form));
        value_t *closure = make_closure(p_vm, lambda);
        vm_push_exec_state(p_vm, closure);
		i++;
    }

	

	free(input);

	return p_vm->t;
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


	return p_vm->m_stack[p_vm->m_sp - 1];
}


value_t *cons(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	p_vm->m_stack[p_vm->m_bp] = value_create_cons(p_vm, first, second);
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;
	return value_create_cons(p_vm, first, second);
}

value_t *car(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];

	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;
	if(is_null(p_vm, first)) {
		p_vm->m_stack[p_vm->m_bp] = first;
		return first;
	}

verify(is_cons(p_vm, first) == true, "car'ing something not a cons %d", first->m_type);

	p_vm->m_stack[p_vm->m_bp] = first->m_cons[0];
	return first->m_cons[0];
}

value_t *cdr(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];

	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;
	if(is_null(p_vm, first)) {
		p_vm->m_stack[p_vm->m_bp] = first;
		return first;
	}
	assert(is_cons(p_vm, first));

	p_vm->m_stack[p_vm->m_bp] = first->m_cons[1];
	return first->m_cons[1];
}

value_t *eq(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	bool first_num = is_fixnum(first);
	bool second_num = is_fixnum(second);

	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;

	if (first_num != second_num) {
		p_vm->m_stack[p_vm->m_bp] = p_vm->nil;
		return p_vm->nil;
	}

	if (first_num == true && second_num == true) {
		p_vm->m_stack[p_vm->m_bp] = to_fixnum(first) == to_fixnum(second) ? p_vm->t : p_vm->nil;
		return to_fixnum(first) == to_fixnum(second) ? p_vm->t : p_vm->nil;
	}


	if (first->m_type != second->m_type) {
		p_vm->m_stack[p_vm->m_bp] = p_vm->nil;
		return p_vm->nil;
	}

	verify(first->m_type == second->m_type, "eq: Types are not the same %d != %d\n",
		first->m_type, second->m_type);

	switch (first->m_type) {
		case VT_NUMBER:
		case VT_SYMBOL:
		case VT_STRING:
			if (value_equal(first, second) == true) {
				p_vm->m_stack[p_vm->m_bp] = p_vm->t;
				return p_vm->t;
			} else {
				p_vm->m_stack[p_vm->m_bp] = p_vm->nil;
				return p_vm->nil;
			}

		default:
			assert(!"Trying to test unsupported type with eq");
			break;
	}

	p_vm->m_stack[p_vm->m_bp] = p_vm->nil;
	return p_vm->nil;
}

value_t *less_then(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	verify(is_fixnum(first), "argument X is not a number: %s", value_sprint(p_vm, first));
	verify(is_fixnum(second), "argument Y is not a number: %s", value_sprint(p_vm, second));

	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;

	if (to_fixnum(first) < to_fixnum(second)) {
		p_vm->m_stack[p_vm->m_bp] = p_vm->t;
		return p_vm->t;
	} else {
		p_vm->m_stack[p_vm->m_bp] = p_vm->nil;
		return p_vm->nil;
	}
}

value_t *minus(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	verify(is_fixnum(first), "argument X is not a number: %s", value_sprint(p_vm, first));
	verify(is_fixnum(second), "argument Y is not a number: %s", value_sprint(p_vm, second));

	p_vm->m_ip++;
	p_vm->m_stack[p_vm->m_bp] = make_fixnum(to_fixnum(first) - to_fixnum(second));
	p_vm->m_sp = p_vm->m_bp + 1;
	return make_fixnum(to_fixnum(first) - to_fixnum(second));
}

value_t *times(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	verify(is_fixnum(first), "argument X is not a number: %s", value_sprint(p_vm, first));
	verify(is_fixnum(second), "argument Y is not a number: %s", value_sprint(p_vm, second));

	p_vm->m_stack[p_vm->m_bp] = make_fixnum(to_fixnum(first) * to_fixnum(second));
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;

	return make_fixnum(to_fixnum(first) * to_fixnum(second));
}
value_t *plus(vm_t *p_vm)
{
	value_t *first = p_vm->m_stack[p_vm->m_bp + 1];
	value_t *second = p_vm->m_stack[p_vm->m_bp + 2];

	verify(is_fixnum(first), "argument X is not a number: %s", value_sprint(p_vm, first));
	verify(is_fixnum(second), "argument Y is not a number: %s", value_sprint(p_vm, second));

	
	p_vm->m_stack[p_vm->m_bp] = make_fixnum(to_fixnum(first) + to_fixnum(second));
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;

	return make_fixnum(to_fixnum(first) + to_fixnum(second));
}

value_t *print(vm_t *p_vm)
{

	//printf("sp: %lu bp: %lu>> \n", p_vm->m_sp, p_vm->m_bp);
	//vm_print_stack(p_vm);
	//printf("<<\n");


	value_t *arg = p_vm->m_stack[p_vm->m_bp + 1];
	value_print(p_vm, arg); printf("\n");
#if 0
	value_t *last = arg;
	while(arg != p_vm->nil) {
		assert(is_cons(p_vm, arg));
	//	value_print(p_vm, arg->m_cons[0]);
	//	value_print(p_vm, arg->m_cons[0]);
		printf(" ");
		last = arg;
		arg = arg->m_cons[1];
	}
	printf("\n");
#endif

	p_vm->m_stack[p_vm->m_bp] = arg;
//	p_vm->m_stack[p_vm->m_bp] = last->m_cons[0];
	p_vm->m_sp = p_vm->m_bp + 1;
	p_vm->m_ip++;

	//return last->m_cons[0];
	return arg;
}




void lib_init(vm_t *p_vm)
{
	int i;

	// Must create this before symbols
	p_vm->voidobj = value_create(p_vm, VT_VOID, 0, true);

	for (i = 0; i < NUM_IFUNCS; i++) {
		vm_bindf(p_vm, g_ifuncs[i].m_name, g_ifuncs[i].m_func, g_ifuncs[i].m_arg_count, g_ifuncs[i].m_is_macro, g_ifuncs[i].m_dynamic);
	}

	ftime(&g_time);
	

	p_vm->nil = value_create_symbol(p_vm, "NIL");
	bind_internal(p_vm, p_vm->nil, p_vm->nil, false, true);
//	vm_bind(p_vm, "NIL", p_vm->nil);
	p_vm->t = value_create_symbol(p_vm, "T");
	vm_bind(p_vm, "T", p_vm->t, true);

}
