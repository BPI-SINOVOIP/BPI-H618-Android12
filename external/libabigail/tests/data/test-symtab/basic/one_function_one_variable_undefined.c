extern int undefined_variable;
void undefined_function(int);


static void local_function(){
	undefined_function(undefined_variable);
}
