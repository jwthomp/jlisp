#include "vm.h"
#include "value.h"
#include "assert.h"

//
// 1 the definition of lispobj * and utility functions like car, cdr, cons -- at the C level, not the VM level)
// 2 the VM - stack manipulation, function calls ,etc
// 3 a reader - something that takes a string/stream and returns a lispobj* from it
// 4 compiler - given a lispobj * generate bytecode for it
//

//
// eval(const char *s) {
//	lispobj *p = read(s);
//	lispobj *fn = compile(p);
//	lispobj *result = execute(fn);
//	return results;
// }
/*
compile_form(value_t *form)
	value_t *fn = car(form);
	value_t *args = cdr(form);
	compile_function(fn);
	int nargs = compile_args(args);
	compile_call(nargs);


compile_function(lispobj *form)
	lispobj *fn = car(form)
	if (fn == QUOTE) {
		compile_push(x)
	} else {
		compile_args...
	}

compile_quote(lispobj *args)
{
	lispobj *x = car(args);
	assert(cdr(args) == NIL);
	assemble(PUSH, x);
}
*/

value_t * car(value_t *p_form)
{
	assert(p_form != 0);
	assert(p_form->m_type == VT_CONS);

	return p_form->m_cons[0];
}

value_t * cdr(value_t *p_form)
{
	assert(p_form != 0);
	assert(p_form->m_type == VT_CONS);

	return p_form->m_cons[1];
}

bool is_cons(value_t *p_form)
{
	return p_form->m_type == VT_CONS ? true : false;
}

bool is_symbol(value_t *p_form)
{
	return p_form->m_type == VT_SYMBOL ? true : false;
}

void compile_function(value_t *p_form)
{
}

int compile_args(value_t * p_form)
{
}

void compile_call(int p_arg_count)
{
}

void assemble(opcode_e p_opcode, value_t *p_form)
{
}

value_t * compile(value_t *p_form)
{
	if (is_cons(p_form)) {
		value_t *fn = car(p_form);
		value_t *args = cdr(p_form);

		compile_function(fn);
		int nargs = compile_args(args);
		compile_call(nargs);
	} else if (is_symbol) {
		assemble(OP_LOAD, p_form);
	} else {
		assemble(OP_PUSH, p_form);
	}
}
