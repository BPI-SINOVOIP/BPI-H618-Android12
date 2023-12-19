struct opaque_type;

struct container_type
{
  struct opaque_type* s;
};

long long bar(struct container_type*);
long long baz(struct container_type*);
long long foo1(struct container_type*);
long long foo2(struct container_type*);
