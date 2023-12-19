struct opaque_type;

struct container_type
{
  struct stub_type* s;
};

long long bar(struct container_type*);
long long baz(struct container_type*);
