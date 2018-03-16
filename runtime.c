static void setup();
static void cleanup();

/* exported functions */

void
__cyg_profile_func_enter(void *fn, void *cs)
{
	Frame f = {
		.fn = (uintptr_t)fn,
		.cs = (uintptr_t)cs,
	};
	if (!instenter(&sp, f))
		setinststate(OFF);
}

void
__cyg_profile_func_exit(void *fn, void *cs)
{
	if (!instexit(&sp))
		setinststate(OFF);
}

/* private functions */

__attribute__ ((constructor (101)))
void
setup()
{
}

__attribute__ ((destructor (101)))
void
cleanup()
{
}
