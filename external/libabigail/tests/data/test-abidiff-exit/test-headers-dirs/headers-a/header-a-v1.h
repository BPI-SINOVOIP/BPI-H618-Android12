typedef struct opaque_struct * opaque_struct_pointer_type;

typedef struct public_struct_type *public_struct_pointer_type;
typedef struct public_struct_type2 *public_struct_pointer_type2;

typedef void (*FuncPointerType0) (public_struct_pointer_type,
				  public_struct_pointer_type);

typedef void (*FuncPointerType1) (public_struct_pointer_type, int);

typedef struct public_struct_type2
{
  FuncPointerType0 m0;
  FuncPointerType1 m1;
} public_struct_type2;

typedef struct public_struct_type
{
  opaque_struct_pointer_type m0;
  public_struct_type2 *m1;
} public_struct_type;

void foo(public_struct_pointer_type p1);

void bar(second_public_struct_pointer_type p1,
	 second_opaque_struct_pointer_type p2);
