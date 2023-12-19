extern int undefined_variable;

static void other_local_function(int variable) { (void)variable; }

static void local_function(){
	other_local_function(undefined_variable);
}
